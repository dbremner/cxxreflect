//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_HPP_
#define CXXREFLECT_CORE_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

#ifdef _DEBUG
#define CXXREFLECT_LOGIC_CHECKS
#endif

// #define CXXREFLECT_ENABLE_WINRT_RESOLVER

namespace CxxReflect {

    struct VerificationFailure : std::logic_error
    {
        explicit VerificationFailure(char const* const message = "")
            : std::logic_error(message)
        {
        }
    };

    struct RuntimeError : std::runtime_error
    {
        explicit RuntimeError(char const* const message = "")
            : std::runtime_error(message)
        {
        }
    };

    struct HResultException : RuntimeError
    {
        explicit HResultException(int hresult, char const* const message = "")
            : RuntimeError(message), _hresult(hresult)
        {
        }

        int GetHResult() const { return _hresult; }

    private:

        int _hresult;
    };

}

namespace CxxReflect { namespace Detail {

    #ifdef CXXREFLECT_LOGIC_CHECKS

    inline void VerifyFail(char const* const message = "")
    {
        throw VerificationFailure(message);
    }

    inline void VerifyNotNull(void const* const p)
    {
        if (p == nullptr)
            throw VerificationFailure("Unexpected null pointer");
    }

    template <typename TCallable>
    void Verify(TCallable&& callable, char const* const message = "")
    {
        if (!callable())
             throw VerificationFailure(message);
    }

    #else

    inline void VerifyFail(char const*) { }

    inline void VerifyNotNull(void const*, char const* = "") { }

    template <typename TCallable>
    void Verify(TCallable&&) { }

    #endif

    // A handful of useful algorithms that we use throughout the library.

    template <typename TInIt0, typename TInIt1>
    bool RangeCheckedEqual(TInIt0 first0, TInIt0 const last0, TInIt1 first1, TInIt1 const last1)
    {
        while (first0 != last0 && first1 != last1 && *first0 == *first1)
        {
            ++first0;
            ++first1;
        }

        return first0 == last0 && first1 == last1;
    }

    // A string class that provides a simplified std::string-like interface around a C string.  This
    // class does not perform any memory management:  it simply has pointers into an existing null-
    // terminated string;.  the creator is responsible for managing the memory of the underlying data.

    template <typename T>
    class EnhancedCString
    {
    public:

        typedef T                 value_type;
        typedef std::size_t       size_type;
        typedef std::ptrdiff_t    difference_type;

        // We only provide read-only access to the encapsulated data.
        typedef value_type const& reference;
        typedef value_type const& const_reference;
        typedef value_type const* pointer;
        typedef value_type const* const_pointer;

        typedef pointer                               iterator;
        typedef const_pointer                         const_iterator;
        typedef std::reverse_iterator<iterator>       reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        EnhancedCString()
            : _first(nullptr), _last(nullptr)
        {
        }

        explicit EnhancedCString(pointer const first)
            : _first(first), _last(first)
        {
            if (first == nullptr)
                return;

            while (*_last != 0)
                ++_last;

            ++_last; // One-past-the-end of the null terminator
        }

        EnhancedCString(pointer const first, pointer const last)
            : _first(first), _last(last)
        {
        }

        template <size_type N>
        EnhancedCString(value_type const (&data)[N])
            : _first(data), _last(data + N)
        {
        }

        const_iterator begin()  const { return _first; }
        const_iterator end()    const { return _last;  }

        const_iterator cbegin() const { return _first; }
        const_iterator cend()   const { return _last;  }

        const_reverse_iterator rbegin()  const { return reverse_iterator(_last);  }
        const_reverse_iterator rend()    const { return reverse_iterator(_first); }

        const_reverse_iterator crbegin() const { return reverse_iterator(_last);  }
        const_reverse_iterator crend()   const { return reverse_iterator(_first); }

        size_type size()     const { return _last - _first;                          }
        size_type length()   const { return size();                                  }
        size_type max_size() const { return std::numeric_limits<std::size_t>::max(); }
        size_type capacity() const { return size();                                  }
        bool      empty()    const { return size() == 0;                             }

        const_reference operator[](size_type const n) const
        {
            return _first[n];
        }

        const_reference at(size_type const n) const
        {
            if (n >= size())
                throw std::out_of_range("n");

            return _first[n];
        }

        const_reference front() const { return *_first;      }
        const_reference back()  const { return *(_last - 1); }

        const_pointer c_str() const { return _first; }
        const_pointer data()  const { return _first; }

        friend bool operator==(EnhancedCString const& lhs, EnhancedCString const& rhs)
        {
            return RangeCheckedEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }

