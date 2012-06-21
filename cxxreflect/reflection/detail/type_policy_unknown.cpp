
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy_utility.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_policy_unknown.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_unknown(type_def_or_signature_with_module const& t) -> void
    {
        core::assert_true([&]
        { 
            return !t.type().is_token()
                && !t.type().as_blob().as<metadata::type_signature>().is_by_ref()
                && !t.type().as_blob().as<metadata::type_signature>().is_general_array()
                && !t.type().as_blob().as<metadata::type_signature>().is_generic_instance()
                && !t.type().as_blob().as<metadata::type_signature>().is_pointer()
                && !t.type().as_blob().as<metadata::type_signature>().is_simple_array();
        });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto unknown_type_policy::base_type(type_def_or_signature_with_module const& t) const -> type_def_or_signature_with_module
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::base_type);
    }

    auto unknown_type_policy::declaring_type(type_def_or_signature_with_module const& t) const -> type_def_or_signature_with_module
    {
        assert_unknown(t);

        if (is_nested(t))
        {
            return resolve_and_defer_to_definition(t, &definition_type_policy::declaring_type);
        }

        return type_def_or_signature_with_module();
    }

    auto unknown_type_policy::layout(type_def_or_signature_with_module const& t) const -> type_attribute_layout
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::layout);
    }

    auto unknown_type_policy::string_format(type_def_or_signature_with_module const& t) const -> type_attribute_string_format
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::string_format);
    }
    
    auto unknown_type_policy::visibility(type_def_or_signature_with_module const& t) const -> type_attribute_visibility
    {
        assert_unknown(t);

        metadata::type_signature const signature(t.type().as_blob().as<metadata::type_signature>());
        switch (signature.get_kind())
        {
        case metadata::type_signature::kind::function_pointer:
        case metadata::type_signature::kind::variable:
            return type_attribute_visibility::not_public;
        }

        return resolve_and_defer_to_definition(t, &definition_type_policy::visibility);
    }

    auto unknown_type_policy::is_abstract(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_abstract);
    }
    
    auto unknown_type_policy::is_sealed(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_sealed);
    }
    
        
    auto unknown_type_policy::is_com_object(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_com_object);
    }
    
    auto unknown_type_policy::is_contextful(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_contextful);
    }
    
    auto unknown_type_policy::is_enum(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_enum);
    }
    
    auto unknown_type_policy::is_interface(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_interface);
    }
    
    auto unknown_type_policy::is_marshal_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);

        metadata::type_signature const signature(t.type().as_blob().as<metadata::type_signature>());
        switch (signature.get_kind())
        {
        case metadata::type_signature::kind::function_pointer:
        case metadata::type_signature::kind::variable:
            return false;
        }

        return resolve_and_defer_to_definition(t, &definition_type_policy::is_marshal_by_ref);
    }
    
    auto unknown_type_policy::is_primitive(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return false;
    }
    
    auto unknown_type_policy::is_value_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_value_type);
    }

    auto unknown_type_policy::is_generic_parameter(type_def_or_signature_with_module const& t) const -> bool
    {
        return false; // TODO
    }
   
    auto unknown_type_policy::is_generic_type(type_def_or_signature_with_module const& t) const -> bool
    {
        return false; // TODO
    }
    
    auto unknown_type_policy::is_generic_type_definition(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return false;
    }

    auto unknown_type_policy::is_array(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return false;
    }
    
    auto unknown_type_policy::is_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return false;
    }
    
    auto unknown_type_policy::is_pointer(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return false;
    }

    auto unknown_type_policy::is_import(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_import);
    }
    
    auto unknown_type_policy::is_nested(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_nested);
    }
    
    auto unknown_type_policy::is_serializable(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_serializable);
    }
    
    auto unknown_type_policy::is_special_name(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_special_name);
    }
    
    auto unknown_type_policy::is_visible(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_unknown(t);
        return resolve_and_defer_to_definition(t, &definition_type_policy::is_visible);
    }
    
} } }

// AMDG //
