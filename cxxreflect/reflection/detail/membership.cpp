
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/membership.hpp"
#include "cxxreflect/reflection/detail/type_resolution.hpp"





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

        explicit type_def_and_signature(metadata::blob const& signature)
            : _signature(signature)
        {
            core::assert_initialized(signature);
        }

        type_def_and_signature(metadata::type_def_token const& token, metadata::blob const& signature)
            : _type_def(token), _signature(signature)
        {
            core::assert_initialized(token);
            core::assert_initialized(signature);
        }

        auto type_def()      const -> metadata::type_def_token const& { return _type_def;                   }
        auto has_type_def()  const -> bool                            { return _type_def.is_initialized();  }
        auto signature()     const -> metadata::blob const&           { return _signature;                  }
        auto has_signature() const -> bool                            { return _signature.is_initialized(); }

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

        // If we resolved the type to a TypeDef, it has no signature so we may return it directly:
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
                metadata::blob(signature));
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
            return type_def_and_signature(metadata::blob(signature));
        }
        default:
            core::assert_not_yet_implemented();
        }
    }





    /// Resolves a `type_def_spec_token` into either its `type_def` or the `type_spec`'s signature
    auto get_type_def_or_signature(metadata::type_def_spec_token const& token) -> metadata::type_def_or_signature
    {
        if (token.is<metadata::type_def_token>())
            return token.as<metadata::type_def_token>();

        return row_from(token.as<metadata::type_spec_token>()).signature();
    }





    /// Tests whether a type or method has generic parameters
    auto has_generic_params(metadata::type_or_method_def_token const& token) -> bool
    {
        core::assert_initialized(token);

        return !metadata::find_generic_params(token).empty();
    }





    /// Creates arguments for signature instantiation from the type signature `signature_blob`
    ///
    /// The signature must be a type signature or must be uninitialized.  The `scope` must be
    /// non-null and, if the `signature_blob` is initialized, its scope must be the same as `scope`.
    /// The signature must be a `generic_instance` type signature; if it is not, the metadata is
    /// invalid.
    auto create_instantiator_arguments(metadata::database const* const scope,
                                       type_def_and_signature    const type) -> metadata::signature_instantiation_arguments
    {
        core::assert_not_null(scope);
        core::assert_true([&]{ return !type.has_signature() || scope == &type.signature().scope(); });

        if (!type.has_signature())
            return metadata::signature_instantiation_arguments(scope);

        metadata::type_signature const signature(type.signature().as<metadata::type_signature>());

        // We are only expecting to encounter base classes here, so we should have a GenericInst:
        if (signature.get_kind() != metadata::type_signature::kind::generic_instance)
            throw core::runtime_error(L"unexpected type provided for instantiation");

        return metadata::signature_instantiator::create_arguments(signature, type.type_def());
    }





    /// Gets the method instantiatiation source to be used when constructing an instantiator
    ///
    /// This function returns an uninitialized `method_def_token` if the source `token` is not
    /// initialized or if it does not have generic parameters.  Otherwise, the token is returned
    /// unchanged.
    ///
    /// There is also a function template that handles non-`method_def` tokens and no-ops them.
    auto get_method_instantiation_source(metadata::method_def_token const& token) -> metadata::method_def_token
    {
        if (!token.is_initialized())
            return metadata::method_def_token();

        if (!has_generic_params(token))
            return metadata::method_def_token();

        return token;
    }

    /// Function template to no-op the getting of an instantiation source for non-method tokens
    template <typename Token>
    auto get_method_instantiation_source(Token const&) -> metadata::method_def_token
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
    auto get_type_instantiation_source(metadata::type_def_token const& token) -> metadata::type_def_token
    {
        if (!token.is_initialized())
            return metadata::type_def_token();

        if (!has_generic_params(token))
            return metadata::type_def_token();

        return token;
    }





    auto compute_slot_for(metadata::method_def_token const& method, metadata::type_def_or_signature const& type)
        -> method_traits::override_slot
    {
        core::assert_initialized(method);

        metadata::method_def_row   const method_row      (row_from(method));
        metadata::method_signature const method_signature(method_row.signature().as<metadata::method_signature>());
        metadata::type_def_token   const defining_type   (metadata::find_owner_of_method_def(method).token());

        auto const implementations  (metadata::find_method_impls(defining_type));
        auto const implementation_it(core::find_if(implementations, [&](metadata::method_impl_row const& r)
        {
            return r.method_body() == method;
        }));

        if (implementation_it == end(implementations))
            return method_traits::override_slot();

        metadata::method_impl_row const implementation(*implementation_it);

        auto const overridden_method(implementation.method_declaration());
        switch (overridden_method.table())
        {
        case metadata::table_id::method_def:
        {
            metadata::method_def_token const real_overridden_method(overridden_method.as<metadata::method_def_token>());
            return method_traits::override_slot(
                resolve_type(metadata::find_owner_of_method_def(real_overridden_method).token()),
                real_overridden_method);
        }
        case metadata::table_id::member_ref:
        {
            metadata::member_ref_token        const real_overridden_method  (overridden_method.as<metadata::member_ref_token>());
            metadata::member_ref_parent_token const overridden_method_parent(row_from(real_overridden_method).parent());

            return method_traits::override_slot(
                resolve_type(overridden_method_parent.as<metadata::type_ref_spec_token>()),
                loader_context::from(method.scope()).resolve_member(real_overridden_method).as<metadata::method_def_token>());
        }
        default:
        {
            // The other two scopes--module_ref and method_def--are not reachable in this context
            core::assert_unreachable();
        }
        }
    }





    template <member_kind MemberTag>
    class built_table
    {
    public:
        typedef typename member_table_iterator_generator<MemberTag>::type       iterator_type;
        typedef typename member_table_iterator_generator<MemberTag>::range_type range_type;

        built_table()
        {
        }

        explicit built_table(core::const_byte_range const range_, bool const is_instantiated_)
            : _range(range_), _is_instantiated(is_instantiated_)
        {
        }

        auto byte_range()      const -> core::const_byte_range const& { return _range;           }
        auto is_instantiated() const -> bool                          { return _is_instantiated; }

        auto iterator_range() const -> range_type
        {
            core::size_type const stride(_is_instantiated.get()
                ? sizeof(member_table_entry_with_instantiation)
                : sizeof(member_table_entry));

            return range_type(
                iterator_type(MemberTag, core::stride_iterator(_range.begin(), stride)),
                iterator_type(MemberTag, core::stride_iterator(_range.end(),   stride)));
        }

    private:

        core::const_byte_range        _range;
        core::value_initialized<bool> _is_instantiated;
    };





    template <member_kind MemberTag>
    class recursive_table_builder
    {
    public:

        typedef member_traits<MemberTag>                        traits_type;
        typedef typename traits_type::interim_sequence_type     interim_sequence_type;
        typedef typename traits_type::interim_type              interim_type;
        typedef typename traits_type::row_iterator_type         row_iterator_type;
        typedef typename traits_type::row_range_type            row_range_type;
        typedef typename traits_type::row_type                  row_type;
        typedef typename traits_type::signature_type            signature_type;
        typedef typename traits_type::token_type                token_type;
        typedef core::array_range<interim_type const>           member_table_type;
        typedef metadata::signature_instantiator                instantiator_type;
        typedef metadata::signature_instantiation_arguments     instantiator_arguments_type;
        typedef built_table<MemberTag>                          internal_table;
        typedef member_table_entry_facade<MemberTag>            entry_type;

        recursive_table_builder(metadata::type_resolver const* const resolver, membership_storage* const storage)
            : _resolver(resolver), _storage(storage)
        {
            core::assert_not_null(resolver);
            core::assert_not_null(storage);
        }

        /// Gets an existing table or creates a new table containing the elements of `type`
        auto create_table(metadata::type_def_or_signature const& type) const -> internal_table
        {
            core::assert_initialized(type);

            // Test to see whether we've already created the table; if we have we can return now:
            auto& membership(_storage->get_membership(type).context(core::internal_key()));
            if (membership.get_state().is_set(membership_context::primary_state_flag_for(MemberTag)))
            {
                return internal_table(
                    membership.get_range<MemberTag>().value(),
                    membership.get_state().is_set(membership_context::instantiated_state_flag_for(MemberTag)));
            }

            // Resolve the type to its definition and signature.  If resolution was successful and
            // we found a type definition, compute a new table for the type and return it:
            type_def_and_signature const def_and_sig(resolve_type_def_and_signature(*_resolver, type));
            if (def_and_sig.has_type_def())
            {
                return create_table_for_type(def_and_sig);
            }

            // If we don't have a signature or a definition, we can simply return an empty table:
            if (!def_and_sig.has_signature())
            {
                membership.set_table<MemberTag>(core::const_byte_range(), false);
                return internal_table();
            }

            // If we have just a signature, we may have a generic type parameter (variable).  If so,
            // we fabricate a table for the generic type parameter using its constraints.
            metadata::type_signature const signature(def_and_sig.signature().as<metadata::type_signature>());
            if (signature.get_kind() == metadata::type_signature::kind::variable)
            {
                // We should never have an unannotated variable at this point:
                core::assert_true([&]() -> bool
                {
                    metadata::element_type const signature_type(signature.get_element_type());
                    return signature_type == metadata::element_type::annotated_mvar
                        || signature_type == metadata::element_type::annotated_var;
                });

                metadata::generic_param_token const variable_token(metadata::find_generic_param(
                    signature.variable_context(),
                    signature.variable_number()).token());

                return create_table_for_generic_parameter(type, variable_token);
            }

            // Otherwise, this is a signature for which we do not need to create a table:
            membership.set_table<MemberTag>(core::const_byte_range(), false);
            return internal_table();
        }

    private:

        /// Entry point for the recursive table creation process
        ///
        /// This is called by `get_or_create_table` when a new table needs to be created.  This
        /// creates a new table for an ordinary type (a type definition or signature).  The type
        /// must have an associated definition (i.e., `type.has_type_def()` must be `true`).
        auto create_table_for_type(type_def_and_signature const& type) const -> internal_table
        {
            core::assert_true([&]{ return type.has_type_def(); });

            // We'll use different instantiators throughout the table creation process, but the
            // instantiator arguments are always the same.  They are also potentially expensive to
            // construct, so we'll construct them once here:
            auto const instantiator_arguments(create_instantiator_arguments(&type.type_def().scope(), type));

            interim_sequence_type new_table;

            // To start off, we get the instantiated contexts from the base class.  This process
            // recurses until it reaches the root type (Object) then iteratively builds the table as
            // it works its way down the hierarchy to the current type's base.
            //
            // Note that the root type (Object) will not have a base type.
            //
            // We enumerate the inherited elements first so that we can correctly emulate overriding
            // and hiding, similar to what is done during reflection on a class at runtime.
            metadata::type_def_ref_spec_token const base_token(row_from(type.type_def()).extends());
            if (base_token.is_initialized())
            {
                new_table = get_or_create_table_with_base_elements(
                    get_type_def_or_signature(_resolver->resolve_type(base_token)),
                    type.signature(),
                    instantiator_arguments);
            }

            core::size_type inherited_element_count(core::convert_integer(new_table.size()));

            // Next, we enumerate the elements defined by 'type' itself, and insert them into the
            // table.  Due to overriding and hiding, these may not create new elements inthe table;
            // each may replace an element that was already present in the table.
            row_range_type const members(traits_type::get_members(type.type_def()));

            // The method instantiation source will be different for each element if we are 
            // instantiating methods, so we'll create a new instantiator for each element.  We only
            // have one type instantiation source, though, so we hoist it out of the loop:
            metadata::type_def_token const type_instantiation_source(get_type_instantiation_source(type.type_def()));

            core::for_all(members, [&](row_type const& element_row)
            {
                // Create the instantiator with the current type and method instantiation contexts:
                instantiator_type const instantiator(
                    &instantiator_arguments,
                    type_instantiation_source,
                    get_method_instantiation_source(element_row.token()));

                // Create the new context, insert it into the table, and perform post-recurse:
                interim_type const new_context(create_element(element_row.token(), type, instantiator, traits_type()));

                inherited_element_count = traits_type::insert_member(new_table, new_context, inherited_element_count);

                post_insertion_recurse_with_context(new_context, new_table, inherited_element_count, traits_type());
            });

            return create_internal_table(type.best_match(), new_table);
        }

        auto create_internal_table(metadata::type_def_or_signature const& type, interim_sequence_type const& new_table) const
            -> internal_table
        {
            if (new_table.empty())
                return internal_table();

            bool const use_instantiated_contexts(std::any_of(begin(new_table), end(new_table), [&](interim_type const& x)
            {
                return x.instantiated_signature().is_initialized();
            }));

            auto& membership(_storage->get_membership(type).context(core::internal_key()));
            if (use_instantiated_contexts)
            {
                std::vector<member_table_entry_with_instantiation> entries(new_table.size());
                std::transform(begin(new_table), end(new_table), begin(entries), [&](interim_type const& x) { return x; });

                membership.set_table<MemberTag>(_storage->allocate_table(core::const_byte_range(
                    reinterpret_cast<core::const_byte_iterator>(entries.data()),
                    reinterpret_cast<core::const_byte_iterator>(entries.data() + entries.size())), core::internal_key()), true);

                return internal_table(membership.get_range<MemberTag>().value(), true);
            }
            else
            {
                // TODO Rework allocation scheme here
                std::vector<member_table_entry> entries(new_table.size());
                std::transform(begin(new_table), end(new_table), begin(entries), [&](interim_type const& x)
                {
                    return member_table_entry(x.member_token());
                });

                membership.set_table<MemberTag>(_storage->allocate_table(core::const_byte_range(
                    reinterpret_cast<core::const_byte_iterator>(entries.data()),
                    reinterpret_cast<core::const_byte_iterator>(entries.data() + entries.size())), core::internal_key()), false);

                return internal_table(membership.get_range<MemberTag>().value(), false);
            }
        }

        /// Gets or creates the element table for base type and clones and instantiates the elements
        ///
        /// The `base_type` is the base type for which to obtain an element table.  The table is
        /// computed, then the elements are instantiated with the provided `instantiator_arguments`,
        /// if there are any.  The resulting table is then returned.
        ///
        /// The returned table is always a new sequence that is cloned from the base type's table.
        /// Note that this function is called both for ordinary types and for generic parameters.
        ///
        /// The derived type signature may be uninitialized.
        auto get_or_create_table_with_base_elements(metadata::type_def_or_signature const& base_type,
                                                    metadata::blob                  const& derived_type_signature,
                                                    instantiator_arguments_type     const& instantiator_arguments) const
            -> interim_sequence_type
        {
            core::assert_initialized(base_type);

            auto const base_table(create_table(base_type).iterator_range());
            if (base_table.empty())
                return interim_sequence_type();

            // Now that we have the element table for the base class, we must instantiate each of
            // its elements to replace any generic type variables with the arguments provided by our
            // caller.  Note that we need only to instantiate generic type variables.  We do not
            // originate any new element contexts here, so we do not need to annotate any generic
            // type variables.  Therefore, we do not provide the instantiator with type or method
            // sources.
            instantiator_type const instantiator(&instantiator_arguments);

            interim_sequence_type new_table(base_table.size());
            core::transform_all(base_table, begin(new_table), [&](entry_type const* const c) -> interim_type
            {
                auto const signature(c->member_signature());
                if (!signature.is_initialized() || !instantiator.would_instantiate(signature))
                {
                    if (c->has_instantiating_type())
                        return interim_type(c->member_token(), c->instantiating_type(), c->instantiated_signature());

                    return interim_type(member_table_entry(c->member_token()));
                }

                return interim_type(c->member_token(), derived_type_signature, instantiate(signature, instantiator));
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
        auto post_insertion_recurse_with_context(interim_type          const& context,
                                                 interim_sequence_type      & table,
                                                 core::size_type              inherited_element_count,
                                                 member_traits<member_kind::interface_>) const -> void
        {
            entry_type const& typed_context(*entry_type::from(&context));

            type_def_and_signature const interface_type(resolve_type_def_and_signature(
                *_resolver,
                interface_traits::get_interface_type(typed_context.member_token())));

            core::assert_true([&]{ return interface_type.has_type_def(); });

            // First, get the set of interfaces implemented by this interface type:
            auto const interface_table(create_table(interface_type.best_match()).iterator_range());

            // We instantiate each interface from the context of the interface:
            instantiator_arguments_type const instantiator_arguments(create_instantiator_arguments(
                &interface_type.type_def().scope(),
                interface_type));

            instantiator_type const instantiator(
                &instantiator_arguments,
                get_type_instantiation_source(interface_type.type_def()));

            // Iterate over the interfaces and insert each of them into the table.  The insertion
            // function eliminates duplicates as we insert new elements.  Note that this process is
            // recursive:  for each interface that we touch, we call this function again to resolve
            // the interfaces that it implements.  This allows us to compute the complete set of
            // interfaces.
            core::for_all(interface_table, [&](entry_type const* new_entry)
            {
                metadata::type_def_token const parent(new_entry->member_token().is<metadata::interface_impl_token>()
                    ? row_from(new_entry->member_token().as<metadata::interface_impl_token>()).parent()
                    : metadata::type_def_token());

                signature_type const signature(new_entry->member_signature());
                if (signature.is_initialized() && instantiator.would_instantiate(signature))
                {
                    interim_type const new_entry(create_element(
                        new_entry->member_token(),
                        resolve_type_def_and_signature(*_resolver, parent),
                        instantiator,
                        traits_type()));

                    inherited_element_count = traits_type::insert_member(table, new_entry, inherited_element_count);

                    // TODO This should check whether insert_element inserted a new element; if it did
                    // not, we don't need to recurse.
                    post_insertion_recurse_with_context(new_entry, table, inherited_element_count, traits_type());
                }
                else
                {
                    inherited_element_count = traits_type::insert_member(table, new_entry->realize(), inherited_element_count);

                    // TODO This should check whether insert_element inserted a new element; if it did
                    // not, we don't need to recurse.
                    post_insertion_recurse_with_context(new_entry->realize(), table, inherited_element_count, traits_type());
                }
            });
        }

        /// Function template to no-op the post-insertion recursion for non-interface context types
        ///
        /// We only need to recurse post-insertion for interfaces; all other element types no-op
        /// this phase.  To make everything compile, we use this catch-all function template.
        template <typename TraitsType>
        auto post_insertion_recurse_with_context(interim_type const&, interim_sequence_type&, core::size_type, TraitsType) const -> void
        {
            return;
        }





        /// Entry point for the recursive table creation process
        ///
        /// This is called by `get_or_create_table` when a new table needs to be created.  This
        /// creates a new table for a generic parameter (a type or method variable).
        auto create_table_for_generic_parameter(metadata::type_def_or_signature const& type,
                                                metadata::generic_param_token   const& param_token) const -> internal_table
        {
            core::assert_initialized(type);
            core::assert_initialized(param_token);

            auto const constraints(metadata::find_generic_param_constraints(param_token));

            // First, enumerate this generic parameter's constraints and look to see if any of them
            // is a class type constraint (i.e., not an interface type).  If there is a class type
            // constraint, then we will use this type as the base type for the generic parameter
            // when computing its element table.
            auto const base_constraint(std::find_if(begin(constraints), end(constraints), [&](metadata::generic_param_constraint_row const& c)
            {
                type_def_and_signature const resolved_type(resolve_type_def_and_signature(*_resolver, c.constraint()));
                if (!resolved_type.has_type_def())
                    return false;

                metadata::type_flags const flags(row_from(resolved_type.type_def()).flags());
                if (flags.with_mask(metadata::type_attribute::class_semantics_mask) != metadata::type_attribute::class_)
                    return false;

                return true;
            }));

            // Determine which type to use as a base type.  There are three possibilities:
            metadata::type_def_or_signature const base_type([&]() -> metadata::type_def_or_signature
            {
                // If we found a non-interface type constraint, we use that constraint as the base
                // type.  A type may have at most one non-interface type constraint.
                if (base_constraint != end(constraints))
                    return resolve_type_def_and_signature(*_resolver, base_constraint->constraint()).best_match();

                // If the type is constrained to be a non-nullable value type, we use ValueType as
                // the base type for the object:
                bool const is_constrained_as_value_type(row_from(param_token)
                    .flags()
                    .with_mask(metadata::generic_parameter_attribute::special_constraint_mask)
                    .is_set(metadata::generic_parameter_attribute::non_nullable_value_type_constraint));

                if (is_constrained_as_value_type)
                    return _resolver->resolve_fundamental_type(metadata::element_type::value_type);

                // Finally, if neither of the above cases selected a base type, we use Object, the
                // one base type to rule them all:
                return _resolver->resolve_fundamental_type(metadata::element_type::object);
            }());

            // When we get the base type table, we never have any arguments with which to instantiate
            // the base type.  Only after we instantiate the generic type will we have arguments with
            // which we will instantiate the elements, and at that point, we'll be using the other
            // create table path (for ordinary types).
            instantiator_arguments_type const empty_arguments(&base_type.scope());

            // We construct a new table, then recursively process any constraints, allowing us to
            // correctly generate interface sets.  The process_generic_parameter_constraints does
            // not itself recurse, but it sets up the context that is required to share the same
            // post-insertion logic used by the other create table path.
            interim_sequence_type new_table(get_or_create_table_with_base_elements(base_type, metadata::blob(), empty_arguments));

            process_generic_parameter_constraints(new_table, constraints, traits_type());

            return create_internal_table(type, new_table);
        }

        /// Processes the generic parameters for potential insertion into a context table
        ///
        /// We only need to perform this step for interface contexts.  For all other contexts, the
        /// only elements that go into the table are those inherited by the base type that we
        /// select.
        auto process_generic_parameter_constraints(interim_sequence_type                             & table,
                                                   metadata::generic_param_constraint_row_range const& constraints,
                                                   member_traits<member_kind::interface_>) const -> void
        {
            if (constraints.empty())
                return;

            metadata::type_or_method_def_token const parent(row_from(begin(constraints)->parent()).parent());

            // First, we need to compute the instantiation sources for the constraints.  If this is
            // a method variable, we'll have both method and type sources; otherwise we will only
            // have a type source.  In any case, we will always have a type source.
            metadata::type_def_token   original_type_instantiation_source;
            metadata::method_def_token original_method_instantiation_source;

            if (parent.is<metadata::type_def_token>())
            {
                original_type_instantiation_source = parent.as<metadata::type_def_token>();
            }
            else if (parent.is<metadata::method_def_token>())
            {
                original_method_instantiation_source = parent.as<metadata::method_def_token>();
                original_type_instantiation_source   = find_owner_of_method_def(original_method_instantiation_source).token();
            }
            else
            {
                core::assert_unreachable();
            }

            core::assert_initialized(original_type_instantiation_source);

            // We'll never have any instantiator arguments at this point; we only need our
            // instantiator to annotate variables, so we create a new instantiator with an empty
            // arguments sequence:
            instantiator_arguments_type const empty_arguments(&original_type_instantiation_source.scope());

            instantiator_type const instantiator(
                &empty_arguments,
                get_type_instantiation_source(original_type_instantiation_source),
                get_method_instantiation_source(original_method_instantiation_source));

            // When we create the elements, we need to track the instantiating type, at least for
            // elements that end up being instantiated:
            type_def_and_signature const resolved_type_source(resolve_type_def_and_signature(
                *_resolver,
                original_type_instantiation_source));

            // Iterate over the interfaces and insert each of them into the table.  We skip any non-
            // interface constraints.  There should be at most one such constraint, and it indicates
            // the base type from which the generic argument must derive.  Note that this process is
            // recursive:  for each interface that we touch, we back into the post-insertion element
            // recursion, just as we do for ordinary type contexts.
            core::for_all(constraints, [&](metadata::generic_param_constraint_row const& c)
            {
                type_def_and_signature const resolved_constraint_type(resolve_type_def_and_signature(*_resolver, c.constraint()));
                if (!resolved_constraint_type.has_type_def())
                    return; // TODO Check correctness?

                metadata::type_flags const flags(row_from(resolved_constraint_type.type_def()).flags());
                if (flags.with_mask(metadata::type_attribute::class_semantics_mask) != metadata::type_attribute::interface_)
                    return;

                // Create the new context, insert it into the table, and perform post-recurse:
                interim_type const new_context(create_element(c.token(), resolved_type_source, instantiator, traits_type()));

                traits_type::insert_member(table, new_context, 0);

                post_insertion_recurse_with_context(new_context, table, 0, traits_type());
            });
        }
        
        /// Function template to no-op the constraint processing for non-interface context types
        ///
        /// We only need to process constraints for interfaces; all other element types no-op
        /// this phase.  To make everything compile, we use this catch-all function template.
        template <typename T, typename U, typename V>
        auto process_generic_parameter_constraints(T const&, U const&, V const&) const -> void
        {
            return;
        }





        /// Creates an element for insertion into a table
        ///
        /// The `token` identifies the element to be inserted.  The element is resolved, its
        /// signature is obtained, and it is instantiated via `instantiator` if instantiation is
        /// required.
        template <typename TraitsType>
        auto create_element(token_type             const& token,
                            type_def_and_signature const& instantiating_type,
                            instantiator_type      const& instantiator,
                            TraitsType) const -> interim_type
        {
            core::assert_initialized(token);

            metadata::blob const signature_blob(traits_type::get_signature(token));
            if (!signature_blob.is_initialized())
                return interim_type(member_table_entry(token));

            signature_type const signature(signature_blob.as<signature_type>());

            if (!instantiator.would_instantiate(signature))
                return interim_type(token, instantiating_type.best_match(), core::const_byte_range());

            return interim_type(token, instantiating_type.best_match(), instantiate(signature, instantiator));
        }

        /// Instantiates the `signature` via `instantiator`, storing the result in `_storage`
        template <typename Signature>
        auto instantiate(Signature const& signature, instantiator_type const& instantiator) const -> core::const_byte_range
        {
            core::assert_initialized(signature);
            core::assert_true([&]{ return instantiator.would_instantiate(signature); });

            Signature const& instantiation(instantiator.instantiate(signature));
            return _storage->allocate_signature(core::const_byte_range(instantiation.begin_bytes(), instantiation.end_bytes()), core::internal_key());
        }

        core::checked_pointer<metadata::type_resolver const>         _resolver;
        core::checked_pointer<membership_storage>            mutable _storage;
    };

    template <member_kind MemberTag>
    auto internal_create_table(membership_storage& storage, metadata::type_def_or_signature const& type) -> void
    {
        recursive_table_builder<MemberTag>(&loader_context::from(type.scope()), &storage).create_table(type);
    }

} } } }





