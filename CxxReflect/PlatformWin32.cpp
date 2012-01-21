//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Core.hpp"

#include <windows.h>
#include <wincrypt.h>

namespace CxxReflect { namespace Detail {

    unsigned ComputeUtf16LengthOfUtf8String(char const* source)
    {
        Detail::VerifyNotNull(source);

        return MultiByteToWideChar(CP_UTF8, 0, source, -1, nullptr, 0);
    }

    bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength)
    {
        Detail::VerifyNotNull(source);
        Detail::VerifyNotNull(target);

        int const actualLength(MultiByteToWideChar(CP_UTF8, 0, source, -1, target, targetLength));
        return actualLength >= 0 &&  static_cast<unsigned>(actualLength) == targetLength;
    }

    Sha1Hash ComputeSha1Hash(std::uint8_t const* const first, std::uint8_t const* const last)
    {
        Detail::VerifyNotNull(first);
        Detail::VerifyNotNull(last);

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

    bool FileExists(wchar_t const* path)
    {
        
        return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
    }

} }
