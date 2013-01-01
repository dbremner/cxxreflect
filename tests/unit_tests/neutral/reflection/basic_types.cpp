
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

namespace cxxreflect_test {

    CXXREFLECTTEST_DEFINE_BETA_TEST(reflection_basic_beta_types_enumerate_types)
    {
        c.verify(beta.find_type(L"", L"ZArgument").is_initialized());
        c.verify(beta.find_type(L"", L"ZReturn").is_initialized());
        c.verify(beta.find_type(L"", L"ZBaseInterface").is_initialized());
        c.verify(beta.find_type(L"", L"ZBase").is_initialized());
    }

}