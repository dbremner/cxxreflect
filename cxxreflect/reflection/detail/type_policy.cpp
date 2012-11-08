
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy.hpp"
#include "cxxreflect/reflection/detail/type_policy_array.hpp"
#include "cxxreflect/reflection/detail/type_policy_by_ref.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_instantiation.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_variable.hpp"
#include "cxxreflect/reflection/detail/type_policy_pointer.hpp"
#include "cxxreflect/reflection/detail/type_policy_reference.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    auto type_policy::get_for(unresolved_type_context const& t) -> type_policy const&
    {
        static array_type_policy                 const array_instance;
        static by_ref_type_policy                const by_ref_instance;
        static definition_type_policy            const definition_instance;
        static generic_instantiation_type_policy const generic_instantitaion_instance;
        static generic_variable_type_policy      const generic_variable_instance;
        static pointer_type_policy               const pointer_instance;
        static reference_type_policy             const reference_instance;
        static specialization_type_policy        const specialization_instance;

        if (t.is_token())
        {
            switch (t.as_token().table())
            {
            case metadata::table_id::type_def: return definition_instance;
            case metadata::table_id::type_ref: return reference_instance;
            default:                           core::assert_unreachable();
            }
        }

        metadata::type_signature const signature(t.as_blob().as<metadata::type_signature>());

        // The ByRef check must come first:
        if (signature.is_by_ref())
            return by_ref_instance;

        if (signature.is_simple_array() || signature.is_general_array())
            return array_instance;

        if (signature.is_generic_instance())
            return generic_instantitaion_instance;

        if (signature.is_pointer())
            return pointer_instance;

        if (signature.is_class_variable() || signature.is_method_variable())
            return generic_variable_instance;

        return specialization_instance; // Oh :'(
    }





    auto type_policy::is_array(unresolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_by_ref(unresolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_generic_type_instantiation(unresolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_nested(unresolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_pointer(unresolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_primitive(unresolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::namespace_name(unresolved_type_context const&) const -> core::string_reference
    {
        core::assert_fail();
    }

    auto type_policy::primary_name(unresolved_type_context const&) const -> core::string_reference
    {
        core::assert_fail();
    }

    auto type_policy::declaring_type(unresolved_type_context const&) const -> unresolved_type_context
    {
        core::assert_fail();
    }





    auto type_policy::attributes(resolved_type_context const&) const -> metadata::type_flags
    {
        core::assert_fail();
    }

    auto type_policy::base_type(resolved_type_context const&) const -> unresolved_type_context
    {
        core::assert_fail();
    }

    auto type_policy::is_abstract(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_com_object(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_contextful(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_enum(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_generic_parameter(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_generic_type(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_generic_type_definition(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_import(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_interface(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_marshal_by_ref(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_sealed(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_serializable(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_special_name(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_value_type(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::is_visible(resolved_type_context const&) const -> bool
    {
        core::assert_fail();
    }

    auto type_policy::layout(resolved_type_context const&) const -> type_layout
    {
        core::assert_fail();
    }

    auto type_policy::metadata_token(resolved_type_context const&) const -> core::size_type
    {
        core::assert_fail();
    }

    auto type_policy::string_format(resolved_type_context const&) const -> type_string_format
    {
        core::assert_fail();
    }

    auto type_policy::visibility(resolved_type_context const&) const -> type_visibility
    {
        core::assert_fail();
    }





    type_policy::~type_policy()
    {
        // Empty virtual constructor required for use as base class
    }

} } }