        friend bool operator<(EnhancedCString const& lhs, EnhancedCString const& rhs)
        {
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }
        
        friend bool operator!=(EnhancedCString const& lhs, EnhancedCString const& rhs) { return !(lhs == rhs); }
        friend bool operator> (EnhancedCString const& lhs, EnhancedCString const& rhs) { return   rhs <  lhs ; }
        friend bool operator<=(EnhancedCString const& lhs, EnhancedCString const& rhs) { return !(rhs <  lhs); }
        friend bool operator>=(EnhancedCString const& lhs, EnhancedCString const& rhs) { return !(lhs <  rhs); }

        // TODO Consider implementing some of the rest of the std::string interface

    private:

        pointer _first;
        pointer _last;
    };

    template <typename T>
    std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, EnhancedCString<T> const& s)
    {
        os << s.c_str();
        return os;
    }

    // Utility types and functions for encapsulating reinterpretation of an object as a char[].
    // These help reduce the occurrence of reinterpret_cast in the code and make it easier to
    // copy data into and out of POD-struct types.

    typedef std::uint8_t                             Byte;
    typedef std::uint8_t*                            ByteIterator;
    typedef std::uint8_t const*                      ConstByteIterator;
    typedef std::reverse_iterator<ByteIterator>      ReverseByteIterator;
    typedef std::reverse_iterator<ConstByteIterator> ConstReverseByteIterator;

    template <typename T>
    ByteIterator BeginBytes(T& x)
    {
        return reinterpret_cast<ByteIterator>(&x);
    }

    template <typename T>
    ByteIterator EndBytes(T& x)
    {
        return reinterpret_cast<ByteIterator>(&x + 1);
    }

    template <typename T>
    ConstByteIterator BeginBytes(T const& x)
    {
        return reinterpret_cast<ConstByteIterator>(&x);
    }

    template <typename T>
    ConstByteIterator EndBytes(T const& x)
    {
        return reinterpret_cast<ConstByteIterator>(&x + 1);
    }

    template <typename T>
    ReverseByteIterator ReverseBeginBytes(T& p)
    {
        return ReverseByteIterator(EndBytes(p));
    }

    template <typename T>
    ReverseByteIterator ReverseEndBytes(T& p)
    {
        return ReverseByteIterator(BeginBytes(p));
    }

    template <typename T>
    ConstReverseByteIterator ReverseBeginBytes(T const& p)
    {
        return ConstReverseByteIterator(EndBytes(p));
    }

    template <typename T>
    ConstReverseByteIterator ReverseEndBytes(T const& p)
    {
        return ConstReverseByteIterator(BeginBytes(p));
    }

    // A scope-guard class that performs an operation on destruction.  The implementation is "good
    // enough" for most uses, though its use of std::function, which may itself perform dynamic
    // allocation, makes it unsuitable for "advanced" use.

    class ScopeGuard
    {
    public:

        typedef std::function<void()> FunctionType;

        explicit ScopeGuard(FunctionType const f)
            : _f(f)
        {
        }

        ~ScopeGuard()
        {
            if (_f != nullptr)
                _f();
        }

        void Unset()
        {
            _f = nullptr;
        }

    private:

        FunctionType _f;
    };

    // A basic RAII wrapper around the cstdio file interfaces; this allows us to get the performance
    // of the C runtime APIs wih the convenience of the C++ iostream interfaces.

    struct FileReadException : std::runtime_error
    {
        FileReadException(char const* const message)
            : std::runtime_error(message)
        {
        }
    };

    class FileHandle
    {
    public:

        typedef std::int64_t PositionType;
        typedef std::size_t  SizeType;

        enum OriginType
        {
            Begin   = SEEK_SET,
            Current = SEEK_CUR,
            End     = SEEK_END
        };

        FileHandle(wchar_t const* const fileName, wchar_t const* const mode = L"rb")
        {
            // TODO PORTABILITY
            errno_t const result(_wfopen_s(&_handle, fileName, mode));
            if (result != 0)
                throw FileReadException("File open failed");
        }

        FileHandle(FileHandle&& other)
            : _handle(other._handle)
        {
            other._handle = nullptr;
        }

        FileHandle& operator=(FileHandle&& other)
        {
            Swap(other);
        }

        ~FileHandle()
        {
            if (_handle != nullptr)
                fclose(_handle);
        }

        void Swap(FileHandle& other)
        {
            std::swap(_handle, other._handle);
        }

