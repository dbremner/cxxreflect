#ifndef CXXREFLECT_FUNDAMENTALS_HPP_
#define CXXREFLECT_FUNDAMENTALS_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Fundamental types, functions, and constants used throughout the library.

#include "CxxReflect/Configuration.hpp"
#include "CxxReflect/ExternalFunctions.hpp"
#include "CxxReflect/ExternalFunctionsWin32.hpp"
#include "CxxReflect/ExternalFunctionsWinRT.hpp"





//
//
// EXCEPTIONS, ASSERTIONS, AND ERROR HANDLING
//
//

namespace CxxReflect {

    // We have our own exception hierarchy because everything in this library uses wide strings, but
    // the C++ Standard Library exceptions use narrow strings.  We still derive from std::exception
    // though, and we override what() to return a narrow string.

    class Exception : public std::exception
    {
    public:

        explicit Exception(String message = L"")
            : std::exception(Externals::ConvertWideStringToNarrowString(message.c_str()).c_str()),
              _message(std::move(message))
        {
        }

        ~Exception() throw()
        {
            // Required to override base-class virtual destructor
        }

        String const& GetMessage() const
        {
            return _message;
        }

    private:

        String _message;
    };

    class LogicError : public Exception
    {
    public:

        explicit LogicError(String message = L"")
            : Exception(message)
        {
        }
    };

    class RuntimeError : public Exception
    {
    public:

        explicit RuntimeError(String message = L"")
            : Exception(std::move(message))
        {
        }
    };

    class HResultRuntimeError : public RuntimeError
    {
    public:

        explicit HResultRuntimeError(HResult const hresult, String message = L"")
            : RuntimeError(std::move(message)), _hresult(hresult)
        {
        }

        HResult GetHResult() const
        {
            return _hresult;
        }

    private:

        HResult _hresult;
    };

    class FileIOError : public RuntimeError
    {
    public:

        explicit FileIOError(CharacterIterator const message, int const error = errno)
            : RuntimeError(message), _error(error)
        {
        }

        explicit FileIOError(int const error = errno)
            : RuntimeError(Externals::ConvertNarrowStringToWideString(std::strerror(error))),
              _error(error)
        {
        }

        int GetError() const
        {
            return _error;
        }

    private:

        int _error;
    };

    class MetadataReadError : public RuntimeError
    {
    public:

        MetadataReadError(String message)
            : RuntimeError(std::move(message))
        {
        }
    };

}

namespace CxxReflect { namespace Detail {

    #ifdef CXXREFLECT_ENABLE_DEBUG_ASSERTIONS

    inline void AssertFail(CharacterIterator const message = L"")
    {
        throw LogicError(message);
    }

    inline void AssertNotNull(void const* const p)
    {
        if (p == nullptr)
            throw LogicError(L"Unexpected null pointer");
    }

    template <typename TCallable>
    void Assert(TCallable&& callable, CharacterIterator const message = L"")
    {
        if (!callable())
             throw LogicError(message);
    }

    inline void AssertSuccess(HResult const hresult, CharacterIterator const message = L"")
    {
        if (hresult < 0)
            throw HResultRuntimeError(hresult, message);
    }

    #else

    inline void AssertFail(CharacterIterator = L"") { }

    inline void AssertNotNull(void const*) { }

    template <typename TCallable>
    void Assert(TCallable&&, CharacterIterator = L"") { }

    inline void AssertSuccess(HResult, CharacterIterator = L"") { }

    #endif

    template <typename TCallable>
    void Verify(TCallable&& callable, CharacterIterator const message = L"")
    {
        if (!callable())
             throw RuntimeError(message);
    }

    inline void VerifySuccess(HResult const hresult, CharacterIterator const message = L"")
    {
        if (hresult < 0)
            throw HResultRuntimeError(hresult, message);
    }

} }





//
//
// ALGORITHMS AND STANDARD LIBRARY ALGORITHM WRAPPERS
//
//

namespace CxxReflect { namespace Detail {

    #ifdef CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS

    // These Assert that a sequence is ordered according to a particular strict weak ordering. These
    // are useful with the unchecked debug algorithms defined here because they allow us to Assert a
    // sequence's ordering once, then assume that it is ordered for all future searches.
    
    template <typename TForIt>
    void AssertStrictWeakOrdering(TForIt const first, TForIt const last)
    {
        for (TForIt current(first), next(first); current != last && ++next != last; ++current)
            if (*next < *current)
                throw LogicError("Sequence is not ordered");
    }

    template <typename TForIt, typename TPredicate>
    void AssertStrictWeakOrdering(TForIt const first, TForIt const last, TPredicate predicate)
    {
        for (TForIt current(first), next(first); current != last && ++next != last; ++current)
            if (predicate(*next, *current))
                throw LogicError("Sequence is not ordered");
    }

