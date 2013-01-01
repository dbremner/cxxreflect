
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_ITERATOR_HPP_
#define CXXREFLECT_CORE_ITERATOR_HPP_

#include "cxxreflect/core/utility.hpp"

namespace cxxreflect { namespace core {

    template <typename T>
    auto begin_bytes(T& x) -> byte_iterator
    {
        return reinterpret_cast<byte_iterator>(&x);
    }

    template <typename T>
    auto end_bytes(T& x) -> byte_iterator
    {
        return reinterpret_cast<byte_iterator>(&x + 1);
    }

    template <typename T>
    auto begin_bytes(T const& x) -> const_byte_iterator
    {
        return reinterpret_cast<const_byte_iterator>(&x);
    }

    template <typename T>
    auto end_bytes(T const& x) -> const_byte_iterator
    {
        return reinterpret_cast<const_byte_iterator>(&x + 1);
    }

    // We explicitly overload for rvalues to ensure that we do not bind an rvalue to a const& above
    template <typename T> auto begin_bytes(T&&) -> typename std::enable_if<!std::is_reference<T>::value>::type;
    template <typename T> auto end_bytes  (T&&) -> typename std::enable_if<!std::is_reference<T>::value>::type;






    /// A wrapper type on which and indirection operation yields a pointer to a stored object
    ///
    /// This is used to enable a proxy iterator to return something from its `operator->()` through
    /// which indirection may be performed.  It isn't particularly performant, since it requires
    /// a copy of the object, but it should be good enough for enabling the few scenarios we care
    /// about.
    template <typename T>
    class indirectable
    {
    public:

        typedef T        value_type;
        typedef T &      reference;
        typedef T const& const_reference;
        typedef T *      pointer;
        typedef T const* const_pointer;

        indirectable(const_reference value)
            : _value(value)
        {
        }

        auto get()       -> reference       { return _value;  }
        auto get() const -> const_reference { return _value;  }

        auto operator->()       -> pointer         { return &_value; }
        auto operator->() const -> const_pointer   { return &_value; }

    private:

        value_type _value;
    };





    /// An iterator that concatenates ranges obtained from iterating over an outer range
    ///
    /// This iterator provides convenient a way of iterating over a range of ranges.  The outer
    /// range is "flattened," yielding what is in effect a concatenation of all of the inner ranges.
    template <typename OuterIterator,
              typename InnerIterator,
              typename OuterValueType,
              typename InnerValueType,
              InnerIterator (*BeginInner)(OuterValueType const&),
              InnerIterator (*EndInner)  (OuterValueType const&)
    >
    class concatenating_iterator
    {
    public:

        typedef std::forward_iterator_tag                                iterator_category;
        typedef typename std::iterator_traits<InnerIterator>::value_type value_type;
        typedef typename std::iterator_traits<InnerIterator>::reference  reference;
        typedef typename std::iterator_traits<InnerIterator>::pointer    pointer;
        typedef difference_type                                          difference_type;

        concatenating_iterator() { }

        concatenating_iterator(OuterIterator const outer_it, OuterIterator const outer_end)
            : _outer_it(outer_it), _outer_end(outer_end)
        {
            compute_inner_iterators();

            // If the inner range of the initial element of the outer range is empty, we advance,
            // which will advance either until a nonempty inner range is found or until the end of
            // the outer range is reached, whichever comes first:
            if (_inner_it.get() == _inner_end.get())
                advance();
        }

        concatenating_iterator(OuterIterator const outer_end)
            : _outer_it(outer_end), _outer_end(outer_end)
        {
            compute_inner_iterators();
        }

        auto operator*() const -> reference
        {
            assert_dereferenceable();
            return _inner_it.get().operator*();
        }

        auto operator->() const -> pointer
        {
            assert_dereferenceable();
            return _inner_it.get().operator->();
        }

        auto operator++() -> concatenating_iterator&
        {
            assert_dereferenceable();
            advance();
            return *this;
        }

        auto operator++(int) -> concatenating_iterator
        {
            assert_dereferenceable();
            auto const it(*this);
            ++*this;
            return it;
        }

        friend auto operator==(concatenating_iterator const& lhs, concatenating_iterator const& rhs) -> bool
        {
            return (lhs._outer_it.get() == rhs._outer_it.get() && lhs._inner_it.get() == rhs._inner_it.get())
                || (!lhs.is_dereferenceable() && !rhs.is_dereferenceable());
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(concatenating_iterator)

    private:

        auto advance() -> void
        {
            if (_inner_it.get() != _inner_end.get())
            {
                ++_inner_it.get();
                return;
            }

            if (_outer_it.get() != _outer_end.get())
            {
                do
                {
                    ++_outer_it.get();
                    compute_inner_iterators();
                }
                while (_outer_it.get() != _outer_end.get() && _inner_it.get() == _inner_end.get());

                return;
            }

            assert_fail(L"Attempted to advance iterator past the end of the sequence");
        }

        auto compute_inner_iterators() -> void
        {
            if (_outer_it.get() == _outer_end.get())
            {
                _inner_it.get()  = InnerIterator();
                _inner_end.get() = InnerIterator();
            }
            else
            {
                _inner_it.get()  = BeginInner(*_outer_it.get());
                _inner_end.get() = EndInner  (*_outer_it.get());
            }
        }

        auto is_dereferenceable() const -> bool
        {
             return _outer_it.get() != _outer_end.get() && _inner_it.get() != _inner_end.get();
        }

        auto assert_dereferenceable() const -> void
        {
            assert_true([&]{ return is_dereferenceable(); });
        }

        value_initialized<OuterIterator> _outer_it;
        value_initialized<OuterIterator> _outer_end;
        value_initialized<InnerIterator> _inner_it;
        value_initialized<InnerIterator> _inner_end;
    };





