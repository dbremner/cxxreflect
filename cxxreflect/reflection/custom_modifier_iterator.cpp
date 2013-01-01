
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/custom_modifier_iterator.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection {

    custom_modifier_iterator::custom_modifier_iterator()
    {
    }

    custom_modifier_iterator::custom_modifier_iterator(kind const filter_kind, inner_iterator const& it)
        : _kind(filter_kind), _it(it)
    {
        core::assert_true([&]{ return filter_kind != kind::unknown; });

        if (should_advance())
            advance();
    }

    auto custom_modifier_iterator::operator*() const -> reference
    {
        core::assert_initialized(*this);

        return type(_it->type(), core::internal_key());
    }

    auto custom_modifier_iterator::operator->() const -> pointer
    {
        core::assert_initialized(*this);

        return **this;
    }

    auto custom_modifier_iterator::operator++() -> custom_modifier_iterator&
    {
        advance();
        return *this;
    }

    auto custom_modifier_iterator::operator++(int) -> custom_modifier_iterator
    {
        custom_modifier_iterator const it(*this);
        ++*this;
        return it;
    }

    auto custom_modifier_iterator::is_initialized() const -> bool
    {
        return _kind.get() != kind::unknown;
    }

    auto operator==(custom_modifier_iterator const& lhs, custom_modifier_iterator const& rhs) -> bool
    {
        custom_modifier_iterator::assert_comparable(lhs, rhs);
        return lhs._it == rhs._it;
    }

    auto custom_modifier_iterator::advance() -> void
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return _it != inner_iterator(); });

        do
        {
            ++_it;
        }
        while (should_advance());
    }

    auto custom_modifier_iterator::should_advance() -> bool
    {
        core::assert_initialized(*this);

        if (_it == inner_iterator())
            return false;

        if (_it->is_required() == (_kind.get() == kind::required))
            return false;

        return true;
    }

    auto custom_modifier_iterator::assert_comparable(custom_modifier_iterator const& lhs, custom_modifier_iterator const& rhs) -> void
    {
        core::assert_true([&]{ return lhs._kind.get() == rhs._kind.get(); });
    }

    auto get_required_custom_modifiers(metadata::type_signature const& signature) -> custom_modifier_range
    {
        core::assert_initialized(signature);

        auto const range(signature.custom_modifiers());
        return custom_modifier_range(
            custom_modifier_iterator(custom_modifier_iterator::kind::required, begin(range)),
            custom_modifier_iterator(custom_modifier_iterator::kind::required, end  (range)));
    }

    auto get_optional_custom_modifiers(metadata::type_signature const& signature) -> custom_modifier_range
    {
        core::assert_initialized(signature);

        auto const range(signature.custom_modifiers());
        return custom_modifier_range(
            custom_modifier_iterator(custom_modifier_iterator::kind::optional, begin(range)),
            custom_modifier_iterator(custom_modifier_iterator::kind::optional, end  (range)));
    }

} }
