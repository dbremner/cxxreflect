//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header contains private implementation details only.  It defines a number of usefule utility
// classes, templates, and functions that are used throughout the library.
#ifndef CXXREFLECT_UTILITY_HPP_
#define CXXREFLECT_UTILITY_HPP_

#include "CxxReflect/CoreDeclarations.hpp"
#include "CxxReflect/Exceptions.hpp"

#include <array>
#include <cstdint>
#include <functional>

namespace CxxReflect { namespace Detail {

    class SimpleScopeGuard
    {
    public:

        SimpleScopeGuard(std::function<void()> f)
            : f_(f)
        {
        }

        void Unset() { f_ = nullptr; }

        ~SimpleScopeGuard()
        {
            if (f_) { f_(); }
        }

    private:

        std::function<void()> f_;
    };

    template <typename T>
    void VerifyNotNull(T const& x)
    {
        if (!x) { throw std::logic_error("wtf"); }
    }

    inline void ThrowOnFailure(long hr)
    {
        if (hr < 0) { throw HResultException(hr); }
    }

    static const std::uint32_t InvalidMetadataTokenValue = 0x00000000;
    static const std::uint32_t MetadataTokenTypeMask     = 0xFF000000;

    class MetadataToken
    {
    public:

        MetadataToken()
            : _token(InvalidMetadataTokenValue)
        {
        }

        MetadataToken(std::uint32_t token)
            : _token(token)
        {
        }

        void Set(std::uint32_t token)
        {
            _token = token;
        }

        std::uint32_t Get() const
        {
            //TODO Verify([&]{ return IsInitialized(); });
            return _token;
        }

        MetadataTokenType GetType() const
        {
            //TODO Verify([&]{ return IsInitialized(); });
            return static_cast<MetadataTokenType>(_token & MetadataTokenTypeMask);
        }

        bool IsInitialized() const
        {
            return _token != InvalidMetadataTokenValue;
        }

    private:

        std::uint32_t _token;
    };

    typedef std::array<std::uint8_t, 20> Sha1Hash;

    // Computes the 20 byte SHA1 hash for the bytes in the range [first, last).
    Sha1Hash ComputeSha1Hash(std::uint8_t const* first, std::uint8_t const* last);

    AssemblyName GetAssemblyNameFromToken(IMetaDataAssemblyImport* import, MetadataToken token);

    // These provide friendly support for char[] aliasing, one of the few forms of aliasing that is
    // permitted by the language standard.
    template <typename T>
    std::uint8_t const* BeginBytes(T const& p)
    {
        return reinterpret_cast<std::uint8_t const*>(&p);
    }

    template <typename T>
    std::uint8_t const* EndBytes(T const& p)
    {
        return reinterpret_cast<std::uint8_t const*>(&p + 1);
    }

} }

#endif
