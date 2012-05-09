
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/WindowsRuntimeInspection.hpp"
#include "CxxReflect/WindowsRuntimeInternals.hpp"

#include <RoApi.h>
#include <RoMetadata.h>
#include <RoMetadataApi.h>
#include <RoMetadataResolution.h>
#include <WRL/Client.h>

#include <Windows.ApplicationModel.h>
#include <Windows.Foundation.h>
#include <Windows.Storage.h>

using Microsoft::WRL::ComPtr;

namespace CxxReflect { namespace WindowsRuntime { namespace Internal {

    String ToString(HSTRING const hstring)
    {
        ConstCharacterIterator const buffer(::WindowsGetStringRawBuffer(hstring, nullptr));
        return buffer == nullptr ? L"" : buffer;
    }





    String GetCurrentPackageRoot()
    {
        using namespace ABI::Windows::ApplicationModel;
        using namespace ABI::Windows::Storage;

        Utility::SmartHString const packageTypeName(RuntimeClass_Windows_ApplicationModel_Package);

        ComPtr<IPackageStatics> packageStatics;
        void** const activationFactoryResult = reinterpret_cast<void**>(packageStatics.GetAddressOf());
        if (Detail::Failed(::RoGetActivationFactory(packageTypeName.value(), IID_IPackageStatics, activationFactoryResult)))
            return String();

        if (packageStatics == nullptr)
            return String();

        ComPtr<IPackage> currentPackage;
        if (Detail::Failed(packageStatics->get_Current(currentPackage.GetAddressOf())) || currentPackage == nullptr)
            return String();

        ComPtr<IStorageFolder> packageFolder;
        if (Detail::Failed(currentPackage->get_InstalledLocation(packageFolder.GetAddressOf())) || packageFolder == nullptr)
            return String();

        ComPtr<IStorageItem> packageFolderItem;
        if (Detail::Failed(packageFolder.As(&packageFolderItem)) || packageFolderItem == nullptr)
            return String();

        Utility::SmartHString path;
        if (Detail::Failed(packageFolderItem->get_Path(path.proxy())))
            return String();

        if (path.empty())
            return String();

        String pathString(path.c_str());
        if (!pathString.empty() && pathString.back() != L'\\')
            pathString.push_back(L'\\');

        return pathString;
    }





    void EnumeratePackageMetadataFilesRecursive(Utility::SmartHString const rootNamespace, std::vector<String>& result)
    {
        Utility::SmartHStringArray filePaths;
        Utility::SmartHStringArray nestedNamespaces;

        Detail::VerifySuccess(RoResolveNamespace(
            rootNamespace.empty() ? nullptr : rootNamespace.value(),
            nullptr,
            0,
            nullptr,
            rootNamespace.empty() ? nullptr : &filePaths.GetCount(),
            rootNamespace.empty() ? nullptr : &filePaths.GetArray(),
            &nestedNamespaces.GetCount(),
            &nestedNamespaces.GetArray()));

        std::transform(filePaths.Begin(), filePaths.End(), std::back_inserter(result), [](HSTRING const path)
        {
            return ToString(path);
        });

        String baseNamespace(rootNamespace.c_str());
        if (!baseNamespace.empty())
            baseNamespace.push_back(L'.');

        std::for_each(nestedNamespaces.Begin(), nestedNamespaces.End(), [&](HSTRING const nestedNamespace)
        {
            Utility::SmartHString const fullNamespaceName((baseNamespace + ToString(nestedNamespace)).c_str());
            EnumeratePackageMetadataFilesRecursive(fullNamespaceName, result);
        });
    }

