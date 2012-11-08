
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_instantiation.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_generic_instance(type_policy::unresolved_type_context const& t) -> void
    {
        core::assert_true([&]{ return t.as_blob().as<metadata::type_signature>().is_generic_instance(); });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto generic_instantiation_type_policy::is_generic_type_instantiation(unresolved_type_context const& t) const -> bool
    {
        assert_generic_instance(t);

        return true;
    }





    auto generic_instantiation_type_policy::is_generic_type(resolved_type_context const& t) const -> bool
    {
        assert_generic_instance(t);

        return true;
    }

    auto generic_instantiation_type_policy::is_visible(resolved_type_context const& t) const -> bool
    {
        assert_generic_instance(t);

        // A generic instance is visible if and only if the generic type definition is visible and
        // all of the generic type arguments are visible.  We'll check the arguments first:
        metadata::type_signature const signature(t.as_blob().as<metadata::type_signature>());

        auto const first(signature.begin_generic_arguments());
        auto const last (signature.end_generic_arguments());
        bool const all_arguments_visible(std::all_of(first, last, [&](metadata::type_signature const& argument) -> bool
        {
            return type_policy::get_for(metadata::blob(argument)).is_visible(metadata::blob(argument));
        }));

        if (!all_arguments_visible)
            return false;

        // All of the arguments are visible; now let's check the type definition:
        return resolve_element_type_and_call(t, &type_policy::is_visible);
    }

    auto generic_instantiation_type_policy::metadata_token(resolved_type_context const& t) const -> core::size_type
    {
        assert_generic_instance(t);

        return resolve_primary_type_and_call(t, &type_policy::metadata_token);
    }

} } }
