
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

namespace
{
    bool const initialize([]() -> bool
    {
        cxxreflect::core::externals::initialize(cxxreflect::externals::win32_externals());
        return true;
    }());
}
