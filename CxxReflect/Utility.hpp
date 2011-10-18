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

    template <typename T>
    class EnhancedCString
    {
    public:

        typedef T                               ValueType;
        typedef T const*                        Pointer;
        typedef T const&                        Reference;
        typedef std::size_t                     SizeType;

        typedef T const*                        Iterator;
        typedef std::reverse_iterator<Iterator> ReverseIterator;

        EnhancedCString()
            : _first(nullptr), _last(nullptr)
        {
        }

        explicit EnhancedCString(Pointer first)
            : _first(first)
        {
            if (first == nullptr)
                return;

            for (_last = first; *_last != 0; ++_last);
            ++_last; // One-past-the-end of the null terminator
        }

        EnhancedCString(Pointer first, Pointer last)
            : _first(first), _last(last)
        {
        }

        template <SizeType N>
        EnhancedCString(ValueType (&data)[N])
            : _first(data), _last(data + N)
        {
        }

        Iterator        Begin()        const { return _first;                  }
        Iterator        End()          const { return _last;                   }

        ReverseIterator ReverseBegin() const { return ReverseIterator(_last);  }
        ReverseIterator ReverseEnd()   const { return ReverseIterator(_first); }

        Pointer         CStr()         const { return _first;                  }
        Pointer         Data()         const { return _first;                  }

    private:

        Pointer _first;
        Pointer _last;
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

        typedef TEnumeration                                      EnumerationType;
        typedef typename std::underlying_type<TEnumeration>::type IntegerType;

        FlagSet()
            : _value()
        {
        }

        explicit FlagSet(EnumerationType const value)
            : _value(value)
        {
        }

        explicit FlagSet(IntegerType const value)
            : _value(static_cast<EnumerationType>(value))
        {
        }

        EnumerationType Get()        const { return _value; }
        IntegerType     GetInteger() const { return static_cast<IntegerType>(_value); }

    private:

        TEnumeration _value;
    };

    // A linear allocator for arrays; this is most useful for the allocation of strings.
    template <typename T, std::size_t TBlockSize>
    class LinearArrayAllocator
    {
    public:

        typedef std::size_t SizeType;
        typedef T           ValueType;
        typedef T*          Pointer;

        enum { BlockSize = TBlockSize };

        class Range
        {
        public:

            Range()
                : _begin(nullptr), _end(nullptr)
            {
            }

            Range(Pointer const begin, Pointer const end)
                : _begin(begin), _end(end)
            {
            }

            Pointer Begin() const { return _begin; }
            Pointer End()   const { return _end;   }

        private:

            Pointer _begin;
            Pointer _end;
        };

        LinearArrayAllocator()
        {
        }

        LinearArrayAllocator(LinearArrayAllocator&& other)
            : _blocks(std::move(other._blocks)),
              _current(std::move(other._current))
        {
            other._current = BlockIterator();
        }

        LinearArrayAllocator& operator=(LinearArrayAllocator&& other)
        {
            Swap(other);
            return *this;
        }

        void Swap(LinearArrayAllocator& other)
        {
            std::swap(other._blocks,  _blocks);
            std::swap(other._current, _current);
        }

        Range Allocate(SizeType const n)
        {
            EnsureAvailable(n);

            Range const r(&*_current, &*_current + n);
            _current += n;
            return r;
        }

    private:

        typedef std::array<ValueType, BlockSize> BlockType;
        typedef std::unique_ptr<BlockType>       BlockPointer;
        typedef std::vector<BlockPointer>        BlockSequence;
        typedef typename BlockType::iterator     BlockIterator;

        LinearArrayAllocator(LinearArrayAllocator const&);
        LinearArrayAllocator& operator=(LinearArrayAllocator const&);

        void EnsureAvailable(SizeType const n)
        {
            if (n > BlockSize)
                throw std::bad_alloc("Size exceeds maximum block size");

            if (_blocks.size() > 0)
            {
                if (static_cast<SizeType>(std::distance(_current, _blocks.back()->end())) >= n)
                    return;
            }

            _blocks.emplace_back(new BlockType);
            _current = _blocks.back()->begin();
        }

        BlockSequence _blocks;
        BlockIterator _current;
    };

    static const std::uint32_t InvalidMetadataTokenValue = 0x00000000;
    static const std::uint32_t MetadataTokenKindMask     = 0xFF000000;

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

        MetadataTokenKind GetType() const
        {
            //TODO Verify([&]{ return IsInitialized(); });
            return static_cast<MetadataTokenKind>(_token & MetadataTokenKindMask);
        }

        bool IsInitialized() const
        {
            return _token != InvalidMetadataTokenValue;
        }

    private:

        std::uint32_t _token;
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

    template <typename T>
    std::reverse_iterator<std::uint8_t const*> BeginReverseBytes(T const& p)
    {
        return std::reverse_iterator<std::uint8_t const*>(EndBytes(p));
    }

    template <typename T>
    std::reverse_iterator<std::uint8_t const*> EndReverseBytes(T const& p)
    {
        return std::reverse_iterator<std::uint8_t const*>(BeginBytes(p));
    }

    template <typename T>
    std::reverse_iterator<std::uint8_t const*> BeginBigEndianBytes(T const& p)
    {
        return std::reverse_iterator<std::uint8_t const*>(EndBytes(p)); // TODO Fix for big-endian machines?
    }

    template <typename T>
    std::reverse_iterator<std::uint8_t const*> EndBigEndianBytes(T const& p)
    {
        return std::reverse_iterator<std::uint8_t const*>(BeginBytes(p)); // TODO Fix for big-endian machines?
    }

} }

#endif
