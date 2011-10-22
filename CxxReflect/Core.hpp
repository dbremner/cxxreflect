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

namespace CxxReflect { namespace detail {

    struct verification_failure : std::logic_error
    {
        verification_failure(char const* const message = "")
            : std::logic_error(message)
        {
        }
    };

    #ifdef CXXREFLECT_LOGIC_CHECKS

    inline void verify_fail(char const* const message = "")
    {
        throw verification_failure(message);
    }

    inline void verify_not_null(void const* const p)
    {
        if (p == nullptr)
            throw verification_failure("Unexpected null pointer");
    }

    template <typename TCallable>
    void verify(TCallable&& callable, char const* const message = "")
    {
        if (!callable())
             throw verification_failure(message);
    }

    #else

    inline void verify_fail(char const*) { }

    inline void verify_not_null(void const*, char const* = "") { }

    template <typename TCallable>
    void verify(TCallable&&) { }

    #endif

    // A handful of useful algorithms that we use throughout the library.

    template <typename TInIt0, typename TInIt1>
    bool range_checked_equal(TInIt0 first0, TInIt0 const last0, TInIt1 first1, TInIt1 const last1)
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
    class enhanced_cstring
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

        enhanced_cstring()
            : _first(nullptr), _last(nullptr)
        {
        }

        explicit enhanced_cstring(pointer const first)
            : _first(first), _last(first)
        {
            if (first == nullptr)
                return;

            while (*_last != 0)
                ++_last;

            ++_last; // One-past-the-end of the null terminator
        }

        enhanced_cstring(pointer const first, pointer const last)
            : _first(first), _last(last)
        {
        }

        template <size_type N>
        enhanced_cstring(value_type const (&data)[N])
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

        friend bool operator==(enhanced_cstring const& lhs, enhanced_cstring const& rhs)
        {
            return range_checked_equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }

        friend bool operator<(enhanced_cstring const& lhs, enhanced_cstring const& rhs)
        {
            return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
        }
        
        friend bool operator!=(enhanced_cstring const& lhs, enhanced_cstring const& rhs) { return !(lhs == rhs); }
        friend bool operator> (enhanced_cstring const& lhs, enhanced_cstring const& rhs) { return   rhs <  lhs ; }
        friend bool operator<=(enhanced_cstring const& lhs, enhanced_cstring const& rhs) { return !(rhs <  lhs); }
        friend bool operator>=(enhanced_cstring const& lhs, enhanced_cstring const& rhs) { return !(lhs <  rhs); }

        // TODO Consider implementing some of the rest of the std::string interface

    private:

        pointer _first;
        pointer _last;
    };

    template <typename T>
    std::basic_ostream<T>& operator<<(std::basic_ostream<T>& os, enhanced_cstring<T> const& s)
    {
        os << s.c_str();
        return os;
    }

    /*
    template <typename T>
    class ownable_string
    {
    public:

        typedef enhanced_cstring<T>  nonowned_string;
        typedef std::basic_string<T> owned_string;

        ownable_string(nonowned_string const& s)
            : _kind(kind::nonowned)
        {
            new (&_data) nonowned_string(s);
        }

        ownable_string(owned_string const& s)
            : _kind(kind::owned)
        {
            new (&_data) owned_string(s);
        }

        void own_copy()
        {
            if (_kind != kind::nonowned)
                return;

            nonowned_string s(get_nonowned());

            get_nonowned().~nonowned_string();

            _kind = kind::owned;
            scope_guard reset_kind([&] { _kind = kind::none; });
            new (&_data) owned_string(s.c_str());
            reset_kind.unset();
        }

    private:

        enum class kind { none, owned, nonowned };

        typedef typename std::aligned_union<0, nonowned_string, owned_string>::type storage_type;

        owned_string      & get_owned()       { return reinterpret_cast<owned_string      &>(_data); }
        owned_string const& get_owned() const { return reinterpret_cast<owned_string const&>(_data); }

        nonowned_string      & get_nonowned()       { return reinterpret_cast<nonowned_string      &>(_data); }
        nonowned_string const& get_nonowned() const { return reinterpret_cast<nonowned_string const&>(_data); }

        kind         _kind;
        storage_type _data;
    };
    */

