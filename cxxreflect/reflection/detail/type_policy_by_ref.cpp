
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_by_ref.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_by_ref(type_policy::unresolved_type_context const& t) -> void
    {
        core::assert_true([&]{ return t.as_blob().as<metadata::type_signature>().is_by_ref(); });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto by_ref_type_policy::is_by_ref(unresolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return true;
    }

    auto by_ref_type_policy::is_nested(unresolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }





    auto by_ref_type_policy::attributes(resolved_type_context const& t) const -> metadata::type_flags
    {
        assert_by_ref(t);

        return metadata::type_flags();
    }

    auto by_ref_type_policy::base_type(resolved_type_context const& t) const -> unresolved_type_context
    {
        assert_by_ref(t);

        return unresolved_type_context();
    }

    auto by_ref_type_policy::is_abstract(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::is_enum(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::is_import(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::is_interface(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::is_marshal_by_ref(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::is_sealed(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::is_serializable(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::is_special_name(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::is_value_type(resolved_type_context const& t) const -> bool
    {
        assert_by_ref(t);

        return false;
    }

    auto by_ref_type_policy::layout(resolved_type_context const& t) const -> type_layout
    {
        assert_by_ref(t);

        return type_layout::auto_layout;
    }

    auto by_ref_type_policy::string_format(resolved_type_context const& t) const -> type_string_format
    {
        assert_by_ref(t);

        return type_string_format::ansi_string_format;
    }

    auto by_ref_type_policy::visibility(resolved_type_context const& t) const -> type_visibility
    {
        assert_by_ref(t);

        return type_visibility::not_public;
    }

} } }
