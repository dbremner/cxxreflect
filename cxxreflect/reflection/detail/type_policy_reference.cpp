
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_reference.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_reference(type_policy::unresolved_type_context const& t) -> void
    {
        core::assert_true([&]{ return t.is_token() && t.as_token().is<metadata::type_ref_token>(); });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {
    
    auto reference_type_policy::is_array(unresolved_type_context const& t) const -> bool
    {
        assert_reference(t);

        return false;
    }

    auto reference_type_policy::is_by_ref(unresolved_type_context const& t) const -> bool
    {
        assert_reference(t);

        return false;
    }

    auto reference_type_policy::is_generic_type_instantiation(unresolved_type_context const& t) const -> bool
    {
        assert_reference(t);

        return false;
    }

    auto reference_type_policy::is_nested(unresolved_type_context const& t) const -> bool
    {
        assert_reference(t);

        return false;
    }

    auto reference_type_policy::is_pointer(unresolved_type_context const& t) const -> bool
    {
        assert_reference(t);

        return false;
    }

    auto reference_type_policy::is_primitive(unresolved_type_context const& t) const -> bool
    {
        assert_reference(t);

        return false;
    }

    auto reference_type_policy::namespace_name(unresolved_type_context const& t) const -> core::string_reference
    {
        assert_reference(t);

        return row_from(t.as_token().as<metadata::type_ref_token>()).name();
    }

    auto reference_type_policy::primary_name(unresolved_type_context const& t) const -> core::string_reference
    {
        assert_reference(t);

        return row_from(t.as_token().as<metadata::type_ref_token>()).namespace_name();
    }

    auto reference_type_policy::declaring_type(unresolved_type_context const& t) const -> unresolved_type_context
    {
        assert_reference(t);

        // A TypeRef names a nested type if and only if its resolution scope is another TypeRef.  In
        // this case, the TypeRef resolution scope names the enclosing (declaring) type.
        metadata::resolution_scope_token const resolution_scope(row_from(t.as_token().as<metadata::type_ref_token>()).resolution_scope());
        if (!resolution_scope.is<metadata::type_ref_token>())
            return unresolved_type_context();

        return compute_type(resolution_scope.as<metadata::type_ref_token>());
    }

} } }