    // An iterator that instantiates objects of type TResult from a range pointed to by TCurrent
    // pointers or indices.  Each TResult is constructed by calling its constructor that takes a
    // TParameter, amd a TCurrent.  The parameter is the value provided when the
    // InstantiatingIterator is constructed; the current is the current value of the iterator.
    template <typename Current,
              typename Result,
              typename Parameter,
              typename Constructor = constructor_forwarder<Result>,
              typename Transformer = identity_transformer,
              typename Category    = std::random_access_iterator_tag>
    class instantiating_iterator
    {
    public:

        typedef Category             iterator_category;
        typedef Result               value_type;
        typedef Result               reference;
        typedef indirectable<Result> pointer;
        typedef difference_type      difference_type;

        instantiating_iterator()
            : _parameter(), _current()
        {
        }

        instantiating_iterator(Parameter const parameter, Current const current)
            : _parameter(parameter), _current(current)
        {
        }

        auto get()        const -> reference { return Constructor()(_parameter, Transformer()(_current)); }
        auto operator*()  const -> reference { return Constructor()(_parameter, Transformer()(_current)); }
        auto operator->() const -> pointer   { return Constructor()(_parameter, Transformer()(_current)); }

        reference operator[](difference_type const n) const
        {
            return Constructor()(_parameter, Transformer()(_current + n));
        }

        friend auto operator==(instantiating_iterator const& lhs, instantiating_iterator const& rhs) -> bool
        {
            return lhs._current == rhs._current;
        }

        friend auto operator< (instantiating_iterator const& lhs, instantiating_iterator const& rhs) -> bool
        {
            return lhs._current < rhs._current;
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(instantiating_iterator)
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS_TEMPLATE(instantiating_iterator, _current, difference_type)

    private:

        Parameter _parameter;
        Current   _current;
    };





    template <typename Iterator>
    class iterator_range
    {
    public:

        typedef Iterator                            iterator_type;
        typedef std::iterator_traits<iterator_type> traits_type;
        typedef typename traits_type::value_type    value_type;
        typedef typename traits_type::reference     reference_type;

        iterator_range()
        {
        }

        iterator_range(iterator_type const first, iterator_type const last)
            : _first(first), _last(last)
        {
        }

        auto begin() const -> iterator_type { return _first.get(); }
        auto end()   const -> iterator_type { return _last.get();  }

        auto empty() const -> bool      { return _first.get() == _last.get(); }
        auto size()  const -> size_type { return _last.get()  - _first.get(); }

    private:

        value_initialized<iterator_type> _first;
        value_initialized<iterator_type> _last;
    };





    template <typename ForwardIterator, typename Filter>
    class static_filter_iterator
    {
    public:

        typedef std::forward_iterator_tag                                  iterator_category;
        typedef typename std::iterator_traits<ForwardIterator>::value_type value_type;
        typedef typename std::iterator_traits<ForwardIterator>::reference  reference;
        typedef typename std::iterator_traits<ForwardIterator>::pointer    pointer;
        typedef std::ptrdiff_t                                             difference_type;

        static_filter_iterator()
        {
        }

        static_filter_iterator(ForwardIterator const current, ForwardIterator const last, Filter const filter)
            : _current(current), _last(last), _filter(filter)
        {
            filter_advance();
        }

        auto operator*() const -> reference
        {
            assert_dereferenceable();
            return _current.get().operator*();
        }

        auto operator->() const -> pointer
        {
            assert_dereferenceable();
            return _current.get().operator->();
        }

        auto operator++() -> static_filter_iterator&
        {
            assert_dereferenceable();
            ++_current.get();
            filter_advance();
            return *this;
        }

        auto operator++(int) -> static_filter_iterator
        {
            static_filter_iterator const it(*this);
            ++*this;
            return it;
        }

        auto is_dereferenceable() const -> bool
        {
            return _current.get() != _last.get();
        }

        friend auto operator==(static_filter_iterator const& lhs, static_filter_iterator const& rhs) -> bool
        {
            return (!lhs.is_dereferenceable() && !rhs.is_dereferenceable())
                || lhs._current.get() == rhs._current.get();
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(static_filter_iterator)

    private:

        auto operator=(static_filter_iterator const&) -> static_filter_iterator&;

        void assert_dereferenceable() const
        {
            assert_true([&]{ return is_dereferenceable(); });
        }

        void filter_advance()
        {
            while (_current.get() != _last.get() && !_filter.get()(*_current.get()))
                ++_current.get();
        }

        value_initialized<ForwardIterator> _current;
        value_initialized<ForwardIterator> _last;
        value_initialized<Filter         > _filter;
    };

