
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_INITIALIZATION_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_INITIALIZATION_HPP_

#include "cxxreflect/windows_runtime/common.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace cxxreflect { namespace windows_runtime {

    auto begin_initialization() -> void;
    auto has_initialization_begun() -> bool;
    auto is_initialized() -> bool;

    auto when_initialized_call(std::function<void()> callable) -> void;

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 

// AMDG //
