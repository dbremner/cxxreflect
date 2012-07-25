
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy_utility.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_policy_array.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_array(type_def_or_signature_with_module const& t) -> void
    {
        core::assert_true([&]
        {
            return t.type().as_blob().as<metadata::type_signature>().is_general_array()
                || t.type().as_blob().as<metadata::type_signature>().is_simple_array();
        });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {
    
    auto array_type_policy::base_type(type_def_or_signature_with_module const& t) const -> type_def_or_signature_with_module
    {
        assert_array(t);

        detail::loader_context const& root(detail::loader_context::from(t.module().context()));

        metadata::type_def_token const array_type(root.resolve_fundamental_type(metadata::element_type::array));
        return type_def_or_signature_with_module(&root.module_from_scope(array_type.scope()), array_type);
    }

    auto array_type_policy::is_abstract(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_array(t);
        return false;
    }

    auto array_type_policy::is_array(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_array(t);
        return true;
    }

    auto array_type_policy::is_interface(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_array(t);
        return false;
    }

    auto array_type_policy::is_marshal_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_array(t);
        return false;
    }

    auto array_type_policy::is_nested(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_array(t);
        return false;
    }
    
    auto array_type_policy::is_sealed(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_array(t);
        return true;
    }

    auto array_type_policy::is_serializable(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_array(t);
        return true;
    }
    
    auto array_type_policy::is_value_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_array(t);
        return false;
    }

    auto array_type_policy::layout(type_def_or_signature_with_module const& t) const -> type_attribute_layout
    {
        assert_array(t);
        return type_attribute_layout::auto_layout;
    }
    
    auto array_type_policy::visibility(type_def_or_signature_with_module const& t) const -> type_attribute_visibility
    {
        assert_array(t);
        return type_attribute_visibility::public_;
    }

} } }