    // Visual C++ Iterator Debugging verifies that the input range satisfies the strict-weak order
    // requirement each time that equal_range is called. This is extrodinarily slow for large ranges
    // (or even for small ranges, if we call equal_range enough times). We have copied here the
    // implementation of the Visual C++ Standard Library's equal_range but omitted the _DEBUG_ORDER
    // call from the beginning of reach function.
    template <typename TForIt, typename TValue>
    ::std::pair<TForIt, TForIt> EqualRange(TForIt first, TForIt last, const TValue& value)
    {
        auto const result(::std::_Equal_range(
            ::std::_Unchecked(first),
            ::std::_Unchecked(last),
            value,
            ::std::_Dist_type(first)));

        return ::std::pair<TForIt, TForIt>(
            ::std::_Rechecked(first, result.first),
            ::std::_Rechecked(last, result.second));
    }

    template <typename TForIt, typename TValue, typename TPredicate>
    ::std::pair<TForIt, TForIt> EqualRange(TForIt first, TForIt last, const TValue& value, TPredicate predicate)
    {
        auto const result(::std::_Equal_range(
            ::std::_Unchecked(first),
            ::std::_Unchecked(last),
            value,
            predicate,
            ::std::_Dist_type(first)));

        return ::std::pair<TForIt, TForIt>(
            ::std::_Rechecked(first, result.first),
            ::std::_Rechecked(last, result.second));
    }

    #else // !defined(CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS)

    template <typename TForIt>
    void AssertStrictWeakOrdering(TForIt const&, TForIt const&)
    {
        return;
    }

    template <typename TForIt, typename TPredicate>
    void AssertStrictWeakOrdering(TForIt const&, TForIt const&, TPredicate const&)
    {
        return;
    }

    template <typename TForIt, typename TValue>
    ::std::pair<TForIt, TForIt> EqualRange(TForIt first, TForIt last, const TValue& value)
    {
        return ::std::equal_range(first, last, value);
    }

    template <typename TForIt, typename TValue, typename TPredicate>
    ::std::pair<TForIt, TForIt> EqualRange(TForIt first, TForIt last, const TValue& value, TPredicate predicate)
    {
        return ::std::equal_range(first, last, value, predicate);
    }

    #endif

    // We frequently compute the distance between two iterators and compare with a size; to avoid
    // lots of casts and to avoid unsigned/signed comparison warnings, we cast to unsigned here:
    template <typename TForIt>
    SizeType Distance(TForIt const first, TForIt const last)
    {
        // Note:  The result of std::distance is always positive.
        return static_cast<SizeType>(std::distance(first, last));
    }

    template <typename TRanIt, typename TValue, typename TComparer>
    TRanIt BinarySearch(TRanIt const first, TRanIt const last, TValue const& value, TComparer const comparer)
    {
        TRanIt const it(::std::lower_bound(first, last, value, comparer));
        if (it == last || comparer(*it, value) || comparer(value, *it))
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

    template <typename TInIt0, typename TInIt1, typename TPred>
    bool RangeCheckedEqual(TInIt0 first0, TInIt0 const last0, TInIt1 first1, TInIt1 const last1, TPred const pred)
    {
        while (first0 != last0 && first1 != last1 && pred(*first0, *first1))
        {
            ++first0;
            ++first1;
        }

        return first0 == last0 && first1 == last1;
    }

    template <typename TString>
    TString MakeLowercase(TString s)
    {
        std::transform(s.begin(), s.end(), s.begin(), (int(*)(std::wint_t))std::tolower);
        return s;
    }

    template <typename T>
    struct Identity
    {
        typedef T Type;
    };

} }





//
//
// SCOPED ENUMERATION UTILITIES
//
//

namespace CxxReflect { namespace Detail {

    template <typename TEnumeration>
    typename std::underlying_type<TEnumeration>::type AsInteger(TEnumeration value)
    {
        return static_cast<typename std::underlying_type<TEnumeration>::type>(value);
    }

    #define CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EVAL_EVAL(E, Op)               \
        inline E operator Op(E const lhs, E const rhs)                                      \
        {                                                                                   \
            return static_cast<E>(::CxxReflect::Detail::AsInteger(lhs)                      \
                               Op ::CxxReflect::Detail::AsInteger(rhs));                    \
        }

    #define CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EREF_EVAL(E, Op)               \
        inline E& operator Op##=(E& lhs, E const rhs)                                       \
        {                                                                                   \
            lhs = static_cast<E>(::CxxReflect::Detail::AsInteger(lhs)                       \
                              Op ::CxxReflect::Detail::AsInteger(rhs));                     \
            return lhs;                                                                     \
        }

