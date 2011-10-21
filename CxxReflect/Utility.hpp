//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header contains private implementation details only.  It defines a number of usefule utility
// classes, templates, and functions that are used throughout the library.
#ifndef CXXREFLECT_UTILITY_HPP_
#define CXXREFLECT_UTILITY_HPP_

#include "CxxReflect/Core.hpp"
#include "CxxReflect/Exceptions.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iterator>
#include <sstream>

#define CXXREFLECT_DEBUG

namespace CxxReflect { namespace Utility {

    template <typename T>
    String ToString(T const& x)
    {
        std::wostringstream iss;
        if (!(iss << x))
            throw std::logic_error("wtf");

        return iss.str();
    }

    #ifdef CXXREFLECT_DEBUG

    template <typename T>
    void DebugVerifyNotNull(T const& x)
    {
        if (!x) 
            throw std::logic_error("wtf");
    }

    template <typename TCallable>
    void DebugVerify(TCallable const& callable, char const* const message)
    {
        if (!callable())
            throw VerificationFailure(message);
    }

    inline void DebugFail(char const* const message)
    {
        throw VerificationFailure(message);
    }

    #else

    template <typename T>
    void DebugVerifyNotNull(T const&) { }

    template <typename TCallable>
    void DebugVerify(TCallable const&, char const*) { }

    inline void DebugFail(char const*) { }

    #endif

    template <typename TEnumeration>
    typename std::underlying_type<TEnumeration>::type AsInteger(TEnumeration value)
    {
        return static_cast<typename std::underlying_type<TEnumeration>::type>(value);
    }

    template <typename TInteger>
    TInteger RoundUp(TInteger value, TInteger roundToNearest)
    {
        return value + (value % roundToNearest);
    }

    inline void ThrowOnFailure(long hr)
    {
        if (hr < 0) { throw HResultException(hr); }
    }

    template <typename T>
    class Dereferenceable
    {
    public:

        Dereferenceable(T const& value)
            : _value(value)
        {
        }

        T& Get() const { return _value; }

        T* operator->() const { return &_value; }

    private:

        T _value;
    };

    struct FileReadException : std::runtime_error
    {
        FileReadException(char const* const message) : std::runtime_error(message) { }
    };

    // A lightweight RAII and interface wrapper around the <cstdio> file I/O interface
    class FileHandle
    {
    public:

        enum Origin
        {
            Begin,   // SEEK_SET
            Current, // SEEK_CUR
            End      // SEEK_END
        };

        FileHandle(wchar_t const* const fileName, wchar_t const* const mode = L"rb")
        {
            errno_t const result(_wfopen_s(&_handle, fileName, mode));
            if (result != 0)
                throw FileReadException("Failed to open file");
        }

        ~FileHandle()
        {
            fclose(_handle);
        }

        void Seek(std::int64_t const position, Origin const origin)
        {
            int realOrigin(0);
            switch (origin)
            {
            case Begin:   realOrigin = SEEK_SET; break;
            case Current: realOrigin = SEEK_CUR; break;
            case End:     realOrigin = SEEK_END; break;
            default:      throw std::logic_error("Unexpected origin provided");
            }
            if (_fseeki64(_handle, position, origin) != 0)
                throw FileReadException("Failed to read file");
        }

        void Read(void* const buffer, std::size_t const size, std::size_t const count)
        {
            if (fread(buffer, size, count, _handle) != count)
                throw FileReadException("Failed to seek file");
        }

    private:

        FileHandle(FileHandle const&);
        FileHandle& operator=(FileHandle const&);

        FILE* _handle;
    };

    template <typename TEnumeration>
    class FlagSet
    {
    public:

        static_assert(std::is_enum<TEnumeration>::value, "TEnumeration must be an enumeration");

        typedef TEnumeration                                      EnumerationType;
        typedef typename std::underlying_type<TEnumeration>::type IntegerType;

        FlagSet()
            : _value()
        {
        }

        FlagSet(EnumerationType const value)
            : _value(static_cast<IntegerType>(value))
        {
        }

        FlagSet(IntegerType const value)
            : _value(value)
        {
        }

        EnumerationType Get()        const { return static_cast<EnumerationType>(_value); }
        IntegerType     GetInteger() const { return _value;                               }

        FlagSet WithMask(EnumerationType const mask) { return WithMask(static_cast<IntegerType>(mask)); }
        FlagSet WithMask(IntegerType     const mask) { return FlagSet(_value & mask);                   }

    private:

        IntegerType _value;
    };

    // A simple, non-thread-safe, intrusive reference-counted base class.
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

        CXXREFLECT_NONCOPYABLE(RefCounted);

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

        // Lowercase versions for easier interop with C++ Standard Library
        typedef T  value_type;
        typedef T* pointer;
        typedef T& reference;

        typedef T  ValueType;
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

    template <typename T>
    inline bool operator!=(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename T>
    inline bool operator> (RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return (rhs < lhs);
    }

    template <typename T>
    inline bool operator>=(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return !(lhs < rhs);
    }

    template <typename T>
    inline bool operator<=(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return !(rhs < lhs);
    }

    // TODO AssemblyName GetAssemblyNameFromToken(IMetaDataAssemblyImport* import, MetadataToken token);

} }

#endif