    template <typename ForwardIterator, typename Filter>
    class static_filtered_range
    {
    public:

        static_filtered_range(ForwardIterator const first, ForwardIterator const last, Filter const filter)
            : _first(first), _last(last), _filter(filter)
        {
        }

        auto begin() const -> static_filter_iterator<ForwardIterator, Filter>
        {
            return static_filter_iterator<ForwardIterator, Filter>(_first.get(), _last.get(), _filter.get());
        }

        auto end() const -> static_filter_iterator<ForwardIterator, Filter>
        {
            return static_filter_iterator<ForwardIterator, Filter>(_last.get(), _last.get(), _filter.get());
        }

    private:

        auto operator=(static_filtered_range const&) -> static_filtered_range&;

        value_initialized<ForwardIterator> _first;
        value_initialized<ForwardIterator> _last;
        value_initialized<Filter         > _filter;
    };

    template <typename ForwardIterator, typename Filter>
    auto create_static_filtered_range(ForwardIterator  const first,
                                      ForwardIterator  const last, 
                                      Filter           const filter) -> static_filtered_range<ForwardIterator, Filter>
    {
        return static_filtered_range<ForwardIterator, Filter>(first, last, filter);
    }





    /// An iterator that iterates a range of bytes in strides
    class stride_iterator
    {
    public:

        typedef std::random_access_iterator_tag     iterator_category;
        typedef difference_type                     difference_type;
        typedef const_byte_iterator                 value_type;
        typedef value_type                          reference;
        typedef indirectable<value_type>            pointer;

        stride_iterator() { }

        stride_iterator(const_byte_iterator const current, size_type const stride)
            : _current(current), _stride(stride)
        {
            // Note:  It is valid to have a nullptr current or a stride of zero; this will be the
            // case if we have a stride iterator into an empty range.
        }

        auto stride()     const -> size_type { return _stride.get(); }
        auto get()        const -> reference { return value();       }
        auto operator*()  const -> reference { return value();       }
        auto operator->() const -> pointer   { return value();       }

        auto operator++() -> stride_iterator&
        {
            assert_initialized(*this);
            _current.get() += _stride.get();
            return *this;
        }

        auto operator++(int) -> stride_iterator
        {
            stride_iterator const it(*this);
            ++*this;
            return it;
        }

        auto operator--() -> stride_iterator&
        {
            assert_initialized(*this);
            _current.get() -= _stride.get();
            return *this;
        }

        auto operator--(int) -> stride_iterator
        {
            stride_iterator const it(*this);
            --*this;
            return it;
        }

        auto operator+=(difference_type const n) -> stride_iterator&
        {
            assert_initialized(*this);
            _current.get() += n * _stride.get();
            return *this;
        }

        auto operator-=(difference_type const n) -> stride_iterator&
        {
            assert_initialized(*this);
            _current.get() -= n * _stride.get();
            return *this;
        }

        auto operator[](difference_type const n) const -> reference
        {
            assert_initialized(*this);
            return _current.get() + n * _stride.get();
        }

        auto is_initialized() const -> bool
        {
            return _current.get() != nullptr;
        }

        friend auto operator+(stride_iterator it, difference_type const n) -> stride_iterator { return it +=  n; }
        friend auto operator+(difference_type const n, stride_iterator it) -> stride_iterator { return it +=  n; }
        friend auto operator-(stride_iterator it, difference_type const n) -> stride_iterator { return it += -n; }

        friend auto operator-(stride_iterator const& lhs, stride_iterator const& rhs) -> difference_type
        {
            assert_comparable(lhs, rhs);

            // Iterators into an empty table will have a stride of zero.  All iterators into such a
            // table compare equal and are thus end iterators, so the difference between any two
            // iterators into such a table is zero.
            if (lhs._stride.get() == 0)
                return 0;

            return static_cast<difference_type>((lhs._current.get() - rhs._current.get()) / lhs._stride.get());
        }

        friend auto operator==(stride_iterator const& lhs, stride_iterator const& rhs) -> bool
        {
            if (lhs.is_initialized() != rhs.is_initialized())
                return false;

            assert_comparable(lhs, rhs);
            return lhs._current.get() == rhs._current.get();
        }

        friend auto operator<(stride_iterator const& lhs, stride_iterator const& rhs) -> bool
        {
            assert_initialized(lhs);
            assert_initialized(rhs);
            assert_comparable(lhs, rhs);
            return lhs._current.get() < rhs._current.get();
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(stride_iterator)

    private:

        static auto assert_comparable(stride_iterator const& lhs, stride_iterator const& rhs) -> void
        {
            assert_true([&]{ return lhs._stride.get() == rhs._stride.get(); });
        }

        auto value() const -> reference
        {
            assert_initialized(*this);
            return _current.get();
        }

        value_initialized<const_byte_iterator> _current;
        value_initialized<size_type>           _stride;
    };

} }

#endif
