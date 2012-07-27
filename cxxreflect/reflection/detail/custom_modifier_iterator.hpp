
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_CUSTOM_MODIFIER_ITERATOR_HPP_
#define CXXREFLECT_REFLECTION_CUSTOM_MODIFIER_ITERATOR_HPP_

#include "cxxreflect/metadata/metadata.hpp"
#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/independent_handles.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    class custom_modifier_iterator
    {
    public:

        typedef type                      value_type;
        typedef type                      reference;
        typedef core::indirectable<type>  pointer;
        typedef core::difference_type     difference_type;
        typedef std::forward_iterator_tag iterator_category;

        typedef metadata::type_signature::custom_modifier_iterator inner_iterator;

        enum class kind { unknown, required, optional };

        custom_modifier_iterator()
        {
        }

        custom_modifier_iterator(kind const filter_kind, module_handle const& module, inner_iterator const& it)
            : _kind(filter_kind), _module(module), _it(it)
        {
            core::assert_true([&]{ return filter_kind != kind::unknown; });
            core::assert_initialized(module);

            if (should_advance())
                advance();
        }

        // The definitions require 'type' to be complete
        auto operator*()  const -> reference;
        auto operator->() const -> pointer;

        auto operator++() -> custom_modifier_iterator&
        {
            advance();
            return *this;
        }

        auto operator++(int) -> custom_modifier_iterator
        {
            custom_modifier_iterator const it(*this);
            ++*this;
            return it;
        }

        auto is_initialized() const -> bool
        {
            return _kind.get() != kind::unknown;
        }

        friend auto operator==(custom_modifier_iterator const& lhs, custom_modifier_iterator const& rhs) -> bool
        {
            assert_comparable(lhs, rhs);
            return lhs._it == rhs._it;
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(custom_modifier_iterator)

    private:

        auto advance() -> void
        {
            core::assert_initialized(*this);
            core::assert_true([&]{ return _it != inner_iterator(); });

            do
            {
                ++_it;
            }
            while (should_advance());
        }

        auto should_advance() -> bool
        {
            core::assert_initialized(*this);

            if (_it == inner_iterator())
                return false;

            if (_it->is_required() == (_kind.get() == kind::required))
                return false;

            return true;
        }

        static auto assert_comparable(custom_modifier_iterator const& lhs, custom_modifier_iterator const& rhs) -> void
        {
            core::assert_true([&]{ return lhs._kind.get() == rhs._kind.get(); });
        }

        module_handle                 _module;
        core::value_initialized<kind> _kind;
        inner_iterator                _it;
    };

    auto begin_custom_modifiers(metadata::type_signature const& signature) -> custom_modifier_iterator;
    auto end_custom_modifiers  (metadata::type_signature const& signature) -> custom_modifier_iterator;

} } }

#endif 