    #define CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE(E, Op)                 \
        inline bool operator Op(E const lhs, std::underlying_type<E>::type rhs)             \
        {                                                                                   \
            return ::CxxReflect::Detail::AsInteger(lhs) Op rhs;                             \
        }                                                                                   \
                                                                                            \
        inline bool operator Op(std::underlying_type<E>::type const lhs, E const rhs)       \
        {                                                                                   \
            return lhs Op ::CxxReflect::Detail::AsInteger(rhs);                             \
        }

    #define CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(E)                                    \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EVAL_EVAL(E, |)                    \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EVAL_EVAL(E, &)                    \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EVAL_EVAL(E, ^)                    \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EREF_EVAL(E, |)                    \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EREF_EVAL(E, &)                    \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EREF_EVAL(E, ^)                    \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, ==)                   \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, !=)                   \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, < )                   \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, > )                   \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, <=)                   \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, >=)

} }





//
//
// GENERATORS FOR COMMON OPERATOR OVERLOADS
//
//

namespace CxxReflect { namespace Detail {

    // NOTE WELL:  We use macros here for performance.  These overloads can also be introduced via a
    // base class template, however, Visual C++ will not fully perform Empty-Base Optimization (EBO)
    // in the presence of multiple inheritance. This has a disasterous effect on many other critical
    // optimizations (for example, member function inlining is hindered due to the additional offset
    // computations that are required).  Thus, we use macros to generate the overloads.

    // Generators for comparison operators, based on == and < operators that must be user-defined.
    #define CXXREFLECT_GENERATE_EQUALITY_OPERATORS(T)                                       \
        friend bool operator!=(T const& lhs, T const& rhs) { return !(lhs == rhs); }

    #define CXXREFLECT_GENERATE_RELATIONAL_OPERATORS(T) \
        friend bool operator> (T const& lhs, T const& rhs) { return   rhs <  lhs ; }        \
        friend bool operator<=(T const& lhs, T const& rhs) { return !(rhs <  lhs); }        \
        friend bool operator>=(T const& lhs, T const& rhs) { return !(lhs <  rhs); }

    #define CXXREFLECT_GENERATE_COMPARISON_OPERATORS(T)                                     \
        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(T)                                           \
        CXXREFLECT_GENERATE_RELATIONAL_OPERATORS(T)





    // Generators for addition and subtraction operators for types that are pointer-like (i.e. types
    // that use indices or pointers of some kind to point to elements in a sequence).  These are a
    // bit flakey and we abuse them for types that don't support all of them, but they work well.
    #define CXXREFLECT_GENERATE_ADDITION_OPERATORS(the_type, get_value, difference)         \
        the_type& operator++()    { ++(*this).get_value; return *this;              }       \
        the_type  operator++(int) { auto const it(*this); ++*this; return *this;    }       \
                                                                                            \
        the_type& operator+=(difference const n)                                            \
        {                                                                                   \
            typedef std::remove_reference<decltype((*this).get_value)>::type ValueType;     \
            (*this).get_value = static_cast<ValueType>((*this).get_value + n);              \
            return *this;                                                                   \
        }                                                                                   \
                                                                                            \
        friend the_type operator+(the_type it, difference n) { return it += n; }            \
        friend the_type operator+(difference n, the_type it) { return it += n; }

    #define CXXREFLECT_GENERATE_SUBTRACTION_OPERATORS(the_type, get_value, difference)      \
        the_type& operator--()    { --(*this).get_value; return *this;              }       \
        the_type  operator--(int) { auto const it(*this); --*this; return *this;    }       \
                                                                                            \
        the_type& operator-=(difference const n)                                            \
        {                                                                                   \
            typedef std::remove_reference<decltype((*this).get_value)>::type ValueType;     \
            (*this).get_value = static_cast<ValueType>((*this).get_value - n);              \
            return *this;                                                                   \
        }                                                                                   \
                                                                                            \
        friend the_type operator-(the_type it, difference n) { return it -= n; }            \
                                                                                            \
        friend difference operator-(the_type lhs, the_type rhs)                             \
        {                                                                                   \
            return static_cast<difference>(lhs.get_value - rhs.get_value);                  \
        }

    #define CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(type, get_value, difference) \
        CXXREFLECT_GENERATE_ADDITION_OPERATORS(type, get_value, difference)                 \
        CXXREFLECT_GENERATE_SUBTRACTION_OPERATORS(type, get_value, difference)




