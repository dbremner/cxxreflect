
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/FundamentalUtilities.hpp"
#include "CxxReflect/WindowsRuntimeUtility.hpp"

#include <io.h>
#include <shlwapi.h>
#include <windows.h>
#include <wincrypt.h>

namespace CxxReflect { namespace { namespace Private {

    std::auto_ptr<IExternalFunctions> Externals;

} } }

namespace CxxReflect {

    IExternalFunctions::~IExternalFunctions()
    {
    }





    void Externals::Initialize(std::auto_ptr<IExternalFunctions> externals)
    {
        if (Private::Externals.get() != nullptr)
            throw CxxReflect::LogicError(L"Externals was already initialized");

        Private::Externals.reset(externals.release());
    }

    IExternalFunctions& Externals::Get()
    {
        if (Private::Externals.get() == nullptr)
            throw CxxReflect::LogicError(L"Externals was not initialized before use");

        return *Private::Externals.get();
    }





    Sha1Hash Externals::ComputeSha1Hash(ConstByteIterator const first, ConstByteIterator const last)
    {
        return Get().ComputeSha1Hash(first, last);
    }

    unsigned Externals::ComputeUtf16LengthOfUtf8String(char const* const source)
    {
        return Get().ComputeUtf16LengthOfUtf8String(source);
    }

    bool Externals::ConvertUtf8ToUtf16(char const* const source, wchar_t* const target, unsigned const targetLength)
    {
        return Get().ConvertUtf8ToUtf16(source, target, targetLength);
    }

    String Externals::ComputeCanonicalUri(ConstCharacterIterator const pathOrUri)
    {
        return Get().ComputeCanonicalUri(pathOrUri);
    }

    FILE* Externals::OpenFile(ConstCharacterIterator const fileName, ConstCharacterIterator const mode)
    {
        return Get().OpenFile(fileName, mode);
    }

    Detail::FileRange Externals::MapFileRange(FILE* const file, SizeType const index, SizeType const size)
    {
        return Get().MapFileRange(file, index, size);
    }

    bool Externals::FileExists(ConstCharacterIterator const filePath)
    {
        return Get().FileExists(filePath);
    }

}

namespace CxxReflect { namespace Detail { namespace { namespace Private {

    unsigned ComputeUtf16LengthOfUtf8String(char const* const source)
    {
        Detail::AssertNotNull(source);

        return MultiByteToWideChar(CP_UTF8, 0, source, -1, nullptr, 0);
    }

    bool ConvertUtf8ToUtf16(char const* const source, wchar_t* const target, unsigned const targetLength)
    {
        Detail::AssertNotNull(source);
        Detail::AssertNotNull(target);

        int const actualLength(MultiByteToWideChar(CP_UTF8, 0, source, -1, target, targetLength));
        return actualLength >= 0 &&  static_cast<unsigned>(actualLength) == targetLength;
    }

    FILE* OpenFile(ConstCharacterIterator const fileName, ConstCharacterIterator const mode)
    {
        FILE* handle(nullptr);

        errno_t const error(_wfopen_s(&handle, fileName, mode));
        if (error != 0)
            throw FileIOError(error);

        if (handle == nullptr)
            throw LogicError(L"Expected non-null file handle or error.");

        return handle;
    }





    class UnmapViewOfFileDestructible : public IDestructible
    {
    public:

        UnmapViewOfFileDestructible(void const* const base = nullptr)
            : _base(base)
        {
        }

        void Set(void const* const base)
        {
            if (_base.Get() != nullptr)
                throw LogicError(L"Base pointer is already set");

            _base.Get() = base;
        }

        ~UnmapViewOfFileDestructible()
        {
            if (_base.Get() != nullptr)
                UnmapViewOfFile(_base.Get());
        }

    private:

        Detail::ValueInitialized<void const*> _base;
    };





    class UniqueByteArrayDestructible : public IDestructible
    {
    public:

        UniqueByteArrayDestructible(std::unique_ptr<Byte[]> data)
            : _data(std::move(data))
        {
        }

        ~UniqueByteArrayDestructible()
        {
        }

    private:

        std::unique_ptr<Byte[]> _data;
    };





    class SmartHandle
    {
    public:

        SmartHandle(HANDLE const handle)
            : _handle(handle)
        {
        }

        HANDLE& Get()       { return _handle.Get(); }
        HANDLE  Get() const { return _handle.Get(); }

        ~SmartHandle()
        {
            if (_handle.Get() != INVALID_HANDLE_VALUE)
                CloseHandle(_handle.Get());
        }

    private:

        Detail::ValueInitialized<HANDLE> _handle;
    };

    FileRange MapFileRange(FILE* const file, SizeType const index, SizeType const size)
    {
        if (file == nullptr)
            throw RuntimeError(L"Specified file is not valid");

        ValueInitialized<SYSTEM_INFO> systemInfo;
        GetNativeSystemInfo(&systemInfo.Get());

        // If the requested size is less than half of our memory mapped I/O allocation granularity,
        // just read the bytes into a byte array.
        if (size < systemInfo.Get().dwAllocationGranularity / 2)
        {
            std::unique_ptr<Byte[]> data(new Byte[size]());
            if (std::fread(data.get(), size, 1, file) != 1)
                throw FileIOError(L"Failed to read from file");

            ConstByteIterator const rawData(data.get());

            UniqueDestructible release(new UniqueByteArrayDestructible(std::move(data)));
            return FileRange(rawData, rawData + size, std::move(release));
        }

        // Note:  We do not close this handle.  When 'file' is closed, it will close this handle.
        HANDLE const fileHandle(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file))));
        if (fileHandle == INVALID_HANDLE_VALUE)
            throw RuntimeError(L"Failed to get handle for file");