namespace cxxreflect { namespace reflection { namespace detail {

    auto event_traits::get_members(metadata::type_def_token const& type) -> row_range_type
    {
        core::assert_initialized(type);

        return metadata::find_events(type);
    }

    auto event_traits::get_signature(token_type const& member) -> metadata::blob
    {
        core::assert_initialized(member);

        metadata::type_resolver const& resolver(loader_context::from(member.scope()));

        metadata::type_def_ref_spec_token const original_type(row_from(member).type());
        metadata::type_def_spec_token     const resolved_type(resolver.resolve_type(original_type));

        // If the type is a TypeDef, it has no distinct signature so we can simply return an empty
        // signature here:
        if (resolved_type.is<metadata::type_def_token>())
            return metadata::blob();

        // Otherwise, we have a TypeSpec, so we should return its signature:
        core::assert_true([&]{ return resolved_type.is<metadata::type_spec_token>(); });

        return row_from(resolved_type.as<metadata::type_spec_token>()).signature();
    }

    auto event_traits::insert_member(interim_sequence_type& member_table,
                                     interim_type    const& new_member,
                                     core::size_type const  inherited_member_count) -> core::size_type
    {
        core::assert_initialized(new_member);

        // TODO Do we need to handle hiding or overriding for events?
        member_table.push_back(new_member);
        return inherited_member_count;
    }





