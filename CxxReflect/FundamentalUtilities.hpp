#ifndef CXXREFLECT_FUNDAMENTALUTILITIES_HPP_
#define CXXREFLECT_FUNDAMENTALUTILITIES_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Fundamental types, functions, and constants used throughout the library.

#include "CxxReflect/Configuration.hpp"
#include "CxxReflect/ExternalFunctions.hpp"





//
//
// EXCEPTIONS, ASSERTIONS, AND ERROR HANDLING
//
//

namespace CxxReflect {

    /// The root of the CxxReflect Exception hierarchy.
    ///
    /// All exceptions thrown by CxxReflect are derived from this `Exception` class.  Note that
    /// it derives from `std::exception` so that code catching `std::exception` will catch our
    /// exceptions, but calling `std::exception::what()` will not return the exception message.
    ///
    /// We use wide strings everywhere in CxxReflect, so `Exception` has its own `what()`-like
    /// function called `GetMessage()`.
    class Exception : public std::exception
    {
    public:

        /// \nothrows
        virtual ~Exception() throw()
        {
            // Required to override base-class virtual destructor
        }

        /// Gets the exception text.
        ///
        /// Call this instead of `std::exception::what()`.
        ///
        /// \returns  The exception message text.
        /// \nothrows
        String const& GetMessage() const
        {
            return _message;
        }

    protected:

        /// The sole (non-copy/move) constructor is protected because we never want to throw an
        /// `Exception` directly:  we always want to throw one of the derived exceptions.
        ///
        /// \param    message The message to be associated with this exception object.
        /// \nothrows
        explicit Exception(String const message = L"")
            : _message(std::move(message))
        {
        }

    private:

        String _message;
    };

    /// An exception class to represent a logic error.
    ///
    /// A logic error is any error that should never occur if the code is written correctly.  Do not
    /// catch a `LogicError`.  If you encounter a `LogicError` exception, please report a bug.
    class LogicError : public Exception
    {
    public:

        explicit LogicError(String message = L"")
            : Exception(message)
        {
        }
    };

    /// An exception class to represent a runtime error.
    class RuntimeError : public Exception
    {
    public:

        explicit RuntimeError(String message = L"")
            : Exception(std::move(message))
        {
        }
    };

    /// An exception class to represent a runtime error with an HRESULT.
    class HResultRuntimeError : public RuntimeError
    {
    public:

        explicit HResultRuntimeError(HResult const hresult = 0x80004005, String message = L"")
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

    /// An exception class to represent a runtime error due to I/O failure.
    class FileIOError : public RuntimeError
    {
    public:

        explicit FileIOError(CharacterIterator const message, int const error = errno)
            : RuntimeError(message), _error(error)
        {
        }

        explicit FileIOError(int const error = errno)
            : _error(error)
        {
        }

        int GetError() const
        {
            return _error;
        }

    private:

        int _error;
    };

    /// An exception class to represent a runtime error due to an invalid metadata database.
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

    inline void VerifyNotNull(void const* const p)
    {
        if (p == nullptr)
            throw RuntimeError(L"Unexpected null pointer");
    }

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

    inline bool Failed(HResult const hresult)
    {
        return hresult < 0;
    }

} }





//
//
// ALGORITHMS AND STANDARD LIBRARY ALGORITHM WRAPPERS
//
//

namespace CxxReflect { namespace Detail {

    #ifdef CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS

    /// Checks that a range is ordered correctly and throws a `LogicError` if it is not.
    ///
    /// This is useful with the unchecked debug algorithms defined here because they allow us to
    /// assert an immutable sequence's ordering once, then assume that it is ordered for all future
    /// searches.
    ///
    /// This function is only used when `CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS` is defined.
    /// If that macro is not defined, this function is a no-op.  When compiling a release (non-debug)
    /// build or when iterator debugging is disabled, this macro is expressly not defined.
    ///
    /// If no comparer is provided, `operator<` is used as the default.
    ///
    /// \param  first, last The presumably-ordered range to be checked.
    /// \param  comparer    The ordering comparer by which the sequence is expected to be ordered.
    /// \throws LogicError  If the sequence is not ordered according to the comparer.
    template <typename TForIt, typename TComparer>
    void AssertStrictWeakOrdering(TForIt const first, TForIt const last, TComparer comparer)
    {
        for (TForIt current(first), next(first); current != last && ++next != last; ++current)
            if (comparer(*next, *current))
                throw LogicError("Sequence is not ordered");
    }