        // Note:  We do want to close this handle; it does not need to be kept open once we map
        // the view of the file.
        SmartHandle const mappingHandle(CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr));
        if (mappingHandle.Get() == INVALID_HANDLE_VALUE)
            throw RuntimeError(L"Failed to create file mapping");  

        std::unique_ptr<UnmapViewOfFileDestructible> release(new UnmapViewOfFileDestructible());

        SizeType const alignedIndex (index - (index % systemInfo.Get().dwAllocationGranularity));
        SizeType const alignedOffset(index - alignedIndex);
        SizeType const alignedSize  (size + alignedOffset);

        void const* const viewOfFile(MapViewOfFileEx(
            mappingHandle.Get(),
            FILE_MAP_READ,
            0,
            alignedIndex,
            alignedSize,
            nullptr));

        if (viewOfFile == nullptr)
            throw RuntimeError(L"Failed to map view of file");

        release->Set(viewOfFile);

        ConstByteIterator const baseAddress(static_cast<ConstByteIterator>(viewOfFile) + alignedOffset);
        
        return FileRange(baseAddress, baseAddress + size, std::move(release));
    }

} } } }

namespace CxxReflect { namespace Detail {

    Sha1Hash Win32ExternalFunctions::ComputeSha1Hash(ConstByteIterator const first,
                                                     ConstByteIterator const last) const
    {
        Detail::AssertNotNull(first);
        Detail::AssertNotNull(last);

        HCRYPTPROV provider(0);
        Detail::ScopeGuard cleanupProvider([&](){ if (provider) { CryptReleaseContext(provider, 0); } });
        if (!CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
            throw RuntimeError(L"Failed to acquire cryptographic context");

        HCRYPTHASH hash(0);
        Detail::ScopeGuard cleanupHash([&](){ if (hash) { CryptDestroyHash(hash); } });
        if (!CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash))
            throw RuntimeError(L"Failed to create cryptographic hash");

        if (!CryptHashData(hash, first, static_cast<DWORD>(last - first), 0))
            throw RuntimeError(L"Failed to hash data");

        Sha1Hash result = { 0 };
        DWORD resultLength(static_cast<DWORD>(result.size()));
        if (!CryptGetHashParam(hash, HP_HASHVAL, result.data(), &resultLength, 0) || resultLength != 20)
            throw RuntimeError(L"Failed to obtain hash value");

        return result;
    }

    unsigned Win32ExternalFunctions::ComputeUtf16LengthOfUtf8String(char const* const source) const
    {
        return Private::ComputeUtf16LengthOfUtf8String(source);
    }

    bool Win32ExternalFunctions::ConvertUtf8ToUtf16(char const* const source,
                                                    wchar_t*    const target,
                                                    unsigned    const targetLength) const
    {
        return Private::ConvertUtf8ToUtf16(source, target, targetLength);
    }

    String Win32ExternalFunctions::ComputeCanonicalUri(ConstCharacterIterator const pathOrUri) const
    {
        ValueInitialized<std::array<wchar_t, 2048>> buffer;

        DWORD length(static_cast<DWORD>(buffer.Get().size()));
        Detail::VerifySuccess(UrlCanonicalize(pathOrUri, buffer.Get().data(), &length, 0));

        return String(buffer.Get().data());
    }

    FILE* Win32ExternalFunctions::OpenFile(ConstCharacterIterator const fileName,
                                           ConstCharacterIterator const mode) const
    {
        return Private::OpenFile(fileName, mode);
    }

    Detail::FileRange Win32ExternalFunctions::MapFileRange(FILE*    const file,
                                                           SizeType const index,
                                                           SizeType const size) const
    {
        return Private::MapFileRange(file, index, size);
    }

    bool Win32ExternalFunctions::FileExists(ConstCharacterIterator const filePath) const
    {
        return GetFileAttributes(filePath) != INVALID_FILE_ATTRIBUTES;
    }

    Win32ExternalFunctions::~Win32ExternalFunctions()
    {
    }





    Sha1Hash WinRTExternalFunctions::ComputeSha1Hash(ConstByteIterator const first,
                                                     ConstByteIterator const last) const
    {
        return Sha1Hash(); // TODO Implement
    }

    unsigned WinRTExternalFunctions::ComputeUtf16LengthOfUtf8String(char const* const source) const
    {
        return Private::ComputeUtf16LengthOfUtf8String(source);
    }

    bool WinRTExternalFunctions::ConvertUtf8ToUtf16(char const* const source,
                                                    wchar_t*    const target,
                                                    unsigned    const targetLength) const
    {
        return Private::ConvertUtf8ToUtf16(source, target, targetLength);
    }

    String WinRTExternalFunctions::ComputeCanonicalUri(ConstCharacterIterator const pathOrUri) const
    {
        return WindowsRuntime::Internal::ComputeCanonicalUri(pathOrUri);
    }

    FILE* WinRTExternalFunctions::OpenFile(ConstCharacterIterator const fileName,
                                           ConstCharacterIterator const mode) const
    {
        return Private::OpenFile(fileName, mode);
    }

    Detail::FileRange WinRTExternalFunctions::MapFileRange(FILE*    const file,
                                                           SizeType const index,
                                                           SizeType const size) const
    {
        return Private::MapFileRange(file, index, size);
    }

    bool WinRTExternalFunctions::FileExists(ConstCharacterIterator const filePath) const
    {
        FILE* handle(nullptr);

        errno_t const error(_wfopen_s(&handle, filePath, L"rb"));
        if (error == 0)
        {
            std::fclose(handle);
            return true;
        }
        else
        {
            return false;
        }
    }

    WinRTExternalFunctions::~WinRTExternalFunctions()
    {
    }

} }