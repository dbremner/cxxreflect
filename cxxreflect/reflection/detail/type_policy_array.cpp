
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/type_policy_array.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_array(type_policy::unresolved_type_context const& t) -> void
    {
        core::assert_true([&]
        {
            return t.as_blob().as<metadata::type_signature>().is_general_array()
                || t.as_blob().as<metadata::type_signature>().is_simple_array();
        });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto array_type_policy::is_array(unresolved_type_context const& t) const -> bool
    {
        assert_array(t);

        return true;
    }

    auto array_type_policy::is_nested(unresolved_type_context const& t) const -> bool
    {
        assert_array(t);

        return false;
    }





    auto array_type_policy::base_type(resolved_type_context const& t) const -> unresolved_type_context
    {
        assert_array(t);

        loader_context const& root(loader_context::from(t.scope()));
        return root.resolve_fundamental_type(metadata::element_type::array);
    }

    auto array_type_policy::is_abstract(resolved_type_context const& t) const -> bool
    {
        assert_array(t);
        
        return false;
    }

    auto array_type_policy::is_interface(resolved_type_context const& t) const -> bool
    {
        assert_array(t);

        return false;
    }

    auto array_type_policy::is_marshal_by_ref(resolved_type_context const& t) const -> bool
    {
        assert_array(t);

        return false;
    }

    auto array_type_policy::is_sealed(resolved_type_context const& t) const -> bool
    {
        assert_array(t);

        return true;
    }

    auto array_type_policy::is_serializable(resolved_type_context const& t) const -> bool
    {
        assert_array(t);

        return true;
    }

    auto array_type_policy::is_value_type(resolved_type_context const& t) const -> bool
    {
        assert_array(t);

        return false;
    }

    auto array_type_policy::layout(resolved_type_context const& t) const -> type_layout
    {
        assert_array(t);

        return type_layout::auto_layout;
    }

    auto array_type_policy::visibility(resolved_type_context const& t) const -> type_visibility
    {
        assert_array(t);

        return type_visibility::public_;
    }

} } }
