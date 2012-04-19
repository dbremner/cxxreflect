
//               Copyright James P. McNellis (james@jamesmcnellis.com) 2011 - 2012.               //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"
#include "CxxReflect/WindowsRuntimeInspection.hpp"
#include "CxxReflect/WindowsRuntimeLoader.hpp"
#include "CxxReflect/WindowsRuntimeUtility.hpp"

#include <inspectable.h>

namespace CxxReflect { namespace WindowsRuntime {

    std::vector<Type> GetImplementers(Type const interfaceType)
    {
        return GlobalLoaderContext::Get().GetImplementers(interfaceType);
    }

    std::vector<Type> GetImplementers(StringReference const interfaceFullName)
    {
        Type const interfaceType(GetType(interfaceFullName));
        if (!interfaceType.IsInitialized())
            throw RuntimeError(L"Failed to locate type");

        return GetImplementers(interfaceType);
    }

    std::vector<Type> GetImplementers(StringReference const namespaceName, StringReference const interfaceSimpleName)
    {
        Type const interfaceType(GetType(namespaceName, interfaceSimpleName));
        if (!interfaceType.IsInitialized())
            throw RuntimeError(L"Failed to locate type");

        return GetImplementers(interfaceType);
    }





    Type GetType(StringReference const typeFullName)
    {
        return GlobalLoaderContext::Get().GetType(typeFullName);
    }

    Type GetType(StringReference const namespaceName, StringReference const typeSimpleName)
    {
        return GlobalLoaderContext::Get().GetType(namespaceName, typeSimpleName);
    }

    Type GetTypeOf(IInspectable* const object)
    {
        if (object == nullptr)
            throw LogicError(L"Cannot get type of null inspectable object");

        Internal::SmartHString typeNameHString;
        if (Detail::Failed(object->GetRuntimeClassName(typeNameHString.proxy())) || typeNameHString.empty())
            throw RuntimeError(L"Failed to get runtime class name from inspectable object");

        return GetType(typeNameHString.c_str());
    }





    bool IsDefaultConstructible(Type const& type)
    {
        // TODO We really should check the activation factory for this.
        BindingFlags const flags(BindingAttribute::Instance | BindingAttribute::Public);
        auto const it(std::find_if(type.BeginConstructors(flags), type.EndConstructors(), [](Method const& c)
        {
            return c.GetParameterCount() == 0;
        }));

        return type.BeginConstructors(flags) == type.EndConstructors() || it != type.EndConstructors();
    }

    Guid GetGuid(Type const& type)
    {
        return GlobalLoaderContext::Get().GetGuid(type);
    }


} }
