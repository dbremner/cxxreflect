
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/precompiled_headers.hpp"
#include "cxxreflect/metadata/relationships.hpp"
#include "cxxreflect/metadata/rows.hpp"
#include "cxxreflect/metadata/utility.hpp"

namespace cxxreflect { namespace metadata {

    auto find_owner_of_event(event_token const& element) -> type_def_row
    {
        core::assert_initialized(element);

        auto const map_row(detail::get_owning_row<table_id::event_map, table_id::event>(
            element,
            column_id::event_map_first_event));

        return row_from(map_row.parent());
    }
    
    auto find_owner_of_method_def(method_def_token const& element) -> type_def_row
    {
        core::assert_initialized(element);

        return detail::get_owning_row<table_id::type_def, table_id::method_def>(
            element,
            column_id::type_def_first_method);
    }

    auto find_owner_of_field(field_token const& element) -> type_def_row
    {
        core::assert_initialized(element);

        return detail::get_owning_row<table_id::type_def, table_id::field>(
            element,
            column_id::type_def_first_field);
    }

    auto find_owner_of_property(property_token const& element) -> type_def_row
    {
        core::assert_initialized(element);

        auto const map_row(detail::get_owning_row<table_id::property_map, table_id::property>(
            element,
            column_id::property_map_first_property));

        return row_from(map_row.parent());
    }

    auto find_owner_of_param(param_token const& element) -> method_def_row
    {
        core::assert_initialized(element);

        return detail::get_owning_row<table_id::method_def, table_id::param>(
            element,
            column_id::method_def_first_parameter);
    }





    auto find_constant(has_constant_token const& parent) -> constant_row
    {
        core::assert_initialized(parent);

        auto const range(detail::composite_index_primary_key_equal_range(
            parent,
            composite_index::has_constant,
            table_id::constant,
            column_id::constant_parent));

        // Not every row has a constant value:
        if (range.empty())
            return constant_row();

        // If a row has a constant, it must have exactly one:
        database_table const& constant_table(parent.scope().tables()[table_id::constant]);
        if (range.size() != constant_table.row_size())
            throw core::metadata_error(L"constant table has non-unique parent index");

        return create_row<constant_row>(&parent.scope(), begin(range));
    }

