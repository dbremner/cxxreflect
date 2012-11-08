
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_specialization.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_specialization(type_policy::unresolved_type_context const& t) -> void
    {
        core::assert_true([&]{ return t.is_blob(); });
    }

    auto specialization_from(type_policy::unresolved_type_context const& t) -> metadata::type_signature
    {
        return t.as_blob().as<metadata::type_signature>();
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto specialization_type_policy::is_array(unresolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return false;
    }

    auto specialization_type_policy::is_by_ref(unresolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return false;
    }

    auto specialization_type_policy::is_generic_type_instantiation(unresolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return false;
    }

    auto specialization_type_policy::is_nested(unresolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return compute_element_type_and_call(t, &type_policy::is_nested);
    }

    auto specialization_type_policy::is_pointer(unresolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return false;
    }

    auto specialization_type_policy::is_primitive(unresolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        // This will never be true:  no TypeSpec ever represents a primitive type because we always
        // resolve a primitive type TypeSpec to its TypeDef before obtaining a policy for the type.
        return false;
    }

    auto specialization_type_policy::namespace_name(unresolved_type_context const& t) const -> core::string_reference
    {
        assert_specialization(t);

        return compute_element_type_and_call(t, &type_policy::namespace_name);
    }

    auto specialization_type_policy::primary_name(unresolved_type_context const& t) const -> core::string_reference
    {
        assert_specialization(t);

        return compute_element_type_and_call(t, &type_policy::primary_name);
    }


    auto specialization_type_policy::declaring_type(unresolved_type_context const& t) const -> unresolved_type_context
    {
        assert_specialization(t);

        return compute_element_type_and_call(t, &type_policy::declaring_type);
    }





    auto specialization_type_policy::attributes(resolved_type_context const& t) const -> metadata::type_flags
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::attributes);
    }

    auto specialization_type_policy::base_type(resolved_type_context const& t) const -> unresolved_type_context
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::base_type);
    }


    auto specialization_type_policy::is_abstract(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_abstract);
    }

    auto specialization_type_policy::is_com_object(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_com_object);
    }

    auto specialization_type_policy::is_contextful(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_contextful);
    }

    auto specialization_type_policy::is_enum(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_enum);
    }

    auto specialization_type_policy::is_generic_parameter(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        // This will never be true:  any TypeSpec that represents a generic parameter will be
        // represented by a generic_parameter_type_policy, which overrides this and returns true.
        return false;
    }

    auto specialization_type_policy::is_generic_type(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return is_generic_type_definition(t);
    }

    auto specialization_type_policy::is_generic_type_definition(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        // This will never be true:  only a TypeDef may be a generic type definition:
        return false;
    }

    auto specialization_type_policy::is_import(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_import);
    }

    auto specialization_type_policy::is_interface(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_interface);
    }

    auto specialization_type_policy::is_marshal_by_ref(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_marshal_by_ref);
    }

    auto specialization_type_policy::is_sealed(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_sealed);
    }

    auto specialization_type_policy::is_serializable(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_serializable);
    }

    auto specialization_type_policy::is_special_name(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_special_name);
    }

    auto specialization_type_policy::is_value_type(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_value_type);
    }

    auto specialization_type_policy::is_visible(resolved_type_context const& t) const -> bool
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::is_visible);
    }

    auto specialization_type_policy::layout(resolved_type_context const& t) const -> type_layout
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::layout);
    }

    auto specialization_type_policy::metadata_token(resolved_type_context const& t) const -> core::size_type
    {
        assert_specialization(t);

        return 0x02000000;
    }

    auto specialization_type_policy::string_format(resolved_type_context const& t) const -> type_string_format
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::string_format);
    }

    auto specialization_type_policy::visibility(resolved_type_context const& t) const -> type_visibility
    {
        assert_specialization(t);

        return resolve_element_type_and_call(t, &type_policy::visibility);
    }

} } }