    // Generator for the safe-bool operator
    #define CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(type)                                  \
    private:                                                                                \
                                                                                            \
        typedef void (type::*FauxBoolType)() const;                                         \
                                                                                            \
        void ThisTypeDoesNotSupportComparisons() const { }                                  \
                                                                                            \
    public:                                                                                 \
                                                                                            \
        operator FauxBoolType() const                                                       \
        {                                                                                   \
            return !(*this).operator!()                                                     \
                ? &type::ThisTypeDoesNotSupportComparisons                                  \
                : nullptr;                                                                  \
        }

} }





//
//
// CHAR[] REINTERPRETATION AND BYTE ITERATION UTILITIES
//
//

namespace CxxReflect { namespace Detail {

    // The low-level database components and other library components rely heavily on reinterpreting
    // objects as arrays of char.  This is legit, and we use these functions to avoid having to use
    // reinterpret_cast all over the freaking place.

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

} }





//
//
// ENHANCED C STRING WRAPPER
//
//

namespace CxxReflect { namespace Detail {



    // A string class that provides a simplified std::string-like interface around a C string.  This
    // class does not perform any memory management:  it simply has pointers into an existing null-
    // terminated string.  the creator is responsible for managing the memory of the underlying data.
    template <typename T>
    class EnhancedCString
    {
    public:

        typedef T                 value_type;
        typedef std::size_t       size_type;
        typedef std::ptrdiff_t    difference_type;

        // We only provide read-only access to the encapsulated data, so all of these are const; we
        // provide the full set of typedefs though so this is a drop-in replacement for std::string
        // (at least for core uses).
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

        // We mark this explicit so that the constructor templates taking value_type[N] are
        // preferred for string literals (and other arrays); that constructor provides O(1) end
        // pointer computation.
        explicit EnhancedCString(const_pointer const first)
            : _first(first), _last(nullptr)
        {
        }

        EnhancedCString(const_pointer const first, const_pointer const last)
            : _first(first), _last(last)
        {
        }

        template <size_type N>
        EnhancedCString(value_type const (&data)[N])
            : _first(data), _last(data + N - 1)
        {
        }

        template <size_type N>
        EnhancedCString(value_type (&data)[N])
            : _first(data), _last(data + N - 1)
        {
        }

        const_iterator begin()  const { return _first;          }
        const_iterator end()    const { return compute_last();  }

        const_iterator cbegin() const { return _first;          }
        const_iterator cend()   const { return compute_last();  }

        const_reverse_iterator rbegin()  const { return reverse_iterator(compute_last());  }
        const_reverse_iterator rend()    const { return reverse_iterator(_first); }

        const_reverse_iterator crbegin() const { return reverse_iterator(compute_last());  }
        const_reverse_iterator crend()   const { return reverse_iterator(_first);          }

        // Note that unlike std::string, the size of an EnhancedCString includes its null terminator
        size_type size()     const { return compute_last() - _first;                 }
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
        const_reference back()  const { return *(compute_last() - 1); }

        const_pointer c_str() const { return _first; }
        const_pointer data()  const { return _first; }

        // We avoid using equal_range and lexicographic_compare here because they potentially
        // require two passes over each string:  once to compute end(), which is lazy, and once to
        // perform the comparison.  We can do the computation and comparison in one pass, which
        // we do in compare_until_end<compare>().

        friend bool operator==(EnhancedCString const& lhs, EnhancedCString const& rhs)
        {
            return compare_until_end<std::equal_to>(lhs, rhs);
        }

        friend bool operator<(EnhancedCString const& lhs, EnhancedCString const& rhs)
        {
            return compare_until_end<std::less>(lhs, rhs);
        }

        template <template <typename> class TCompare>
        static bool compare_until_end(EnhancedCString const& lhs, EnhancedCString const& rhs)
        {
            const_pointer lhs_it(lhs.begin());
            const_pointer rhs_it(rhs.begin());

            // First, treat a null pointer as an empty string:
            if (lhs_it == nullptr && rhs_it == nullptr)
                return TCompare<value_type>()(0, 0);

            else if (lhs_it == nullptr && rhs_it != nullptr)
                return TCompare<value_type>()(0, 1);

            else if (lhs_it != nullptr && rhs_it == nullptr)
                return TCompare<value_type>()(1, 0);

            // Next, if both strings are valid, compare them using the provided comparator:
            while (*lhs_it != 0 && *rhs_it != 0 && TCompare<value_type>()(*lhs_it, *rhs_it))
            {
                ++lhs_it;
                ++rhs_it;
            }

            // Finally, set the '_last' pointers for both strings if they don't have them set:
            if (lhs._last == nullptr && *lhs_it == '\0')
                lhs._last = lhs_it;

            if (rhs._last == nullptr && *rhs_it == '\0')
                rhs._last = rhs_it;

            return *lhs_it == 0 && *rhs_it == 0;
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(EnhancedCString)

        // TODO Consider implementing some of the rest of the std::string interface

    private:

        pointer compute_last() const
        {
            if (_last != nullptr)
                return _last;

            if (_first == nullptr)
                return _last;

            _last = _first;
            while (*_last != 0)
                ++_last;

            return _last;
        }

        pointer         _first;

        // NOTE:  Do not access '_last' directly:  it is lazily computed by 'compute_last()'.  Call
        // that function instead.  In the case where we get only a pointer to a C String, computation
        // of the '_last' pointer requires a linear scan of the string.  We don't typically need the
        // '_last' pointer, and profiling shows that the linear scan is absurdly expensive.
        pointer mutable _last;
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
        EnhancedCString<U> const temporaryRhs(rhs);
        return RangeCheckedEqual(lhs.begin(), lhs.end(), temporaryRhs.begin(), temporaryRhs.end());
    }