    auto field_traits::get_members(metadata::type_def_token const& type) -> row_range_type
    {
        core::assert_initialized(type);

        return row_range_type(
            row_iterator_type(&type.scope(), row_from(type).first_field().index()),
            row_iterator_type(&type.scope(), row_from(type).last_field().index()));
    }

    auto field_traits::get_signature(token_type const& member) -> metadata::blob
    {
        core::assert_initialized(member);

        return row_from(member).signature();
    }

    auto field_traits::insert_member(interim_sequence_type& member_table,
                                     interim_type    const& new_member,
                                     core::size_type const  inherited_member_count) -> core::size_type
    {
        core::assert_initialized(new_member);

        // TODO Do we need to handle hiding or overriding for fields?
        member_table.push_back(new_member);
        return inherited_member_count;
    }





    auto interface_traits::get_members(metadata::type_def_token const& type) -> row_range_type
    {
        core::assert_initialized(type);

        return metadata::find_interface_impls(type);
    }

    auto interface_traits::get_signature(token_type const& member) -> metadata::blob
    {
        core::assert_initialized(member);

        metadata::type_resolver const& resolver(loader_context::from(member.scope()));

        metadata::type_def_ref_spec_token const original_type(get_interface_type(member));
        metadata::type_def_spec_token     const resolved_type(resolver.resolve_type(original_type));

        // If the type is a TypeDef, it has no distinct signature so we can simply return an empty
        // signature here:
        if (resolved_type.is<metadata::type_def_token>())
            return metadata::blob();

        // Otherwise, we have a TypeSpec, so we should return its signature:
        core::assert_true([&]{ return resolved_type.is<metadata::type_spec_token>(); });

        return row_from(resolved_type.as<metadata::type_spec_token>()).signature();
    }

