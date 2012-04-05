//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/ExternalFunctionsWin32.hpp"
#include "CxxReflect/Fundamentals.hpp"

#include <shlwapi.h>
#include <windows.h>
#include <wincrypt.h>

namespace CxxReflect { namespace Detail {

    Sha1Hash Win32ExternalFunctions::ComputeSha1Hash(ConstByteIterator const first,
                                                     ConstByteIterator const last) const
    {
        Detail::AssertNotNull(first);
        Detail::AssertNotNull(last);

        HCRYPTPROV provider(0);
        Detail::ScopeGuard cleanupProvider([&](){ if (provider) { CryptReleaseContext(provider, 0); } });
        if (!CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
            throw std::logic_error("wtf");

        HCRYPTHASH hash(0);
        Detail::ScopeGuard cleanupHash([&](){ if (hash) { CryptDestroyHash(hash); } });
        if (!CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash))
            throw std::logic_error("wtf");

        if (!CryptHashData(hash, first, static_cast<DWORD>(last - first), 0))
            throw std::logic_error("wtf");

        Sha1Hash result = { 0 };
        DWORD resultLength(static_cast<DWORD>(result.size()));
        if (!CryptGetHashParam(hash, HP_HASHVAL, result.data(), &resultLength, 0) || resultLength != 20)
            throw std::logic_error("wtf");

        return result;
    }

    String Win32ExternalFunctions::ConvertNarrowStringToWideString(char const* const /* narrowString */) const
    {
        return String(); // TODO
    }

    NarrowString Win32ExternalFunctions::ConvertWideStringToNarrowString(wchar_t const* const /* wideString */) const
    {
        return NarrowString(); // TODO
    }

    unsigned Win32ExternalFunctions::ComputeUtf16LengthOfUtf8String(char const* const source) const
    {
        Detail::AssertNotNull(source);

        return MultiByteToWideChar(CP_UTF8, 0, source, -1, nullptr, 0);
    }

    bool Win32ExternalFunctions::ConvertUtf8ToUtf16(char const* const source,
                                                    wchar_t*    const target,
                                                    unsigned    const targetLength) const
    {
        Detail::AssertNotNull(source);
        Detail::AssertNotNull(target);

        int const actualLength(MultiByteToWideChar(CP_UTF8, 0, source, -1, target, targetLength));
        return actualLength >= 0 &&  static_cast<unsigned>(actualLength) == targetLength;
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
        FILE* handle(nullptr);

        errno_t const error(_wfopen_s(&handle, fileName, mode));
        if (error != 0)
            throw FileIOError(error);

        if (handle == nullptr)
            throw LogicError(L"Expected non-null file handle or error.");

        return handle;
    }

    bool Win32ExternalFunctions::FileExists(ConstCharacterIterator const filePath) const
    {
        return GetFileAttributes(filePath) != INVALID_FILE_ATTRIBUTES;
    }

    Win32ExternalFunctions::~Win32ExternalFunctions()
    {
    }

} }