    template <typename T, typename U>
    bool operator==(U const* const lhs, EnhancedCString<U> const& rhs)
    {
        EnhancedCString<T> const temporaryLhs(lhs);
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

} }





//
//
// MISCELLANEOUS UTILITY CLASSES (ScopeGuard, FlagSet, Dereferenceable, ValueInitialized)
//
//

namespace CxxReflect { namespace Detail {

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

        void Set(EnumerationType const mask) { _value |= AsInteger(mask); }
        void Set(IntegralType    const mask) { _value |= mask;            }

        void Unset(EnumerationType const mask) { _value &= ~AsInteger(mask); }
        void Unset(IntegralType    const mask) { _value &= ~mask;            }

        void Reset() { _value = 0; }

        bool IsSet(EnumerationType const mask) const { return WithMask(mask) != 0; }
        bool IsSet(IntegralType    const mask) const { return WithMask(mask) != 0; }

        FlagSet WithMask(EnumerationType const mask) const
        {
            return _value & static_cast<IntegralType>(mask);
        }

        FlagSet WithMask(IntegralType const mask) const
        {
            return _value & mask;
        }

        friend bool operator==(FlagSet const& lhs, FlagSet const& rhs) { return lhs._value == rhs._value; }
        friend bool operator< (FlagSet const& lhs, FlagSet const& rhs) { return lhs._value <  rhs._value; }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(FlagSet)

    private:

        IntegralType _value;
    };

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

    // A value-initialization wrapper for use with member variables of POD type, to ensure that they
    // are always initialized.
    template <typename T>
    class ValueInitialized
    {
    public:

        typedef T ValueType;

        ValueInitialized()
            : _value()
        {
        }

        explicit ValueInitialized(ValueType const& value)
            : _value(value)
        {
        }

        ValueType      & Get()       { return _value; }
        ValueType const& Get() const { return _value; }

        void Reset()
        {
            _value.~ValueType();
            new (&_value) ValueType();
        }

    private:

        ValueType _value;
    };

} }





//
//
// C STDIO FILE API RAII WRAPPER
//
//

namespace CxxReflect { namespace Detail {

    // We avoid using the <iostream> library for performance reasons. (Really, its performance sucks;
    // this isn't a rant, it really does suck.  The <cstdio> library outperforms <iostream> for one
    // of the main unit test apps by well over 30x.  This wrapper gives us most of the convenience of
    // <iostream> with the awesome performance of <cstdio>.

    // Wrap a number with HexFormat before inserting the number into the stream. This will cause the
    // number to be written in hexadecimal format.
    class HexFormat
    {
    public:

        explicit HexFormat(unsigned int const value)
            : _value(value)
        {
        }

        unsigned int GetValue() const { return _value; }

    private:

        unsigned int _value;
    };

    enum class FileMode : std::uint8_t
    {
        ReadWriteAppendMask = 0x03,
        Read                = 0x01, // r
        Write               = 0x02, // w
        Append              = 0x03, // a

        UpdateMask          = 0x04,
        NonUpdate           = 0x00,
        Update              = 0x04, // +

        TextBinaryMask      = 0x08,
        Text                = 0x00,
        Binary              = 0x08  // b
    };

    typedef FlagSet<FileMode> FileModeFlags;

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(FileMode)

    enum class FileOrigin : std::uint8_t
    {
        Begin   = SEEK_SET,
        Current = SEEK_CUR,
        End     = SEEK_END
    };

    class FileHandle
    {
    public:

        typedef std::int64_t PositionType;
        typedef std::size_t  SizeType;

        // This is the mapping of <cstdio> functions to FileHandle member functions:
        // fclose    Close
        // feof      IsEof
        // ferror    IsError
        // fflush    Flush
        // fgetc     GetChar
        // fgetpos   GetPosition
        // fgets
        // fopen     [Constructor]
        // fprintf   operator<<
        // fputc     PutChar
        // fputs     operator<<
        // fread     Read
        // freopen   [Not Implemented]
        // fscanf
        // fseek     Seek
        // fsetpos   SetPosition
        // ftell     Tell
        // fwrite    Write
        // getc      GetChar
        // putc      PutChar
        // puts      operator<<
        // rewind    [Not Implemented]