        void Seek(PositionType const position, OriginType const origin)
        {
            // TODO PORTABILITY
            if (_fseeki64(_handle, position, origin) != 0)
                throw FileReadException("File seek failed");
        }

        void Read(void* const buffer, SizeType const size, SizeType const count)
        {
            if (fread(buffer, size, count, _handle) != count)
                throw FileReadException("File seek failed");
        }

    private:

        FileHandle(FileHandle const&);
        FileHandle& operator=(FileHandle const&);

        FILE* _handle;
    };

    // A flag set, similar to std::bitset, but with implicit conversions to and from an enumeration
    // type.  This is essential for working with C++11 enum classes, which do not have implicit
    // conversions to and from their underlying integral type.

    template <typename TEnumeration>
    class FlagSet
    {
    public:

        static_assert(std::is_enum<TEnumeration>::value, "TEnumeration must be an enumeration");

        typedef TEnumeration                                      EnumerationType;
        typedef typename std::underlying_type<TEnumeration>::type IntegralType;

        FlagSet()
            : _value()
        {
        }

        FlagSet(EnumerationType const value)
            : _value(static_cast<IntegralType>(value))
        {
        }

        FlagSet(IntegralType const value)
            : _value(value)
        {
        }

        EnumerationType GetEnum() const
        {
            return static_cast<EnumerationType>(_value);
        }

        IntegralType GetIntegral() const
        {
            return _value;
        }

        FlagSet with_mask(EnumerationType const mask) const
        {
            return WithMask(static_cast<IntegralType>(mask));
        }

        FlagSet WithMask(IntegralType const mask) const
        {
            return FlagSet(_value & mask);
        }

        // TODO This is lacking a lot of functionality

    private:

        IntegralType _value;
    };

    template <typename TEnumeration>
    typename std::underlying_type<TEnumeration>::type AsInteger(TEnumeration value)
    {
        return static_cast<typename std::underlying_type<TEnumeration>::type>(value);
    }


    // A fake dereferenceable type.  This is useful for implementing operator-> for an iterator
    // where the element referenced by the iterator does not actually exist (e.g., where the
    // iterator materializes elements, and where the iterator's reference is not a reference type).

    template <typename T>
    class Dereferenceable
    {
    public:

        typedef T        ValueType;
        typedef T&       Reference;
        typedef T const& ConstReference;
        typedef T*       Pointer;
        typedef T const* ConstPointer;

        Dereferenceable(ConstReference value)
            : _value(value)
        {
        }

        Reference      Get()              { return _value;  }
        ConstReference Get()        const { return _value;  }

        Pointer        operator->()       { return &_value; }
        ConstPointer   operator->() const { return &_value; }

    private:

        ValueType _value;
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

        typedef std::array<ValueType, BlockSize> Block;
        typedef typename Block::iterator         BlockIterator;
        typedef std::unique_ptr<Block>           BlockPointer;
        typedef std::vector<BlockPointer>        BlockSequence;
        

        // Noncopyable
        LinearArrayAllocator(LinearArrayAllocator const&);
        LinearArrayAllocator& operator=(LinearArrayAllocator const&);

        void EnsureAvailable(SizeType const n)
        {
            if (n > BlockSize)
                throw std::out_of_range("n");

            if (_blocks.size() > 0)
            {
                if (static_cast<SizeType>(std::distance(_current, _blocks.back()->end())) >= n)
                    return;
            }

            _blocks.emplace_back(new Block);
            _current = _blocks.back()->begin();
        }

        BlockSequence _blocks;
        BlockIterator _current;
    };

    // Platform functionality wrappers:  these functions use platform-specific, third-party, or non-
    // standard types and functions; we encapsulate these functions here to make it easier to port
    // the library.

    unsigned ComputeUtf16LengthOfUtf8String(char const* source);
    bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength);

    typedef std::array<std::uint8_t, 20> Sha1Hash;

    // Computes the 20 byte SHA1 hash for the bytes in the range [first, last).
    Sha1Hash ComputeSha1Hash(std::uint8_t const* first, std::uint8_t const* last);

    bool FileExists(wchar_t const* filePath);

} }

namespace CxxReflect {

    // These types are used throughout the library.  TODO:  Currently we assume that wchar_t is
    // a UTF-16 string representation, as is the case on Windows.  We should make that more general
    // and allow multiple encodings in the public interface and support platforms that use other
    // encodings by default for wchar_t.
    typedef wchar_t                            Character;
    typedef std::size_t                        SizeType;
    typedef std::uint8_t                       Byte;
    typedef std::uint8_t const*                ByteIterator;

    typedef Detail::EnhancedCString<Character> String;

}

#endif
