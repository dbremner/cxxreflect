#ifndef CXXREFLECT_CONFIGURATION_HPP_
#define CXXREFLECT_CONFIGURATION_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Hi there!  This header allows you to define or undefine various macros to configure which library
// features are available and which are not.

#include "CxxReflect/StandardLibrary.hpp"

// This macro controls whether assertions throw exceptions or not.  If it is set to true, all debug
// assertions will throw LogicError exceptions.  If it is set to false, debug assertions are no-ops.
#define CXXREFLECT_ENABLE_DEBUG_ASSERTIONS

// This macro controls whether CxxReflect subverts the Visual C++ Standard Library's iterator
// debugging facilities when compiled in Debug mode.  Iterator debugging is extremely useful for
// finding misuse of iterators or algorithms, but it can be extraordinarily expensive.  For example,
// equal_range will scan the entire range to verify that it is ordered according to the specified
// predicate.  Useful?  Very.  But also very expensive:  the O(lg N) operation becomes O(N).
//
// CxxReflect makes heavy use of the C++ Standard Library algorithms and often uses them over large
// ranges of data, so the performance impact of iterator debugging is substantial.  It defines its
// own algorithms that work around iterator debugging so that you can leave iterator debugging
// enabled and use it to verify your own code, but effectively disable it for CxxReflect library
// code.
//
// Warning:  The unchecked algorithms rely on undocumented Visual C++ Standard Library implementation
// details.  The implementation may change at any time.  If it changes, our unchecked algorithms may
// break.
#define CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS

// This macro controls whether the Windows Runtime integration features are compiled.  Define this
// macro if you are compiling CxxReflect for use in a Metro style application.  If you are building
// on Windows 7 or an older version of Windows, do not define this macro (if you do, the library
// will not compile).  If you are building on Windows 8 and do not require Windows Runtime features,
// it is harmless to define this macro.
//
// Note that CxxReflect works fine with both "low-level" (ISO C++) code and "high-level" (C++/CX)
// code.
#define CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION





//
// MODIFY THE LINES BELOW AT YOUR OWN PERIL
//





// If iterator debugging is disabled, we do not need to use our own unchecked debug operations:
#if _ITERATOR_DEBUG_LEVEL != 2
#undef CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS
#endif

// Determine the target architecture for which we are being built.  This is primarily used in the
// Windows Runtime integration to select the correct calling convention for function invocations.
#define CXXREFLECT_ARCHITECTURE_X86 1
#define CXXREFLECT_ARCHITECTURE_X64 2
#define CXXREFLECT_ARCHITECTURE_ARM 3

#if defined(_M_IX86)
#    define CXXREFLECT_ARCHITECTURE CXXREFLECT_ARCHITECTURE_X86
#elif defined (_M_X64)
#    define CXXREFLECT_ARCHITECTURE CXXREFLECT_ARCHITECTURE_X64
#elif false // TODO ARM
#    define CXXREFLECT_ARCHITECTURE_ARM
#else
#    error Compiling for an unknown platform
#endif

// Windows Runtime and C++/CLI are mutually exclusive, so if we are being built in a C++/CLI
// translation unit, we disable support for Windows Runtime:
#if defined (__cplusplus_cli)
#   undef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif

// If support for Windows Runtime is enabled and we are being built in a C++/CX translation unit,
// we enable the C++/CX integration, which adds additional overloads and functions that support
// hat types (T^).
#if defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION) && defined(__cplusplus_winrt)
#    define CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
#endif

// Ensure that we do not mix object files built with /ZW (i.e., C++/CX turned on) and without /ZW.
// The layout of many classes in the Visual C++ threading and synchronization headers depends on
// the /ZW flag, so if we mix object files, we'll get ugly, horrible, silent ODR violations that
// lead to crashes.
#if defined(_MSC_VER)
#    if defined(__cplusplus_winrt)
#        pragma detect_mismatch("CxxReflectIsZwEnabled", "1")
#    else
#        pragma detect_mismatch("CxxReflectIsZwEnabled", "0")
#    endif
#endif

#endif
