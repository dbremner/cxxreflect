
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy.hpp"
#include "cxxreflect/reflection/detail/type_policy_array.hpp"
#include "cxxreflect/reflection/detail/type_policy_by_ref.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_instance.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_variable.hpp"
#include "cxxreflect/reflection/detail/type_policy_pointer.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    auto resolve_type_for_construction(type_def_ref_spec_or_signature_with_module const& t) 
        -> type_def_or_signature_with_module
    {
        core::assert_initialized(t);

        detail::loader_context const& root(detail::loader_context::from(t.module().context()));

        if (t.type().is_blob())
        {
            // The token refers to a type signature; we make some attempts to resolve it to a
            // TypeDef to ensure that we have unique representation of type definitions.

            metadata::type_signature const signature(t.type().as_blob().as<metadata::type_signature>());

            // The ByRef tag occurs in metadata before the element type, so any ByRef type must be
            // handled via its signature; it must not be resolved to an underlying type.
            if (signature.is_by_ref())
            {
                return type_def_or_signature_with_module(t.module(), t.type().as_blob());
            }

            // If we have a primitive type, resolve it to the TypeDef that represents it and return
            // that type:
            if (signature.get_kind() == metadata::type_signature::kind::primitive)
            {
                metadata::element_type   const primitive_type_tag(signature.primitive_type());
                metadata::type_def_token const primitive_type(root.resolve_fundamental_type(primitive_type_tag));

                return type_def_or_signature_with_module(
                    &root.module_from_scope(primitive_type.scope()),
                    primitive_type);
            }

            // If we have a class type, recurse with the named class type and re-resolve it:
            if (signature.get_kind() == metadata::type_signature::kind::class_type)
            {
                metadata::type_def_ref_spec_token const class_type(signature.class_type());
                return resolve_type_for_construction(type_def_ref_spec_or_signature_with_module(
                    &root.module_from_scope(class_type.scope()),
                    class_type));
            }

            // Otherwise, we have one of the other signature types, which is handled directly by the
            // type class.  No further resolution attempt is required:
            return type_def_or_signature_with_module(t.module(), t.type().as_blob());
        }
        else
        {
            // Otherwise, this is a type token.  If it resolves to a TypeDef, we can return that
            // directly; if it resolves to a TypeSpec, we'll call ourselves again with the signature
            // and run it through the signature case above.

            metadata::type_def_ref_spec_token const tdrs_token(
                t.type().as_token().as<metadata::type_def_ref_spec_token>());

            metadata::type_def_spec_token const tds_token(root.resolve_type(tdrs_token));
            if (tds_token.is<metadata::type_def_token>())
            {
                return type_def_or_signature_with_module(
                    &root.module_from_scope(tds_token.scope()),
                    tds_token.as<metadata::type_def_token>());
            }

            return resolve_type_for_construction(type_def_ref_spec_or_signature_with_module(
                &root.module_from_scope(tds_token.scope()),
                row_from(tds_token.as<metadata::type_spec_token>()).signature()));
        }
    }





    auto type_policy::get_for(type_def_or_signature_with_module const& t) -> type_policy_handle
    {
        static array_type_policy            const array_instance;
        static by_ref_type_policy           const by_ref_instance;
        static definition_type_policy       const definition_instance;
        static generic_instance_type_policy const generic_instance_instance;
        static generic_variable_type_policy const generic_variable_instance;
        static pointer_type_policy          const pointer_instance;
        static specialization_type_policy   const specialization_instance;

        if (t.type().is_token())
            return &definition_instance;

        metadata::type_signature const signature(t.type().as_blob().as<metadata::type_signature>());

        // The ByRef check must come first:
        if (signature.is_by_ref())
            return &by_ref_instance;

        if (signature.is_simple_array() || signature.is_general_array())
            return &array_instance;

        if (signature.is_generic_instance())
            return &generic_instance_instance;

        if (signature.is_pointer())
            return &pointer_instance;

        if (signature.is_class_variable() || signature.is_method_variable())
            return &generic_variable_instance;

        return &specialization_instance; // Oh :'(
    }

    type_policy::~type_policy()
    {
        // Virtual destructor required for polymorphic base class
    }





    type_policy_handle::type_policy_handle()
    {
    }

    type_policy_handle::type_policy_handle(type_policy const* const policy)
        : _policy(policy)
    {
        core::assert_not_null(policy);
    }

    auto type_policy_handle::operator->() const -> type_policy const*
    {
        core::assert_initialized(*this);
        return _policy.get();
    }

    auto type_policy_handle::is_initialized() const -> bool
    {
        return _policy.get() != nullptr;
    }





    auto resolve_type_def(type_def_or_signature_with_module const& t) -> type_def_with_module
    {
        core::assert_initialized(t);

        // First, if the type is not initialized or if we already have a TypeDef, just return it:
        if (t.type().is_token())
            return type_def_with_module(t.module(), t.type().as_token());

        type_def_or_signature_with_module next(t);
        while (next.is_initialized() && !t.type().is_token())
        {
            type_def_or_signature_with_module const next_next(resolve_element_type(next));
            if (!next_next.is_initialized())
                break;

            next = next_next;
        }

        if (!next.is_initialized() || !next.type().is_token())
            return type_def_with_module();

        core::assert_true([&]{ return next.type().is_token(); });

        return type_def_with_module(next.module(), next.type().as_token());
    }

    auto resolve_element_type(type_def_or_signature_with_module const& t) -> type_def_or_signature_with_module
    {
        core::assert_initialized(t);

        if (!t.is_initialized() || t.type().is_token())
            return type_def_or_signature_with_module();

        metadata::type_signature const signature(t.type().as_blob().as<metadata::type_signature>());

        metadata::type_def_ref_spec_or_signature const next_type([&]() -> metadata::type_def_ref_spec_or_signature
        {
            switch (signature.get_kind())
            {
            case metadata::type_signature::kind::general_array:
                return metadata::blob(signature.array_type());

            case metadata::type_signature::kind::class_type:
                return signature.class_type();

            case metadata::type_signature::kind::function_pointer:
                throw core::logic_error(L"not yet implemented");

            case metadata::type_signature::kind::generic_instance:
                return signature.generic_type();

            case metadata::type_signature::kind::primitive:
                return loader_context::from(t.module().context()).resolve_fundamental_type(signature.primitive_type());

            case metadata::type_signature::kind::pointer:
                return metadata::blob(signature.pointer_type());

            case metadata::type_signature::kind::simple_array:
                return metadata::blob(signature.array_type());

            case metadata::type_signature::kind::variable:
                // A Class or Method Variable is never itself a TypeDef:
                return metadata::type_def_ref_spec_or_signature();

            default:
                throw core::runtime_error(L"unknown signature type");
            }
        }());

        if (!next_type.is_initialized())
            return type_def_with_module();

        type_def_ref_spec_or_signature_with_module const next_type_with_module(t.module(), next_type);
        return resolve_type_for_construction(next_type_with_module);
    }


} } }

// AMDG //
