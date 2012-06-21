
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_RELATIONSHIPS_HPP_
#define CXXREFLECT_METADATA_RELATIONSHIPS_HPP_

#include "cxxreflect/metadata/database.hpp"

namespace cxxreflect { namespace metadata {

    auto find_owner_of_event(event_token const& element) -> type_def_row;
    auto find_owner_of_method_def(method_def_token const& element) -> type_def_row;
    auto find_owner_of_field(field_token const& element) -> type_def_row;
    auto find_owner_of_property(property_token const& element) -> type_def_row;
    auto find_owner_of_param(param_token const& element) -> method_def_row;





    auto find_constant(has_constant_token const& parent) -> constant_row;
    auto find_field_layout(field_token const& parent) -> field_layout_row;





    auto find_custom_attribute_range(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator_pair;
    auto begin_custom_attributes(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator;
    auto end_custom_attributes(has_custom_attribute_token const& parent) -> custom_attribute_row_iterator;





    auto find_events_range(type_def_token const& parent) -> event_row_iterator_pair;
    auto begin_events(type_def_token const& parent) -> event_row_iterator;
    auto end_events(type_def_token const& parent) -> event_row_iterator;





    auto find_interface_impl_range(type_def_token const& parent) -> interface_impl_row_iterator_pair;
    auto begin_interface_impls(type_def_token const& parent) -> interface_impl_row_iterator;
    auto end_interface_impls(type_def_token const& parent) -> interface_impl_row_iterator;





    auto find_method_impl_range(type_def_token const& parent) -> method_impl_row_iterator_pair;
    auto begin_method_impls(type_def_token const& parent) -> method_impl_row_iterator;
    auto end_method_impls(type_def_token const& parent) -> method_impl_row_iterator;





    auto find_method_semantics_range(has_semantics_token const& parent) -> method_semantics_row_iterator_pair;
    auto begin_method_semantics(has_semantics_token const& parent) -> method_semantics_row_iterator;
    auto end_method_semantics(has_semantics_token const& parent) -> method_semantics_row_iterator;




    auto find_properties_range(type_def_token const& parent) -> property_row_iterator_pair;
    auto begin_properties(type_def_token const& parent) -> property_row_iterator;
    auto end_properties(type_def_token const& parent) -> property_row_iterator;

} }

#endif

// AMDG //