        enum OriginType
        {
            Begin   = SEEK_SET,
            Current = SEEK_CUR,
            End     = SEEK_END
        };

        FileHandle(wchar_t const* const fileName, FileModeFlags const mode)
            : _mode(mode), _handle(Externals::OpenFile(fileName, TranslateMode(mode)))
        {
        }

        FileHandle(FileHandle&& other)
            : _handle(other._handle)
        {
            other._handle = nullptr;
        }

        FileHandle& operator=(FileHandle&& other)
        {
            Swap(other);
            return *this;
        }

        ~FileHandle()
        {
            if (_handle != nullptr)
                std::fclose(_handle);
        }

        void Swap(FileHandle& other)
        {
            std::swap(_handle, other._handle);
        }

        void Close()
        {
            // This is safe to call on a closed stream
            FILE* const localHandle(_handle);
            _handle = nullptr;

            if (localHandle != nullptr && std::fclose(localHandle) == EOF)
                throw FileIOError();
        }

        void Flush()
        {
            AssertOutputStream();
            if (std::fflush(_handle) == EOF)
                throw FileIOError();
        }

        int GetChar()
        {
            AssertInputStream();
            int const value(std::fgetc(_handle));
            if (value == EOF)
                throw FileIOError();

            return value;
        }

        fpos_t GetPosition() const
        {
            AssertInitialized();

            fpos_t position((fpos_t()));
            if (std::fgetpos(_handle, &position) != 0)
                throw FileIOError();

            return position;
        }

        bool IsEof() const
        {
            AssertInitialized();
            return std::feof(_handle) != 0;
        }

        bool IsError() const
        {
            AssertInitialized();
            return std::ferror(_handle) != 0;
        }

        void PutChar(unsigned char const character)
        {
            AssertOutputStream();
            if (std::fputc(character, _handle))
                throw FileIOError();
        }

        void Read(void* const buffer, SizeType const size, SizeType const count)
        {
            AssertInputStream();
            if (std::fread(buffer, size, count, _handle) != count)
                throw FileIOError();
        }

        template <typename T>
        void Read(T* const buffer, SizeType const count)
        {
            Detail::AssertNotNull(buffer);
            Detail::Assert([&](){ return count > 0; });

            return this->Read(buffer, sizeof *buffer, count);
        }

        void Seek(PositionType const position, OriginType const origin)
        {
            AssertInitialized();
            // TODO PORTABILITY
            if (::_fseeki64(_handle, position, origin) != 0)
                throw FileIOError();
        }

        void SetPosition(fpos_t const position)
        {
            AssertInitialized();
            if (std::fsetpos(_handle, &position) != 0)
                throw FileIOError();
        }

        PositionType Tell() const
        {
            AssertInitialized();
            // TODO PORTABILITY
            return ::_ftelli64(_handle);
        }

        void UngetChar(unsigned char character)
        {
            AssertInputStream();
            // No errors are specified for ungetc, so if an error occurs, we don't know what it is:
            if (std::ungetc(character, _handle) == EOF)
                throw FileIOError(L"An unknown error occurred when ungetting");
        }

        void Write(void const* const data, SizeType const size, SizeType const count)
        {
            AssertOutputStream();
            if (std::fwrite(data, size, count, _handle) != count)
                throw FileIOError();
        }

        #define CXXREFLECT_GENERATE(t, f)       \
            FileHandle& operator<<(t x)         \
            {                                   \
                std::fprintf(_handle, f, x);    \
                return *this;                   \
            }

        CXXREFLECT_GENERATE(char const* const,    "%s" )
        CXXREFLECT_GENERATE(wchar_t const* const, "%ls")
        CXXREFLECT_GENERATE(int const,            "%i" )
        CXXREFLECT_GENERATE(unsigned int const,   "%u" )
        CXXREFLECT_GENERATE(double const,         "%g" )

        #undef CXXREFLECT_GENERATE

        FileHandle& operator<<(HexFormat const x)
        {
            std::fprintf(_handle, "%08x", x.GetValue());
            return *this;
        }

    private:

        FileHandle(FileHandle const&);
        FileHandle& operator=(FileHandle const&);

