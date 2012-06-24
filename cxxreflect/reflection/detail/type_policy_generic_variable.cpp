
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_variable.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_generic_variable(type_def_or_signature_with_module const& t) -> void
    {
        core::assert_true([&]() -> bool
        {
            metadata::type_signature const signature(t.type().as_blob().as<metadata::type_signature>());
            return signature.is_class_variable() || signature.is_method_variable();
        });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto generic_variable_type_policy::is_generic_parameter(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_generic_variable(t);
        return true;
    }
    
} } }

// AMDG //
