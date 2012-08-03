
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/utility.hpp"
#include "cxxreflect/windows_runtime/detail/runtime_utility.hpp"

#include <roapi.h>
#include <rometadata.h>
#include <rometadataapi.h>
#include <rometadataresolution.h>
#include <wrl/client.h>

#include <windows.applicationModel.h>
#include <windows.foundation.h>
#include <windows.security.cryptography.h>
#include <windows.security.cryptography.core.h>
#include <windows.storage.h>

using namespace Microsoft::WRL;

namespace cxxreflect { namespace windows_runtime { namespace detail { namespace {

    auto enumerate_package_metadata_files_recursive(utility::smart_hstring const root_namespace,
                                                    std::vector<core::string>&   result) -> void
    {
        utility::smart_hstring_array file_paths;
        utility::smart_hstring_array nested_namespaces;

        utility::throw_on_failure(RoResolveNamespace(
            root_namespace.empty() ? nullptr : root_namespace.value(),
            nullptr,
            0,
            nullptr,
            root_namespace.empty() ? nullptr : &file_paths.count(),
            root_namespace.empty() ? nullptr : &file_paths.array(),
            &nested_namespaces.count(),
            &nested_namespaces.array()));

        std::transform(begin(file_paths), end(file_paths), std::back_inserter(result), [](HSTRING const path)
        {
            return utility::to_string(path);
        });

        core::string base_namespace(root_namespace.c_str());
        if (!base_namespace.empty())
            base_namespace.push_back(L'.');

        std::for_each(begin(nested_namespaces), end(nested_namespaces), [&](HSTRING const nested_namespace)
        {
            utility::smart_hstring const full_namespace((base_namespace + utility::to_string(nested_namespace)).c_str());
            enumerate_package_metadata_files_recursive(full_namespace, result);
        });
    }

} } } }

namespace cxxreflect { namespace windows_runtime { namespace detail {

    auto compute_canonical_uri(core::string path) -> core::string
    {
        using namespace ABI::Windows::Foundation;

        if (path.empty())
            return path;

        ComPtr<IUriRuntimeClassFactory> uri_factory;
        core::hresult const hr0(RoGetActivationFactory(
            utility::smart_hstring(RuntimeClass_Windows_Foundation_Uri).value(),
            IID_IUriRuntimeClassFactory,
            reinterpret_cast<void**>(uri_factory.GetAddressOf())));

        if (FAILED(hr0) || uri_factory.Get() == nullptr)
            throw core::runtime_error(L"Failed to get URI Activation Factory");

        ComPtr<IUriRuntimeClass> uri;   
        if (FAILED(uri_factory->CreateUri(utility::smart_hstring(path.c_str()).value(), uri.GetAddressOf())))
            throw core::runtime_error(L"Failed to create URI");

        utility::smart_hstring absolute_uri;
        if (FAILED(uri->get_AbsoluteUri(absolute_uri.proxy())))
            throw core::runtime_error(L"Failed to get absolute URI");

        return absolute_uri.c_str();
    }

    auto compute_sha1_hash(core::const_byte_iterator const first, core::const_byte_iterator const last) -> core::sha1_hash
    {
        using namespace ABI::Windows::Security::Cryptography;
        using namespace ABI::Windows::Security::Cryptography::Core;
        using namespace ABI::Windows::Storage::Streams;

        core::assert_not_null(first);
        core::assert_not_null(last);

        // Get the hash provider:
        utility::smart_hstring const hash_provider_type_name(RuntimeClass_Windows_Security_Cryptography_Core_HashAlgorithmProvider);

        ComPtr<IHashAlgorithmProviderStatics> hash_provider_statics;
        utility::throw_on_failure(::RoGetActivationFactory(
            hash_provider_type_name.value(),
            IID_IHashAlgorithmProviderStatics,
            reinterpret_cast<void**>(hash_provider_statics.GetAddressOf())));
        utility::throw_if_null(hash_provider_statics);

        utility::smart_hstring const hash_algorithm_name(L"SHA1");

        ComPtr<IHashAlgorithmProvider> hash_provider;
        utility::throw_on_failure(hash_provider_statics->OpenAlgorithm(hash_algorithm_name.value(), hash_provider.GetAddressOf()));

        UINT32 expected_hash_length(0);
        utility::throw_on_failure(hash_provider->get_HashLength(&expected_hash_length));

        if (expected_hash_length != core::sha1_hash().size())
            throw core::logic_error(L"length of sha1 hash is not the expected length");


        // Get the buffer creation mechanics:
        utility::smart_hstring const buffer_type_name(RuntimeClass_Windows_Security_Cryptography_CryptographicBuffer);

        ComPtr<ICryptographicBufferStatics> buffer_statics;
        utility::throw_on_failure(::RoGetActivationFactory(
            buffer_type_name.value(),
            IID_ICryptographicBufferStatics,
            reinterpret_cast<void**>(buffer_statics.GetAddressOf())));
        utility::throw_if_null(buffer_statics);

        
        // Create the source buffer.  WinRT has no const semantics (*sigh*), so we'll just hope that
        // the CreateFromByteArray() does not modify the data we pass in.  This seems reasonable...
        UINT32 const data_length(core::distance(first, last));
        BYTE*  const data_pointer(const_cast<BYTE*>(first));

        ComPtr<IBuffer> source_buffer;
        utility::throw_on_failure(buffer_statics->CreateFromByteArray(data_length, data_pointer, source_buffer.GetAddressOf()));
        utility::throw_if_null(source_buffer);

        
        // Hash the data:
        ComPtr<IBuffer> hash_buffer;
        utility::throw_on_failure(hash_provider->HashData(source_buffer.Get(), hash_buffer.GetAddressOf()));
        utility::throw_if_null(hash_buffer);

        UINT32 hash_length(0);
        utility::throw_on_failure(hash_buffer->get_Length(&hash_length));

        if (hash_length != expected_hash_length)
            throw core::logic_error(L"length of computed hash is not the expected length");


        // Copy the hash value
        UINT32 hash_data_length(0);
        utility::smart_com_array<BYTE> hash_data;
        utility::throw_on_failure(buffer_statics->CopyToByteArray(hash_buffer.Get(), &hash_data_length, hash_data.get_address_of()));
        utility::throw_if_null(hash_data.get());

        core::sha1_hash hash_value;
        core::range_checked_copy(hash_data.get(), hash_data.get() + hash_data_length, hash_value.begin(), hash_value.end());
        return hash_value;
    }

