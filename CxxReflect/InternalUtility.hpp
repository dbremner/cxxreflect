//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header contains private implementation details only.  
#ifndef CXXREFLECT_INTERNALUTILITY_HPP_
#define CXXREFLECT_INTERNALUTILITY_HPP_

#include "CxxReflect/CxxReflect.hpp"
#include "CxxReflect/Utility.hpp"

#include <cor.h>
#include <wincrypt.h>

#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace CxxReflect { namespace Detail {

    inline void VerifyNotNull(void const* p)
    {
        if (!p)
        {
            throw std::logic_error("wtf");
        }
    }

    inline void Verify(bool b)
    {
        if (!b)
        {
            throw std::logic_error("wtf");
        }
    }

    template <typename Callable>
    void Verify(Callable const& callable)
    {
        if (!callable())
        {
            throw std::logic_error("wtf");
        }
    }

    template <typename T>
    RefCounted const* RefPointer<T>::GetBase()
    {
        return pointer_;
    }

    class SimpleScopeGuard
    {
    public:

        SimpleScopeGuard(std::function<void()> f)
            : f_(f)
        {
        }

        void Unset()
        {
            f_ = nullptr;
        }

        ~SimpleScopeGuard()
        {
            if (f_)
            {
                f_();
            }
        }

    private:

        std::function<void()> f_;
    };

    template <typename T>
    class FlagSet
    {
    public:

        FlagSet()
            : value_()
        {
        }

        void Set(T x)         { value_ = static_cast<T>(value_ | x); }
        void Unset(T x)       { value_ = static_cast<T>(value_ ^ x); }
        bool IsSet(T x) const { return (value_ & x) != 0;            }

    private:

        T value_;
    };

    template <typename T, std::size_t N>
    class LinearAllocator
    {
    public:

        typedef T              ValueType;
        typedef T*             Pointer;
        typedef std::size_t    SizeType;

        enum { BlockSize = N };

        LinearAllocator()
            : next_()
        {
        }

        Pointer Allocate()
        {
            if (blocks_.size() == 0 || next_ == blocks_.back()->end())
            {
                blocks_.emplace_back(new BlockType);
                next_ = blocks_.back()->begin();
            }

            Pointer p(&*next_);
            next_ += 1;

            return p;
        }

    private:

        LinearAllocator(LinearAllocator const&);
        LinearAllocator& operator=(LinearAllocator const&);

        typedef std::array<ValueType, BlockSize>    BlockType;
        typedef std::unique_ptr<BlockType>          BlockPointer;
        typedef std::vector<BlockPointer>           BlockSequence;
        typedef typename BlockSequence::iterator    BlockSequenceIterator;

        BlockSequence            blocks_;
        BlockSequenceIterator    next_;
    };

    class AllowConversionToArbitraryConstReference
    {
    public:

        explicit AllowConversionToArbitraryConstReference(void const* pointer)
            : pointer_(pointer)
        {
        }

        template <typename T>
        operator T const&() const
        {
            return *static_cast<T const*>(pointer_);
        }

    private:

        void const* pointer_;
    };

    static const mdToken InvalidMetadataTokenValue = 0xFFFFFFFF;
    static const mdToken MetadataTokenTypeMask     = 0xFF000000;

    class MetadataToken
    {
    public:

        MetadataToken()
            : token_(InvalidMetadataTokenValue)
        {
        }

        MetadataToken(mdToken token)
            : token_(token)
        {
        }

        void Set(mdToken token)
        {
            token_ = token;
        }

        mdToken Get() const
        {
            Verify([&]{ return IsInitialized(); });
            return token_;
        }

        CorTokenType GetType() const
        {
            Verify([&]{ return IsInitialized(); });
            return static_cast<CorTokenType>(token_ & MetadataTokenTypeMask);
        }

        bool IsInitialized() const
        {
            return token_ != InvalidMetadataTokenValue;
        }

        bool IsValid(IMetaDataImport* import) const
        {
            VerifyNotNull(import);
            return import->IsValidToken(token_) == TRUE;
        }

    private:

        mdToken token_;
    };

    template <CorTokenType TokenType>
    class CheckedMetadataToken
    {
    public:

        CheckedMetadataToken()
            : token_(InvalidMetadataTokenValue)
        {
        }

        CheckedMetadataToken(MetadataToken token)
            : token_(token.Get())
        {
            Verify([&]{ return IsStateValid(); });
        }

        CheckedMetadataToken(mdToken token)
            : token_(token)
        {
            Verify([&]{ return IsStateValid(); });
        }

        void Set(mdToken token)
        {
            token_ = token;
            Verify([&]{ return IsStateValid(); });
        }

        mdToken Get() const
        {
            Verify([&]{ return IsInitialized(); });
            return token_;
        }

        CorTokenType GetType() const
        {
            return TokenType;
        }

        bool IsInitialized() const
        {
            return token_ != InvalidMetadataTokenValue;
        }

        bool IsValid(IMetaDataImport* import) const
        {
            RuntimeCheck::VerifyNotNull(import);
            return IsInitialized() && import->IsValidToken(token_);
        }

        friend bool operator==(CheckedMetadataToken const& lhs, CheckedMetadataToken const& rhs)
        {
            return lhs.token_ == rhs.token_;
        }

        friend bool operator<(CheckedMetadataToken const& lhs, CheckedMetadataToken const& rhs)
        {
            return lhs.token_ < rhs.token_;
        }

    private:

        bool IsStateValid() const
        {
            return (token_ & MetadataTokenTypeMask) == TokenType;
        }

        mdToken token_;
    };

    typedef CheckedMetadataToken<mdtModule>                    ModuleToken;
    typedef CheckedMetadataToken<mdtTypeRef>                   TypeRefToken;
    typedef CheckedMetadataToken<mdtTypeDef>                   TypeDefToken;
    typedef CheckedMetadataToken<mdtFieldDef>                  FieldDefToken;
    typedef CheckedMetadataToken<mdtMethodDef>                 MethodDefToken;
    typedef CheckedMetadataToken<mdtParamDef>                  ParamDefToken;
    typedef CheckedMetadataToken<mdtInterfaceImpl>             InterfaceImplToken;
    typedef CheckedMetadataToken<mdtMemberRef>                 MemberRefToken;
    typedef CheckedMetadataToken<mdtCustomAttribute>           CustomAttributeToken;
    typedef CheckedMetadataToken<mdtPermission>                PermissionToken;
    typedef CheckedMetadataToken<mdtSignature>                 SignatureToken;
    typedef CheckedMetadataToken<mdtEvent>                     EventToken;
    typedef CheckedMetadataToken<mdtProperty>                  PropertyToken;
    typedef CheckedMetadataToken<mdtModuleRef>                 ModuleRefToken;
    typedef CheckedMetadataToken<mdtTypeSpec>                  TypeSpecToken;
    typedef CheckedMetadataToken<mdtAssembly>                  AssemblyToken;
    typedef CheckedMetadataToken<mdtAssemblyRef>               AssemblyRefToken;
    typedef CheckedMetadataToken<mdtFile>                      FileToken;
    typedef CheckedMetadataToken<mdtExportedType>              ExportedTypeToken;
    typedef CheckedMetadataToken<mdtManifestResource>          ManifestResourceToken;
    typedef CheckedMetadataToken<mdtGenericParam>              GenericParamToken;
    typedef CheckedMetadataToken<mdtMethodSpec>                MethodSpecToken;
    typedef CheckedMetadataToken<mdtGenericParamConstraint>    GenericParamConstraintToken;
    typedef CheckedMetadataToken<mdtString>                    StringToken;
    typedef CheckedMetadataToken<mdtName>                      NameToken;
    typedef CheckedMetadataToken<mdtBaseType>                  BaseTypeToken;

    typedef std::array<std::uint8_t, 20> Sha1Result;

    inline Sha1Result ComputeSha1Hash(std::uint8_t* data, std::size_t length)
    {
        HCRYPTPROV provider(0);
        SimpleScopeGuard cleanupProvider([&](){ if (provider) { CryptReleaseContext(provider, 0); } });
        if (!CryptAcquireContext(&provider, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
            throw std::logic_error("wtf");

        HCRYPTHASH hash(0);
        SimpleScopeGuard cleanupHash([&](){ if (hash) { CryptDestroyHash(hash); } });
        if (!CryptCreateHash(provider, CALG_SHA1, 0, 0, &hash))
            throw std::logic_error("wtf");

        if (!CryptHashData(hash, data, length, 0))
            throw std::logic_error("wtf");

        Sha1Result result;
        DWORD resultLength(result.size());
        if (!CryptGetHashParam(hash, HP_HASHVAL, result.data(), &resultLength, 0) || resultLength != 20)
            throw std::logic_error("wtf");

        return result;
    }

    inline AssemblyName GetAssemblyNameFromToken(IMetaDataAssemblyImport* import, MetadataToken token)
    {
        VerifyNotNull(import);

        const void* publicKeyOrToken(nullptr);
        ULONG publicKeyOrTokenLength(0);

        ULONG hashAlgorithmId(0);

        std::array<wchar_t, 512> nameChars = { };
        ULONG nameLength(0);

        ASSEMBLYMETADATA metadata = { };

        const void* hashValue(nullptr);
        ULONG hashValueLength(0);

        DWORD flags(0);

        if (token.GetType() == mdtAssembly)
        {
            CxxReflect::Detail::ThrowOnFailure(import->GetAssemblyProps(
                token.Get(),
                &publicKeyOrToken,
                &publicKeyOrTokenLength,
                &hashAlgorithmId,
                nameChars.data(),
                nameChars.size(),
                &nameLength,
                &metadata,
                &flags));
        }
        else if (token.GetType() == mdtAssemblyRef)
        {
            CxxReflect::Detail::ThrowOnFailure(import->GetAssemblyRefProps(
                token.Get(),
                &publicKeyOrToken,
                &publicKeyOrTokenLength,
                nameChars.data(),
                nameChars.size(),
                &nameLength,
                &metadata,
                &hashValue,
                &hashValueLength,
                &flags));
        }

        std::wstring name(nameChars.data());

        CxxReflect::Version version(
            metadata.usMajorVersion,
            metadata.usMinorVersion,
            metadata.usBuildNumber,
            metadata.usRevisionNumber);

        std::wstring locale(metadata.szLocale == nullptr
            ? L"neutral"
            : metadata.szLocale);

        CxxReflect::PublicKeyToken publicKeyToken;
        if ((flags & afPublicKey) != 0)
        {
            Sha1Result fullHash(ComputeSha1Hash((std::uint8_t*)publicKeyOrToken, publicKeyOrTokenLength));
            std::copy(fullHash.rbegin(), fullHash.rbegin() + 8, publicKeyToken.begin());
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

        return CxxReflect::AssemblyName(name, version, locale, publicKeyToken); // TODO public key
    }

} }

#endif