    // Utility types and functions for encapsulating reinterpretation of an object as a char[].
    // These help reduce the occurrence of reinterpret_cast in the code and make it easier to
    // copy data into and out of POD-struct types.

    typedef std::uint8_t                               byte;
    typedef std::uint8_t*                              byte_iterator;
    typedef std::uint8_t const*                        const_byte_iterator;
    typedef std::reverse_iterator<byte_iterator>       reverse_byte_iterator;
    typedef std::reverse_iterator<const_byte_iterator> const_reverse_byte_iterator;

    template <typename T>
    byte_iterator begin_bytes(T& x)
    {
        return reinterpret_cast<byte_iterator>(&x);
    }

    template <typename T>
    byte_iterator end_bytes(T& x)
    {
        return reinterpret_cast<byte_iterator>(&x + 1);
    }

    template <typename T>
    const_byte_iterator begin_bytes(T const& x)
    {
        return reinterpret_cast<const_byte_iterator>(&x);
    }

    template <typename T>
    const_byte_iterator end_bytes(T const& x)
    {
        return reinterpret_cast<const_byte_iterator>(&x + 1);
    }

    template <typename T>
    reverse_byte_iterator rbegin_bytes(T& p)
    {
        return reverse_byte_iterator(end_bytes(p));
    }

    template <typename T>
    reverse_byte_iterator rend_bytes(T& p)
    {
        return reverse_byte_iterator(begin_bytes(p));
    }

    template <typename T>
    const_reverse_byte_iterator rbegin_bytes(T const& p)
    {
        return const_reverse_byte_iterator(end_bytes(p));
    }

    template <typename T>
    const_reverse_byte_iterator rend_bytes(T const& p)
    {
        return const_reverse_byte_iterator(begin_bytes(p));
    }

    // A scope-guard class that performs an operation on destruction.  The implementation is "good
    // enough" for most uses, though its use of std::function, which may itself perform dynamic
    // allocation, makes it unsuitable for "advanced" use.

    class scope_guard
    {
    public:

        typedef std::function<void()> function_type;

        explicit scope_guard(function_type const f)
            : _f(f)
        {
        }

        ~scope_guard()
        {
            if (_f != nullptr)
                _f();
        }

        void unset()
        {
            _f = nullptr;
        }

    private:

        function_type _f;
    };

    // A basic RAII wrapper around the cstdio file interfaces; this allows us to get the performance
    // of the C runtime APIs wih the convenience of the C++ iostream interfaces.

    struct file_read_exception : std::runtime_error
    {
        file_read_exception(char const* const message)
            : std::runtime_error(message)
        {
        }
    };

    class file_handle
    {
    public:

        typedef std::int64_t position_type;
        typedef std::size_t  size_type;

        enum origin_type
        {
            begin,   // SEEK_SET
            current, // SEEK_CUR
            end      // SEEK_END
        };

        file_handle(wchar_t const* const fileName, wchar_t const* const mode = L"rb")
        {
            // TODO PORTABILITY
            errno_t const result(_wfopen_s(&_handle, fileName, mode));
            if (result != 0)
                throw file_read_exception("File open failed");
        }

        file_handle(file_handle&& other)
            : _handle(other._handle)
        {
            other._handle = nullptr;
        }

        file_handle& operator=(file_handle&& other)
        {
            swap(other);
        }

        ~file_handle()
        {
            if (_handle != nullptr)
                fclose(_handle);
        }

        void swap(file_handle& other)
        {
            std::swap(_handle, other._handle);
        }

