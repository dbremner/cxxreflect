
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_MEMBER_ITERATOR_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_MEMBER_ITERATOR_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    template
    <
        typename Type,
        typename Member,
        typename InnerIterator,
        bool (*Filter)(metadata::binding_flags, Type const&, typename InnerIterator::value_type const&)
    >
    class member_iterator
    {
    public:

        typedef std::forward_iterator_tag  iterator_category;
        typedef Member                     value_type;
        typedef Member                     reference;
        typedef core::indirectable<Member> pointer;
        typedef core::difference_type      difference_type;
        typedef InnerIterator              inner_iterator;
        
        member_iterator()
        {
        }

        member_iterator(Type                    const& reflected_type,
                        inner_iterator          const  current,
                        inner_iterator          const  last,
                        metadata::binding_flags const  filter)
            : _current(current), _last(last), _reflected_type(reflected_type), _filter(filter)
        {
            core::assert_initialized(reflected_type);

            filter_advance();
        }

        auto operator*() const -> reference
        {
            assert_dereferenceable();
            return value_type(_reflected_type, *_current.get(), core::internal_key());
        }

        auto operator->() const -> pointer
        {
            assert_dereferenceable();
            return value_type(_reflected_type, *_current.get(), core::internal_key());
        }

        auto operator++() -> member_iterator&
        {
            assert_dereferenceable();
            ++_current.get();
            filter_advance();
            return *this;
        }

        auto operator++(int) -> member_iterator
        {
            member_iterator const it(*this);
            ++*this;
            return it;
        }

        auto is_initialized() const -> bool
        {
            return _reflected_type.is_initialized();
        }

        auto is_dereferenceable() const -> bool
        {
            return is_initialized() && _current.get() != _last.get();
        }

        friend auto operator==(member_iterator const& lhs, member_iterator const& rhs) -> bool
        {
            return (!lhs.is_dereferenceable() && !rhs.is_dereferenceable())
                || lhs._current.get() == rhs._current.get();
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(member_iterator)

    private:

        auto assert_dereferenceable() const -> void
        {
            core::assert_true([&]{ return is_dereferenceable(); });
        }

        auto filter_advance() -> void
        {
            while (_current.get() != _last.get() && Filter(_filter, _reflected_type, *_current.get()))
                ++_current.get();
        }

        core::value_initialized<inner_iterator> _current;
        core::value_initialized<inner_iterator> _last;
        Type                                    _reflected_type;
        metadata::binding_flags                 _filter;
    };

} } }

#endif