    std::vector<String> EnumeratePackageMetadataFiles(StringReference const /* packageRoot */)
    {
        std::vector<String> result;

        EnumeratePackageMetadataFilesRecursive(Utility::SmartHString(), result);

        // WORKAROUND:  For some Application Packages, RoResolveNamespace doesn't seem to find all
        // metadata files in the package.  This is a brute-force workaround that just enumerates
        // the .winmd files in the package root.
        // for (std::tr2::sys::wdirectory_iterator it(packageDirectory.c_str()), end; it != end; ++it)
        // {
        //     if (it->path().extension() != L".winmd")
        //         continue;
        //
        //     result.push_back(String(packageDirectory.c_str()) + it->path().filename());
        // }

        std::sort(begin(result), end(result));
        result.erase(std::unique(begin(result), end(result)), end(result));

        return result;
    }





    GUID ToComGuid(Guid const& cxxGuid)
    {
        GUID comGuid;
        Detail::RangeCheckedCopy(
            cxxGuid.AsByteArray().begin(), cxxGuid.AsByteArray().end(),
            Detail::BeginBytes(comGuid), Detail::EndBytes(comGuid));
        return comGuid;
    }

    Guid ToCxxGuid(GUID const& comGuid)
    {
        return Guid(
            comGuid.Data1, comGuid.Data2, comGuid.Data3, 
            comGuid.Data4[0], comGuid.Data4[1], comGuid.Data4[2], comGuid.Data4[3],
            comGuid.Data4[4], comGuid.Data4[5], comGuid.Data4[6], comGuid.Data4[7]);
    }





    void RemoveRightmostTypeNameComponent(String& typeName)
    {
        if (typeName.empty())
            return;

        // TODO This does not handle generics.  Does it need to handle generics?
        auto it(std::find(typeName.rbegin(), typeName.rend(), L'.').base());
        if (it == typeName.begin())
        {
            typeName = String();
        }
        else
        {
            typeName.erase(it - 1, typeName.end());
        }
    }





    UniqueInspectable GetActivationFactoryInterface(String const& typeFullName, Guid const& interfaceGuid)
    {
        Detail::Verify([&]{ return !typeFullName.empty() && interfaceGuid != Guid::Empty; });

        Utility::SmartHString const typeFullNameHString(typeFullName.c_str());

        ComPtr<IInspectable> factory;
        HResult const hr(::RoGetActivationFactory(
            typeFullNameHString.value(),
            ToComGuid(interfaceGuid),
            reinterpret_cast<void**>(factory.GetAddressOf())));

        Detail::VerifySuccess(hr, L"Failed to get requested activation factory interface");

        return UniqueInspectable(factory.Detach());
    }

    UniqueInspectable QueryInterface(IInspectable* const instance, Type const& interfaceType)
    {
        Detail::Verify([&]{ return instance != nullptr && interfaceType.IsInterface(); });

        Guid const interfaceGuid(GetGuid(interfaceType));
        
        ComPtr<IInspectable> interfacePointer;
        Detail::VerifySuccess(instance->QueryInterface(
            ToComGuid(interfaceGuid),
            reinterpret_cast<void**>(interfacePointer.GetAddressOf())));

        return WindowsRuntime::UniqueInspectable(interfacePointer.Detach());
    }

    String ComputeCanonicalUri(String path)
    {
        using namespace ABI::Windows::Foundation;

        if (path.empty())
            return path;

        ComPtr<IUriRuntimeClassFactory> uriFactory;
        HResult const hr0(RoGetActivationFactory(
            Utility::SmartHString(RuntimeClass_Windows_Foundation_Uri).value(),
            IID_IUriRuntimeClassFactory,
            reinterpret_cast<void**>(uriFactory.GetAddressOf())));

        if (Detail::Failed(hr0) || uriFactory.Get() == nullptr)
            throw RuntimeError(L"Failed to get URI Activation Factory");

        ComPtr<IUriRuntimeClass> uri;   
        if (Detail::Failed(uriFactory->CreateUri(Utility::SmartHString(path.c_str()).value(), uri.GetAddressOf())))
            throw RuntimeError(L"Failed to create URI");

        Utility::SmartHString absoluteUri;
        if (Detail::Failed(uri->get_AbsoluteUri(absoluteUri.proxy())))
            throw RuntimeError(L"Failed to get absolute URI");

        return absoluteUri.c_str();
    }

} } }