    auto find_field_layout(field_token const& parent) -> field_layout_row
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::field,
            table_id::field_layout,
            column_id::field_layout_parent));

        // Not every row has a field layout value:
        if (range.empty())
            return field_layout_row();

        // If a row has a field layout, it must have exactly one:
        database_table const& field_layout_table(parent.scope().tables()[table_id::field_layout]);
        if (range.size() != field_layout_table.row_size())
            throw core::metadata_error(L"field layout table has non-unique parent index");

        return create_row<field_layout_row>(&parent.scope(), begin(range));
    }





    auto find_custom_attributes(has_custom_attribute_token const& parent) -> custom_attribute_row_range
    {
        core::assert_initialized(parent);

        auto const range(detail::composite_index_primary_key_equal_range(
            parent,
            composite_index::has_custom_attribute,
            table_id::custom_attribute,
            column_id::custom_attribute_parent));

        return custom_attribute_row_range(
            custom_attribute_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            custom_attribute_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }





    auto find_events(type_def_token const& parent) -> event_row_range
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::type_def,
            table_id::event_map,
            column_id::event_map_parent));

        // Not every type has events; if this is such a type, return an empty range:
        if (range.empty())
            return event_row_range(
                event_row_iterator(&parent.scope(), 0),
                event_row_iterator(&parent.scope(), 0));

        // If a row has an event_map row, it must have exactly one:
        database_table const& event_map_table(parent.scope().tables()[table_id::event_map]);
        if (range.size() != event_map_table.row_size())
            throw core::metadata_error(L"event map table has non-unique parent index");

        event_map_row const map_row(*event_map_row_iterator::from_row_pointer(&parent.scope(), begin(range)));
        return event_row_range(
            event_row_iterator(&parent.scope(), map_row.first_event().index()),
            event_row_iterator(&parent.scope(), map_row.last_event().index()));
    }





    auto find_fields(type_def_token const& parent) -> field_row_range
    {
        core::assert_initialized(parent);

        type_def_row const row(row_from(parent));
        return field_row_range(
            field_row_iterator(&parent.scope(), row.first_field().index()),
            field_row_iterator(&parent.scope(), row.last_field().index()));
    }





    auto find_generic_params(type_or_method_def_token const& parent) -> generic_param_row_range
    {
        core::assert_initialized(parent);

        auto const range(detail::composite_index_primary_key_equal_range(
            parent,
            composite_index::type_or_method_def,
            table_id::generic_param,
            column_id::generic_param_parent));

        return generic_param_row_range(
            generic_param_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            generic_param_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }

    auto find_generic_param(type_or_method_def_token const& parent, core::size_type const index) -> generic_param_row
    {
        core::assert_initialized(parent);

        auto const range(find_generic_params(parent));
        if (core::distance(begin(range), end(range)) < index)
            throw core::runtime_error(L"generic param index out of range");

        return *std::next(begin(range), index);
    }





    auto find_generic_param_constraints(generic_param_token const& parent) -> generic_param_constraint_row_range
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::generic_param,
            table_id::generic_param_constraint,
            column_id::generic_param_constraint_parent));

        return generic_param_constraint_row_range(
            generic_param_constraint_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            generic_param_constraint_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }





    auto find_interface_impls(type_def_token const& parent) -> interface_impl_row_range
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::type_def,
            table_id::interface_impl,
            column_id::interface_impl_parent));

        return interface_impl_row_range(
            interface_impl_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            interface_impl_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }





    auto find_method_defs(type_def_token const& parent) -> method_def_row_range
    {
        core::assert_initialized(parent);

        type_def_row const row(row_from(parent));
        return method_def_row_range(
            method_def_row_iterator(&parent.scope(), row.first_method().index()),
            method_def_row_iterator(&parent.scope(), row.last_method().index()));
    }





    auto find_method_impls(type_def_token const& parent) -> method_impl_row_range
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::type_def,
            table_id::method_impl,
            column_id::method_impl_parent));
        
        return method_impl_row_range(
            method_impl_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            method_impl_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }





    auto find_method_semantics(has_semantics_token const& parent) -> method_semantics_row_range
    {
        core::assert_initialized(parent);

        auto const range(detail::composite_index_primary_key_equal_range(
            parent,
            composite_index::has_semantics,
            table_id::method_semantics,
            column_id::method_semantics_parent));

        return method_semantics_row_range(
            method_semantics_row_iterator::from_row_pointer(&parent.scope(), begin(range)),
            method_semantics_row_iterator::from_row_pointer(&parent.scope(), end(range)));
    }





    auto find_properties(type_def_token const& parent) -> property_row_range
    {
        core::assert_initialized(parent);

        auto const range(detail::table_id_primary_key_equal_range(
            parent,
            table_id::type_def,
            table_id::property_map,
            column_id::property_map_parent));

        // Not every type has properties; if this is such a type, return an empty range:
        if (range.empty())
            return property_row_range(
                property_row_iterator(&parent.scope(), 0),
                property_row_iterator(&parent.scope(), 0));

        // If a row has an property_map row, it must have exactly one:
        database_table const& property_map_table(parent.scope().tables()[table_id::property_map]);
        if (range.size() != property_map_table.row_size())
            throw core::metadata_error(L"property map table has non-unique parent index");

        property_map_row const map_row(*property_map_row_iterator::from_row_pointer(&parent.scope(), begin(range)));
        return property_row_range(
            property_row_iterator(&parent.scope(), map_row.first_property().index()),
            property_row_iterator(&parent.scope(), map_row.last_property().index()));
    }

} }