    auto current_package_root() -> core::string
    {
        using namespace ABI::Windows::ApplicationModel;
        using namespace ABI::Windows::Storage;

        utility::smart_hstring const package_type_name(RuntimeClass_Windows_ApplicationModel_Package);

        ComPtr<IPackageStatics> package_statics;
        void** const activation_factory_result = reinterpret_cast<void**>(package_statics.GetAddressOf());
        if (FAILED(::RoGetActivationFactory(package_type_name.value(), IID_IPackageStatics, activation_factory_result)))
            return core::string();

        if (package_statics == nullptr)
            return core::string();

        ComPtr<IPackage> current_package;
        if (FAILED(package_statics->get_Current(current_package.GetAddressOf())) || current_package == nullptr)
            return core::string();

        ComPtr<IStorageFolder> package_folder;
        if (FAILED(current_package->get_InstalledLocation(package_folder.GetAddressOf())) || package_folder == nullptr)
            return core::string();

        ComPtr<IStorageItem> package_folder_item;
        if (FAILED(package_folder.As(&package_folder_item)) || package_folder_item == nullptr)
            return core::string();

        utility::smart_hstring path;
        if (FAILED(package_folder_item->get_Path(path.proxy())))
            return core::string();

        if (path.empty())
            return core::string();

        core::string path_string(path.c_str());
        if (!path_string.empty() && path_string.back() != L'\\')
            path_string.push_back(L'\\');

        return path_string;
    }

    auto enumerate_package_metadata_files(core::string_reference) -> std::vector<core::string>
    {
        std::vector<core::string> result;

        enumerate_package_metadata_files_recursive(utility::smart_hstring(), result);

        // WORKAROUND:  If the runtime hasn't been initialized (`RoInitialize` hasn't been called),
        // then `RoResolveNamespace` will only return Windows platform metadata files.  We can also
        // enumerate most of the package's metadata files by manually enumerating the contents of
        // the root of the package directory.  This should never be necessary, but it's here just
        // in case someone needs it.
        // for (std::tr2::sys::wdirectory_iterator it(package_root.c_str()), end; it != end; ++it)
        // {
        //     if (it->path().extension() != L".winmd")
        //         continue;
        //
        //     result.push_back(String(package_root.c_str()) + it->path().filename());
        // }

        std::sort(begin(result), end(result));
        result.erase(std::unique(begin(result), end(result)), end(result));

        return result;
    }

    auto remove_rightmost_type_name_component(core::string& type_name) -> void
    {
        if (type_name.empty())
            return;

        // TODO This does not handle generics.  That may be okay...
        auto const it(std::find(type_name.rbegin(), type_name.rend(), L'.').base());
        if (it == begin(type_name))
        {
            type_name = core::string();
        }
        else
        {
            type_name.erase(it - 1, end(type_name));
        }
    }

    auto to_com_guid(reflection::guid const& cxx_guid) -> GUID
    {
        GUID com_guid;
        core::range_checked_copy(
            begin(cxx_guid.bytes()), end(cxx_guid.bytes()),
            core::begin_bytes(com_guid), core::end_bytes(com_guid));
        return com_guid;
    }

    auto to_cxx_guid(GUID const& com_guid) -> reflection::guid
    {
        return reflection::guid(
            com_guid.Data1, com_guid.Data2, com_guid.Data3, 
            com_guid.Data4[0], com_guid.Data4[1], com_guid.Data4[2], com_guid.Data4[3],
            com_guid.Data4[4], com_guid.Data4[5], com_guid.Data4[6], com_guid.Data4[7]);
    }

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
