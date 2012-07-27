
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/core/precompiled_headers.hpp"
#include "cxxreflect/core/externals/win32_externals.hpp"

#include <io.h>
#include <shlwapi.h>
#include <windows.h>
#include <wincrypt.h>

namespace cxxreflect { namespace externals { namespace {

    class smart_handle
    {
    public:

        smart_handle(HANDLE const handle)
            : _handle(handle)
        {
        }

        auto get()       -> HANDLE& { return _handle.get(); }
        auto get() const -> HANDLE  { return _handle.get(); }

        ~smart_handle()
        {
            if (_handle.get() != INVALID_HANDLE_VALUE)
                ::CloseHandle(_handle.get());
        }

    private:

        core::value_initialized<HANDLE> _handle;
    };

    auto compute_file_size(FILE* const file) -> core::size_type
    {
        if (std::fseek(file, 0, SEEK_END) != 0)
            return 0;

        return std::ftell(file);
    }

    auto map_file_range(FILE*           const file,
                        core::size_type const index,
                        core::size_type const size) -> core::unique_byte_array
    {
        if (file == nullptr)
            return core::default_value();

        core::value_initialized<SYSTEM_INFO> system_info;
        ::GetNativeSystemInfo(&system_info.get());

        // Note:  We do not close this handle.  When 'file' is closed, it will close this handle.
        HANDLE const file_handle(reinterpret_cast<HANDLE>(::_get_osfhandle(::_fileno(file))));
        if (file_handle == INVALID_HANDLE_VALUE)
            return core::default_value();

        // Note:  We do want to close this handle; it does not need to be kept open once we map
        // the view of the file.
        smart_handle const mapping_handle(CreateFileMapping(file_handle, nullptr, PAGE_READONLY, 0, 0, nullptr));
        if (mapping_handle.get() == INVALID_HANDLE_VALUE)
            return core::default_value();

        core::size_type const aligned_index (index - (index % system_info.get().dwAllocationGranularity));
        core::size_type const aligned_offset(index - aligned_index);
        core::size_type const aligned_size  (size + aligned_offset);

        void const* const view_of_file(::MapViewOfFileEx(
            mapping_handle.get(),
            FILE_MAP_READ,
            0,
            aligned_index,
            aligned_size,
            nullptr));

        if (view_of_file == nullptr)
            return core::default_value();

        core::const_byte_iterator const base_address(static_cast<core::const_byte_iterator>(view_of_file) + aligned_offset);
        
        return core::unique_byte_array(
            base_address,
            base_address + size,
            [=]() { ::UnmapViewOfFile(base_address); });
    }

} } }

namespace cxxreflect { namespace externals {

    auto base_win32_externals::compute_utf16_length_of_utf8_string(char const* const source) const -> unsigned
    {
        core::assert_not_null(source);

        return ::MultiByteToWideChar(CP_UTF8, 0, source, -1, nullptr, 0);
    }

    auto base_win32_externals::convert_utf8_to_utf16(char const* const source,
                                                wchar_t*    const target,
                                                unsigned    const length) const -> bool
    {
        core::assert_not_null(source);
        core::assert_not_null(target);

        int const actual_length(::MultiByteToWideChar(CP_UTF8, 0, source, -1, target, length));
        return actual_length >= 0 && static_cast<unsigned>(actual_length) == length;
    }

    auto base_win32_externals::open_file(wchar_t const* const file_name, wchar_t const* const mode) const -> FILE*
    {
        FILE* handle(nullptr);

        errno_t const error(_wfopen_s(&handle, file_name, mode));
        if (error != 0)
            throw core::io_error(L"an error occurred when opening the file");

        if (handle == nullptr)
            throw core::logic_error(L"expected non-null file handle or error");

        return handle;
    }

    auto base_win32_externals::map_file(FILE* const file) const -> core::unique_byte_array
    {
        return map_file_range(file, 0, compute_file_size(file));
    }

    base_win32_externals::~base_win32_externals()
    {
    }

    auto win32_externals::compute_sha1_hash(core::const_byte_iterator const first, core::const_byte_iterator const last) const
        -> core::sha1_hash
    {
        core::assert_not_null(first);
        core::assert_not_null(last);

        HCRYPTPROV provider(0);
        core::scope_guard cleanup_provider([&](){ if (provider) { CryptReleaseContext(provider, 0); } });
        if (!CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
            throw core::runtime_error(L"failed to acquire cryptographic context");

        HCRYPTHASH hash(0);
        core::scope_guard cleanup_hash([&](){ if (hash) { CryptDestroyHash(hash); } });
        if (!CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash))
            throw core::runtime_error(L"failed to create cryptographic hash");

        if (!CryptHashData(hash, first, static_cast<DWORD>(last - first), 0))
            throw core::runtime_error(L"failed to hash data");

        core::sha1_hash result = { 0 };
        DWORD resultLength(static_cast<DWORD>(result.size()));
        if (!CryptGetHashParam(hash, HP_HASHVAL, result.data(), &resultLength, 0) || resultLength != 20)
            throw core::runtime_error(L"failed to obtain hash value");

        return result;
    }

    auto win32_externals::compute_canonical_uri(wchar_t const* const path_or_uri) const -> core::string
    {
        core::value_initialized<std::array<wchar_t, 2048>> buffer;

        DWORD length(core::convert_integer(buffer.get().size()));
        if (FAILED(::UrlCanonicalize(path_or_uri, buffer.get().data(), &length, 0)))
            throw core::runtime_error(L"uri canonicalization failed");

        return core::string(buffer.get().data());
    }

    auto win32_externals::file_exists(wchar_t const* const file_path) const -> bool
    {
        return ::GetFileAttributes(file_path) != INVALID_FILE_ATTRIBUTES;
    }

} }
