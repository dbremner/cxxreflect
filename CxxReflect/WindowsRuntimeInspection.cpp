
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/WindowsRuntimeInspection.hpp"
#include "CxxReflect/WindowsRuntimeLoader.hpp"
#include "CxxReflect/WindowsRuntimeInternals.hpp"

#include <inspectable.h>

namespace CxxReflect { namespace WindowsRuntime {

    std::vector<Type> GetImplementers(Type const& interfaceType)
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

        Utility::SmartHString typeNameHString;
        if (Detail::Failed(object->GetRuntimeClassName(typeNameHString.proxy())))
            throw RuntimeError(L"Failed to get runtime class name from inspectable object");

        if (typeNameHString.empty())
            throw RuntimeError(L"Failed to get runtime class name from inspectable object");

        return GetType(typeNameHString.c_str());
    }





    bool IsDefaultConstructible(Type const& type)
    {
        Detail::Verify([&]{ return type.IsInitialized(); }, L"Type is not initialized");

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
        Detail::Verify([&]{ return type.IsInitialized(); }, L"Type is not initialized");

        return GlobalLoaderContext::Get().GetGuid(type);
    }





    std::vector<Enumerator> GetEnumerators(Type const& enumerationType)
    {
        return GlobalLoaderContext::Get().GetEnumerators(enumerationType);
    }

    std::vector<Enumerator> GetEnumerators(StringReference const enumerationFullName)
    {
        Type const enumerationType(GetType(enumerationFullName));
        if (!enumerationType.IsInitialized())
            throw RuntimeError(L"Failed to locate type");

        return GetEnumerators(enumerationType);
    }

    std::vector<Enumerator> GetEnumerators(StringReference const namespaceName,
                                           StringReference const enumerationSimpleName)
    {
        Type const enumerationType(GetType(namespaceName, enumerationSimpleName));
        if (!enumerationType.IsInitialized())
            throw RuntimeError(L"Failed to locate type");

        return GetEnumerators(enumerationType);
    }




} }
