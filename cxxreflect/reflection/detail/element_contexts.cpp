
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/element_contexts.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    /// A pair type that represents a type definition and a type signature
    ///
    /// We have many cases where we may have a type signature or a type definition, and if we have
    /// a type signature, we may optionally have a primary type definition associated with it.  This
    /// class contains both the definition and signature.
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

        /// Returns the signature if one exists, otherwise returns the definition
        auto best_match() const -> metadata::type_def_or_signature
        {
            if (has_signature())
                return signature();
            
            return type_def();
        }

    private:

        metadata::type_def_token _type_def;
        metadata::blob           _signature;
    };





    /// Gets the `type_signature` that defines the provided type spec
    auto get_type_spec_signature(metadata::type_spec_token const& type) -> metadata::type_signature
    {
        core::assert_initialized(type);

        return row_from(type).signature().as<metadata::type_signature>();
    }





    /// Resolves the type definition and signature for an arbitrary type
    ///
    /// Type references are resolved via `resolver`.  If the resolved type is a type definition, the
    /// definition is returned alone.  If the resolved type is a type signture, the signature is 
    /// returned, but we also attempt to find its primary type definition.
    ///
    /// A caller must assume that either the definition or the signature may not be present.  At
    /// least one of them will always be present, though, otherwise the type is invalid and we 
    /// will throw.
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





    /// Resolves a `type_def_spec_token` into either its `type_def` or the `type_spec`'s signature
    auto get_type_def_or_signature(metadata::type_def_spec_token const& token) -> metadata::type_def_or_signature
    {
        if (token.is<metadata::type_def_token>())
            return token.as<metadata::type_def_token>();

        return row_from(token.as<metadata::type_spec_token>()).signature();
    }





    /// Creates arguments for signature instantiation from the type signature `signature_blob`
    ///
    /// The signature must be a type signature or must be uninitialized.  The `scope` must be
    /// non-null and, if the `signature_blob` is initialized, its scope must be the same as `scope`.
    /// The signature must be a `generic_instance` type signature; if it is not, the metadata is
    /// invalid.
    auto create_instantiator_arguments(metadata::database const* const scope,
                                       metadata::blob            const signature_blob) -> metadata::signature_instantiation_arguments
    {
        core::assert_not_null(scope);
        core::assert_true([&]{ return !signature_blob.is_initialized() || scope == &signature_blob.scope(); });

        if (!signature_blob.is_initialized())
            return metadata::signature_instantiation_arguments(scope);

        metadata::type_signature const signature(signature_blob.as<metadata::type_signature>());

        // We are only expecting to encounter base classes here, so we should have a GenericInst:
        if (signature.get_kind() != metadata::type_signature::kind::generic_instance)
            throw core::runtime_error(L"unexpected type provided for instantiation");

        return metadata::signature_instantiator::create_arguments(signature);
    }





    /// Implementation of `element_context_table_collection::get_or_create_table()`
    ///
    /// This class builds element tables for types.  We recurse in two passes:  a pre-insertion
    /// recursion and a post-insertion recursion.
    template <typename ContextTag>
    class recursive_table_builder
    {
    public:

        typedef element_context_traits<ContextTag>              traits_type;
        typedef typename traits_type::context_sequence_type     context_sequence_type;
        typedef typename traits_type::context_type              context_type;
        typedef typename traits_type::row_iterator_type         row_iterator_type;
        typedef typename traits_type::row_type                  row_type;
        typedef typename traits_type::signature_type            signature_type;
        typedef typename traits_type::token_type                token_type;
        typedef core::array_range<context_type const>           context_table_type;
        typedef element_context_table_collection<ContextTag>    collection_type;
        typedef metadata::signature_instantiator                instantiator_type;
        typedef metadata::signature_instantiation_arguments     instantiator_arguments_type;
        typedef element_context_table_storage::storage_lock     storage_type;

        /// Constructs a new `recursive_table_builder`
        ///
        /// The newly constructed instance will use `resolver` to resolve types, will call back into
        /// the `collection` to store the resulting table, and will store instantiated signatures in
        /// the `storage` signature storage buffer.  Call `get_or_create_table` to construct the
        /// table for the type.
        recursive_table_builder(metadata::type_resolver const* const resolver,
                                collection_type         const* const collection,
                                storage_type            const* const storage)
            : _resolver(resolver), _collection(collection), _storage(storage)
        {
            core::assert_not_null(resolver);
            core::assert_not_null(collection);
            core::assert_not_null(storage);
        }

        /// Gets an existing table or creates a new table containing the elements of `type`
        ///
        /// We never call `create_table` directly from within this class; instead, we always call
        /// this function to test whether the table has already been built.  No need to do expensive
        /// work twice.
        auto get_or_create_table(metadata::type_def_or_signature const& type) const -> context_table_type
        {
            core::assert_initialized(type);
            
            // First handle the 'get' of the 'get or create':
            auto const result(_storage->find_table<ContextTag>(type));
            if (result.first)
                return result.second;

            // Ok, we haven't created a table yet; let's create a new one.  First, resolve the type
            // definition and signature; if the type has no definition (e.g., it is a ByRef type)
            // then it has no elements, so we can allocate an empty table and return it:
            type_def_and_signature const def_and_sig(resolve_type_def_and_signature(*_resolver, type));
            if (!def_and_sig.has_type_def())
            {
                return _storage->allocate_table<ContextTag>(type, nullptr, nullptr);
            }

            // Otherwise, we have a definition, so let's build the table for it and return it:
            return create_table(def_and_sig);
        }

    private:

        /// Entry point for the recursive table creation process
        ///
        /// This is called by `get_or_create_table` when a new table needs to be created.
        auto create_table(type_def_and_signature const& type) const -> context_table_type
        {
            core::assert_true([&]{ return type.has_type_def(); });

            // We'll use different instantiators throughout the table creation process, but the
            // instantiator arguments are always the same.  They are also potentially expensive to
            // construct, so we'll construct them once here:
            auto const instantiator_arguments(create_instantiator_arguments(&type.type_def().scope(), type.signature()));

            // To start off, we get the instantiated contexts from the base class.  This process
            // recurses until it reaches the root type (Object) then iteratively builds the table as
            // it works its way down the hierarchy to the current type's base.
            //
            // We enumerate the inherited elements first so that we can correctly emulate overriding
            // and hiding, similar to what is done during reflection on a class at runtime.
            context_sequence_type new_table(get_or_create_table_with_base_elements(type, instantiator_arguments));

            core::size_type const inherited_element_count(core::convert_integer(new_table.size()));

            // Next, we enumerate the elements defined by 'type' itself, and insert them into the
            // table.  Due to overriding and hiding, these may not create new elements inthe table;
            // each may replace an element that was already present in the table.
            row_iterator_type const first_element(traits_type::begin_elements(type.type_def()));
            row_iterator_type const last_element (traits_type::end_elements  (type.type_def()));

            // The method instantiation source will be different for each element if we are 
            // instantiating methods, so we'll create a new instantiator for each element.  We only
            // have one type instantiation source, though, so we hoist it out of the loop:
            metadata::type_def_token const type_instantiation_source(get_type_instantiation_source(type.type_def()));

            std::for_each(first_element, last_element, [&](row_type const& element_row)
            {
                // Create the instantiator with the current type and method instantiation contexts:
                instantiator_type const instantiator(
                    &instantiator_arguments,
                    type_instantiation_source,
                    get_method_instantiation_source(element_row.token()));

                // Create the new context, insert it into the table, and perform post-recurse:
                context_type const new_context(create_element(element_row.token(), type, instantiator));

                traits_type::insert_element(*_resolver, new_table, new_context, inherited_element_count);

                post_insertion_recurse_with_context(new_context, new_table, inherited_element_count);
            });

            return _storage->allocate_table<ContextTag>(type.best_match(), new_table.data(), new_table.data() + new_table.size());
        }

        /// Gets a context sequence containing the elements inherited from the type's base type
        ///
        /// The `type` is the source type, not the base type.  Its base type will be located and its
        /// table will be obtained.  The elements in the table will be instantiated with the generic
        /// arguments provided by `instantiator_arguments`, if there are any.  This table is then
        /// returned.
        ///
        /// The returned table is always a new sequence that is cloned from the base type's table.
        /// If `type` has no base type or if its base type has no elements, an empty sequence is
        /// returned.
        auto get_or_create_table_with_base_elements(type_def_and_signature      const& type,
                                                    instantiator_arguments_type const& instantiator_arguments) const
            -> context_sequence_type
        {
            core::assert_true([&]{ return type.has_type_def(); });

            // The root type (Object) and interface types do not have a base type.  If the type does
            // not have a base type, we just return an empty sequence:
            metadata::type_def_ref_spec_token const base_token(row_from(type.type_def()).extends());
            if (!base_token.is_initialized())
                return context_sequence_type();

            // Resolve the base type and get (or create!) its element table.  This will recurse
            // until we reach the root type (Object) or a type whose table has already been built:
            context_table_type const base_table(get_or_create_table(get_type_def_or_signature(_resolver->resolve_type(base_token))));
            if (base_table.empty())
                return context_sequence_type();

            // Now that we have the element table for the base class, we must instantiate each of
            // its elements to replace any generic type variables with the arguments provided by our
            // caller.  Note that we need only to instantiate generic type variables.  We do not
            // originate any new element contexts here, so we do not need to annotate any generic
            // type variables.  Therefore, we do not provide the instantiator with type or method
            // sources.
            instantiator_type const instantiator(&instantiator_arguments);

            context_sequence_type new_table;
            new_table.reserve(base_table.size());

            std::transform(begin(base_table), end(base_table), std::back_inserter(new_table), [&](context_type const& c) -> context_type
            {
                auto const signature(c.element_signature(*_resolver));
                if (!signature.is_initialized() || !instantiator.would_instantiate(signature))
                    return c;

                return context_type(c.element(), type.signature(), instantiate(signature, instantiator));
            });

            return new_table;
        }

        /// Performs the post-insertion recursion for interface contexts
        ///
        /// We only need to perform post-insertion recursion for interface contexts.  For all other
        /// context types, no post-insertion recursion is required.  There is a function template 
        /// defined below that no-ops the rest of the context types.
        ///
        /// The post-insertion recursion allows us to walk the entire tree of interface
        /// implementations.  An interface can also implement N other interfaces, so walking the
        /// base class hierarchy is insufficient for interface classes.
        auto post_insertion_recurse_with_context(interface_context const& context,
                                                 context_sequence_type  & table,
                                                 core::size_type   const  inherited_element_count) const -> void
        {
            metadata::type_def_ref_spec_token const interface_token(context.element_row().interface());

            type_def_and_signature const interface_type(resolve_type_def_and_signature(*_resolver, interface_token));

            auto const instantiator_arguments(create_instantiator_arguments(
                &interface_type.type_def().scope(),
                interface_type.signature()));

            auto const interface_table(get_or_create_table(interface_type.best_match()));

            std::for_each(begin(interface_table), end(interface_table), [&](interface_context new_context)
            {
                instantiator_type const instantiator(
                    &instantiator_arguments,
                    get_type_instantiation_source(interface_type.type_def()));

                signature_type const signature(new_context.element_signature(*_resolver));
                if (signature.is_initialized() && instantiator.would_instantiate(signature))
                    new_context = create_element(
                        new_context.element(),
                        resolve_type_def_and_signature(*_resolver, new_context.element_row().parent()),
                        instantiator);

                traits_type::insert_element(*_resolver, table, new_context, inherited_element_count);
                post_insertion_recurse_with_context(new_context, table, inherited_element_count);
            });
        }

        /// Function template to no-op the post-insertion recursion for non-interface context types
        template <typename ContextType>
        auto post_insertion_recurse_with_context(ContextType const&, context_sequence_type&, core::size_type) const -> void
        {
            return;
        }

        /// Creates an element for insertion into a table
        ///
        /// The `token` identifies the element to be inserted.  The element is resolved, its
        /// signature is obtained, and it is instantiated via `instantiator` if instantiation is
        /// required.
        auto create_element(token_type             const& token,
                            type_def_and_signature const& instantiating_type,
                            instantiator_type      const& instantiator) const -> context_type
        {
            core::assert_initialized(token);

            metadata::blob const signature_blob(traits_type::get_signature(*_resolver, token));
            if (!signature_blob.is_initialized())
                return context_type(token);

            signature_type const signature(signature_blob.as<signature_type>());

            if (!instantiator.would_instantiate(signature))
                return context_type(token);

            return context_type(token, instantiating_type.best_match(), instantiate(signature, instantiator));
        }

        /// Instantiates the `signature` via `instantiator`, storing the result in `_storage`
        template <typename Signature>
        auto instantiate(Signature const& signature, instantiator_type const& instantiator) const -> core::const_byte_range
        {
            core::assert_initialized(signature);
            core::assert_true([&]{ return instantiator.would_instantiate(signature); });

            Signature const& instantiation(instantiator.instantiate(signature));
            return _storage->allocate_signature(instantiation.begin_bytes(), instantiation.end_bytes());
        }

        /// Gets the method instantiatiation source to be used when constructing an instantiator
        ///
        /// This function returns an uninitialized `method_def_token` if the source `token` is not
        /// initialized or if it does not have generic parameters.  Otherwise, the token is returned
        /// unchanged.
        ///
        /// There is also a function template that handles non-`method_def` tokens and no-ops them.
        static auto get_method_instantiation_source(metadata::method_def_token const& token) -> metadata::method_def_token
        {
            if (!token.is_initialized())
                return metadata::method_def_token();

            if (!has_generic_params(token))
                return metadata::method_def_token();

            return token;
        }

        /// Function template to no-op the getting of an instantiation source for non-method tokens
        template <typename Token>
        static auto get_method_instantiation_source(Token const&) -> metadata::method_def_token
        {
            return metadata::method_def_token();
        }

        /// Gets the type instantiation source to be used when constructing an instantiator
        ///
        /// This function returns an uninitialized `type_def_token` if the source `token` is not
        /// initialized or if it does not have generic parameters.  Otherwise, the token is returned
        /// unchanged.
        ///
        /// This function only accepts `type_def_token` tokens because we will always have a type
        /// for this check:  it is always the owning type whose elements are being enumerated.
        static auto get_type_instantiation_source(metadata::type_def_token const& token) -> metadata::type_def_token
        {
            if (!token.is_initialized())
                return metadata::type_def_token();

            if (!has_generic_params(token))
                return metadata::type_def_token();

            return token;
        }

        /// Tests whether a type or method has generic parameters
        static auto has_generic_params(metadata::type_or_method_def_token const& token) -> bool
        {
            core::assert_initialized(token);

            metadata::generic_param_row_iterator_pair const parameters(metadata::find_generic_params_range(token));
            return parameters.first != parameters.second;
        }

        core::checked_pointer<metadata::type_resolver const> _resolver;
        core::checked_pointer<collection_type const>         _collection;
        core::checked_pointer<storage_type const>            mutable _storage;
    };

    /// A `recursive_table_builder` factory that deduces the context type
    template <typename ContextTag>
    auto create_recursive_table_builder(metadata::type_resolver                      const* const resolver,
                                        element_context_table_collection<ContextTag> const* const collection,
                                        element_context_table_storage::storage_lock  const* const storage)
        -> recursive_table_builder<ContextTag>
    {
        return recursive_table_builder<ContextTag>(resolver, collection, storage);
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
    auto element_context_table_collection<ContextTag>::operator=(element_context_table_collection&& other)
        -> element_context_table_collection&
    {
        core::assert_initialized(other);

        _resolver = other._resolver;
        _storage  = other._storage;

        other._resolver.reset();
        other._storage.reset();

        return *this;
    }

    template <typename ContextTag>
    auto element_context_table_collection<ContextTag>::get_or_create_table(metadata::type_def_or_signature const& type) const
        -> context_table_type
    {
        core::assert_initialized(*this);

        // Obtain a lock on the storage for the duration of the table lookup or creation.  In theory
        // we could do this in two stages and lock separately for each stage, but it is unlikely
        // that this lock will be contentious.
        auto const storage(_storage.get()->lock());

        return create_recursive_table_builder(_resolver.get(), this, &storage).get_or_create_table(type);
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