    auto interface_traits::insert_member(interim_sequence_type& member_table,
                                         interim_type    const& new_member,
                                         core::size_type const  inherited_member_count) -> core::size_type
    {
        core::assert_initialized(new_member);

        metadata::type_resolver const& resolver(loader_context::from(new_member.member_token().scope()));

        interface_table_entry         const& typed_new_member(*interface_table_entry::from(&new_member));
        metadata::type_def_spec_token const  new_if(resolver.resolve_type(get_interface_type(typed_new_member.member_token())));

        // Iterate over the interface table and see if it already contains the new interface.  This
        // can happen if two classes in a class hierarchy both implement an interface.  If there are
        // two classes that implement an interface, we keep the most derived one.
        auto const it(std::find_if(begin(member_table), end(member_table), [&](interim_type const& old_member) -> bool
        {
            interface_table_entry const& typed_old_member(*interface_table_entry::from(&old_member));
            metadata::type_def_spec_token const old_if(resolver.resolve_type(get_interface_type(typed_old_member.member_token())));

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
            auto const old_signature(typed_old_member.member_signature());
            auto const new_signature(typed_new_member.member_signature());

            metadata::signature_comparer const compare(&resolver);

            return compare(old_signature, new_signature);
        }));

        it == end(member_table)
            ? (void)member_table.push_back(new_member)
            : (void)(*it = new_member);

        return inherited_member_count;
    }

