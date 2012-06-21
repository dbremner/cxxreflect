
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //





#include "cxxreflect/cxxreflect.hpp"
#include "cxxreflect/windows_runtime/utility.hpp"
#include "test/unit_tests/test_driver.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::reflection;
    using namespace cxxreflect::windows_runtime;
    using namespace cxxreflect::windows_runtime::utility;
}

auto main(Platform::Array<Platform::String^>^) -> int
{
    cxr::guarded_roinitialize const runtime_initializer;
    cxr::guarded_console      const console_initializer;

    cxr::begin_initialization();

    cxxreflect_test::driver::start(static_cast<wchar_t const**>(nullptr), static_cast<wchar_t const**>(nullptr));
    std::getchar();
}

// AMDG //
