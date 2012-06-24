
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy_utility.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_policy_specialization.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_blob(type_def_or_signature_with_module const& t) -> void
    {
        core::assert_true([&]{ return t.type().is_blob(); });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto specialization_type_policy::attributes(type_def_or_signature_with_module const& t) const -> metadata::type_flags
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::attributes);
    }
    
    auto specialization_type_policy::base_type(type_def_or_signature_with_module const& t) const -> type_def_or_signature_with_module
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::base_type);
    }

    auto specialization_type_policy::declaring_type(type_def_or_signature_with_module const& t) const -> type_def_or_signature_with_module
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::declaring_type);
    }

    auto specialization_type_policy::is_abstract(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_abstract);
    }

    auto specialization_type_policy::is_array(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return false;
    }

    auto specialization_type_policy::is_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return false;
    }

    auto specialization_type_policy::is_com_object(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_com_object);
    }
    
    auto specialization_type_policy::is_contextful(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_contextful);
    }
    
    auto specialization_type_policy::is_enum(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_enum);
    }

    auto specialization_type_policy::is_generic_parameter(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);

        // This will never be true:  any TypeSpec that represents a generic parameter will be
        // represented by a generic_parameter_type_policy, which overrides this member and returns
        // true.
        return false;
    }
   
    auto specialization_type_policy::is_generic_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);

        // This will never be true:  only a TypeDef may be a generic type definition, and a TypeSpec
        // that represents a generic instance will be represented by a generic_instance_type_policy,
        // which overrides this member and returns true.
        return false;
    }
    
    auto specialization_type_policy::is_generic_type_definition(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);

        // This will never be true:  only a TypeDef may be a generic type definition
        return false;
    }

    auto specialization_type_policy::is_import(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_import);
    }

    auto specialization_type_policy::is_interface(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_interface);
    }
    
    auto specialization_type_policy::is_marshal_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_marshal_by_ref);
    }

    auto specialization_type_policy::is_nested(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_nested);
    }

    auto specialization_type_policy::is_pointer(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return false;
    }

    auto specialization_type_policy::is_primitive(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        
        // This will never be true:  no TypeSpec ever represents a primitive type because we always
        // resolve a primitive type TypeSpec to its TypeDef before obtaining a policy for the type.
        return false;
    }

    auto specialization_type_policy::is_sealed(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_sealed);
    }

    auto specialization_type_policy::is_serializable(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_serializable);
    }
    
    auto specialization_type_policy::is_special_name(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_special_name);
    }
    
    auto specialization_type_policy::is_value_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_value_type);
    }
    
    auto specialization_type_policy::is_visible(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::is_visible);
    }

    auto specialization_type_policy::layout(type_def_or_signature_with_module const& t) const -> type_attribute_layout
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::layout);
    }

    auto specialization_type_policy::metadata_token(type_def_or_signature_with_module const& t) const -> core::size_type
    {
        assert_blob(t);
        return 0x02000000;
    }

    auto specialization_type_policy::string_format(type_def_or_signature_with_module const& t) const -> type_attribute_string_format
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::string_format);
    }
    
    auto specialization_type_policy::visibility(type_def_or_signature_with_module const& t) const -> type_attribute_visibility
    {
        assert_blob(t);
        return resolve_element_type_and_call(t, &type_policy::visibility);
    }
    
} } }

// AMDG //
