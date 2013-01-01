
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_CUSTOM_MODIFIER_ITERATOR_HPP_
#define CXXREFLECT_REFLECTION_CUSTOM_MODIFIER_ITERATOR_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"

namespace cxxreflect { namespace reflection {

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

        custom_modifier_iterator();
        custom_modifier_iterator(kind const filter_kind, inner_iterator const& it);

        // The definitions require 'type' to be complete
        auto operator*()  const -> reference;
        auto operator->() const -> pointer;

        auto operator++()    -> custom_modifier_iterator&;
        auto operator++(int) -> custom_modifier_iterator;

        auto is_initialized() const -> bool;

        friend auto operator==(custom_modifier_iterator const&, custom_modifier_iterator const&) -> bool;

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(custom_modifier_iterator)

    private:

        auto advance()        -> void;
        auto should_advance() -> bool;

        static auto assert_comparable(custom_modifier_iterator const& lhs, custom_modifier_iterator const& rhs) -> void;

        core::value_initialized<kind> _kind;
        inner_iterator                _it;
    };

    auto get_required_custom_modifiers(metadata::type_signature const& signature) -> custom_modifier_range;
    auto get_optional_custom_modifiers(metadata::type_signature const& signature) -> custom_modifier_range;

} }

#endif 
