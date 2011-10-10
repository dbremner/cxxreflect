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

    // A simple reference-counting base class.  This implementation is not thread-safe by design; we
    // use it as a high-performance alternative to std::shared_ptr, which must use interlocked
    // operations.
    class RefCounted
    {
    protected:

        RefCounted()
            : _references(0)
        {
        }

        virtual ~RefCounted()
        {
        }

    private:

        template <typename T>
        friend class RefPointer;

        void Increment() const
        {
            ++_references;
        }

        void Decrement() const
        {
            --_references;
            if (_references == 0)
            {
                delete this;
            }
        }

        mutable unsigned _references;
    };

    // Smart pointer that manages reference counting of an object of a type derived from RefCounted.
    template <typename T>
    class RefPointer
    {
    public:

        typedef T* Pointer;
        typedef T& Reference;

        explicit RefPointer(T* p = 0)
            : _pointer(p)
        {
            if (IsValid())
            {
                GetBase()->Increment();
            }
        }

        RefPointer(RefPointer const& other)
            : _pointer(other._pointer)
        {
            if (IsValid())
            {
                GetBase()->Increment();
            }
        }

        RefPointer& operator=(RefPointer other)
        {
            Swap(other);
            return *this;
        }

        ~RefPointer()
        {
            if (IsValid())
            {
                GetBase()->Decrement();
            }
        }

        Reference operator*()  const { return *_pointer;     }
        Pointer   operator->() const { return _pointer;      }
        Pointer   Get()        const { return _pointer;      }
        bool      IsValid()    const { return _pointer != 0; }

        void Reset(T* p = 0)
        {
            RefPointer interim(p);
            Swap(interim);
        }

        void Swap(RefPointer& other)
        {
            std::swap(_pointer, other._pointer);
        }

    private:

        RefCounted const* GetBase()
        {
            return _pointer;
        }

        Pointer _pointer;
    };

    template <typename T>
    inline bool operator==(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return lhs.Get() == rhs.Get();
    }

    template <typename T>
    inline bool operator<(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return std::less<T*>()(lhs.Get(), rhs.Get());
    }

    template <typename T> inline bool operator!=(RefPointer<T> const& lhs, RefPointer<T> const& rhs) { return !(lhs == rhs); }
    template <typename T> inline bool operator> (RefPointer<T> const& lhs, RefPointer<T> const& rhs) { return  (rhs <  lhs); }
    template <typename T> inline bool operator>=(RefPointer<T> const& lhs, RefPointer<T> const& rhs) { return !(lhs <  rhs); }
    template <typename T> inline bool operator<=(RefPointer<T> const& lhs, RefPointer<T> const& rhs) { return !(rhs <  lhs); }

    typedef std::array<std::uint8_t, 20> Sha1Hash;

    // Computes the 20 byte SHA1 hash for the bytes in the range [first, last).
    Sha1Hash ComputeSha1Hash(std::uint8_t const* first, std::uint8_t const* last);

    AssemblyName GetAssemblyNameFromToken(IMetaDataAssemblyImport* import, MetadataToken token);

} }

#endif