    auto interface_traits::get_interface_type(token_type const& parent)
            -> metadata::type_def_ref_spec_token
    {
        core::assert_initialized(parent);

        if (parent.is<metadata::interface_impl_token>())
        {
            return row_from(parent.as<metadata::interface_impl_token>()).interface_();
        }
        else if (parent.is<metadata::generic_param_constraint_token>())
        {
            return row_from(parent.as<metadata::generic_param_constraint_token>()).constraint();
        }
        else
        {
            core::assert_unreachable();
        }
    }





    method_traits::override_slot::override_slot()
    {
    }

    method_traits::override_slot::override_slot(metadata::type_def_or_signature const& type,
                                                metadata::method_def_token      const& method)
        : _declaring_type (type  ),
          _declared_method(method)
    {
        core::assert_initialized(type);
        core::assert_initialized(method);
    }

    auto method_traits::override_slot::declaring_type() const -> metadata::type_def_or_signature const&
    {
        core::assert_initialized(*this);

        return _declaring_type;
    }

    auto method_traits::override_slot::declared_method() const -> metadata::method_def_token const&
    {
        core::assert_initialized(*this);

        return _declared_method;
    }

    auto method_traits::override_slot::is_initialized() const -> bool
    {
        return _declaring_type.is_initialized() && _declared_method.is_initialized();
    }

    auto operator==(method_traits::override_slot const& lhs, method_traits::override_slot const& rhs) -> bool
    {
        if (lhs.is_initialized() != rhs.is_initialized())
            return false;

        if (!lhs.is_initialized())
            return true;

        if (lhs.declared_method() != rhs.declared_method())
            return false;

        if (lhs.declaring_type().is_blob() != rhs.declaring_type().is_blob())
            return false;

        if (!lhs.declaring_type().is_blob())
            return lhs.declaring_type().as_token() == rhs.declaring_type().as_token();

        metadata::signature_comparer const compare(&loader_context::from(lhs.declaring_type().scope()));
        return compare(
            lhs.declaring_type().as_blob().as<metadata::type_signature>(),
            rhs.declaring_type().as_blob().as<metadata::type_signature>());
    }
    
    auto method_traits::get_members(metadata::type_def_token const& type) -> row_range_type
    {
        core::assert_initialized(type);

        return row_range_type(
            row_iterator_type(&type.scope(), row_from(type).first_method().index()),
            row_iterator_type(&type.scope(), row_from(type).last_method().index()));
    }

    auto method_traits::get_signature(token_type const& member) -> metadata::blob
    {
        core::assert_initialized(member);

        return row_from(member).signature();
    }