        void seek(position_type const position, origin_type const whence)
        {
            int seek_whence(0);
            switch (whence)
            {
            case begin:   seek_whence = SEEK_SET; break;
            case current: seek_whence = SEEK_CUR; break;
            case end:     seek_whence = SEEK_END; break;
            default:      verify_fail("Invalid origin specified");
            }

            // TODO PORTABILITY
            if (_fseeki64(_handle, position, seek_whence) != 0)
                throw file_read_exception("File seek failed");
        }

        void read(void* const buffer, size_type const size, size_type const count)
        {
            if (fread(buffer, size, count, _handle) != count)
                throw file_read_exception("File seek failed");
        }

    private:

        file_handle(file_handle const&);
        file_handle& operator=(file_handle const&);

        FILE* _handle;
    };

    // A flag set, similar to std::bitset, but with implicit conversions to and from an enumeration
    // type.  This is essential for working with C++11 enum classes, which do not have implicit
    // conversions to and from their underlying integral type.

    template <typename TEnumeration>
    class flag_set
    {
    public:

        static_assert(std::is_enum<TEnumeration>::value, "TEnumeration must be an enumeration");

        typedef TEnumeration                                      enumeration_type;
        typedef typename std::underlying_type<TEnumeration>::type integral_type;

        flag_set()
            : _value()
        {
        }

        flag_set(enumeration_type const value)
            : _value(static_cast<integral_type>(value))
        {
        }

        flag_set(integral_type const value)
            : _value(value)
        {
        }

        enumeration_type get_enum() const
        {
            return static_cast<enumeration_type>(_value);
        }

        integral_type get_integral() const
        {
            return _value;
        }

        flag_set with_mask(enumeration_type const mask) const
        {
            return with_mask(static_cast<integral_type>(mask));
        }

        flag_set with_mask(integral_type const mask) const
        {
            return flag_set(_value & mask);
        }

        // TODO This is lacking a lot of functionality

    private:

        integral_type _value;
    };

    template <typename TEnumeration>
    typename std::underlying_type<TEnumeration>::type as_integer(TEnumeration value)
    {
        return static_cast<typename std::underlying_type<TEnumeration>::type>(value);
    }


    // A fake dereferenceable type.  This is useful for implementing operator-> for an iterator
    // where the element referenced by the iterator does not actually exist (e.g., where the
    // iterator materializes elements, and where the iterator's reference is not a reference type).

    template <typename T>
    class dereferenceable
    {
    public:

        typedef T        value_type;
        typedef T&       reference;
        typedef T const& const_reference;
        typedef T*       pointer;
        typedef T const* const_pointer;

        dereferenceable(const_reference value)
            : _value(value)
        {
        }

        reference       get()              { return _value;  }
        const_reference get()        const { return _value;  }

        pointer         operator->()       { return &_value; }
        const_pointer   operator->() const { return &_value; }

    private:

        value_type _value;
    };

    // A linear allocator for arrays; this is most useful for the allocation of strings.
    template <typename T, std::size_t TBlockSize>
    class linear_array_allocator
    {
    public:

        typedef std::size_t size_type;
        typedef T           value_type;
        typedef T*          pointer;

        enum { block_size = TBlockSize };

        class range
        {
        public:

            range()
                : _begin(nullptr), _end(nullptr)
            {
            }

            range(pointer const begin, pointer const end)
                : _begin(begin), _end(end)
            {
            }

            pointer begin() const { return _begin; }
            pointer end()   const { return _end;   }

        private:

            pointer _begin;
            pointer _end;
        };

        linear_array_allocator()
        {
        }

        linear_array_allocator(linear_array_allocator&& other)
            : _blocks(std::move(other._blocks)),
              _current(std::move(other._current))
        {
            other._current = block_iterator();
        }

        linear_array_allocator& operator=(linear_array_allocator&& other)
        {
            swap(other);
            return *this;
        }

