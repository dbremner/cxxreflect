
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_instance.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_generic_instance(type_def_or_signature_with_module const& t) -> void
    {
        core::assert_true([&]{ return t.type().as_blob().as<metadata::type_signature>().is_generic_instance(); });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto generic_instance_type_policy::is_generic_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_generic_instance(t);
        return true;
    }
    
    auto generic_instance_type_policy::is_visible(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_generic_instance(t);

        // A generic instance is visible if and only if the generic type definition is visible and
        // all of the generic type arguments are visible.  We'll check the arguments first:
        metadata::type_signature const signature(t.type().as_blob().as<metadata::type_signature>());

        auto const first(signature.begin_generic_arguments());
        auto const last (signature.end_generic_arguments());
        bool const all_arguments_visible(std::all_of(first, last, [&](metadata::type_signature const& argument) -> bool
        {
            type_def_ref_spec_or_signature_with_module const unresolved_argument(t.module(), metadata::blob(argument));
            type_def_or_signature_with_module          const resolved_argument(resolve_type_for_construction(unresolved_argument));

            return type_policy::get_for(resolved_argument)->is_visible(resolved_argument);
        }));

        if (!all_arguments_visible)
            return false;

        // All of the arguments are visible; now let's check the type definition:
        return resolve_element_type_and_call(t, &type_policy::is_visible);
    }

    auto generic_instance_type_policy::metadata_token(type_def_or_signature_with_module const& t) const -> core::size_type
    {
        assert_generic_instance(t);
        return resolve_type_def_and_call(t, &type_policy::metadata_token);
    }
    
} } }