    /// \sa AssertStrictWeakOrdering(TForIt, TForIt, TComparer)
    template <typename TForIt>
    void AssertStrictWeakOrdering(TForIt const first, TForIt const last)
    {
        for (TForIt current(first), next(first); current != last && ++next != last; ++current)
            if (*next < *current)
                throw LogicError("Sequence is not ordered");
    }

    /// Replacement for `std::equal_range`.
    /// 
    /// Visual C++ Iterator Debugging verifies that the input range satisfies the strict-weak order
    /// requirement each time that `std::equal_range` is called.  This is extraordinarily time
    /// consuming for large ranges (or even small ranges, if we call `std::equal_range` frequently
    /// enough).  We have copied here the implementation of the Visual C++ Standard Library's
    /// `std::equal_range` but omitted the `_DEBUG_ORDER` call from the beginning of each function.
    ///
    /// This function is only used when `CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS` is defined.
    /// If that macro is not defined, this function will delegate directly to `std::equal_range`.
    /// When compiling a release (non-debug) build or when iterator debugging is disabled, this
    /// macro is expressly not defined.
    ///
    /// If you wish to verify that a range is ordered according to a strict-weak comparer, use the
    /// `AssertStrictWeakOrdering()` function template.
    ///
    /// If no comparer is provided, `operator<` is used as the default.
    ///
    /// \param    first, last The range to be searched.
    /// \param    value       The value to search for.
    /// \param    comparer    The ordering comparer by which the range is ordered.
    /// \returns  The lower and upper bound results of the equal range search.
    /// \nothrows
    template <typename TForIt, typename TValue, typename TComparer>
    ::std::pair<TForIt, TForIt> EqualRange(TForIt first, TForIt last, const TValue& value, TComparer comparer)
    {
        auto const result(::std::_Equal_range(
            ::std::_Unchecked(first),
            ::std::_Unchecked(last),
            value,
            comparer,
            ::std::_Dist_type(first)));

        return ::std::pair<TForIt, TForIt>(
            ::std::_Rechecked(first, result.first),
            ::std::_Rechecked(last, result.second));
    }

    /// \sa EqualRange(TForIt, TForIt, const TValue&, TComparer)
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

    #else // !defined(CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS)

    template <typename TForIt>
    void AssertStrictWeakOrdering(TForIt const&, TForIt const&)
    {
        return;
    }

    template <typename TForIt, typename TComparer>
    void AssertStrictWeakOrdering(TForIt const&, TForIt const&, TComparer const&)
    {
        return;
    }

    template <typename TForIt, typename TValue>
    ::std::pair<TForIt, TForIt> EqualRange(TForIt first, TForIt last, const TValue& value)
    {
        return ::std::equal_range(first, last, value);
    }

    template <typename TForIt, typename TValue, typename TComparer>
    ::std::pair<TForIt, TForIt> EqualRange(TForIt first, TForIt last, const TValue& value, TComparer comparer)
    {
        return ::std::equal_range(first, last, value, comparer);
    }

    #endif

    template <typename TForIt, typename TValue>
    bool All(TForIt const first, TForIt const last, TValue const& value)
    {
        for (TForIt it(first); it != last; ++it)
            if (*it != value)
                return false;

        return true;
    }

    template <typename TForIt, typename TValue>
    bool Any(TForIt const first, TForIt const last, TValue const& value)
    {
        for (TForIt it(first); it != last; ++it)
            if (*it == value)
                return true;

        return false;
    }

    /// Computes the distance between a range of iterators.
    ///
    /// We require an unsigned distance quantity in various places to use in comparisons with calls
    /// to `size()` or likewise.  This function encapsulates the cast to `SizeType` in one place.
    ///
    /// \param    first, last The range whose size is to be computed.
    /// \returns  The distance between `first` and `last`.
    /// \nothrows
    template <typename TForIt>
    SizeType Distance(TForIt const first, TForIt const last)
    {
        return static_cast<SizeType>(std::distance(first, last));
    }

