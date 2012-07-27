
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/custom_modifier_iterator.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    auto custom_modifier_iterator::operator*() const -> reference
    {
        core::assert_initialized(*this);

        return type(_module.realize(), _it->type(), core::internal_key());
    }

    auto custom_modifier_iterator::operator->() const -> pointer
    {
        core::assert_initialized(*this);

        return **this;
    }

} } }