    auto method_traits::insert_member(interim_sequence_type& member_table,
                                      interim_type    const& new_member,
                                      core::size_type const  inherited_member_count) -> core::size_type
    {
        core::assert_initialized(new_member);
        core::assert_true([&]{ return inherited_member_count <= member_table.size(); });

        metadata::type_resolver const& resolver(loader_context::from(new_member.member_token().scope()));

        method_table_entry         const& typed_new_member(*method_table_entry::from(&new_member));
        metadata::method_def_row   const new_method_def(row_from(typed_new_member.member_token()));
        metadata::method_signature const new_method_sig(typed_new_member.member_signature());

        // If the method occupies a new slot, it does not override any other method.  A static
        // method is always a new method.
        if (new_method_def.flags().with_mask(metadata::method_attribute::vtable_layout_mask) == metadata::method_attribute::new_slot ||
            new_method_def.flags().is_set(metadata::method_attribute::static_))
        {
            member_table.push_back(new_member);
            return inherited_member_count;
        }

        auto const table_begin(member_table.rbegin() + (member_table.size() - inherited_member_count));
        auto const table_end  (member_table.rend());

        // There are two ways that a new method may override a method from the base class:  it may
        // override by name and signature or it may override by slot (via the MethodImpl table).  We
        // must search for both possible overridden methods in the base table because we may override
        // by slot and hide by name and signature.  For example, consider:
        //
        //     ref struct B {
        //         virtual void F();
        //         virtual void G();
        //     };
        //
        //     ref struct D : B {
        //         virtual void G() = B::F;
        //     };
        //
        // Here, D::G overrides B::F, but when we process it we must also remove B::G from the table
        // otherwise there will be two methods with identical names and signatures and would thus be
        // indistinguishable during overload resolution.  *sigh*
        auto slot_override_it     (table_end);
        auto signature_override_it(table_end);

        for (auto table_it(table_begin); table_it != table_end; ++table_it)
        {
            interim_type               const& old_member(*table_it);
            method_table_entry         const& typed_old_member(*method_table_entry::from(&old_member));
            metadata::method_def_row   const  old_method_def(row_from(typed_old_member.member_token()));
            metadata::method_signature const  old_method_sig(typed_old_member.member_signature());

            // Note that by skipping nonvirtual methods, we also skip the name hiding feature.  We
            // do not hide any names by name or signature; we only hide overridden virtual methods.
            // This matches the runtime behavior of the CLR, not the compiler behavior.
            if (!old_method_def.flags().is_set(metadata::method_attribute::virtual_))
            {
                continue;
            }

            if (new_member.slot().is_initialized() && new_member.slot() == old_member.slot())
            {
                core::assert_true([&]{ return slot_override_it == table_end; });
                slot_override_it = table_it;
                continue;
            }

            if (old_method_def.name() != new_method_def.name())
            {
                continue;
            }

            metadata::signature_comparer const compare(&resolver);

            // If the signature of the method in the derived class is different from the signature
            // of the method in the base class, it is not an override:
            if (!compare(old_method_sig, new_method_sig))
            {
                continue;
            }

            // If the base class method is final, the derived class method is a new method:
            if (old_method_def.flags().is_set(metadata::method_attribute::final))
            {
                continue;
            }

            if (signature_override_it != table_end)
                throw core::metadata_error(L"method signatures not unique");

            signature_override_it = table_it;
        }

        if (slot_override_it == table_end && signature_override_it == table_end)
        {
            member_table.push_back(new_member);
            return inherited_member_count;
        }
        else if (slot_override_it != table_end && signature_override_it == table_end)
        {
            *slot_override_it = new_member;
            return inherited_member_count;
        }
        else if (slot_override_it == table_end && signature_override_it != table_end)
        {
            *signature_override_it = new_member;
            return inherited_member_count;
        }
        else
        {
            *slot_override_it = new_member;
            member_table.erase(std::prev(signature_override_it.base()));
            return inherited_member_count - 1;
        }
    }





    auto property_traits::get_members(metadata::type_def_token const& type) -> row_range_type
    {
        core::assert_initialized(type);

        return metadata::find_properties(type);
    }

    auto property_traits::get_signature(token_type const& member) -> metadata::blob
    {
        core::assert_initialized(member);

        return row_from(member).signature();
    }

    auto property_traits::insert_member(interim_sequence_type& member_table,
                                        interim_type    const& new_member,
                                        core::size_type const  inherited_member_count) -> core::size_type
    {
        core::assert_initialized(new_member);

        // TODO Do we need to handle hiding or overriding for properties?
        member_table.push_back(new_member);
        return inherited_member_count;
    }





    member_table_entry::member_table_entry()
    {
    }

    member_table_entry::member_table_entry(metadata::unrestricted_token const& member_token_)
        : _member_token(member_token_)
    {
        core::assert_initialized(member_token_);
    }

    auto member_table_entry::member_token() const -> metadata::unrestricted_token const&
    {
        core::assert_initialized(*this);

        return _member_token;
    }

    auto member_table_entry::is_initialized() const -> bool
    {
        return _member_token.is_initialized();
    }





    member_table_entry_with_instantiation::member_table_entry_with_instantiation()
    {
    }

    member_table_entry_with_instantiation::member_table_entry_with_instantiation(member_table_entry const& context)
        : _member_token(context.member_token())
    {
        core::assert_initialized(context);
    }

    member_table_entry_with_instantiation::member_table_entry_with_instantiation(
        metadata::unrestricted_token    const& member_token_,
        metadata::type_def_or_signature const& instantiating_type_,
        core::const_byte_range          const& instantiated_signature_)
        : _member_token          (member_token_          ),
          _instantiating_type    (instantiating_type_    ),
          _instantiated_signature(instantiated_signature_)
    {
        core::assert_initialized(member_token_);
    }

    auto member_table_entry_with_instantiation::member_token() const -> metadata::unrestricted_token const&
    {
        core::assert_initialized(*this);

        return _member_token;
    }

    auto member_table_entry_with_instantiation::instantiating_type() const -> metadata::type_def_or_signature
    {
        core::assert_initialized(*this);

        return _instantiating_type;
    }

    auto member_table_entry_with_instantiation::instantiated_signature() const -> core::const_byte_range
    {
        core::assert_initialized(*this);

        return _instantiated_signature;
    }

    auto member_table_entry_with_instantiation::is_initialized() const -> bool
    {
        return _member_token.is_initialized();
    }





    member_table_entry_with_override_slot::member_table_entry_with_override_slot()
    {
    }

    member_table_entry_with_override_slot::member_table_entry_with_override_slot(member_table_entry const& context,
                                                                                 override_slot      const& slot)
        : _entry(context.member_token()), _override_slot(slot)
    {
        core::assert_initialized(context);

        if (!_override_slot.is_initialized())
            _override_slot = compute_slot_for(_entry.member_token().as<metadata::method_def_token>(), metadata::type_def_or_signature());
    }

    member_table_entry_with_override_slot::member_table_entry_with_override_slot(metadata::unrestricted_token    const& member_token,
                                                                                 metadata::type_def_or_signature const& instantiating_type,
                                                                                 core::const_byte_range          const& instantiated_signature,
                                                                                 override_slot                   const& slot)
        : _entry(member_token, instantiating_type, instantiated_signature), _override_slot(slot)
    {
        core::assert_initialized(member_token);

        if (!_override_slot.is_initialized())
            _override_slot = compute_slot_for(
                _entry.member_token().as<metadata::method_def_token>(),
                _entry.instantiating_type());
    }

    member_table_entry_with_override_slot::operator member_table_entry_with_instantiation const&() const
    {
        return _entry;
    }

    auto member_table_entry_with_override_slot::member_token() const -> metadata::unrestricted_token const&
    {
        core::assert_initialized(*this);

        return _entry.member_token();
    }

    auto member_table_entry_with_override_slot::instantiating_type() const -> metadata::type_def_or_signature const&
    {
        core::assert_initialized(*this);

        return _entry.instantiating_type();
    }

    auto member_table_entry_with_override_slot::instantiated_signature() const -> core::const_byte_range const&
    {
        core::assert_initialized(*this);

        return _entry.instantiated_signature();
    }

    auto member_table_entry_with_override_slot::slot() const -> override_slot const&
    {
        core::assert_initialized(*this);

        return _override_slot;
    }