        static wchar_t const* TranslateMode(FileModeFlags const mode)
        {
            #define CXXREFLECT_GENERATE(x, y, z)                                 \
                static_cast<std::underlying_type<FileMode>::type>(FileMode::x) | \
                static_cast<std::underlying_type<FileMode>::type>(FileMode::y) | \
                static_cast<std::underlying_type<FileMode>::type>(FileMode::z)

            switch (mode.GetIntegral())
            {
            case CXXREFLECT_GENERATE(Read,   NonUpdate, Text)   : return L"r"  ;
            case CXXREFLECT_GENERATE(Write,  NonUpdate, Text)   : return L"w"  ;
            case CXXREFLECT_GENERATE(Append, NonUpdate, Text)   : return L"a"  ;
            case CXXREFLECT_GENERATE(Read,   Update,    Text)   : return L"r+" ;
            case CXXREFLECT_GENERATE(Write,  Update,    Text)   : return L"w+" ;
            case CXXREFLECT_GENERATE(Append, Update,    Text)   : return L"a+" ;

            case CXXREFLECT_GENERATE(Read,   NonUpdate, Binary) : return L"rb" ;
            case CXXREFLECT_GENERATE(Write,  NonUpdate, Binary) : return L"wb" ;
            case CXXREFLECT_GENERATE(Append, NonUpdate, Binary) : return L"ab" ;
            case CXXREFLECT_GENERATE(Read,   Update,    Binary) : return L"rb+";
            case CXXREFLECT_GENERATE(Write,  Update,    Binary) : return L"wb+";
            case CXXREFLECT_GENERATE(Append, Update,    Binary) : return L"ab+";

            default: throw FileIOError(L"Invalid mode specified");
            }

            #undef CXXREFLECT_GENERATE
        }

        void AssertInputStream() const
        {
            AssertInitialized();
            Assert([&]
            {
                return _mode.IsSet(FileMode::Update)
                    || _mode.WithMask(FileMode::ReadWriteAppendMask) != FileMode::Write;
            });
        }

        void AssertOutputStream() const
        {
            AssertInitialized();
            Assert([&]
            {
                return _mode.IsSet(FileMode::Update)
                    || _mode.WithMask(FileMode::ReadWriteAppendMask) != FileMode::Read;
            });
        }

        void AssertInitialized() const
        {
            Assert([&]{ return _handle != nullptr; });
        }

        FileModeFlags _mode;
        FILE*         _handle;
    };

} }





//
//
// BASIC LINEAR ALLOCATOR FOR ARRAYS
//
//

namespace CxxReflect { namespace Detail {

    // We do a lot of allocation of arrays, where the lifetime of many of the arrays are bound to the
    // lifetime of a single object.  This very simple linear allocator allocates blocks of memory and
    // services allocation requests for arrays.  For the canonical example of using this allocator,
    // see its use for storing converted strings from the metadata database.

    // Represents a range of elements in an array.  Begin() and End() point to the first element and
    // the one-past-the-end elements, respectively, just as they do for the STL containers.
    template <typename T>
    class Range
    {
    public:

        typedef std::size_t SizeType;
        typedef T           ValueType;
        typedef T*          Pointer;

        Range() { }

        Range(Pointer const begin, Pointer const end)
            : _begin(begin), _end(end)
        {
            AssertInitialized();
        }

        template <typename U>
        Range(Range<U> const& other)
            : _begin(other.IsInitialized() ? other.Begin() : nullptr),
              _end  (other.IsInitialized() ? other.End()   : nullptr)
        {
        }

        Pointer  Begin()   const { AssertInitialized(); return _begin.Get();                  }
        Pointer  End()     const { AssertInitialized(); return _end.Get();                    }
        SizeType GetSize() const { AssertInitialized(); return _end.Get() - _begin.Get();     }
        bool     IsEmpty() const { AssertInitialized(); return _begin.Get() == _end.Get();    }

        bool IsInitialized() const { return _begin.Get() != nullptr && _end.Get() != nullptr; }

    private:

        void AssertInitialized() const { Assert([&]{ return IsInitialized(); }); }

        ValueInitialized<Pointer> _begin;
        ValueInitialized<Pointer> _end;
    };

    template <typename T, std::size_t NBlockSize>
    class LinearArrayAllocator
    {
    public:

        typedef std::size_t SizeType;
        typedef T           ValueType;
        typedef T*          Pointer;

        enum { BlockSize = NBlockSize };

        typedef Range<T> Range;

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

} }





//
//
// CONTAINER ITERATOR ADAPTERS
//
//

namespace CxxReflect { namespace Detail {

    template <typename TIterator>
    class RandomAccessSequence
    {
    public:

        typedef typename std::iterator_traits<TIterator>::value_type value_type;
        typedef typename std::iterator_traits<TIterator>::reference  reference;
        typedef typename std::iterator_traits<TIterator>::pointer    pointer;
        typedef std::size_t                                          size_type;
        typedef TIterator                                            iterator;
        typedef TIterator                                            const_iterator;
        typedef std::reverse_iterator<TIterator>                     reverse_iterator;
        typedef std::reverse_iterator<TIterator>                     const_reverse_iterator;