    /// Performs a binary search for a unique element in an ordered sequence.
    ///
    /// \param    first, last The random-accessible range to be searched.
    /// \param    value       The value for which to be searched.
    /// \param    comparer    The comparer by which the range is ordered.
    /// \returns  An iterator to the found element, or `last`, if no element is found.
    /// \nothrows
    template <typename TRanIt, typename TValue, typename TComparer>
    TRanIt BinarySearch(TRanIt const first, TRanIt const last, TValue const& value, TComparer const comparer)
    {
        TRanIt const it(std::lower_bound(first, last, value, comparer));
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

    /// Peforms a range-checked equality comparison between two ranges.
    ///
    /// If no comparer is provided, `operator<` is used as a default.
    ///
    /// \param    first0, last0 The first range to be compared.
    /// \param    first1, last1 The second range to be compared.
    /// \param    comparer      The comparer to be used to compare elements from the two ranges.
    /// \returns  `true` if the sequences contain the same number of elements and corresponding
    ///           elements in each sequence compare equal using the comparer.
    /// \nothrows
    template <typename TInIt0, typename TInIt1, typename TComparer>
    bool RangeCheckedEqual(TInIt0          first0,
                           TInIt0    const last0,
                           TInIt1          first1,
                           TInIt1    const last1,
                           TComparer const comparer)
    {
        while (first0 != last0 && first1 != last1 && comparer(*first0, *first1))
        {
            ++first0;
            ++first1;
        }

        return first0 == last0 && first1 == last1;
    }

    /// \sa RangeCheckedEqual(TInIt0,TInIt0,TInIt1,TInIt1,TComparer)
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

    /// Converts a wide string to lowercase
    ///
    /// \tparam   TString A string type that provides `begin()` and `end()` member functions.
    /// \param    s       The string to be converted to lowercase.
    /// \returns  The lowercase string.
    /// \nothrows
    template <typename TString>
    TString MakeLowercase(TString s)
    {
        std::transform(s.begin(), s.end(), s.begin(), (int(*)(std::wint_t))std::tolower);
        return s;
    }

    template <typename T>
    String ToString(T const& x)
    {
        std::wostringstream oss;
        if (!(oss << x))
            throw LogicError(L"Failed to convert object to string");

        return oss.str();
    }

    /// An Identity template, similar to the proposed-but-excluded `std::identity`.
    template <typename T>
    struct Identity
    {
        /// A `typedef` for the template type parameter `T`.
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

        EnhancedCString(const_pointer const first)
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

        template <size_type N>
        static EnhancedCString OfArray(value_type const (&data)[N])
        {
            EnhancedCString value;
            value._first = data;
            value._last = data + N - 1;
            return value;
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
            if (lhs._last == nullptr && *lhs_it == 0)
                lhs._last = lhs_it;

            if (rhs._last == nullptr && *rhs_it == 0)
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

    inline bool StartsWith(ConstCharacterIterator targetIt, ConstCharacterIterator prefixIt)
    {
        if (targetIt == nullptr || prefixIt == nullptr)
            return false;

        for (; *targetIt != L'\0' && *prefixIt != L'\0'; ++targetIt, ++prefixIt)
        {
            if (*targetIt != *prefixIt)
                return false;
        }

        return true;
    }

} }





//
//
// MISCELLANEOUS UTILITY CLASSES (ScopeGuard, FlagSet, Dereferenceable, ValueInitialized)
//
//

namespace CxxReflect { namespace Detail {

    /// A class that is convertible to a value-initialized instance of any type.
    class Default
    {
    public:

        template <typename T>
        operator T() const volatile
        {
            return T();
        }
    };





    /// An interface for virtually-destructible objects.
    class IDestructible
    {
    public:

        /// Virtual destructor provided for interface class.
        virtual ~IDestructible() = 0;
    };

    typedef std::unique_ptr<IDestructible> UniqueDestructible;






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

    /// Value-initialization wrapper.
    ///
    /// This value-initialization wrapper should be used for all member variables of POD type, to
    /// ensure that they are always initialized without having to explicitly initialize them in the
    /// constructor of a class.
    template <typename T>
    class ValueInitialized
    {
    public:

        typedef T ValueType;

        /// Default constructor value-initializes the stored value.
        ///
        /// \nothrows
        ValueInitialized()
            : _value()
        {
        }

        /// Explicit constructor constructs the stored value by copying another instance of it.
        ///
        /// \param value The value to be copied.
        /// \nothrows
        explicit ValueInitialized(ValueType const& value)
            : _value(value)
        {
        }

        /// \returns  A reference to the stored value.
        /// \nothrows
        ValueType& Get()
        {
            return _value;
        }

        /// \returns  A const reference to the stored value.
        /// \nothrows
        ValueType const& Get() const
        {
            return _value;
        }

        /// Resets the value by destroying the object then reconstructing it in-place.
        ///
        /// (Note:  We assume that the default constructor and the destructor of `T` both do not
        /// throw.  If they throw, though, you are in for a world of other trouble.)
        ///
        /// \nothrows
        void Reset()
        {
            _value.~ValueType();
            new (&_value) ValueType();
        }

    private:

        ValueType _value; ///< The stored value.
    };

    template <typename T>
    class Optional
    {
    public:

        Optional()
        {
        }

        Optional(T const& value)
            : _value(value), _hasValue(true)
        {
        }

        bool     HasValue() const { return _hasValue.Get(); }
        T const& GetValue() const { return _value.Get();    }

    private:

        ValueInitialized<T>    _value;
        ValueInitialized<bool> _hasValue;
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

    class FileRange
    {
    public:

        FileRange();
        FileRange(ConstByteIterator first, ConstByteIterator last, UniqueDestructible release);
        FileRange(FileRange&&);
        FileRange& operator=(FileRange&&);

        ConstByteIterator Begin() const;
        ConstByteIterator End()   const;

        bool IsInitialized() const;

    private:

        Detail::ValueInitialized<ConstByteIterator> _first;
        Detail::ValueInitialized<ConstByteIterator> _last;
        UniqueDestructible                          _release;
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

        FILE* GetHandle() const
        {
            return _handle;
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





    /// A FileHandle-like interface for use with an array of bytes.
    ///
    /// This class is provided as a stopgap for migrating the Metadata Database class to exclusively
    /// use memory mapped I/O.  This class has a pointer that serves as a current pointer (or cursor)
    /// and Read and Seek operations advance or retreat the pointer.
    class ConstByteCursor
    {
    public:

        typedef std::int32_t DifferenceType;

        enum OriginType
        {
            Begin   = SEEK_SET,
            Current = SEEK_CUR,
            End     = SEEK_END
        };

        ConstByteCursor(ConstByteIterator const first, ConstByteIterator const last)
            : _first(first), _last(last), _current(first)
        {
        }

        ConstByteIterator GetCurrent() const
        {
            AssertInitialized();
            return _current.Get();
        }

        SizeType GetPosition() const
        {
            AssertInitialized();
            return Distance(_first.Get(), _current.Get());
        }

        bool IsEof() const
        {
            AssertInitialized();
            return _current.Get() == _last.Get();
        }

        void Read(void* const buffer, SizeType const size, SizeType const count)
        {
            AssertInitialized();
            VerifyAvailable(size * count);
            
            ByteIterator bufferIterator(static_cast<ByteIterator>(buffer));
            RangeCheckedCopy(_current.Get(), _current.Get() + size * count,
                             bufferIterator, bufferIterator + size * count);

            _current.Get() += size * count;
        }

        template <typename T>
        void Read(T* const buffer, SizeType const count)
        {
            Assert([&](){ return count > 0; });

            return this->Read(buffer, sizeof *buffer, count);
        }

        bool CanRead(DifferenceType const size) const
        {
            AssertInitialized();
            return std::distance(_current.Get(), _last.Get()) >= size;
        }

        void Seek(DifferenceType const position, OriginType const origin)
        {
            AssertInitialized();
            if (origin == OriginType::Begin)
            {
                _current.Get() = _first.Get();
            }
            else if (origin == OriginType::End)
            {
                _current.Get() = _last.Get();
            }
            
            VerifyAvailable(position);
            _current.Get() += position;
        }

        bool CanSeek(DifferenceType const position, OriginType const origin)
        {
            AssertInitialized();
            if (origin == OriginType::Begin)
            {
                return std::distance(_first.Get(), _last.Get()) >= position;
            }
            else if (origin == OriginType::End)
            {
                return -std::distance(_first.Get(), _last.Get()) <= position;
            }
            else
            {
                return std::distance(_current.Get(), _last.Get()) >= position;
            }
        }

        void VerifyAvailable(DifferenceType const size) const
        {
            if (!CanRead(size))
                throw FileIOError(0);
        }

        bool IsInitialized() const
        {
            return _first.Get() != nullptr && _last.Get() != nullptr && _current.Get() != nullptr;
        }

    private:

        void AssertInitialized() const
        {
            Assert([&]{ return IsInitialized(); });
        }

        ValueInitialized<ConstByteIterator> _first;
        ValueInitialized<ConstByteIterator> _last;
        ValueInitialized<ConstByteIterator> _current;
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

    /// Represents a range of elements in an array.  `Begin()` and `End()` point to the first element
    /// and the one-past-the-end "element," respectively, just as they do for the STL containers.
    template <typename T>
    class Range
    {
    public:

        typedef T           ValueType;
        typedef T*          Pointer;

        Range() { }

        Range(Pointer const begin, Pointer const end)
            : _begin(begin), _end(end)
        {
            AssertInitialized();
        }

        /// Converting constructor to convert `Range<T>` to `Range<cv T>`.
        template <typename U>
        Range(Range<U> const& other,
              typename std::enable_if<
                  std::is_same<
                      typename std::remove_cv<T>::type,
                      typename std::remove_cv<U>::type
                  >::value
              >::type* = nullptr)
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

    /// A linear allocator for arrays of elements
    ///
    /// We do a lot of allocation of arrays, where the lifetimes of the arrays are bound to the
    /// lifetime of another known object.  This very simple linear allocator allocates blocks of
    /// memory and services allocation requests for arrays.  For a canonical example of using this
    /// allocator, see its use for storing UTF-16 converted strings from the metadata database
    /// (in `CxxReflect::Metadata::StringCollection`).
    ///
    /// The arrays are not destroyed until the `LinearArrayAllocator` is destroyed.  No reclamation
    /// of allocated storage is attempted.
    ///
    /// \tparam T The type of element that is allocated.
    /// \tparam NBlockSize The size of each block of `T` elements to be allocated.  This is also the
    /// maximum serviceable allocation size.
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

        /// Allocates an array of `n` elements.
        ///
        /// \param n The extent of the array to be allocated.
        ///
        /// \returns A `Range` representing the newly allocated array.
        ///
        /// \throws RuntimeError If `n` is larger than `NBlockSize`.
        ///
        /// \todo Rather than throwing `RuntimeError` when `n` is larger than `NBlockSize`, we should
        /// allocate that array independently.  This will require us to keep a list of independent
        /// allocations.  This should not happen often, though.
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
                throw RuntimeError(L"n");

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
// STACK ALLOCATOR
//
//

namespace CxxReflect { namespace Detail {

    // This allocator is based on the stack_alloc<T, N> implementation by Howard Hinnant, found at
    // http://home.roadrunner.com/~hinnant/stack_alloc.h.
    template <typename T, SizeType N> class StackAllocator;

    template <SizeType N>
    class StackAllocator<void, N>
    {
    public:

        typedef void const* const_pointer;
        typedef void        value_type;
    };

    template <typename T, SizeType N>
    class StackAllocator
    {
    public:

        typedef SizeType          size_type;
        typedef T                 value_type;
        typedef value_type      * pointer;
        typedef value_type const* const_pointer;
        typedef value_type      & reference;
        typedef value_type const& const_reference;

        StackAllocator()
            : _current(root())
        {
        }

        StackAllocator(StackAllocator const&)
            : _current(root())
        {
        }

        template <typename U>
        StackAllocator(StackAllocator<U, N> const&)
            : _current(root())
        {
        }

        template <typename U>
        struct rebind
        {
            typedef StackAllocator<U, N> other;
        };

        pointer allocate(size_type const n, typename StackAllocator<void, N>::const_pointer = 0)
        {
            if (root() + N - _current >= n)
            {
                pointer const r(_current);
                _current += n;
                return r;
            }

            return static_cast<pointer>(::operator new(n * sizeof(value_type)));
        }

        pointer deallocate(pointer const p, size_type const n)
        {
            if (root() <= p && p < root() + N)
            {
                if (p + n == _current)
                    _current = p;
            }
            else
            {
                ::operator delete(p);
            }
        }

        size_type max_size() const
        {
            return static_cast<size_type>(-1) / sizeof(value_type);
        }

        void destroy(pointer const p)
        {
            p->~value_type();
        }

        void construct(pointer const p)
        {
            ::new(static_cast<void*>(p)) value_type();
        }

        template <typename P0>
        void construct(pointer const p, P0&& a0)
        {
            ::new(static_cast<void*>(p)) value_type(a0);
        }

        friend bool operator==(StackAllocator const& lhs, StackAllocator const& rhs)
        {
            return &lhs._storage == &rhs._storage;
        }

        friend bool operator!=(StackAllocator const& lhs, StackAllocator const& rhs)
        {
            return &lhs._storage != &rhs._storage;
        }

    private:

        typedef typename std::aligned_storage<sizeof(T) * N, 16>::type buffer_type;

        // This class is not assignable:
        StackAllocator& operator=(StackAllocator const&);

        pointer       root()       { return reinterpret_cast<pointer      >(&_storage); }
        const_pointer root() const { return reinterpret_cast<const_pointer>(&_storage); }

        buffer_type _storage;
        pointer     _current;
    };

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





namespace CxxReflect { namespace Detail {

    template <typename TForIt, typename TFilter>
    class StaticFilterIterator
    {
    public:

        typedef std::forward_iterator_tag                         iterator_category;
        typedef typename std::iterator_traits<TForIt>::value_type value_type;
        typedef typename std::iterator_traits<TForIt>::reference  reference;
        typedef typename std::iterator_traits<TForIt>::pointer    pointer;
        typedef std::ptrdiff_t                                    difference_type;

        StaticFilterIterator()
        {
        }

        StaticFilterIterator(TForIt const current, TForIt const last, TFilter const filter)
            : _current(current), _last(last), _filter(filter)
        {
        }

        reference operator*() const
        {
            AssertDereferenceable();
            return _current.Get().operator*();
        }

        pointer operator->() const
        {
            AssertDereferenceable();
            return _current.Get().operator->();
        }

        StaticFilterIterator& operator++()
        {
            AssertDereferenceable();
            ++_current.Get();
            FilterAdvance();
            return *this;
        }

        StaticFilterIterator operator++(int)
        {
            StaticFilterIterator const it(*this);
            ++*this;
            return it;
        }

        bool IsDereferenceable() const
        {
            return _current.Get() != _last.Get();
        }

        friend bool operator==(StaticFilterIterator const& lhs, StaticFilterIterator const& rhs)
        {
            return !lhs.IsDereferenceable() && !rhs.IsDereferenceable()
                || lhs._current.Get() == rhs._current.Get();
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(StaticFilterIterator)

    private:

        StaticFilterIterator& operator=(StaticFilterIterator const&);

        void AssertDereferenceable() const
        {
            Assert([&]{ return IsDereferenceable(); });
        }

        void FilterAdvance()
        {
            while (_current.Get() != _last.Get() && !_filter.Get()(*_current.Get()))
                ++_current.Get();
        }

        ValueInitialized<TForIt > _current;
        ValueInitialized<TForIt > _last;
        ValueInitialized<TFilter> _filter;
    };

    template <typename TForIt, typename TFilter>
    class StaticFilteredRange
    {
    public:

        StaticFilteredRange(TForIt const first, TForIt const last, TFilter const filter)
            : _first(first), _last(last), _filter(filter)
        {
        }

        StaticFilterIterator<TForIt, TFilter> Begin() const
        {
            return StaticFilterIterator<TForIt, TFilter>(_first.Get(), _last.Get(), _filter.Get());
        }

        StaticFilterIterator<TForIt, TFilter> End() const
        {
            return StaticFilterIterator<TForIt, TFilter>(_last.Get(), _last.Get(), _filter.Get());
        }

    private:

        StaticFilteredRange& operator=(StaticFilteredRange const&);

        ValueInitialized<TForIt > _first;
        ValueInitialized<TForIt > _last;
        ValueInitialized<TFilter> _filter;
    };

    template <typename TForIt, typename TFilter>
    StaticFilteredRange<TForIt, TFilter> CreateStaticFilteredRange(TForIt  const first,
                                                                   TForIt  const last, 
                                                                   TFilter const filter)
    {
        return StaticFilteredRange<TForIt, TFilter>(first, last, filter);
    }

} }





//
//
// TEMPLATE INSTANTIATIONS FROM 'DETAIL' TEMPLATES INJECTED INTO THE PRIMARY NAMESPACE:
//
//

namespace CxxReflect {

    typedef Detail::Range<Byte>                ByteRange;
    typedef Detail::Range<Byte const>          ConstByteRange;

    /// A non-owning reference to a C string.
    ///
    /// This class provides a `std::wstring`-like interface over a simple C string.  It does not own
    /// the string to which it refers; some other object must ensure that the string exists for at
    /// least as long as the `StringReference` exists and is being used.
    ///
    /// We do a lot of string manipulation in the library, so to avoid copying strings unnecessarily
    /// we use references to strings.  This also allows greater flexibility with parameter passing.
    ///
    /// As an example, when a metadata database loads strings from an assembly, it will actually
    /// realize the string in an internal, persistent buffer, then return a reference to it.  The
    /// referenced string is cached so that it can be returned on subsequent calls.  Avoiding this
    /// copying has proven to be extremely beneficial for performance during profiling.
    typedef Detail::EnhancedCString<Character> StringReference;

}

#endif
