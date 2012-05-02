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

/// \mainpage CxxReflect
///
/// CxxReflect is a native reflection library.  It is designed to provide type inspection and dynamic
/// invocation capabilities for C++ projects that target the Windows Runtime.
///
/// The library has other uses as well.  It can load any CLI assembly and provide access to the types
/// defined in the assembly's metadata, without having to start the CLR.
///
/// \warning CxxReflect is a work in progress.  While many of its features are complete, many are not
/// yet well tested and many may not function correctly when used with untested scenarios.
///
/// # Copyright
///
/// **Copyright James P. McNellis 2011 - 2012.**
///
/// **Distributed under the Boost Software License, Version 1.0.**
///
/// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
///
/// # Basic Usage
///
/// ## Inclding and Using the Library
///
/// Add the path to the directory in which the CxxReflect library is located to your include search
/// path.  For example, if CxxReflect is located in "c:\dev\CxxReflect", add "c:\dev" to the search
/// path.
///
/// Add cxxreflect.lib to be linked into your project.  There is no CxxReflect dynamic library; only
/// the static library is supported.
///
/// In your source files where you use CxxReflect, include `"CxxReflect\CxxReflect.hpp"`.  This one
/// header will include the entire CxxReflect public API.  It is recommended that you include this
/// header in your project's precompiled headers, if you use precompiled headers, as it is very large
/// once it is preprocessed.
///
/// ## Thread Safety
/// 
/// CxxReflect is internally synchronized.  It may be called concurrently from multiple threads.
///
/// ## C++/CX Support
///
/// CxxReflect is usable in a C++/CX project.  Note that there are two sets of CxxReflect project
/// configurations:  Debug/Release and Debug(ZW)/Release(ZW).  To use CxxReflect in a C++/CX project
/// you must build and link with one of the (ZW) configurations.  (/ZW is the compiler option that
/// enables C++/CX support.)
///
/// CxxReflect makes use of the C++ Standard Library threading libraries, which in Visual C++ 11 are
/// built atop ConcRT.  ConcRT has types that are layout-sensitive to the /ZW flag, so objects built
/// with /ZW cannot be mixed with objects built without /ZW if both sets use ConcRT.  If they are
/// mixed, you will encounter horrible errors caused by ODR violations.  CxxReflect has link-time
/// checks to verify that you do not mix incompatible object files.  If you try to use the wrong
/// CxxReflect library, you will get a linker error.
///
/// ## Namespaces
///
/// The entirety of the CxxReflect library is contained in the CxxReflect namespace.
///
/// For succinctness, many examples and test programs make use of a `cxr` namespace, which is
/// defined as:
///
///     namespace cxr
///     {
///         using namespace CxxReflect;
///         using namespace CxxReflect::WindowsRuntime;
///     }
///
/// That is all.
///
/// ## Type Names
///
/// There are several kinds of type names:
///
/// * **Simple type names:** A simple type name is an unqualified type name.  The name is qualified
///   by neither assembly name or namespace.
///
/// * **Full type name:** A full type name is a namespace-qualified type name.  The `.` token is
///   used as a namespace delimiter.  Even though this library is a C++-based library, we still use
///   the `.` token as a delimiter because that's what the CLI type system uses internally.  For the
///   Windows Runtime type system, a full type name uniquely identifies a type.
///
/// * **Assembly-qualified type name:** A full type name that is not only namespace-qualified but it
///   also qualified by the name of the assembly in which the type is defined.  In the CLI type 
///   system, an assembly-qualified type name uniquely identifies a type.





/// \namespace CxxReflect
///
/// The fundamental CLI type system API is defined in this namespace.

/// \namespace CxxReflect::Metadata
///
/// Provides a low-level database abstraction for a CLI metadata database.
///
/// The `CxxReflect::Metadata` namespace contains the metadata reader logic, which represents a CLI
/// metadata database as a relational database, provides parsing of metadata tables and rows, caches
/// strings read from the string heap, and translates compressed metadata references into higher-
/// level constructs.
///
/// The `CxxReflect::Metadata` namespace also defines a set of signature parsing classes that are
/// used to read and evaluate raw metadata signature blobs, e.g. method and type signatures.

/// \namespace CxxReflect::WindowsRuntime
///
/// Provides integration of the type system API with a Windows Runtime App Package.
///
/// \todo Document this namespace and its members more thoroughly.

/// \namespace CxxReflect::Detail
///
/// Contains internal implementation details not intended for external use.

/// \namespace CxxReflect::WindowsRuntime::Internal
///
/// Contains internal implementation details not intended for external use.


#endif