    auto member_table_entry_with_override_slot::is_initialized() const -> bool
    {
        return _entry.is_initialized();
    }





    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::member_token() const -> token_type
    {
        core::assert_initialized(*this);

        return !is_instantiated()
            ? entry().member_token().as<token_type>()
            : entry_with_instantiation().member_token().as<token_type>();
    }
    
    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::member_signature() const -> signature_type
    {
        core::assert_initialized(*this);

        if (has_instantiated_signature())
        {
            return signature_type(
                &instantiating_type().scope(),
                begin(instantiated_signature()),
                end(instantiated_signature()));
        }
        
        metadata::blob const signature(traits_type::get_signature(member_token()));
        if (!signature.is_initialized())
            return signature_type();

        return signature.as<signature_type>();
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::has_instantiating_type() const -> bool
    {
        core::assert_initialized(*this);

        return is_instantiated() && entry_with_instantiation().instantiating_type().is_initialized();
    }
    
    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::instantiating_type() const -> metadata::type_def_or_signature
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return has_instantiating_type(); });

        return !is_instantiated() ? metadata::type_def_or_signature() : entry_with_instantiation().instantiating_type();
    }
    
    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::has_instantiated_signature() const -> bool
    {
        core::assert_initialized(*this);

        return is_instantiated() && entry_with_instantiation().instantiated_signature().is_initialized();
    }
    
    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::instantiated_signature() const -> core::const_byte_range
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return has_instantiated_signature(); });

        return !is_instantiated() ? core::const_byte_range() : entry_with_instantiation().instantiated_signature();
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::is_initialized() const -> bool
    {
        return !is_instantiated() ? entry().is_initialized() : entry_with_instantiation().is_initialized();
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::is_instantiated() const -> bool
    {
        return (reinterpret_cast<std::uintptr_t>(this) & 1) != 0;
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::entry() const -> member_table_entry const&
    {
        core::assert_true([&]{ return !is_instantiated(); });

        return *reinterpret_cast<member_table_entry const*>(this);
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::entry_with_instantiation() const -> member_table_entry_with_instantiation const&
    {
        core::assert_true([&]{ return is_instantiated(); });

        return *reinterpret_cast<member_table_entry_with_instantiation const*>(reinterpret_cast<std::uintptr_t>(this) & ~1);
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::realize() const -> member_table_entry_with_instantiation
    {
        core::assert_initialized(*this);

        return is_instantiated()
            ? member_table_entry_with_instantiation(entry_with_instantiation())
            : member_table_entry_with_instantiation(entry());
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::from(member_table_entry const* const e)
        -> member_table_entry_facade const*
    {
        if (e == nullptr)
            return nullptr;

        return reinterpret_cast<member_table_entry_facade const*>(e);
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::from(member_table_entry_with_instantiation const* const e)
        -> member_table_entry_facade const*
    {
        if (e == nullptr)
            return nullptr;

        return reinterpret_cast<member_table_entry_facade const*>(reinterpret_cast<std::uintptr_t>(e) | 1);
    }

    template <member_kind MemberTag>
    auto member_table_entry_facade<MemberTag>::from(member_table_entry_with_override_slot const* const e)
        -> member_table_entry_facade const*
    {
        if (e == nullptr)
            return nullptr;

        return reinterpret_cast<member_table_entry_facade const*>(reinterpret_cast<std::uintptr_t>(
            &e->operator const cxxreflect::reflection::detail::member_table_entry_with_instantiation &()) | 1);
    }

    template class member_table_entry_facade<member_kind::event     >;
    template class member_table_entry_facade<member_kind::field     >;
    template class member_table_entry_facade<member_kind::interface_>;
    template class member_table_entry_facade<member_kind::method    >;
    template class member_table_entry_facade<member_kind::property  >;





    template <member_kind MemberTag>
    auto member_table_iterator_constructor<MemberTag>::operator()(member_kind const& kind, core::stride_iterator const& current) const
        -> member_table_entry_facade<MemberTag> const*
    {
        core::assert_true([&]{ return kind == MemberTag; });
        core::assert_initialized(current);

        switch (current.stride())
        {
        case sizeof(member_table_entry):
            return member_table_entry_facade<MemberTag>::from(reinterpret_cast<member_table_entry const*>(*current));

        case sizeof(member_table_entry_with_instantiation):
            return member_table_entry_facade<MemberTag>::from(reinterpret_cast<member_table_entry_with_instantiation const*>(*current));

        default:
            core::assert_unreachable();
        }
    }

    template class member_table_iterator_constructor<member_kind::event     >;
    template class member_table_iterator_constructor<member_kind::field     >;
    template class member_table_iterator_constructor<member_kind::interface_>;
    template class member_table_iterator_constructor<member_kind::method    >;
    template class member_table_iterator_constructor<member_kind::property  >;





    membership_context::membership_context()
    {
    }

    auto membership_context::get_state() const -> state_flags
    {
        return state_flags(_state.load());
    }

    template <member_kind MemberTag>
    auto membership_context::get_table() const
        -> core::optional<typename member_table_iterator_generator<MemberTag>::range_type>
    {
        typedef typename member_table_iterator_generator<MemberTag>::type       iterator_type;
        typedef typename member_table_iterator_generator<MemberTag>::range_type range_type;

        // First, check to see if we've built this table; if we haven't, return immediately:
        state_flags const flags(_state.load());
        if (!flags.is_set(primary_state_flag_for(MemberTag)))
            return core::optional<range_type>();

        // Otherwise, obtain the table range and convert it into an iterable range:
        core::size_type const stride(flags.is_set(instantiated_state_flag_for(MemberTag))
            ? sizeof(member_table_entry_with_instantiation)
            : sizeof(member_table_entry));

        membership_context_base<MemberTag> const& base(*this);

        return range_type(
            iterator_type(MemberTag, core::stride_iterator(base._first.load(), stride)),
            iterator_type(MemberTag, core::stride_iterator(base._last.load(),  stride)));
    }

    template <member_kind MemberTag>
    auto membership_context::get_range() const
        -> core::optional<core::const_byte_range>
    {
        // First, check to see if we've built this table; if we haven't, return immediately:
        state_flags const flags(_state.load());
        if (!flags.is_set(primary_state_flag_for(MemberTag)))
            return core::optional<core::const_byte_range>();

        membership_context_base<MemberTag> const& base(*this);

        return core::const_byte_range(base._first.load(), base._last.load());
    }

    template <member_kind MemberTag>
    auto membership_context::set_table(core::const_byte_range const table_range, bool const is_instantiated)
        -> typename member_table_iterator_generator<MemberTag>::range_type
    {
        state_flags const current_flags(_state.load());
        core::assert_true([&]{ return !current_flags.is_set(primary_state_flag_for(MemberTag)); });

        membership_context_base<MemberTag>& base(*this);

        base._first.store(table_range.begin());
        base._last.store (table_range.end());

        // Note:  The state must be stored last, to ensure that a request to get a table fails
        // until the table pointers have been set.  We require that all possible callers of this
        // function synchronize, so there is no race between reading the flags above and setting
        // the flags here.
        state const new_flag_bits(
            primary_state_flag_for(MemberTag) |
            (is_instantiated ? instantiated_state_flag_for(MemberTag) : state()));

        _state.store(current_flags.enumerator() | new_flag_bits);

        return get_table<MemberTag>().value();
    }

    auto membership_context::primary_state_flag_for(member_kind const kind) -> state
    {
        return (state)((core::size_type)1 << (core::size_type)kind);
    }

    auto membership_context::instantiated_state_flag_for(member_kind const kind) -> state
    {
        return (state)((core::size_type)1 << ((core::size_type)kind + 8));
    }

    template auto membership_context::get_table<member_kind::event     >() const -> core::optional<member_table_iterator_generator<member_kind::event     >::range_type>;
    template auto membership_context::get_table<member_kind::field     >() const -> core::optional<member_table_iterator_generator<member_kind::field     >::range_type>;
    template auto membership_context::get_table<member_kind::interface_>() const -> core::optional<member_table_iterator_generator<member_kind::interface_>::range_type>;
    template auto membership_context::get_table<member_kind::method    >() const -> core::optional<member_table_iterator_generator<member_kind::method    >::range_type>;
    template auto membership_context::get_table<member_kind::property  >() const -> core::optional<member_table_iterator_generator<member_kind::property  >::range_type>;
    
    template auto membership_context::get_range<member_kind::event     >() const -> core::optional<core::const_byte_range>;
    template auto membership_context::get_range<member_kind::field     >() const -> core::optional<core::const_byte_range>;
    template auto membership_context::get_range<member_kind::interface_>() const -> core::optional<core::const_byte_range>;
    template auto membership_context::get_range<member_kind::method    >() const -> core::optional<core::const_byte_range>;
    template auto membership_context::get_range<member_kind::property  >() const -> core::optional<core::const_byte_range>;
    
    template auto membership_context::set_table<member_kind::event     >(core::const_byte_range, bool) -> member_table_iterator_generator<member_kind::event     >::range_type;
    template auto membership_context::set_table<member_kind::field     >(core::const_byte_range, bool) -> member_table_iterator_generator<member_kind::field     >::range_type;
    template auto membership_context::set_table<member_kind::interface_>(core::const_byte_range, bool) -> member_table_iterator_generator<member_kind::interface_>::range_type;
    template auto membership_context::set_table<member_kind::method    >(core::const_byte_range, bool) -> member_table_iterator_generator<member_kind::method    >::range_type;
    template auto membership_context::set_table<member_kind::property  >(core::const_byte_range, bool) -> member_table_iterator_generator<member_kind::property  >::range_type;





    membership_handle::membership_handle()
    {
    }

    membership_handle::membership_handle(membership_storage* const storage,
                                         membership_context* const context,
                                         core::internal_key)
        : _storage(storage), _context(context)
    {
        core::assert_not_null(storage);
        core::assert_not_null(context);
    }

    auto membership_handle::get_events() const -> event_table_range
    {
        return get_table<member_kind::event>();
    }

    auto membership_handle::get_fields() const -> field_table_range
    {
        return get_table<member_kind::field>();
    }

    auto membership_handle::get_interfaces() const -> interface_table_range
    {
        return get_table<member_kind::interface_>();
    }

    auto membership_handle::get_methods() const -> method_table_range
    {
        return get_table<member_kind::method>();
    }

    auto membership_handle::get_properties() const -> property_table_range
    {
        return get_table<member_kind::property>();
    }

    auto membership_handle::context(core::internal_key) const -> membership_context&
    {
        core::assert_initialized(*this);

        return *_context;
    }

    auto membership_handle::is_initialized() const -> bool
    {
        return _storage.is_initialized() && _context.is_initialized();
    }

    template <member_kind MemberTag>
    auto membership_handle::get_table() const -> typename member_table_iterator_generator<MemberTag>::range_type
    {
        core::assert_initialized(*this);

        // First, check to see if we already have a table; if we do, return it:
        auto const existing_table(_context->get_table<MemberTag>());
        if (existing_table.has_value())
            return existing_table.value();

        // Otherwise, build a new table:
        _storage->create_table<MemberTag>(*_context, core::internal_key());
        auto const new_table(_context->get_table<MemberTag>());

        return new_table.has_value()
            ? new_table.value()
            : typename member_table_iterator_generator<MemberTag>::range_type();
    }

    template auto membership_handle::get_table<member_kind::event     >() const -> member_table_iterator_generator<member_kind::event     >::range_type;
    template auto membership_handle::get_table<member_kind::field     >() const -> member_table_iterator_generator<member_kind::field     >::range_type;
    template auto membership_handle::get_table<member_kind::interface_>() const -> member_table_iterator_generator<member_kind::interface_>::range_type;
    template auto membership_handle::get_table<member_kind::method    >() const -> member_table_iterator_generator<member_kind::method    >::range_type;
    template auto membership_handle::get_table<member_kind::property  >() const -> member_table_iterator_generator<member_kind::property  >::range_type;





    membership_storage::membership_storage()
    {
    }

    auto membership_storage::get_membership(key_type const& key) -> membership_handle
    {
        core::recursive_mutex_lock const lock(_sync.lock());

        return membership_handle(this, &_index[key], core::internal_key());
    }

    auto membership_storage::allocate_signature(core::const_byte_range const transient_range, core::internal_key) -> core::const_byte_range
    {
        return allocate_range(_signature_allocator, transient_range);
    }

    auto membership_storage::allocate_table(core::const_byte_range const transient_range, core::internal_key) -> core::const_byte_range
    {
        return allocate_range(_table_allocator, transient_range);
    }
    
    template <member_kind MemberTag>
    auto membership_storage::create_table(membership_context& context, core::internal_key) -> void
    {
        core::recursive_mutex_lock const lock(_sync.lock());

        internal_create_table<MemberTag>(*this, key_from_context(context));
    }

    auto membership_storage::allocate_range(allocator_type& allocator, core::const_byte_range const transient_range) -> core::const_byte_range
    {
        auto persistent_range(allocator.allocate(transient_range.size()));
        core::range_checked_copy(begin(transient_range), end(transient_range), begin(persistent_range), end(persistent_range));
        return persistent_range;
    }

    auto membership_storage::key_from_context(membership_context& key) -> key_type const&
    {
        typedef std::pair<key_type, membership_context> pair_type;

        return reinterpret_cast<pair_type*>(core::begin_bytes(key) - offsetof(pair_type, second))->first;
    }

    template auto membership_storage::create_table<member_kind::event     >(membership_context&, core::internal_key) -> void;
    template auto membership_storage::create_table<member_kind::field     >(membership_context&, core::internal_key) -> void;
    template auto membership_storage::create_table<member_kind::interface_>(membership_context&, core::internal_key) -> void;
    template auto membership_storage::create_table<member_kind::method    >(membership_context&, core::internal_key) -> void;
    template auto membership_storage::create_table<member_kind::property  >(membership_context&, core::internal_key) -> void;

} } }
