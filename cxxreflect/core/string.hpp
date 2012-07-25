
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_STRING_HPP_
#define CXXREFLECT_CORE_STRING_HPP_

#include "cxxreflect/core/algorithm.hpp"
#include "cxxreflect/core/utility.hpp"

namespace cxxreflect { namespace core {

    template <typename Character>
    class enhanced_cstring
    {
    public:

        typedef Character       value_type;
        typedef size_type       size_type;
        typedef difference_type difference_type;

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

        enhanced_cstring(const_pointer const first)
            : _first(first), _last(nullptr)
        {
        }

        enhanced_cstring(const_pointer const first, const_pointer const last)
            : _first(first), _last(last)
        {
        }

        template <size_type N>
        enhanced_cstring(value_type const (&data)[N])
            : _first(data), _last(data + N - 1)
        {
        }

        template <size_type N>
        enhanced_cstring(value_type (&data)[N])
            : _first(data), _last(data + N - 1)
        {
        }

        template <size_type N>
        static auto from_literal(value_type const (&data)[N]) -> enhanced_cstring
        {
            enhanced_cstring value;
            value._first = data;
            value._last = data + N - 1;
            return value;
        }

        auto begin()    const -> const_iterator { return _first;          }
        auto end()      const -> const_iterator { return compute_last();  }
        auto cbegin()   const -> const_iterator { return _first;          }
        auto cend()     const -> const_iterator { return compute_last();  }

        auto rbegin()   const -> const_reverse_iterator { return reverse_iterator(compute_last());  }
        auto rend()     const -> const_reverse_iterator { return reverse_iterator(_first); }
        auto crbegin()  const -> const_reverse_iterator { return reverse_iterator(compute_last());  }
        auto crend()    const -> const_reverse_iterator { return reverse_iterator(_first);          }

        auto size()     const -> size_type { return convert_integer(compute_last() - _first); }
        auto length()   const -> size_type { return size();                                   }
        auto max_size() const -> size_type { return std::numeric_limits<size_type>::max();    }
        auto capacity() const -> size_type { return size();                                   }
        auto empty()    const -> bool      { return size() == 0;                              }

        auto operator[](size_type const n) const -> const_reference
        {
            return _first[n];
        }

        auto at(size_type const n) const -> const_reference
        {
            if (n >= size())
                throw std::out_of_range("n");

            return _first[n];
        }

        auto front() const -> const_reference { return *_first;               }
        auto back()  const -> const_reference { return *(compute_last() - 1); }

        auto c_str() const -> const_pointer   { return _first == nullptr ? L"" : _first; }
        auto data()  const -> const_pointer   { return _first;                           }

        friend auto operator==(enhanced_cstring const& lhs, enhanced_cstring const& rhs) -> bool
        {
            return compare_until_end<std::equal_to>(lhs, rhs);
        }

        friend auto operator<(enhanced_cstring const& lhs, enhanced_cstring const& rhs) -> bool
        {
            return compare_until_end<std::less>(lhs, rhs);
        }

        template <template <typename> class Comparer>
        static auto compare_until_end(enhanced_cstring const& lhs, enhanced_cstring const& rhs) -> bool
        {
            const_pointer lhs_it(lhs.begin());
            const_pointer rhs_it(rhs.begin());

            // First, treat a null pointer as an empty string:
            if (lhs_it == nullptr && rhs_it == nullptr)
                return Comparer<value_type>()(0, 0);

            else if (lhs_it == nullptr && rhs_it != nullptr)
                return Comparer<value_type>()(0, 1);

            else if (lhs_it != nullptr && rhs_it == nullptr)
                return Comparer<value_type>()(1, 0);

            // Next, if both strings are valid, compare them using the provided comparator:
            while (*lhs_it != 0 && *rhs_it != 0 && Comparer<value_type>()(*lhs_it, *rhs_it))
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

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(enhanced_cstring)

    private:

        auto compute_last() const -> pointer
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

    typedef enhanced_cstring<character> string_reference;





    template <typename T, typename U>
    auto operator==(enhanced_cstring<T> const& lhs, std::basic_string<U> const& rhs) -> bool
    {
        return range_checked_equal(begin(lhs), end(lhs), begin(rhs), end(rhs));
    }

    template <typename T, typename U>
    auto operator==(std::basic_string<T> const& lhs, enhanced_cstring<U> const& rhs) -> bool
    {
        return range_checked_equal(begin(lhs), end(lhs), begin(rhs), end(rhs));
    }

    template <typename T, typename U>
    auto operator==(enhanced_cstring<T> const& lhs, U const* const rhs) -> bool
    {
        enhanced_cstring<U> const temporary_rhs(rhs);
        return range_checked_equal(begin(lhs), end(lhs), begin(temporary_rhs), end(temporary_rhs));
    }

    template <typename T, typename U>
    auto operator==(U const* const lhs, enhanced_cstring<U> const& rhs) -> bool
    {
        enhanced_cstring<T> const temporary_lhs(lhs);
        return range_checked_equal(begin(temporary_lhs), end(temporary_lhs), begin(rhs), end(rhs));
    }

    template <typename T, typename U>
    auto operator<(enhanced_cstring<T> const& lhs, std::basic_string<U> const& rhs) -> bool
    {
        return std::lexicographical_compare(begin(lhs), end(lhs), begin(rhs), end(rhs));
    }

    template <typename T, typename U>
    auto operator<(std::basic_string<T> const& lhs, enhanced_cstring<U> const& rhs) -> bool
    {
        return std::lexicographical_compare(begin(lhs), end(lhs), begin(rhs), end(rhs));
    }

    template <typename T>
    auto operator<<(std::basic_ostream<T>& os, enhanced_cstring<T> const& s) -> std::basic_ostream<T>&
    {
        os << s.c_str();
        return os;
    }





    /// Performs a lexical cast from a source type to a target type
    template<typename Target, typename Source>
    auto lexical_cast(Source x) -> Target
    {
        std::stringstream s;
        Target y;
        if (!(s << x) || !(s >> y) || !(s >> std::ws).eof())
            throw runtime_error(L"bad lexical cast");

        return y;
    }





    /// Tests whether the C string pointed to by `target_it` is prefixed by the C string `prefix_it`
    inline auto starts_with(const_character_iterator target_it, const_character_iterator prefix_it) -> bool
    {
        if (target_it == nullptr || prefix_it == nullptr)
            return false;

        for (; *target_it != L'\0' && *prefix_it != L'\0'; ++target_it, ++prefix_it)
        {
            if (*target_it != *prefix_it)
                return false;
        }

        if (*prefix_it != '\0')
            return false;

        return true;
    }





    /// Converts a wide character string to lowercase
    template <typename String>
    auto to_lowercase(String s) -> String
    {
        using std::begin;
        using std::end;

        std::transform(begin(s), end(s), begin(s), (int(*)(std::wint_t))std::tolower);
        return s;
    }





    /// Converts an object to a string, via lexical casting
    template <typename Source>
    auto to_string(Source const& x) -> string
    {
        std::wstringstream s;
        if (!(s << x))
            throw runtime_error(L"bad string conversion");

        return s.str();
    }

} }

#endif
