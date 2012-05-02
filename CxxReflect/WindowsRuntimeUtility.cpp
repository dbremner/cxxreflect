
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/WindowsRuntimeInspection.hpp"
#include "CxxReflect/WindowsRuntimeUtility.hpp"

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

    SmartHString::SmartHString()
    {
    }

    SmartHString::SmartHString(const_pointer const s)
    {
        Detail::VerifySuccess(::WindowsCreateString(s, static_cast<UINT32>(::wcslen(s)), &_value.Get()));
    }

    SmartHString::SmartHString(StringReference const s)
    {
        Detail::VerifySuccess(::WindowsCreateString(s.c_str(), static_cast<UINT32>(::wcslen(s.c_str())), &_value.Get()));
    }

    SmartHString::SmartHString(String const& s)
    {
        Detail::VerifySuccess(::WindowsCreateString(s.c_str(), static_cast<UINT32>(::wcslen(s.c_str())), &_value.Get()));
    }

    SmartHString::SmartHString(SmartHString const& other)
    {
        Detail::VerifySuccess(::WindowsDuplicateString(other._value.Get(), &_value.Get()));
    }

    SmartHString::SmartHString(SmartHString&& other)
        : _value(other._value.Get())
    {
        other._value.Reset();
    }

    SmartHString& SmartHString::operator=(SmartHString other)
    {
        swap(other);
        return *this;
    }

    SmartHString& SmartHString::operator=(SmartHString&& other)
    {
        Detail::VerifySuccess(::WindowsDeleteString(_value.Get()));
        _value.Get() = other._value.Get();
        other._value.Reset();
        return *this;
    }

    SmartHString::~SmartHString()
    {
        // We assert instead of verify to avoid potentially throwing during destruction. Really,
        // though, there is no reason that this should ever fail.
        Detail::AssertSuccess(::WindowsDeleteString(_value.Get()));
    }

    void SmartHString::swap(SmartHString& other)
    {
        using std::swap;
        swap(_value, other._value);
    }

    SmartHString::const_iterator SmartHString::begin() const
    {
        return get_buffer_begin();
    }

    SmartHString::const_iterator SmartHString::end() const
    {
        return get_buffer_end();
    }

    SmartHString::const_iterator SmartHString::cbegin() const
    {
        return get_buffer_begin();
    }

    SmartHString::const_iterator SmartHString::cend() const
    {
        return get_buffer_end();
    }

    SmartHString::const_reverse_iterator SmartHString::rbegin() const
    {
        return const_reverse_iterator(get_buffer_end());
    }

    SmartHString::const_reverse_iterator SmartHString::rend() const
    {
        return const_reverse_iterator(get_buffer_begin());
    }


    SmartHString::const_reverse_iterator SmartHString::crbegin() const
    {
        return const_reverse_iterator(get_buffer_end());
    }

    SmartHString::const_reverse_iterator SmartHString::crend() const
    {
        return const_reverse_iterator(get_buffer_begin());
    }

    SmartHString::size_type SmartHString::size() const
    {
        return end() - begin();
    }

    SmartHString::size_type SmartHString::length() const
    {
        return size();
    }

    SmartHString::size_type SmartHString::max_size() const
    {
        return std::numeric_limits<std::size_t>::max();
    }

    SmartHString::size_type SmartHString::capacity() const
    {
        return size();
    }

    bool SmartHString::empty() const
    {
        return size() == 0;
    }

    SmartHString::const_reference SmartHString::operator[](size_type const n) const
    {
        return get_buffer_begin()[n];
    }

    SmartHString::const_reference SmartHString::at(size_type const n) const
    {
        if (n >= size())
            throw std::out_of_range("n");

        return get_buffer_begin()[n];
    }

    SmartHString::const_reference SmartHString::front() const
    {
        return *get_buffer_begin();
    }

    SmartHString::const_reference SmartHString::back() const
    {
        return *(get_buffer_end() - 1);
    }

    SmartHString::const_pointer SmartHString::c_str() const
    {
        return get_buffer_begin();
    }

    SmartHString::const_pointer SmartHString::data() const
    {
        return get_buffer_begin();
    }

    SmartHString::ReferenceProxy::ReferenceProxy(SmartHString* const value)
        : _value(value), _proxy(value->_value.Get())
    {
        Detail::AssertNotNull(value);
    }

    SmartHString::ReferenceProxy::~ReferenceProxy()
    {
        if (_value.Get()->_value.Get() == _proxy.Get())
            return;

        SmartHString newString;
        newString._value.Get() = _proxy.Get();
                
        _value.Get()->swap(newString);
    }

    SmartHString::ReferenceProxy::operator HSTRING*()
    {
        return &_proxy.Get();
    }

    SmartHString::ReferenceProxy SmartHString::proxy()
    {
        return ReferenceProxy(this);
    }

    HSTRING SmartHString::value() const
    {
        return _value.Get();
    }

    bool operator==(SmartHString const& lhs, SmartHString const& rhs)
    {
        return SmartHString::compare(lhs, rhs) ==  0;
    }

    bool operator< (SmartHString const& lhs, SmartHString const& rhs)
    {
        return SmartHString::compare(lhs, rhs) == -1;
    }

    SmartHString::const_pointer SmartHString::get_buffer_begin() const
    {
        const_pointer const result(::WindowsGetStringRawBuffer(_value.Get(), nullptr));
        return result == nullptr ? L"" : result;
    }

    SmartHString::const_pointer SmartHString::get_buffer_end() const
    {
        std::uint32_t length(0);
        const_pointer const first(::WindowsGetStringRawBuffer(_value.Get(), &length));
        return first + length;
    }

    int SmartHString::compare(SmartHString const& lhs, SmartHString const& rhs)
    {
        std::int32_t result(0);
        Detail::VerifySuccess(::WindowsCompareStringOrdinal(lhs._value.Get(), rhs._value.Get(), &result));
        return result;
    }





    String ToString(HSTRING const hstring)
    {
        ConstCharacterIterator const buffer(::WindowsGetStringRawBuffer(hstring, nullptr));
        return buffer == nullptr ? L"" : buffer;
    }





    RaiiHStringArray::RaiiHStringArray()
    {
    }

    RaiiHStringArray::~RaiiHStringArray()
    {
        Detail::Assert([&]{ return _count.Get() == 0 || _array.Get() != nullptr; });

        // Exception-safety note:  if the deletion fails, something has gone horribly wrong
        std::for_each(begin(), end(), [](HSTRING& s)
        {
            Detail::AssertSuccess(::WindowsDeleteString(s));
        });

        ::CoTaskMemFree(_array.Get());
    }

    DWORD& RaiiHStringArray::GetCount()
    {
        return _count.Get();
    }

    HSTRING*& RaiiHStringArray::GetArray()
    {
        return _array.Get();
    }

    HSTRING* RaiiHStringArray::begin() const
    {
        return _array.Get();
    }

    HSTRING* RaiiHStringArray::end() const
    {
        return _array.Get() + _count.Get();
    }





    String GetCurrentPackageRoot()
    {
        using namespace ABI::Windows::ApplicationModel;
        using namespace ABI::Windows::Storage;

        SmartHString const packageTypeName(RuntimeClass_Windows_ApplicationModel_Package);

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

        SmartHString path;
        if (Detail::Failed(packageFolderItem->get_Path(path.proxy())))
            return String();

        if (path.empty())
            return String();

        String pathString(path.c_str());
        if (!pathString.empty() && pathString.back() != L'\\')
            pathString.push_back(L'\\');

        return pathString;
    }





    void EnumeratePackageMetadataFilesRecursive(SmartHString const rootNamespace, std::vector<String>& result)
    {
        RaiiHStringArray filePaths;
        RaiiHStringArray nestedNamespaces;

        Detail::VerifySuccess(RoResolveNamespace(
            rootNamespace.empty() ? nullptr : rootNamespace.value(),
            nullptr,
            0,
            nullptr,
            rootNamespace.empty() ? nullptr : &filePaths.GetCount(),
            rootNamespace.empty() ? nullptr : &filePaths.GetArray(),
            &nestedNamespaces.GetCount(),
            &nestedNamespaces.GetArray()));

        std::transform(filePaths.begin(), filePaths.end(), std::back_inserter(result), [](HSTRING const path)
        {
            return ToString(path);
        });

        String baseNamespace(rootNamespace.c_str());
        if (!baseNamespace.empty())
            baseNamespace.push_back(L'.');

        std::for_each(nestedNamespaces.begin(), nestedNamespaces.end(), [&](HSTRING const nestedNamespace)
        {
            EnumeratePackageMetadataFilesRecursive(baseNamespace + ToString(nestedNamespace), result);
        });
    }

    std::vector<String> EnumeratePackageMetadataFiles(StringReference const /* packageRoot */)
    {
        std::vector<String> result;

        EnumeratePackageMetadataFilesRecursive(SmartHString(), result);

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

        SmartHString const typeFullNameHString(typeFullName.c_str());

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
            SmartHString(RuntimeClass_Windows_Foundation_Uri).value(),
            IID_IUriRuntimeClassFactory,
            reinterpret_cast<void**>(uriFactory.GetAddressOf())));

        if (Detail::Failed(hr0) || uriFactory.Get() == nullptr)
            throw RuntimeError(L"Failed to get URI Activation Factory");

        ComPtr<IUriRuntimeClass> uri;   
        if (Detail::Failed(uriFactory->CreateUri(SmartHString(path).value(), uri.GetAddressOf())))
            throw RuntimeError(L"Failed to create URI");

        SmartHString absoluteUri;
        if (Detail::Failed(uri->get_AbsoluteUri(absoluteUri.proxy())))
            throw RuntimeError(L"Failed to get absolute URI");

        return absoluteUri.c_str();
    }

} } }
