//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Utility.hpp"

#include <cor.h>
#include <wincrypt.h>

namespace CxxReflect { namespace Utility {

    Sha1Hash ComputeSha1Hash(std::uint8_t const* first, std::uint8_t const* last)
    {
        DebugVerifyNotNull(first);
        DebugVerifyNotNull(last);

        HCRYPTPROV provider(0);
        SimpleScopeGuard cleanupProvider([&](){ if (provider) { CryptReleaseContext(provider, 0); } });
        if (!CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
            throw std::logic_error("wtf");

        HCRYPTHASH hash(0);
        SimpleScopeGuard cleanupHash([&](){ if (hash) { CryptDestroyHash(hash); } });
        if (!CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash))
            throw std::logic_error("wtf");

        if (!CryptHashData(hash, first, last - first, 0))
            throw std::logic_error("wtf");

        Sha1Hash result = { 0 };
        DWORD resultLength(result.size());
        if (!CryptGetHashParam(hash, HP_HASHVAL, result.data(), &resultLength, 0) || resultLength != 20)
            throw std::logic_error("wtf");

        return result;
    }

    AssemblyName GetAssemblyNameFromToken(IMetaDataAssemblyImport* import, MetadataToken token)
    {
        DebugVerifyNotNull(import);

        std::uint8_t const* publicKeyOrToken(nullptr);
        ULONG publicKeyOrTokenLength(0);

        ULONG hashAlgorithmId(0);

        std::array<wchar_t, 512> nameChars = { };
        ULONG nameLength(0);

        ASSEMBLYMETADATA metadata = { };

        void const* hashValue(nullptr);
        ULONG hashValueLength(0);

        DWORD flags(0);

        if (token.GetType() == MetadataTokenKind::Assembly)
        {
            ThrowOnFailure(import->GetAssemblyProps(
                token.Get(),
                reinterpret_cast<void const**>(&publicKeyOrToken),
                &publicKeyOrTokenLength,
                &hashAlgorithmId,
                nameChars.data(),
                nameChars.size(),
                &nameLength,
                &metadata,
                &flags));
        }
        else if (token.GetType() == MetadataTokenKind::AssemblyRef)
        {
            ThrowOnFailure(import->GetAssemblyRefProps(
                token.Get(),
                reinterpret_cast<void const**>(&publicKeyOrToken),
                &publicKeyOrTokenLength,
                nameChars.data(),
                nameChars.size(),
                &nameLength,
                &metadata,
                &hashValue,
                &hashValueLength,
                &flags));
        }
        else
        {
            throw std::logic_error("wtf");
        }

        std::wstring name(nameChars.data());

        Version version(
            metadata.usMajorVersion,
            metadata.usMinorVersion,
            metadata.usBuildNumber,
            metadata.usRevisionNumber);

        String locale(metadata.szLocale == nullptr
            ? L"neutral"
            : metadata.szLocale);

        PublicKeyToken publicKeyToken;
        if ((flags & afPublicKey) != 0)
        {
            Sha1Hash fullHash(ComputeSha1Hash(publicKeyOrToken, publicKeyOrToken + publicKeyOrTokenLength));
            std::copy(fullHash.rbegin(), fullHash.rbegin() + 8, publicKeyToken.begin());
            flags ^= afPublicKey;
        }
        else if (publicKeyOrTokenLength == 8)
        {
            std::copy(static_cast<std::uint8_t const*>(publicKeyOrToken),
                      static_cast<std::uint8_t const*>(publicKeyOrToken) + 8,
                      publicKeyToken.begin());
        }
        else
        {
            throw std::logic_error("wtf");
        }

        return CxxReflect::AssemblyName(name, version, locale, publicKeyToken, static_cast<AssemblyNameFlags>(flags));
    }

} }
