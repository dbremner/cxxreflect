#ifndef CXXREFLECT_WINDOWSRUNTIMEINTERNALS_HPP_
#define CXXREFLECT_WINDOWSRUNTIMEINTERNALS_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Utilities used by our Windows Runtime library components.  DO NOT INCLUDE THIS HEADER IN ANY
// PUBLIC INTERFACE HEADERS.  It includes lots of platform headers that we don't necessarily want to
// push upon our users.

#include "CxxReflect/WindowsRuntimeCommon.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "CxxReflect/WindowsRuntimeUtilities.hpp"

#include <guiddef.h>
#include <hstring.h>
#include <winstring.h>

namespace CxxReflect { namespace WindowsRuntime { namespace Internal {

    // Converts an HSTRING into a String (std::wstring).
    String ToString(HSTRING hstring);





    // Gets the root directory of the app package from which the current executable is executing.
    // This should not fail if called from within an app package.  If it does fail, it will return
    // an empty string.  The returned path will include a trailing backslash.
    String GetCurrentPackageRoot();





    // These functions can be used to enumerate the metadata files resolvable in the current app
    // package.  They will not work correctly if we are not executing in an app package.  The
    // 'packageRoot' parameter is optional; if provided, it should point to the root of the app
    // package (i.e., where the manifest and native DLL files are located).
    std::vector<String> EnumeratePackageMetadataFiles(StringReference packageRoot);
    void EnumeratePackageMetadataFilesRecursive(Utility::SmartHString rootNamespace, std::vector<String>& result);





    // Utility functions for converting between our 'Guid' type and the COM 'GUID' type.
    GUID ToComGuid(Guid const& cxxGuid);
    Guid ToCxxGuid(GUID const& comGuid);





    // Removes the rightmost component of a type name.  So, 'A.B.C' becomes 'A.B' and 'A' becomes an
    // empty string.  If the input is an empty string, the function returns without modifying it.
    void RemoveRightmostTypeNameComponent(String& typeName);
    




    UniqueInspectable GetActivationFactoryInterface(String const& typeFullName, Guid const& interfaceGuid);
    UniqueInspectable QueryInterface(IInspectable* instance, Type const& interfaceType);





    String ComputeCanonicalUri(String path);

} } }

#endif // #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#endif