        RandomAccessSequence()
        {
        }

        RandomAccessSequence(TIterator const first, TIterator const last)
            : _first(first), _last(last)
        {
        }

        iterator  begin() const { return _first.Get();               }
        iterator  end()   const { return _last.Get();                }

        pointer   data()  const { return &*_first.Get();             }
        size_type size()  const { return _last.Get() - _first.Get(); }

        reference operator[](size_type const n) const { return _first.Get()[n]; }


    private:

        ValueInitialized<TIterator> _first;
        ValueInitialized<TIterator> _last;
    };

} }





//
//
// MULTITHREADING AND SYNCHRONIZATION
//
//

namespace CxxReflect { namespace Detail {
    /*
    template <typename T>
    class Lease;

    template <typename T>
    class Synchronized
    {
    public:

        Lease<T> ObtainLease() const
        {
            return Lease<T>(this);
        }

    private:

        friend Lease<T>;

        T&   Get()    const { return _object;  }
        void Lock()   const { _mutex.lock();   }
        void Unlock() const { _mutex.unlock(); }

        T          mutable _object;
        std::mutex mutable _mutex;
    };

    template <typename T>
    class Lease
    {
    public:

        explicit Lease(Synchronized<T> const* object)
            : _object(object)
        {
            Detail::AssertNotNull(object);

            _object->Lock();
        }

        Lease(Lease&& other)
            : _object(other._object)
        {
            AssertNotNull(_object);

            other._object = nullptr;
        }

        Lease& operator=(Lease&& other)
        {
            Detail::AssertNotNull(other._object);

            if (_object != nullptr && _object != other._object)
                _object->Unlock();

            _object = other._object;
            other._object = nullptr;
        }

        ~Lease()
        {
            if (_object != nullptr)
                _object->Unlock();
        }

        T& Get() const
        {
            return _object->Get();
        }

    private:

        Lease(Lease const&);
        Lease& operator=(Lease const&);

        Synchronized<T> const* _object;
    };
    */

} }





//
//
// INSTANTIATING ITERATOR
//
//

namespace CxxReflect { namespace Detail {

    // A functor that always returns a copy of the object that it is given as an argument.
    struct IdentityTransformer
    {
        template <typename T>
        T operator()(T const& x) const volatile
        {
            return x;
        }
    };

    // An iterator that instantiates objects of type TResult from a range pointed to by TCurrent
    // pointers or indices.  Each TResult is constructed by calling its constructor that takes a
    // TParameter, a TCurrent, and an InternalKey.  The parameter is the value provided when the
    // InstantiatingIterator is constructed; the current is the current value of the iterator.
    template <typename TCurrent,
              typename TResult,
              typename TParameter,
              typename TTransformer = IdentityTransformer,
              typename TCategory = std::random_access_iterator_tag>
    class InstantiatingIterator
    {
    public:

        typedef TCategory                       iterator_category;
        typedef TResult                         value_type;
        typedef TResult                         reference;
        typedef Dereferenceable<TResult>        pointer;
        typedef std::int32_t                    difference_type;

        InstantiatingIterator()
        {
        }

        InstantiatingIterator(TParameter const parameter, TCurrent const current)
            : _parameter(parameter), _current(current)
        {
        }

        reference Get()        const { return value_type(_parameter, TTransformer()(_current), InternalKey()); }
        reference operator*()  const { return value_type(_parameter, TTransformer()(_current), InternalKey()); }
        pointer   operator->() const { return value_type(_parameter, TTransformer()(_current), InternalKey()); }

        reference operator[](difference_type const n) const
        {
            return value_type(_parameter, TTransformer()(_current + n));
        }

        friend bool operator==(InstantiatingIterator const& lhs, InstantiatingIterator const& rhs)
        {
            return lhs._current == rhs._current;
        }

        friend bool operator< (InstantiatingIterator const& lhs, InstantiatingIterator const& rhs)
        {
            return lhs._current < rhs._current;
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(InstantiatingIterator)
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(InstantiatingIterator, _current, difference_type)

    private:

        TParameter _parameter;
        TCurrent   _current;
    };

} }





//
//
// TEMPLATE INSTANTIATIONS FROM 'DETAIL' TEMPLATES INJECTED INTO THE PRIMARY NAMESPACE:
//
//

namespace CxxReflect {

    typedef Detail::Range<Byte>                ByteRange;
    typedef Detail::Range<Byte const>          ConstByteRange;

    typedef Detail::EnhancedCString<Character> StringReference;

}

#endif
