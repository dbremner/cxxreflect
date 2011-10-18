//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef PELOADER_UTILITY_HPP_
#define PELOADER_UTILITY_HPP_

#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#define CXXREFLECT_DEBUG

namespace PeLoader { namespace Utility {

    struct VerificationFailure : std::logic_error
    {
        VerificationFailure(char const* const message) : std::logic_error(message) { }
    };

    #ifdef CXXREFLECT_DEBUG

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

} }

#endif