        void swap(linear_array_allocator& other)
        {
            std::swap(other._blocks,  _blocks);
            std::swap(other._current, _current);
        }

        range allocate(size_type const n)
        {
            ensure_available(n);

            range const r(&*_current, &*_current + n);
            _current += n;
            return r;
        }

    private:

        typedef std::array<value_type, block_size> block;
        typedef typename block::iterator           block_iterator;
        typedef std::unique_ptr<block>             block_pointer;
        typedef std::vector<block_pointer>         block_sequence;
        

        // Noncopyable
        linear_array_allocator(linear_array_allocator const&);
        linear_array_allocator& operator=(linear_array_allocator const&);

        void ensure_available(size_type const n)
        {
            if (n > block_size)
                throw std::out_of_range("n");

            if (_blocks.size() > 0)
            {
                if (static_cast<size_type>(std::distance(_current, _blocks.back()->end())) >= n)
                    return;
            }

            _blocks.emplace_back(new block);
            _current = _blocks.back()->begin();
        }

        block_sequence _blocks;
        block_iterator _current;
    };

} }

namespace CxxReflect {

    // These types are used throughout the library.  TODO:  Currently we assume that wchar_t is
    // a UTF-16 string representation, as is the case on Windows.  We should make that more general
    // and allow multiple encodings in the public interface and support platforms that use other
    // encodings by default for wchar_t.
    typedef wchar_t                             Character;
    typedef std::size_t                         SizeType;
    typedef std::uint8_t                        Byte;
    typedef std::uint8_t const*                 ByteIterator;

    typedef detail::enhanced_cstring<Character> String;

}

namespace CxxReflect { namespace Detail {

    template <typename T, typename TAllocator = std::allocator<T>>
    class AllocatorBasedArray
    {
    public:

        AllocatorBasedArray()
            : _data(nullptr), _capacity(0), _size(0)
        {
        }

        ~AllocatorBasedArray()
        {
            if (_data == nullptr) { return; }

            for (std::size_t i(_size - 1); i != static_cast<std::size_t>(-1); --i)
            {
                TAllocator().destroy(_data + i);
            }

            TAllocator().deallocate(_data, _capacity);
        }

        T*          Get()         const { return _data;         }
        std::size_t GetCapacity() const { return _capacity;     }
        std::size_t GetSize()     const { return _size;         }

        T*          Begin()       const { return _data;         }
        T*          End()         const { return _data + _size; }

        void Allocate(std::size_t capacity)
        {
            if (_data != nullptr)
                throw std::logic_error("The array has already been allocated.");

            _data = TAllocator().allocate(capacity);
            _capacity = capacity;
        }

        void EmplaceBack()
        {
            VerifyAvailable();
            TAllocator().construct(_data + _size);
            ++_size;
        }

        template <typename A0>
        void EmplaceBack(A0&& a0)
        {
            VerifyAvailable();
            TAllocator().construct(_data + _size, std::move(a0));
            ++_size;
        }

        template <typename A0, typename A1>
        void EmplaceBack(A0&& a0, A1&& a1)
        {
            VerifyAvailable();
            TAllocator().construct(_data + _size, std::move(a0), std::move(a1));
            ++_size;
        }

        template <typename A0, typename A1, typename A2>
        void EmplaceBack(A0&& a0, A1&& a1, A2&& a2)
        {
            VerifyAvailable();
            TAllocator().construct(_data + _size, std::move(a0), std::move(a1), std::move(a2));
            ++_size;
        }

    private:

        AllocatorBasedArray(AllocatorBasedArray const&);
        AllocatorBasedArray& operator=(AllocatorBasedArray const&);

        void VerifyAvailable() const
        {
            if (_data == nullptr || _capacity - _size == 0)
                throw std::logic_error("There is insufficient space avalilable in the array.");
        }

        T*                 _data;
        std::size_t        _capacity;
        std::size_t        _size;
    };

} }

#endif
