
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/element_contexts.hpp"
#include "cxxreflect/reflection/type.hpp" // REMOVE For debug only

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    class type_def_and_signature
    {
    public:

        type_def_and_signature() { }

        explicit type_def_and_signature(metadata::type_def_token const& token)
            : _type_def(token)
        {
            core::assert_initialized(token);
        }

        type_def_and_signature(metadata::type_def_token const& token, metadata::blob const& signature)
            : _type_def(token), _signature(signature)
        {
            core::assert_initialized(token);
            core::assert_initialized(signature);
        }

        auto type_def()      const -> metadata::type_def_token { return _type_def;                   }
        auto has_type_def()  const -> bool                     { return _type_def.is_initialized();  }
        auto signature()     const -> metadata::blob           { return _signature;                  }
        auto has_signature() const -> bool                     { return _signature.is_initialized(); }

    private:

        metadata::type_def_token _type_def;
        metadata::blob           _signature;
    };

    auto get_type_spec_signature(metadata::type_spec_token const& type) -> metadata::type_signature
    {
        core::assert_initialized(type);

        return row_from(type).signature().as<metadata::type_signature>();
    }

    auto resolve_type_def_and_signature(metadata::type_resolver                  const& resolver,
                                        metadata::type_def_ref_spec_or_signature const& original_type)
        -> type_def_and_signature
    {
        core::assert_initialized(original_type);

        // First, resolve the type to either a TypeDef or TypeSpec:
        metadata::type_def_spec_or_signature const resolved_type(original_type.is_token()
            ? metadata::type_def_spec_or_signature(resolver.resolve_type(original_type.as_token()))
            : metadata::type_def_spec_or_signature(original_type.as_blob()));

        // If we resolved the type to a TypeDef, it has on signature so we may return it directly:
        if (resolved_type.is_token() && resolved_type.as_token().is<metadata::type_def_token>())
            return type_def_and_signature(resolved_type.as_token().as<metadata::type_def_token>());

        // Otherwise, we must have a TypeSpec, which we need to resolve to its primary TypeDef:
        metadata::type_signature const signature(resolved_type.is_token()
            ? get_type_spec_signature(resolved_type.as_token().as<metadata::type_spec_token>())
            : resolved_type.as_blob().as<metadata::type_signature>());

        switch (signature.get_kind())
        {
        case metadata::type_signature::kind::class_type:
        {
            return resolve_type_def_and_signature(
                resolver,
                signature.class_type());
        }

        case metadata::type_signature::kind::primitive:
        {
            return resolve_type_def_and_signature(
                resolver,
                resolver.resolve_fundamental_type(signature.primitive_type()));
        }

        // If we have a generic inst we return its generic type definition and the instantiation:
        case metadata::type_signature::kind::generic_instance:
        {
            // Re-resolve the generic type definition:
            metadata::type_def_spec_token const re_resolved_type(
                resolver.resolve_type(signature.generic_type()));

            // A generic inst should always refer to a type def, never a type spec:
            if (!re_resolved_type.is<metadata::type_def_token>())
                throw core::metadata_error(L"generic type definition did not resolve to type def");

            return type_def_and_signature(
                re_resolved_type.as<metadata::type_def_token>(),
                metadata::blob(&signature.scope(), signature.begin_bytes(), signature.end_bytes()));
        }

        case metadata::type_signature::kind::general_array:
        case metadata::type_signature::kind::simple_array:
        {
            // TODO What we really need to do is treat an Array as a generic type and fabricate a
            // faux Array<T> that implements the generic interfaces.  Otherwise, we'll miss several
            // elements in various categories.  This is a good start, though.
            return resolve_type_def_and_signature(
                resolver,
                resolver.resolve_fundamental_type(metadata::element_type::array));
        }

        case metadata::type_signature::kind::pointer:
        case metadata::type_signature::kind::function_pointer:
        case metadata::type_signature::kind::variable:
        {
            // TODO Support for ptr, fn_ptr, and var types:
            return core::default_value();
        }
        default:
            throw core::logic_error(L"not yet implemented");
        }
    }

    auto get_type_def_or_signature(metadata::type_def_spec_token const& token) -> metadata::type_def_or_signature
    {
        if (token.is<metadata::type_def_token>())
            return token.as<metadata::type_def_token>();

        return row_from(token.as<metadata::type_spec_token>()).signature();
    }

    auto create_instantiator(metadata::type_def_spec_or_signature const& type)
        -> metadata::class_variable_signature_instantiator
    {
        if (!type.is_initialized() || !type.is_blob())
            return metadata::class_variable_signature_instantiator();

        metadata::type_signature const signature(&type.scope(), begin(type.as_blob()), end(type.as_blob()));

        // We are only expecting to get base classes here, so it should be a GenericInst:
        if (signature.get_kind() != metadata::type_signature::kind::generic_instance)
            throw core::runtime_error(L"unexpected type provided for instantiation");

        return metadata::class_variable_signature_instantiator(
            &type.scope(),
            signature.begin_generic_arguments(),
            signature.end_generic_arguments());
    }

    template <typename Storage, typename Signature, typename Instantiator>
    auto instantiate(Storage& storage, Signature const& signature, Instantiator const& instantiator)
        -> core::const_byte_range
    {
        core::assert_initialized(signature);
        core::assert_true([&]{ return Instantiator::requires_instantiation(signature); });

        Signature const& instantiation(instantiator.instantiate(signature));
        return storage.allocate_signature(instantiation.begin_bytes(), instantiation.end_bytes());
    }

    template <typename ContextTag>
    class recursive_table_builder
    {
    public:

        typedef element_context_traits<ContextTag>              traits_type;
        typedef typename traits_type::token_type                token_type;
        typedef typename traits_type::signature_type            signature_type;
        typedef typename traits_type::context_type              context_type;
        typedef element_context_table_collection<ContextTag>    collection_type;
        typedef metadata::class_variable_signature_instantiator instantiator_type;
        typedef element_context_table_storage::storage_lock     storage_type;

        recursive_table_builder(metadata::type_resolver const* const resolver,
                                collection_type         const* const collection,
                                storage_type            const* const storage)
            : _resolver(resolver), _collection(collection), _storage(storage)
        {
            core::assert_not_null(resolver);
            core::assert_not_null(collection);
            core::assert_not_null(storage);
        }

        auto create_element(token_type             const& token,
                            type_def_and_signature const& instantiating_type,
                            instantiator_type      const& instantiator) -> context_type
        {
            metadata::blob const signature_blob(traits_type::get_signature(*_resolver.get()));
            if (!signature_blob.is_initialized())
                return context_type(token);

            signature_type const signature(signature_blob.as<signature_type>());

            bool const requires_instantiation(instantiator.has_arguments() && instantiator_type::requires_instantiation(signature));
            if (requires_instantiation)
            {
                return context_type(
                    token,
                    instantiating_type.signature(),
                    instantiate(*_storage.get(), signature, instantiator));
            }
            else
            {
                return context_type(token);
            }
        }

    private:

        core::value_initialized<metadata::type_resolver const*> _resolver;
        core::value_initialized<collection_type const*>         _collection;
        core::value_initialized<storage_type const*>            _storage;
    };

    template <typename ContextTag>
    auto create_recursive_table_builder(metadata::type_resolver                      const* const resolver,
                                        element_context_table_collection<ContextTag> const* const collection,
                                        element_context_table_storage::storage_lock  const* const storage)
        -> element_context_table_collection<ContextTag>
    {
        return element_context_table_collection<ContextTag>(resolver, collection, storage);
    }

    // This recursive element inserter creates elements using the TCreateElement functor, inserts
    // them into a buffer using the TInsertElement functor, then finally recurses, getting the next
    // set of elements to be processed from the element that was just processed.  We do not recurse
    // for all element types (for the moment, we only recurse for InterfaceContexts).
    template <typename Traits, typename Collection, typename Instantiator, typename CreateElement, typename InsertElement>
    class recursive_element_inserter
    {
    public:

        recursive_element_inserter(metadata::type_resolver const* const resolver,
                                   Collection              const* const collection,
                                   Instantiator            const* const instantiator,
                                   CreateElement                  const create,
                                   InsertElement                  const insert)
            : _resolver(resolver), _collection(collection), _instantiator(instantiator), _create(create), _insert(insert)
        {
        }

        template <typename T>
        auto operator()(T const& x) -> void
        {
            context_type const new_context(_create(x.token(), *_instantiator.get()));
            _insert(new_context);

            recurse(new_context);
        }

    private:

        typedef typename Traits::context_type context_type;

        // This type is copy constructable but not assignable because the TCreateElement and
        // TInsertElement types are likely to be non-assignable lambda types.  This is kind of ugly.
        auto operator=(recursive_element_inserter const&) -> recursive_element_inserter&;

        // By default, we don't actually recurse.  We provide nontemplate overloads for the context
        // types for which we are going to recurse.
        template <typename T>
        void recurse(T const&)
        {
        }

        // For an interface element, we get the interface type and enumerate all of the interfaces
        // that it implements.
        void recurse(interface_context const& context)
        {
            metadata::type_def_ref_spec_token const if_token(context.element_row().interface());

            type_def_and_signature const def_and_sig(resolve_type_def_and_signature(*_resolver.get(), if_token));

            metadata::class_variable_signature_instantiator const sig_instantiator(create_instantiator(def_and_sig.signature()));

            auto const table(_collection.get()->get_or_create_table(def_and_sig.has_signature()
                ? metadata::type_def_or_signature(def_and_sig.signature())
                : metadata::type_def_or_signature(def_and_sig.type_def())));

            std::for_each(begin(table), end(table), [&](interface_context const& c)
            {
                if (sig_instantiator.has_arguments() &&
                    c.element_signature(*_resolver.get()).is_initialized() &&
                    metadata::class_variable_signature_instantiator::requires_instantiation(c.element_signature(*_resolver.get())))
                {
                    auto const ex(_create(c.element(), sig_instantiator));

                    _insert(ex);
                    recurse(ex);
                }
                else
                {
                    _insert(c);
                    recurse(c);
                }
            });
        }

        core::value_initialized<metadata::type_resolver const*> _resolver;
        core::value_initialized<Collection const*>              _collection;
        core::value_initialized<Instantiator const*>            _instantiator;
        CreateElement _create;
        InsertElement _insert;
    };

    template <typename Traits, typename Collection, typename Instantiator, typename CreateElement, typename InsertElement>
    recursive_element_inserter<Traits, Collection, Instantiator, CreateElement, InsertElement>
    create_recursive_element_inserter(metadata::type_resolver const* const resolver,
                                      Collection              const* const collection,
                                      Instantiator            const* const instantiator,
                                      CreateElement                  const create,
                                      InsertElement                  const insert)
    {
        return recursive_element_inserter<Traits, Collection, Instantiator, CreateElement, InsertElement>(resolver, collection, instantiator, create, insert);
    }

    

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto event_context_traits::begin_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return metadata::begin_events(type);
    }

    auto event_context_traits::end_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return metadata::end_events(type);
    }

    auto event_context_traits::get_signature(metadata::type_resolver const& resolver,
                                             token_type              const& element) -> metadata::blob
    {
        core::assert_initialized(element);

        metadata::type_def_ref_spec_token const original_type(row_from(element).type());
        metadata::type_def_spec_token     const resolved_type(resolver.resolve_type(original_type));

        // If the type is a TypeDef, it has no distinct signature so we can simply return an empty
        // signature here:
        if (resolved_type.is<metadata::type_def_token>())
            return metadata::blob();

        // Otherwise, we have a TypeSpec, so we should return its signature:
        core::assert_true([&]{ return resolved_type.is<metadata::type_spec_token>(); });

        return row_from(resolved_type.as<metadata::type_spec_token>()).signature();
    }

    auto event_context_traits::insert_element(metadata::type_resolver const&,
                                              context_sequence_type        & element_table,
                                              context_type            const& new_element,
                                              core::size_type         const) -> void
    {
        core::assert_initialized(new_element);

        // TODO Do we need to handle hiding or overriding for events?
        element_table.push_back(new_element);
    }





    auto field_context_traits::begin_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return row_iterator_type(&type.scope(), row_from(type).first_field().index());
    }

    auto field_context_traits::end_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return row_iterator_type(&type.scope(), row_from(type).last_field().index());
    }


    auto field_context_traits::get_signature(metadata::type_resolver const&,
                                             token_type              const& element) -> metadata::blob
    {
        core::assert_initialized(element);

        return row_from(element).signature();
    }

    auto field_context_traits::insert_element(metadata::type_resolver const&,
                                              context_sequence_type        & element_table,
                                              context_type            const& new_element,
                                              core::size_type         const) -> void
    {
        core::assert_initialized(new_element);

        // TODO Do we need to handle hiding or overriding for fields?
        element_table.push_back(new_element);
    }





    auto interface_context_traits::begin_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return metadata::find_interface_impl_range(type).first;
    }

    auto interface_context_traits::end_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return metadata::find_interface_impl_range(type).second;
    }

    auto interface_context_traits::get_signature(metadata::type_resolver const& resolver,
                                                 token_type              const& element) -> metadata::blob
    {
        core::assert_initialized(element);

        metadata::type_def_ref_spec_token const original_type(row_from(element).interface());
        metadata::type_def_spec_token     const resolved_type(resolver.resolve_type(original_type));

        // If the type is a TypeDef, it has no distinct signature so we can simply return an empty
        // signature here:
        if (resolved_type.is<metadata::type_def_token>())
            return metadata::blob();

        // Otherwise, we have a TypeSpec, so we should return its signature:
        core::assert_true([&]{ return resolved_type.is<metadata::type_spec_token>(); });

        return row_from(resolved_type.as<metadata::type_spec_token>()).signature();
    }

    auto interface_context_traits::insert_element(metadata::type_resolver const& resolver,
                                                  context_sequence_type        & element_table,
                                                  context_type            const& new_element,
                                                  core::size_type         const  inherited_element_count) -> void
    {
        core::assert_initialized(new_element);

        metadata::type_def_spec_token const new_if(resolver.resolve_type(new_element.element_row().interface()));

        // Iterate over the interface table and see if it already contains the new interface.  This
        // can happen if two classes in a class hierarchy both implement an interface.  If there are
        // two classes that implement an interface, we keep the most derived one.
        auto const it(std::find_if(begin(element_table), end(element_table), [&](context_type const& old_element) -> bool
        {
            metadata::type_def_spec_token const old_if(resolver.resolve_type(old_element.element_row().interface()));

            // If the old and new interfaces resolved to different kinds of types, obviously they
            // are not the same (basically, one is a TypeDef, the other is a TypeSpec).
            if (old_if.table() != new_if.table())
                return false;

            // If both interfaces are TypeDefs, they are the same if and only if they point at the
            // same TypeDef row in the same database.
            if (old_if.table() == metadata::table_id::type_def)
                return old_if == new_if;

            // Otherwise, both interfaces are TypeSpecs, so we compare equality using the signature
            // comparison rules:
            auto const old_signature(old_element.element_signature(resolver));
            auto const new_signature(new_element.element_signature(resolver));

            metadata::signature_comparer const compare(&resolver);

            return compare(old_signature, new_signature);
        }));

        return it == end(element_table)
            ? (void)element_table.push_back(new_element)
            : (void)(*it = new_element);
    }





    auto method_context_traits::begin_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return row_iterator_type(&type.scope(), row_from(type).first_method().index());
    }

    auto method_context_traits::end_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return row_iterator_type(&type.scope(), row_from(type).last_method().index());
    }

    auto method_context_traits::get_signature(metadata::type_resolver const&,
                                              token_type              const& element) -> metadata::blob
    {
        core::assert_initialized(element);

        return row_from(element).signature();
    }

    auto method_context_traits::insert_element(metadata::type_resolver const& resolver,
                                              context_sequence_type         & element_table,
                                              context_type             const& new_element,
                                              core::size_type          const  inherited_element_count) -> void
    {
        core::assert_initialized(new_element);
        core::assert_true([&]{ return inherited_element_count <= element_table.size(); });

        metadata::method_def_row   const new_method_def(new_element.element_row());
        metadata::method_signature const new_method_sig(new_element.element_signature(resolver));

        // If the method occupies a new slot, it does not override any other method.  A static
        // method is always a new method.
        if (new_method_def.flags().with_mask(metadata::method_attribute::vtable_layout_mask) == metadata::method_attribute::new_slot ||
            new_method_def.flags().is_set(metadata::method_attribute::static_))
        {
            element_table.push_back(new_element);
            return;
        }

        bool is_new_method(true);

        auto const table_begin(element_table.rbegin() + (element_table.size() - inherited_element_count));
        auto const table_end(element_table.rend());
        auto const table_it(std::find_if(table_begin, table_end, [&](context_type const& old_element) -> bool
        {
            metadata::method_def_row   const old_method_def(old_element.element_row());
            metadata::method_signature const old_method_sig(old_element.element_signature(resolver));

            // Note that by skipping nonvirtual methods, we also skip the name hiding feature.  We
            // do not hide any names by name or signature; we only hide overridden virtual methods.
            // This matches the runtime behavior of the CLR, not the compiler behavior.
            if (!old_method_def.flags().is_set(metadata::method_attribute::virtual_))
                return false;

            // TODO Add support for the method_impl table
            if (old_method_def.name() != new_method_def.name())
                return false;

            metadata::signature_comparer const compare(&resolver);

            // If the signature of the method in the derived class is different from the signature
            // of the method in the base class, it is not an override:
            if (!compare(old_method_sig, new_method_sig))
                return false;

            // If the base class method is final, the derived class method is a new method:
            is_new_method = old_method_def.flags().is_set(metadata::method_attribute::final);
            return true;
        }));

        return is_new_method
            ? (void)element_table.push_back(new_element)
            : (void)(*table_it = new_element);
    }





    auto property_context_traits::begin_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return metadata::begin_properties(type);
    }

    auto property_context_traits::end_elements(metadata::type_def_token const& type) -> row_iterator_type
    {
        core::assert_initialized(type);

        return metadata::end_properties(type);
    }

    auto property_context_traits::get_signature(metadata::type_resolver const&,
                                                token_type              const& element) -> metadata::blob
    {
        core::assert_initialized(element);

        return row_from(element).signature();
    }

    auto property_context_traits::insert_element(metadata::type_resolver const& resolver,
                                                 context_sequence_type        & element_table,
                                                 context_type            const& new_element,
                                                 core::size_type         const  inherited_element_count) -> void
    {
        core::assert_initialized(new_element);

        // TODO Do we need to handle hiding or overriding for properties?
        element_table.push_back(new_element);
    }





    template <typename T>
    element_context<T>::element_context()
    {
    }

    template <typename T>
    element_context<T>::element_context(token_type const& element_token)
        : _element(element_token)
    {
        core::assert_initialized(element_token);
    }

    template <typename T>
    element_context<T>::element_context(token_type                      const& element_token,
                                        metadata::type_def_or_signature const& instantiating_type_token,
                                        core::const_byte_range          const& instantiated_signature_range)
        : _element(element_token),
          _instantiating_type(instantiating_type_token),
          _instantiated_signature(instantiated_signature_range)
    {
        core::assert_initialized(element_token);
        core::assert_initialized(instantiating_type_token);
    }

    // TODO template <typename T>
    // TODO auto element_context<T>::resolve(type const& reflected_type) const -> resolved_type
    // TODO {
    // TODO }

    template <typename T>
    auto element_context<T>::element() const -> token_type
    {
        core::assert_initialized(*this);

        return _element.as<token_type>();
    }

    template <typename T>
    auto element_context<T>::element_row() const -> row_type
    {
        core::assert_initialized(*this);

        return row_from(_element.as<token_type>());
    }

    template <typename T>
    auto element_context<T>::element_signature(metadata::type_resolver const& resolver) const -> signature_type
    {
        core::assert_initialized(*this);

        if (has_instantiated_signature())
        {
            return signature_type(
                &_instantiating_type.scope(),
                begin(_instantiated_signature),
                end(_instantiated_signature));
        }

        metadata::blob const signature(traits_type::get_signature(resolver, element()));
        if (!signature.is_initialized())
        {
            return signature_type();
        }

        return signature.as<signature_type>();
    }

    template <typename T>
    auto element_context<T>::has_instantiating_type() const -> bool
    {
        core::assert_initialized(*this);

        return _instantiating_type.is_initialized();
    }

    template <typename T>
    auto element_context<T>::instantiating_type() const -> metadata::type_def_or_signature
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return has_instantiating_type(); });

        return _instantiating_type;
    }

    template <typename T>
    auto element_context<T>::has_instantiated_signature() const -> bool
    {
        core::assert_initialized(*this);

        return _instantiated_signature.is_initialized();
    }

    template <typename T>
    auto element_context<T>::instantiated_signature() const -> core::const_byte_range
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return has_instantiated_signature(); });

        return _instantiated_signature;
    }

    template <typename T>
    auto element_context<T>::is_initialized() const -> bool
    {
        return _element.is_initialized();
    }





    template class element_context<event_context_tag    >;
    template class element_context<field_context_tag    >;
    template class element_context<interface_context_tag>;
    template class element_context<method_context_tag   >;
    template class element_context<property_context_tag >;





    element_context_table_storage::storage_lock::storage_lock(element_context_table_storage const* const storage)
        : _storage(storage), _lock(storage->_sync.lock())
    {
    }

    element_context_table_storage::storage_lock::storage_lock(storage_lock&& other)
        : _storage(other._storage), _lock(std::move(other._lock))
    {
        other._storage.reset();
    }

    auto element_context_table_storage::storage_lock::operator=(storage_lock&& other) -> storage_lock&
    {
        _lock = std::move(other._lock);
        _storage = other._storage;
        other._storage.reset();
        return *this;
    }

    auto element_context_table_storage::storage_lock::allocate_signature(
        core::const_byte_iterator const first,
        core::const_byte_iterator const last) const
        -> core::const_byte_range
    {
        core::assert_initialized(*this);

        auto& storage(_storage.get()->_signature_storage);
        auto const range(storage.allocate(core::distance(first, last)));
        core::range_checked_copy(first, last, begin(range), end(range));
        return range;
    }

    bool element_context_table_storage::storage_lock::is_initialized() const
    {
        return _storage.get() != nullptr;
    }

    auto element_context_table_storage::storage_lock::get_table(table_index_value& v, event_context_tag)
        -> event_context_table&
    {
        return v._events;
    }
    auto element_context_table_storage::storage_lock::get_table(table_index_value& v, field_context_tag)
        -> field_context_table&
    {
        return v._fields;
    }

    auto element_context_table_storage::storage_lock::get_table(table_index_value& v, interface_context_tag)
        -> interface_context_table&
    {
        return v._interfaces;
    }

    auto element_context_table_storage::storage_lock::get_table(table_index_value& v, method_context_tag)
        -> method_context_table&
    {
        return v._methods;
    }

    auto element_context_table_storage::storage_lock::get_table(table_index_value& v, property_context_tag)
        -> property_context_table&
    {
        return v._properties;
    }

    element_context_table_storage::element_context_table_storage()
    {
    }

    auto element_context_table_storage::lock() const -> storage_lock
    {
        return storage_lock(this);
    }





    template <typename ContextTag>
    element_context_table_collection<ContextTag>::element_context_table_collection(
        metadata::type_resolver       const* resolver,
        element_context_table_storage const* storage)
        : _resolver(resolver), _storage(storage)
    {
        core::assert_not_null(resolver);
        core::assert_not_null(storage);
    }

    template <typename ContextTag>
    element_context_table_collection<ContextTag>::element_context_table_collection(
        element_context_table_collection&& other)
        : _resolver(other._resolver), _storage(other._storage)
    {
        core::assert_initialized(*this);
        other._resolver.reset();
        other._storage.reset();
    }

    template <typename ContextTag>
    auto element_context_table_collection<ContextTag>::operator=(
        element_context_table_collection&& other) -> element_context_table_collection&
    {
        core::assert_initialized(other);

        _resolver = other._resolver;
        _storage  = other._storage;

        other._resolver.reset();
        other._storage.reset();

        return *this;
    }

    template <typename ContextTag>
    auto element_context_table_collection<ContextTag>::get_or_create_table(
        metadata::type_def_or_signature const& type) const -> context_table_type
    {
        core::assert_initialized(*this);

        // Obtain a lock on the storage for the duration of the table lookup or creation.  In theory
        // we could do this in two stages and lock separately for each stage, but it is unlikely
        // that this lock will be contentious.
        auto const storage(_storage.get()->lock());

        // First, handle the "get" of "get or create."  If we already created a table, return it:
        auto const result(storage.find_table<ContextTag>(type));
        if (result.first)
            return result.second;

        // Ok, we haven't yet created a table, so let's create a new one:
        typename traits_type::context_sequence_type new_table;

        // Compute and split the type definition and signature:  if we don't have a definition, then
        // there are no elements for this type, so we record that and return the result:
        type_def_and_signature const def_and_sig(resolve_type_def_and_signature(*_resolver.get(), type));
        if (!def_and_sig.has_type_def())
        {
            return storage.allocate_table<ContextTag>(type, new_table.data(), new_table.data() + new_table.size());
        }

        metadata::type_def_token const td_token(def_and_sig.type_def());
        metadata::type_def_row   const td_row(row_from(td_token));
        metadata::type_signature const td_sig(def_and_sig.has_signature()
            ? def_and_sig.signature().as<metadata::type_signature>()
            : metadata::type_signature());

        instantiator const sig_instantiator(create_instantiator(def_and_sig.signature()));

        // First, recursively handle the base type hierarchy so that inherited members are emplaced
        // into the table first; this allows us to emulate runtime overriding and hiding behaviors.
        metadata::type_def_ref_spec_token const td_base(td_row.extends());
        if (td_base.is_initialized())
        {
            context_table_type const base_table(get_or_create_table(get_type_def_or_signature(_resolver.get()->resolve_type(td_base))));

            std::transform(begin(base_table), end(base_table), std::back_inserter(new_table), [&](context_type const& e) -> context_type
            {
                if (!sig_instantiator.has_arguments())
                    return e;

                auto const e_sig(e.element_signature(*_resolver.get()));
                if (!e_sig.is_initialized() || !instantiator::requires_instantiation(e_sig))
                    return e;

                return context_type(
                    e.element(),
                    e.instantiating_type(),
                    instantiate(storage, e_sig, sig_instantiator));
            });
        }

        core::size_type const inherited_element_count(core::convert_integer(new_table.size()));

        auto const create_element([&](token_type const& element_token, instantiator const& sig_instantiator) -> context_type
        {
            metadata::blob const element_sig_ref(traits_type::get_signature(*_resolver.get(), element_token));

            if (!element_sig_ref.is_initialized())
                return context_type(element_token);

            signature_type const element_sig(element_sig_ref.as<signature_type>());

            bool const requires_instantiation(
                sig_instantiator.has_arguments() &&
                instantiator::requires_instantiation(element_sig));

            core::const_byte_range const instantiated_sig(requires_instantiation
                ? instantiate(storage, element_sig, sig_instantiator)
                : core::const_byte_range(element_sig.begin_bytes(), element_sig.end_bytes()));

            return requires_instantiation
                ? context_type(element_token, metadata::blob(td_sig), instantiated_sig)
                : context_type(element_token);
        });

        auto const insert_into_buffer([&](context_type const& element) -> void
        {
            traits_type::insert_element(*_resolver.get(), new_table, element, inherited_element_count);
        });

        // Second, enumerate the elements declared by this type itself (i.e., not inherited) and
        // insert them into the buffer in the correct location.
        row_iterator_type const first_element(traits_type::begin_elements(td_token));
        row_iterator_type const last_element (traits_type::end_elements  (td_token));

        // TODO This function has become a disaster. :'(
        auto const recursive_inserter(create_recursive_element_inserter<traits_type>(
            _resolver.get(),
            this,
            &sig_instantiator,
            create_element,
            insert_into_buffer));

        std::for_each(first_element, last_element, recursive_inserter);
        
        return storage.allocate_table<ContextTag>(type, new_table.data(), new_table.data() + new_table.size());
    }

    template <typename ContextTag>
    auto element_context_table_collection<ContextTag>::is_initialized() const -> bool
    {
        return _resolver.get() != nullptr;
    }




    template class element_context_table_collection<event_context_tag    >;
    template class element_context_table_collection<field_context_tag    >;
    template class element_context_table_collection<interface_context_tag>;
    template class element_context_table_collection<method_context_tag   >;
    template class element_context_table_collection<property_context_tag >;

} } }

// AMDG //
