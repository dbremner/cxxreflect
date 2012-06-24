
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_by_ref.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_by_ref(type_def_or_signature_with_module const& t) -> void
    {
        core::assert_true([&]{ return t.type().as_blob().as<metadata::type_signature>().is_by_ref(); });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto by_ref_type_policy::attributes(type_def_or_signature_with_module const& t) const -> metadata::type_flags
    {
        assert_by_ref(t);
        return metadata::type_flags();
    }
    
    auto by_ref_type_policy::base_type(type_def_or_signature_with_module const& t) const -> type_def_or_signature_with_module
    {
        assert_by_ref(t);
        return type_def_or_signature_with_module();
    }

    auto by_ref_type_policy::is_abstract(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }

    auto by_ref_type_policy::is_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return true;
    }

    auto by_ref_type_policy::is_com_object(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }
    
    auto by_ref_type_policy::is_contextful(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }

    auto by_ref_type_policy::is_enum(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }

    auto by_ref_type_policy::is_import(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }
    
    auto by_ref_type_policy::is_interface(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }

    auto by_ref_type_policy::is_marshal_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }

    auto by_ref_type_policy::is_nested(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }
    
    auto by_ref_type_policy::is_sealed(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }
    
    auto by_ref_type_policy::is_serializable(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }

    auto by_ref_type_policy::is_special_name(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }
    
    auto by_ref_type_policy::is_value_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_by_ref(t);
        return false;
    }

    auto by_ref_type_policy::layout(type_def_or_signature_with_module const& t) const -> type_attribute_layout
    {
        assert_by_ref(t);
        return type_attribute_layout::auto_layout;
    }

    auto by_ref_type_policy::string_format(type_def_or_signature_with_module const& t) const -> type_attribute_string_format
    {
        assert_by_ref(t);
        return type_attribute_string_format::ansi_string_format;
    }
    
    auto by_ref_type_policy::visibility(type_def_or_signature_with_module const& t) const -> type_attribute_visibility
    {
        assert_by_ref(t);
        return type_attribute_visibility::not_public;
    }
    
} } }

// AMDG //
