
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/FundamentalUtilities.hpp"
#include "CxxReflect/WindowsRuntimeInternals.hpp"

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

    Detail::FileRange Externals::MapFile(FILE* const file)
    {
        return Get().MapFile(file);
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

        ValueInitialized<HANDLE> _handle;
    };

    SizeType ComputeFileSize(FILE* const file)
    {
        if (std::fseek(file, 0, SEEK_END) != 0)
            return 0;

        return std::ftell(file);
    }

    FileRange MapFileRange(FILE* const file, SizeType const index, SizeType const size)
    {
        if (file == nullptr)
            return FileRange();

        ValueInitialized<SYSTEM_INFO> systemInfo;
        GetNativeSystemInfo(&systemInfo.Get());

        // Note:  We do not close this handle.  When 'file' is closed, it will close this handle.
        HANDLE const fileHandle(reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(file))));
        if (fileHandle == INVALID_HANDLE_VALUE)
            return FileRange();

        // Note:  We do want to close this handle; it does not need to be kept open once we map
        // the view of the file.
        SmartHandle const mappingHandle(CreateFileMapping(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr));
        if (mappingHandle.Get() == INVALID_HANDLE_VALUE)
            return FileRange();

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
            return FileRange();

        release->Set(viewOfFile);

        ConstByteIterator const baseAddress(static_cast<ConstByteIterator>(viewOfFile) + alignedOffset);
        
        return FileRange(baseAddress, baseAddress + size, std::move(release));
    }

} } } }

namespace CxxReflect { namespace Detail {

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

        DWORD length(Detail::ConvertInteger(buffer.Get().size()));
        Detail::VerifySuccess(UrlCanonicalize(pathOrUri, buffer.Get().data(), &length, 0));

        return String(buffer.Get().data());
    }

    FILE* Win32ExternalFunctions::OpenFile(ConstCharacterIterator const fileName,
                                           ConstCharacterIterator const mode) const
    {
        return Private::OpenFile(fileName, mode);
    }

    Detail::FileRange Win32ExternalFunctions::MapFile(FILE* const file) const
    {
        return Private::MapFileRange(file, 0, Private::ComputeFileSize(file));
    }

    bool Win32ExternalFunctions::FileExists(ConstCharacterIterator const filePath) const
    {
        return GetFileAttributes(filePath) != INVALID_FILE_ATTRIBUTES;
    }

    Win32ExternalFunctions::~Win32ExternalFunctions()
    {
    }




    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

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

    Detail::FileRange WinRTExternalFunctions::MapFile(FILE* const file) const
    {
        return Private::MapFileRange(file, 0, Private::ComputeFileSize(file));
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

    #endif

} }