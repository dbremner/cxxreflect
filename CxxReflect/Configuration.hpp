#ifndef CXXREFLECT_CONFIGURATION_HPP_
#define CXXREFLECT_CONFIGURATION_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Hi there!  This header allows you to define or undefine various macros to configure which library
// features are available and which are not.  Note that you can also comment out the definitions in
// this header and define them globally for the build (e.g. using /DMACRO_NAME with Visual C++).

#include "CxxReflect/StandardLibrary.hpp"

#define CXXREFLECT_ENABLE_DEBUG_ASSERTIONS

// This macro enables or disables internal usage of the C++ Standard Library algorithms in debug
// builds (i.e., when _DEBUG defined).  When this macro is defined, CxxReflect will use its own
// algorithm implementations that do not perform the extensive debug checks that the Visual C++
// Standard Library uses.  If this macro is not defined, CxxReflect will use the C++ Standard
// Library.
//
// The Visual C++ Standard Library iterator debugging is extremely useful for finding misuse of
// iterators, but for the CxxReflect library, it slows down execution by about 100x [yay O(N) :'(].
//
// Warning: the unchecked algorithms rely on undocumented Visual C++ Standard Library implementation
// details. The implementation can change at any time, and if it changes, it may break this setting.
// If the build fails in the compilation of the unchecked algorithms, investigate by looking at the
// new Visual C++ headers and updating Fundamentals.hpp, or disable this setting and suffer abysmal
// debug build performance.
//
#define CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS

// This macro controls whether Windows Runtime integration is enabled.  Define this macro if you are
// compiling CxxReflect for use in a Metro style application AppContainer.  Otherwise, do not define
// this macro.  Note that this macro does not require the usage of the C++/CX language extensions
// (i.e., /ZW).  CxxReflect works fine with both "low-level" and "high-level" C++ code.
// TODO Make sure the WinRT integration actually works without the /ZW language projections.
//
#define CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

//
// Modify the lines below this point at your own peril.
//

// The unchecked algorithms are only used when crazy-expensive iterator debugging is enabled.
#if _ITERATOR_DEBUG_LEVEL != 2
#undef CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS
#endif

#define CXXREFLECT_ARCHITECTURE_X86 1
#define CXXREFLECT_ARCHITECTURE_X64 2
#define CXXREFLECT_ARCHITECTURE_ARM 3

#if defined(_M_IX86)
#    define CXXREFLECT_ARCHITECTURE CXXREFLECT_ARCHITECTURE_X86
#elif defined (_M_X64)
#    define CXXREFLECT_ARCHITECTURE CXXREFLECT_ARCHITECTURE_X86
#elif false // TODO ARM
#    define CXXREFLECT_ARCHITECTURE_ARM
#else
#    error Compiling for an unknown platform
#endif

#endif
