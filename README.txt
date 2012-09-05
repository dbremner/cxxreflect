
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

A brief explanation of the top level directories in the project structure:

[*] cxxreflect:  This folder contains the CxxReflect libraries.  To improve compilation performance
    and to improve modularity, the library is split into four parts:  core, metadata, reflection,
    and windows_runtime.  Each of these parts builds on the previous parts.  When all four have been
    built, the 'cxxreflect' project aggregates their contents into a single static library.

    The cxxreflect_core library contains a bunch of utilities that are used by other projects in the
    library.  Most of these are standalone or depend only on the C++ Standard Library and can be
    used elsewhere without too much trouble.  This library also contains the configuration for the
    CxxReflect libraries and a system for isolating nonstandard platform dependencies (like those
    introduced by calling Win32 or WinRT APIs).

    The cxxreflect_metadata library contains facilities for loading CLI assemblies and metadata
    files and for presenting their contents through a somewhat relational database API.  This
    library also contains facilities for parsing signature blobs.  This library is designed to
    provide a low level of abstraction, i.e., it provides direct access to the contents of the file
    in as friendly a manner as possible.

    The cxxreflect_reflection library wraps the cxxreflect_metadata library in an API similar to the
    .NET reflection API, defining types that represent assemblies, types, properties, methods, etc.
    
    The cxxreflect_windows_runtime library contains functionality to integrate CxxReflect into a
    packaged Windows Runtime application.  It can enumerate the metadata files in the package, load
    them, and provide Windows Runtime-specific reflection information.  It also contains the
    implementation of the runtime inspection, invocation, and instantiation functionality that
    enables dynamic instantiation and invocation and type inspection.

    Note that the first three of these libraries are written in (mostly) portable C++ and should be
    easy to adapt to compilers and platforms other than Visual C++ and Windows.  The windows_runtime
    project necessarily relies on platform details and is nonportable.

[*] winrt_components:  This folder contains Windows Runtime components that wrap and extend the
    CxxReflect libraries.  These are designed to make usage of CxxReflect from a Windows Runtime
    application much easier, through a model that is accessible from any language that supports
    Windows Runtime development.

[*] tests:  This folder contains a suite of test projects.

[*] utilities:  This folder contains utilities that are used to build the CxxReflect projects.