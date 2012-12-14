
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

// On x86/x64, we can always use the win32_externals for unit tests:  even when we are running in a
// Windows Store app, we can still call the Win32 functions.  On ARM, however, we must use the
// winrt_externals because the Windows SDK does not include all of the Windows API import libraries
// for ARM (only those that are usable from within a Windows Store app are included).
#if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_ARM
#    include "cxxreflect/windows_runtime/windows_runtime.hpp"
#endif

namespace
{
    bool const initialize([]() -> bool
    {
        #if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_ARM
        cxxreflect::core::externals::initialize(cxxreflect::externals::winrt_externals());
        #else
        cxxreflect::core::externals::initialize(cxxreflect::externals::win32_externals());
        #endif
        return true;
    }());
}
