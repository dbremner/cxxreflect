
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "test_driver.hpp"

#include "cxxreflect/cxxreflect.hpp"

auto main(int argc, wchar_t const** argv) -> int
{
    cxxreflect::core::externals::initialize(cxxreflect::externals::win32_externals());
    cxxreflect_test::driver::start(argv, argv + argc);
    std::getchar();
}
