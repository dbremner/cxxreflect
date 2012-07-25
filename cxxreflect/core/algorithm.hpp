
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_ALGORITHM_HPP_
#define CXXREFLECT_CORE_ALGORITHM_HPP_

#include "cxxreflect/core/diagnostic.hpp"

namespace cxxreflect { namespace core {

    /// Tests whether all of the elements in a range match the given predicate
    template <typename InputIterator, typename Predicate>
    auto all(InputIterator const first, InputIterator const last, Predicate predicate) -> bool
    {
        for (InputIterator it(first); it != last; ++it)
        {
            if (!predicate(*it))
                return false;
        }

        return true;
    }

    /// Tests whether all of the elements in a range compare equal to the given value
    template <typename InputIterator, typename T>
    auto all_are(InputIterator const first, InputIterator const last, T const& value) -> bool
    {
        for (InputIterator it(first); it != last; ++it)
        {
            if (*it != value)
                return false;
        }

        return true;
    }

    /// Tests whether any of the elements in a range match the given predicate
    template <typename InputIterator, typename Predicate>
    auto any(InputIterator const first, InputIterator const last, Predicate predicate) -> bool
    {
        for (InputIterator it(first); it != last; ++it)
        {
            if (predicate(*it))
                return true;
        }

        return false;
    }

    /// Tests whether any of the elements in a range compare equal to the given value
    template <typename InputIterator, typename T>
    auto any_are(InputIterator const first, InputIterator const last, T const& value) -> bool
    {
        for (InputIterator it(first); it != last; ++it)
        {
            if (*it == value)
                return true;
        }

        return false;
    }





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
    template <typename ForwardIterator, typename Comparer>
    auto assert_strict_weak_ordering(ForwardIterator const first, ForwardIterator const last, Comparer comparer) -> void
    {
        for (ForwardIterator current(first), next(first); current != last && ++next != last; ++current)
            if (comparer(*next, *current))
                throw logic_error("sequence is not ordered");
    }

    /// \sa AssertStrictWeakOrdering(TForIt, TForIt, TComparer)
    template <typename ForwardIterator>
    auto assert_strict_weak_ordering(ForwardIterator const first, ForwardIterator const last) -> void
    {
        for (ForwardIterator current(first), next(first); current != last && ++next != last; ++current)
            if (*next < *current)
                throw logic_error("sequence is not ordered");
    }

    #else

    template <typename ForwardIterator, typename Comparer>
    auto assert_strict_weak_ordering(ForwardIterator, ForwardIterator, Comparer) -> void { }

    template <typename ForwardIterator>
    auto assert_strict_weak_ordering(ForwardIterator, ForwardIterator) -> void { }

    #endif





    /// Performs a binary search for an element and returns an iterator to the found element
    ///
    /// The range `[first, last)` must be ordered via `comparer`.  If `value` is found in the range,
    /// an iterator to the first element comparing equal to `value` will be returned; if `value` is
    /// not found in the range, `last` is returned.
    template <typename RandomAccessIterator, typename Value, typename Comparer>
    auto binary_search(RandomAccessIterator const  first,
                       RandomAccessIterator const  last,
                       Value                const& value,
                       Comparer                    comparer) -> RandomAccessIterator
    {
        RandomAccessIterator it(std::lower_bound(first, last, value, comparer));
        if (it == last || comparer(*it, value) || comparer(value, *it))
            return last;

        return it;
    }





    template <typename Container, typename Value>
    auto contains(Container&& c, Value&& v) -> bool
    {
        return std::find(begin(c), end(c), v) != end(c);
    }

    



    /// Computes the distance between two iterators (i.e., the size of a range)
    ///
    /// This is identical to `std::distance`, except it returns a `size_type`, to cleanly work
    /// around signed-unsigned comparison warnings elsewhere in the library.
    template <typename InputIterator>
    auto distance(InputIterator const first, InputIterator const last) -> size_type
    {
        return static_cast<core::size_type>(std::distance(first, last));
    }





