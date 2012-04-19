#ifndef CXXREFLECT_WINDOWSRUNTIMEINSPECTION_HPP_
#define CXXREFLECT_WINDOWSRUNTIMEINSPECTION_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header defines the public interface for type inspection using the Windows Runtime.

#include "CxxReflect/WindowsRuntimeCommon.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace CxxReflect { namespace WindowsRuntime {

    // INTERFACE IMPLEMENTATION QUERIES
    //
    // These functions allow you to get the list of types in the package that implement a given
    // interface.  They grovel the entire set of loaded metadata files for implementers.  If no
    // types implement the interface, an empty sequence is returned.  If the interface cannot
    // be found, a RuntimeError is thrown.

    std::vector<Type> GetImplementers(Type const& interfaceType);
    std::vector<Type> GetImplementers(StringReference interfaceFullName);
    std::vector<Type> GetImplementers(StringReference namespaceName, StringReference interfaceSimpleName);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template<typename TInterface>
    std::vector<Type> GetImplementersOf()
    {
        typedef typename Internal::RemoveHat<TInterface>::Type BareHeadedInterfaceType;
        String const interfaceFullName(BareHeadedInterfaceType::typeid->FullName->Data());
        return WindowsRuntime::GetImplementers(StringReference(interfaceFullName.c_str()));
    }
    #endif

    // GET TYPE
    //
    // These functions allow you to get a type from the package given its name or given an inspectable
    // object.
    //
    // TODO We do not handle non-activatable runtime types very well, or types that do not have public
    // metadata.  We need to consider how to distinguish between these and how we can give as much
    // functionality as possible.

    Type GetType(StringReference typeFullName);
    Type GetType(StringReference namespaceName, StringReference typeSimpleName);
    Type GetTypeOf(IInspectable* object);
    
    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template <typename T>
    Type GetTypeOf(T^ object)
    {
        return CxxReflect::WindowsRuntime::GetTypeOf(reinterpret_cast<IInspectable*>(object));
    }
    #endif





    // TYPE PROPERTIES
    //
    // These are Windows Runtime-specific properties, or properties that are implemented differently
    // in Windows Runtime from in the basic CLI metadata.

    bool IsDefaultConstructible(Type const& type);
    Guid GetGuid(Type const& type);

} }

#endif // CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif
