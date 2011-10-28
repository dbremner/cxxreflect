//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header contains declarations and definitions of core types, functions, and constants used
// throughout the CxxReflect library.
//
// The CxxReflect library is divided into several namespaces:
// * ::CxxReflect              The public API
// * ::CxxReflect::Detail      Implementation details that are not part of the public API
// * ::CxxReflect::Metadata    The physical layer metatadata reader for parsing metadata files

#ifndef CXXREFLECT_CORE_HPP_
#define CXXREFLECT_CORE_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#ifdef _DEBUG
#define CXXREFLECT_LOGIC_CHECKS
#endif

// #define CXXREFLECT_ENABLE_WINRT_RESOLVER

namespace CxxReflect {

    // VerificationFailure exceptions are thrown only when CXXREFLECT_LOGIC_CHECKS is set.  It should
    // only be thrown when the error is an actual logic error, never when the error is something that
    // could conceivably occur at runtime if the code is correct.
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

    inline void VerifyNotNull(void const*) { }

    template <typename TCallable>
    void Verify(TCallable&&, char const* = "") { }

    #endif

    // A handful of useful algorithms that we use throughout the library.

    template <typename TRanIt, typename TValue, typename TComparer>
    TRanIt BinarySearch(TRanIt first, TRanIt last, TValue const& value, TComparer const comparer)
    {
        TRanIt const it(std::lower_bound(first, last, value, comparer));
        if (it == last || *it != value)
        {
            return last;
        }

        return it;
    }

    template <typename TInIt, typename TOutIt>
    void RangeCheckedCopy(TInIt first0, TInIt const last0, TOutIt first1, TOutIt const last1)
    {
        while (first0 != last0 && first1 != last1)
        {
            *first1 = *first0;
            ++first0;
            ++first1;
        }
    }

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

        // We only provide read-only access to the encapsulated data, so all of these are const; we
        // provide the full set of typedefs though so this is a drop-in replacement for std::string
        // (at least for core sceanrios).
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

        template <size_type N>
        EnhancedCString(value_type (&data)[N])
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

        // Note that unlike std::string, the size of an EnhancedCString includes its null terminator
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

    template <typename T, typename U>
    bool operator==(EnhancedCString<T> const& lhs, std::basic_string<U> const& rhs)
    {
        return RangeCheckedEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <typename T, typename U>
    bool operator==(std::basic_string<T> const& lhs, EnhancedCString<U> const& rhs)
    {
        return RangeCheckedEqual(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <typename T, typename U>
    bool operator==(EnhancedCString<T> const& lhs, U const* const rhs)
    {
        StringReference const temporaryRhs(rhs);
        return RangeCheckedEqual(lhs.begin(), lhs.end(), temporaryRhs.begin(), temporaryRhs.end());
    }

    template <typename T, typename U>
    bool operator==(U const* const lhs, EnhancedCString<U> const& rhs)
    {
        StringReference const temporaryLhs(lhs);
        return RangeCheckedEqual(temporaryLhs.begin(), temporaryLhs.end(), rhs.begin(), rhs.end());
    }

    template <typename T, typename U>
    bool operator< (EnhancedCString<T> const& lhs, std::basic_string<U> const& rhs)
    {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    template <typename T, typename U>
    bool operator< (std::basic_string<T> const& lhs, EnhancedCString<U> const& rhs)
    {
        return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

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

        bool IsSet(EnumerationType const mask) const { return WithMask(mask) != 0; }
        bool IsSet(IntegralType    const mask) const { return WithMask(mask) != 0; }

        FlagSet WithMask(EnumerationType const mask) const
        {
            return WithMask(static_cast<IntegralType>(mask));
        }

        FlagSet WithMask(IntegralType const mask) const
        {
            return FlagSet(_value & mask);
        }

        friend bool operator==(FlagSet const& lhs, FlagSet const& rhs) { return lhs._value == rhs._value; }
        friend bool operator< (FlagSet const& lhs, FlagSet const& rhs) { return lhs._value <  rhs._value; }

        friend bool operator!=(FlagSet const& lhs, FlagSet const& rhs) { return !(lhs == rhs); }
        friend bool operator> (FlagSet const& lhs, FlagSet const& rhs) { return   rhs <  lhs ; }
        friend bool operator<=(FlagSet const& lhs, FlagSet const& rhs) { return !(rhs <  lhs); }
        friend bool operator>=(FlagSet const& lhs, FlagSet const& rhs) { return !(lhs <  rhs); }

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

    // A "smart" pointer that is guaranteed to be either not initialized or not null.  If it is
    // default-initialized, then it is flagged as being "uninitialized" (meaning it is null);
    // however, it cannot be constructed with a null pointer.

    template <typename T>
    class NonNull
    {
    public:

        typedef typename std::remove_pointer<T>::type ValueType;
        typedef ValueType*                            Pointer;
        typedef ValueType&                            Reference;

        NonNull()
            : _pointer(nullptr)
        {
        }

        NonNull(Pointer const pointer)
            : _pointer(pointer)
        {
            VerifyInitialized();
        }

        template <typename U>
        NonNull(U const pointer)
            : _pointer(pointer)
        {
            VerifyInitialized();
        }

        Pointer   Get()        const { VerifyInitialized(); return _pointer;  }
        Pointer   operator->() const { VerifyInitialized(); return _pointer;  }
        Reference operator*()  const { VerifyInitialized(); return *_pointer; }

        bool IsInitialized() const { return _pointer != nullptr; }

    private:

        void VerifyInitialized() const
        {
            Verify([&] { return IsInitialized(); });
        }

        Pointer _pointer;
    };

    template <typename T, typename U> bool operator==(NonNull<T> const& lhs, NonNull<U> const& rhs) { return lhs.Get() == rhs.Get(); }
    template <typename T, typename U> bool operator==(NonNull<T> const& lhs, U*                rhs) { return lhs.Get() == rhs;       }
    template <typename T, typename U> bool operator==(T*                lhs, NonNull<U> const& rhs) { return lhs       == rhs.Get(); }
    template <typename T>             bool operator==(NonNull<T> const& lhs, std::nullptr_t       ) { return lhs.Get() == nullptr;   }
    template <typename T>             bool operator==(std::nullptr_t,        NonNull<T> const& rhs) { return rhs.Get() == nullptr;   }

    template <typename T, typename U> bool operator!=(NonNull<T> const& lhs, NonNull<U> const& rhs) { return !(lhs == rhs);          }
    template <typename T, typename U> bool operator!=(NonNull<T> const& lhs, U*                rhs) { return !(lhs == rhs);          }
    template <typename T, typename U> bool operator!=(T*                lhs, NonNull<U> const& rhs) { return !(lhs == rhs);          }
    template <typename T>             bool operator!=(NonNull<T> const& lhs, std::nullptr_t       ) { return lhs.IsInitialized();    }
    template <typename T>             bool operator!=(std::nullptr_t,        NonNull<T> const& rhs) { return rhs.IsInitialized();    }

    template <typename T, typename U> bool operator< (NonNull<T> const& lhs, NonNull<U> const& rhs) { return lhs.Get() <  rhs.Get(); }
    template <typename T, typename U> bool operator> (NonNull<T> const& lhs, NonNull<U> const& rhs) { return lhs.Get() >  rhs.Get(); }
    template <typename T, typename U> bool operator<=(NonNull<T> const& lhs, NonNull<U> const& rhs) { return lhs.Get() <= rhs.Get(); }
    template <typename T, typename U> bool operator>=(NonNull<T> const& lhs, NonNull<U> const& rhs) { return lhs.Get() >= rhs.Get(); }


    template <typename T>
    class ValueInitialized
    {
    public:

        typedef T ValueType;

        ValueInitialized()
            : _x()
        {
        }

        explicit ValueInitialized(ValueType const& x)
            : _x(x)
        {
        }

        ValueType      & Get()       { return _x; }
        ValueType const& Get() const { return _x; }

    private:

        ValueType _x;
    };

    template <typename T>
    struct Identity
    {
        typedef T Type;
    };

    // A safe-bool implementation based largely on Bjorn Karlsson's canonical implementation, found
    // at http://www.artima.com/cppsource/safebool.html.

    template <typename TDerived>
    class SafeBoolConvertible
    {
    private:

        typedef void (SafeBoolConvertible::*FauxBoolType)() const;

        void ThisTypeDoesNotSupportComparisons() const { }

    protected:

        SafeBoolConvertible() { }
        SafeBoolConvertible(SafeBoolConvertible const&) { }
        SafeBoolConvertible& operator=(SafeBoolConvertible const&) { return *this; }
        ~SafeBoolConvertible() { }

    public:

        operator FauxBoolType() const
        {
            // !! is no good because if we forget to implement operator! in the derived class, the !!
            // will find this conversion and we will recurse infinitely.  The explicit .operator!()
            // call requires that the conversion operator is present.
            return !static_cast<TDerived const&>(*this).operator!()
                ? &SafeBoolConvertible::ThisTypeDoesNotSupportComparisons
                : 0;
        }
    };

    // Suppress unwanted equatability
    template <typename T, typename U>
    void operator==(SafeBoolConvertible<T> const& lhs, SafeBoolConvertible<U> const&)
    {
        lhs.ThisTypeDoesNotSupportComparisons();
    }

    template <typename T, typename U>
    void operator!=(SafeBoolConvertible<T> const& lhs, SafeBoolConvertible<U> const&)
    {
        lhs.ThisTypeDoesNotSupportComparisons();
    }


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

    // TTableReference is always Metadata::TableReference.  We use a template parameter to make it
    // dependent so that we don't need it defined when we define the template.
    template <typename TTableReference, typename TResult, typename TParameter>
    class TableTransformIterator
    {
    public:

        typedef std::random_access_iterator_tag iterator_category;
        typedef TResult                         value_type;
        typedef TResult                         reference;
        typedef Dereferenceable<TResult>        pointer;
        typedef std::ptrdiff_t                  difference_type;

        typedef value_type                      ValueType;
        typedef reference                       Reference;
        typedef pointer                         Pointer;
        typedef difference_type                 DifferenceType;

        TableTransformIterator()
        {
        }

        TableTransformIterator(TParameter const parameter, TTableReference const current)
            : _parameter(parameter), _current(current)
        {
        }

        Reference Get()        const { return ValueType(_parameter, _current); }
        Reference operator*()  const { return ValueType(_parameter, _current); }
        Pointer   operator->() const { return ValueType(_parameter, _current); }

        TableTransformIterator& operator++()    { *this += 1; return *this;                    }
        TableTransformIterator  operator++(int) { auto const it(*this); *this += 1; return it; }

        TableTransformIterator& operator--()    { *this -= 1; return *this;                    }
        TableTransformIterator  operator--(int) { auto const it(*this); *this -= 1; return it; }

        TableTransformIterator& operator+=(DifferenceType const n)
        {
            _current = TTableReference(_current.GetTable(), _current.GetIndex() + n);
            return *this;
        }

        TableTransformIterator& operator-=(DifferenceType const n) { return *this += -n; }

        Reference operator[](DifferenceType const n) const
        {
            return ValueType(_parameter, TTableReference(_current.GetTable(), _current.GetIndex() + n));
        }

        friend TableTransformIterator operator+(TableTransformIterator it, DifferenceType n) { return it += n; }
        friend TableTransformIterator operator+(DifferenceType n, TableTransformIterator it) { return it += n; }
        friend TableTransformIterator operator-(TableTransformIterator it, DifferenceType n) { return it -= n; }

        friend DifferenceType operator-(TableTransformIterator const& lhs, TableTransformIterator const& rhs)
        {
            return DifferenceType(lhs._current.GetIndex()) - DifferenceType(rhs._current.GetIndex());
        }

        friend bool operator==(TableTransformIterator const& lhs, TableTransformIterator const& rhs)
        {
            return lhs._current.GetIndex() == rhs._current.GetIndex();
        }

        friend bool operator< (TableTransformIterator const& lhs, TableTransformIterator const& rhs)
        {
            return lhs._current.GetIndex() < rhs._current.GetIndex();
        }

        friend bool operator!=(TableTransformIterator const& lhs, TableTransformIterator const& rhs) { return !(lhs == rhs); }
        friend bool operator> (TableTransformIterator const& lhs, TableTransformIterator const& rhs) { return   rhs <  lhs ; }
        friend bool operator<=(TableTransformIterator const& lhs, TableTransformIterator const& rhs) { return !(rhs <  lhs); }
        friend bool operator>=(TableTransformIterator const& lhs, TableTransformIterator const& rhs) { return !(lhs <  rhs); }

    private:

        TParameter      _parameter;
        TTableReference _current;
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

// We forward declare all of physical metadata types so that we don't need to include the
// MetadataDatabase header everywhere; in many cases, declarations are enough.  These types should
// be treated as internal and should not be used by clients of the CxxReflect library.
namespace CxxReflect { namespace Metadata {

    class Database;
    class Stream;
    class StringCollection;
    class BlobReference;
    class TableCollection;
    class Table;
    class TableReference;

    class AssemblyRow;
    class AssemblyOsRow;
    class AssemblyProcessorRow;
    class AssemblyRefRow;
    class AssemblyRefOsRow;
    class AssemblyRefProcessorRow;
    class ClassLayoutRow;
    class ConstantRow;
    class CustomAttributeRow;
    class DeclSecurityRow;
    class EventMapRow;
    class EventRow;
    class ExportedTypeRow;
    class FieldRow;
    class FieldLayoutRow;
    class FieldMarshalRow;
    class FieldRvaRow;
    class FileRow;
    class GenericParamRow;
    class GenericParamConstraintRow;
    class ImplMapRow;
    class InterfaceImplRow;
    class ManifestResourceRow;
    class MemberRefRow;
    class MethodDefRow;
    class MethodImplRow;
    class MethodSemanticsRow;
    class MethodSpecRow;
    class ModuleRow;
    class ModuleRefRow;
    class NestedClassRow;
    class ParamRow;
    class PropertyRow;
    class PropertyMapRow;
    class StandaloneSigRow;
    class TypeDefRow;
    class TypeRefRow;
    class TypeSpecRow;

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

    typedef std::basic_string<Character>       String;
    typedef Detail::EnhancedCString<Character> StringReference;

    class Assembly;
    class AssemblyName;
    class File;
    class IMetadataResolver;
    class MetadataLoader;
    class Module;
    class Type;
    class Version;

    enum class AssemblyAttribute : std::uint32_t
    {
        PublicKey                  = 0x0001,
        Retargetable               = 0x0100,
        DisableJitCompileOptimizer = 0x4000,
        EnableJitCompileTracking   = 0x8000,

        // TODO PORTABILITY THESE ARE NOT IN THE SPEC
        DefaultContentType         = 0x0000,
        WindowsRuntimeContentType  = 0x0200,
        ContentTypeMask            = 0x0E00
    };

    enum class AssemblyHashAlgorithm : std::uint32_t
    {
        None     = 0x0000,
        MD5      = 0x8003,
        SHA1     = 0x8004
    };

    enum class EventAttribute : std::uint16_t
    {
        SpecialName        = 0x0200,
        RuntimeSpecialName = 0x0400
    };

    enum class FieldAttribute : std::uint16_t
    {
        FieldAccessMask    = 0x0007,

        CompilerControlled = 0x0000,
        Private            = 0x0001,
        FamilyAndAssembly  = 0x0002,
        Assembly           = 0x0003,
        Family             = 0x0004,
        FamilyOrAssembly   = 0x0005,
        Public             = 0x0006,

        Static             = 0x0010,
        InitOnly           = 0x0020,
        Literal            = 0x0040,
        NotSerialized      = 0x0080,
        SpecialName        = 0x0200,

        PInvokeImpl        = 0x2000,

        RuntimeSpecialName = 0x0400,
        HasFieldMarshal    = 0x1000,
        HasDefault         = 0x8000,
        HasFieldRva        = 0x0100
    };

    enum class FileAttribute : std::uint32_t
    {
        ContainsMetadata   = 0x0000,
        ContainsNoMetadata = 0x0001
    };

    enum class GenericParameterAttribute : std::uint16_t
    {
        VarianceMask                   = 0x0003,
        None                           = 0x0000,
        Covariant                      = 0x0001,
        Contravariant                  = 0x0002,

        SpecialConstraintMask          = 0x001c,
        ReferenceTypeConstraint        = 0x0004,
        NotNullableValueTypeConstraint = 0x0008,
        DefaultConstructorConstraint   = 0x0010
    };

    enum class ManifestResourceAttribute : std::uint32_t
    {
        VisibilityMask = 0x0007,
        Public         = 0x0001,
        Private        = 0x0002
    };

    enum class MethodAttribute : std::uint16_t
    {
        MemberAccessMask      = 0x0007,
        CompilerControlled    = 0x0000,
        Private               = 0x0001,
        FamilyAndAssembly     = 0x0002,
        Assembly              = 0x0003,
        Family                = 0x0004,
        FamilyOrAssembly      = 0x0005,
        Public                = 0x0006,

        Static                = 0x0010,
        Final                 = 0x0020,
        Virtual               = 0x0040,
        HideBySig             = 0x0080,

        VTableLayoutMask      = 0x0100,
        ReuseSlot             = 0x0000,
        NewSlot               = 0x0100,

        Strict                = 0x0200,
        Abstract              = 0x0400,
        SpecialName           = 0x0800,

        PInvokeImpl           = 0x2000,
        RuntimeSpecialName    = 0x1000,
        HasSecurity           = 0x4000,
        RequireSecurityObject = 0x8000
    };

    enum class MethodImplementationAttribute : std::uint16_t
    {
        CodeTypeMask   = 0x0003,
        IL             = 0x0000,
        Native         = 0x0001,
        Runtime        = 0x0003,

        ManagedMask    = 0x0004,
        Unmanaged      = 0x0004,
        Managed        = 0x0000,

        ForwardRef     = 0x0010,
        PreserveSig    = 0x0080,
        InternalCall   = 0x1000,
        Synchronized   = 0x0020,
        NoInlining     = 0x0008,
        NoOptimization = 0x0040
    };

    enum class MethodSemanticsAttribute : std::uint16_t
    {
        Setter   = 0x0001,
        Getter   = 0x0002,
        Other    = 0x0004,
        AddOn    = 0x0008,
        RemoveOn = 0x0010,
        Fire     = 0x0020
    };

    enum class ParameterAttribute : std::uint16_t
    {
        In              = 0x0001,
        Out             = 0x0002,
        Optional        = 0x0010,
        HasDefault      = 0x1000,
        HasFieldMarshal = 0x2000
    };

    enum class PInvokeAttribute : std::uint16_t
    {
        NoMangle                     = 0x0001,

        CharacterSetMask             = 0x0006,
        CharacterSetNotSpecified     = 0x0000,
        CharacterSetAnsi             = 0x0002,
        CharacterSetUnicode          = 0x0004,
        CharacterSetAuto             = 0x0006,

        SupportsLastError            = 0x0040,

        CallingConventionMask        = 0x0700,
        CallingConventionPlatformApi = 0x0100,
        CallingConventionCDecl       = 0x0200,
        CallingConventionStdCall     = 0x0300,
        CallingConventionThisCall    = 0x0400,
        CallingConventionFastCall    = 0x0500
    };

    enum class PropertyAttribute : std::uint16_t
    {
        SpecialName        = 0x0200,
        RuntimeSpecialName = 0x0400,
        HasDefault         = 0x1000
    };

    enum class TypeAttribute : std::uint32_t
    {
        VisibilityMask          = 0x00000007,
        NotPublic               = 0x00000000,
        Public                  = 0x00000001,
        NestedPublic            = 0x00000002,
        NestedPrivate           = 0x00000003,
        NestedFamily            = 0x00000004,
        NestedAssembly          = 0x00000005,
        NestedFamilyAndAssembly = 0x00000006,
        NestedFamilyOrAssembly  = 0x00000007,

        LayoutMask              = 0x00000018,
        AutoLayout              = 0x00000000,
        SequentialLayout        = 0x00000008,
        ExplicitLayout          = 0x00000010,

        ClassSemanticsMask      = 0x00000020,
        Class                   = 0x00000000,
        Interface               = 0x00000020,

        Abstract                = 0x00000080,
        Sealed                  = 0x00000100,
        SpecialName             = 0x00000400,

        Import                  = 0x00001000,
        Serializable            = 0x00002000,

        StringFormatMask        = 0x00030000,
        AnsiClass               = 0x00000000,
        UnicodeClass            = 0x00010000,
        AutoClass               = 0x00020000,
        CustomFormatClass       = 0x00030000,
        CustomStringFormatMask  = 0x00c00000,

        BeforeFieldInit         = 0x00100000,

        RuntimeSpecialName      = 0x00000800,
        HasSecurity             = 0x00040000,
        IsTypeForwarder         = 0x00200000
    };

    typedef Detail::FlagSet<AssemblyAttribute>             AssemblyFlags;
    typedef Detail::FlagSet<EventAttribute>                EventFlags;
    typedef Detail::FlagSet<FieldAttribute>                FieldFlags;
    typedef Detail::FlagSet<FileAttribute>                 FileFlags;
    typedef Detail::FlagSet<GenericParameterAttribute>     GenericParameterFlags;
    typedef Detail::FlagSet<ManifestResourceAttribute>     ManifestResourceFlags;
    typedef Detail::FlagSet<MethodAttribute>               MethodFlags;
    typedef Detail::FlagSet<MethodImplementationAttribute> MethodImplementationFlags;
    typedef Detail::FlagSet<MethodSemanticsAttribute>      MethodSemanticsFlags;
    typedef Detail::FlagSet<ParameterAttribute>            ParameterFlags;
    typedef Detail::FlagSet<PInvokeAttribute>              PInvokeFlags;
    typedef Detail::FlagSet<PropertyAttribute>             PropertyFlags;
    typedef Detail::FlagSet<TypeAttribute>                 TypeFlags;

}

#endif