    #ifdef CXXREFLECT_ENABLE_UNCHECKED_DEBUG_ALGORITHMS

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
    /// `assert_strict_weak_ordering()` function template.
    ///
    /// If no comparer is provided, `operator<` is used as the default.
    ///
    /// \param    first, last The range to be searched.
    /// \param    value       The value to search for.
    /// \param    comparer    The ordering comparer by which the range is ordered.
    /// \returns  The lower and upper bound results of the equal range search.
    /// \nothrows
    template <typename ForwardIterator, typename Value, typename Comparer>
    auto equal_range(ForwardIterator        first,
                     ForwardIterator        last,
                     Value           const& value,
                     Comparer               comparer) -> std::pair<ForwardIterator, ForwardIterator>
    {
        auto result(std::_Equal_range(
            std::_Unchecked(first),
            std::_Unchecked(last),
            value,
            comparer,
            std::_Dist_type(first)));

        return std::pair<ForwardIterator, ForwardIterator>(
            std::_Rechecked(first, result.first),
            std::_Rechecked(last,  result.second));
    }

    /// \sa equal_range(ForwardIterator, ForwardIterator, Value const&, Comparer)
    template <typename ForwardIterator, typename Value>
    auto equal_range(ForwardIterator        first,
                     ForwardIterator        last,
                     Value           const& value) -> std::pair<ForwardIterator, ForwardIterator>
    {
        auto result(std::_Equal_range(
            std::_Unchecked(first),
            std::_Unchecked(last),
            value,
            std::_Dist_type(first)));

        return std::pair<ForwardIterator, ForwardIterator>(
            std::_Rechecked(first, result.first),
            std::_Rechecked(last,  result.second));
    }

    #else

    template <typename ForwardIterator, typename Value, typename Comparer>
    auto equal_range(ForwardIterator const  first,
                     ForwardIterator const  last,
                     Value           const& value,
                     Comparer        const  comparer) -> std::pair<ForwardIterator, ForwardIterator>
    {
        return std::equal_range(first, last, value, comparer);
    }

    template <typename ForwardIterator, typename Value>
    auto equal_range(ForwardIterator const  first,
                     ForwardIterator const  last,
                     Value           const& value) -> std::pair<ForwardIterator, ForwardIterator>
    {
        return std::equal_range(first, last, value);
    }

    #endif





    /// Copies the range `[first0, last0)` into the range `[first1, last1)`
    ///
    /// This algorithm terminates when the end of either range is reached.
    template <typename InputIterator, typename OutputIterator>
    auto range_checked_copy(InputIterator        first0,
                            InputIterator  const last0,
                            OutputIterator       first1,
                            OutputIterator const last1) -> void
    {
        while (first0 != last0 && first1 != last1)
        {
            *first1++ = *first0++;
        }
    }





    /// Tests whether the ranges `[first0, last0)` and `[first1, last1)` are equal using `comparer`
    ///
    /// If the ranges are not of equal length, `false` is returned.
    template <typename InputIterator0, typename InputIterator1, typename Comparer>
    auto range_checked_equal(InputIterator0       first0,
                             InputIterator0 const last0,
                             InputIterator1       first1,
                             InputIterator1 const last1,
                             Comparer             comparer) -> bool
    {
        while (first0 != last0 && first1 != last1 && comparer(*first0, *first1))
        {
            ++first0;
            ++first1;
        }

        return first0 == last0 && first1 == last1;
    }

    /// Tests whether the ranges `[first0, last0)` and `[first1, last1)` are equal using `==`
    ///
    /// If the ranges are not of equal length, `false` is returned.
    template <typename InputIterator0, typename InputIterator1>
    auto range_checked_equal(InputIterator0       first0,
                             InputIterator0 const last0,
                             InputIterator1       first1,
                             InputIterator1 const last1) -> bool
    {
        while (first0 != last0 && first1 != last1 && *first0 == *first1)
        {
            ++first0;
            ++first1;
        }

        return first0 == last0 && first1 == last1;
    }

} }

#endif
