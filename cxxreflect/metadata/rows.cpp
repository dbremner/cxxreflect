
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/precompiled_headers.hpp"
#include "cxxreflect/metadata/rows.hpp"
#include "cxxreflect/metadata/utility.hpp"

namespace cxxreflect { namespace metadata {

    auto assembly_row::hash_algorithm() const -> assembly_hash_algorithm
    {
        return detail::read_as<assembly_hash_algorithm>(
            iterator(),
            column_offset(column_id::assembly_hash_algorithm));
    }

    auto assembly_row::version() const -> four_component_version
    {
        detail::pe_four_component_version const version(detail::read_as<detail::pe_four_component_version>(
            iterator(),
            column_offset(column_id::assembly_version)));

        return four_component_version(version.major, version.minor, version.build, version.revision);
    }

    auto assembly_row::flags() const -> assembly_flags
    {
        return detail::read_as<assembly_attribute>(
            iterator(),
            column_offset(column_id::assembly_flags));
    }

    auto assembly_row::public_key() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::assembly_public_key));
    }

    auto assembly_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::assembly_name));
    }

    auto assembly_row::culture() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::assembly_culture));
    }





    auto assembly_os_row::platform_id() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::assembly_os_platform_id));
    }

    auto assembly_os_row::major_version() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::assembly_os_major_version));
    }

    auto assembly_os_row::minor_version() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::assembly_os_minor_version));
    }





    auto assembly_processor_row::processor() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::assembly_processor_processor));
    }





    auto assembly_ref_row::version() const -> four_component_version
    {
        detail::pe_four_component_version const version(detail::read_as<detail::pe_four_component_version>(
            iterator(),
            column_offset(column_id::assembly_ref_version)));

        return four_component_version(version.major, version.minor, version.build, version.revision);
    }

    auto assembly_ref_row::flags() const -> assembly_flags
    {
        return detail::read_as<assembly_attribute>(
            iterator(),
            column_offset(column_id::assembly_ref_flags));
    }

    auto assembly_ref_row::public_key() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::assembly_ref_public_key));
    }

    auto assembly_ref_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::assembly_ref_name));
    }

    auto assembly_ref_row::culture() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::assembly_ref_culture));
    }

    auto assembly_ref_row::hash_value() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::assembly_ref_hash_value));
    }





    auto assembly_ref_os_row::platform_id() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::assembly_ref_os_platform_id));
    }

    auto assembly_ref_os_row::major_version() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::assembly_ref_os_major_version));
    }

    auto assembly_ref_os_row::minor_version() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::assembly_ref_os_minor_version));
    }

    auto assembly_ref_os_row::parent() const -> assembly_ref_token
    {
        return detail::read_token<assembly_ref_token>(
            scope(),
            iterator(),
            table_id::assembly_ref,
            column_offset(column_id::assembly_ref_os_parent));
    }





    auto assembly_ref_processor_row::processor() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::assembly_ref_processor_processor));
    }

    auto assembly_ref_processor_row::parent() const -> assembly_ref_token
    {
        return detail::read_token<assembly_ref_token>(
            scope(),
            iterator(),
            table_id::assembly_ref,
            column_offset(column_id::assembly_ref_processor_parent));
    }





    auto class_layout_row::packing_size() const -> std::uint16_t
    {
        return detail::read_as<std::uint16_t>(
            iterator(),
            column_offset(column_id::class_layout_packing_size));
    }

    auto class_layout_row::class_size() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::class_layout_class_size));
    }

    auto class_layout_row::parent() const -> type_def_token
    {
        return detail::read_token<type_def_token>(
            scope(),
            iterator(),
            table_id::type_def,
            column_offset(column_id::class_layout_parent));
    }





    auto constant_row::type() const -> element_type
    {
        core::byte const type(detail::read_as<core::byte>(iterator(), column_offset(column_id::constant_type)));
        if (!is_valid_element_type(type))
            throw core::metadata_error(L"constant row contains invalid element type");

        return static_cast<element_type>(type);
    }

    auto constant_row::parent() const -> has_constant_token
    {
        return detail::read_token<has_constant_token>(
            scope(),
            iterator(),
            composite_index::has_constant,
            column_offset(column_id::constant_parent));
    }

    auto constant_row::parent_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::has_constant,
            column_offset(column_id::constant_parent));
    }

    auto constant_row::value() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::constant_value));
    }





    auto custom_attribute_row::parent() const -> has_custom_attribute_token
    {
        return detail::read_token<has_custom_attribute_token>(
            scope(),
            iterator(),
            composite_index::has_custom_attribute,
            column_offset(column_id::custom_attribute_parent));
    }

    auto custom_attribute_row::parent_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::has_custom_attribute,
            column_offset(column_id::custom_attribute_parent));
    }

    auto custom_attribute_row::type() const -> custom_attribute_type_token
    {
        return detail::read_token<custom_attribute_type_token>(
            scope(),
            iterator(),
            composite_index::custom_attribute_type,
            column_offset(column_id::custom_attribute_type));
    }

    auto custom_attribute_row::type_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::custom_attribute_type,
            column_offset(column_id::custom_attribute_type));
    }

    auto custom_attribute_row::value() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::custom_attribute_value));
    }





    auto decl_security_row::action() const -> std::uint16_t
    {
        return detail::read_as<std::uint16_t>(
            iterator(),
            column_offset(column_id::decl_security_action));
    }

    auto decl_security_row::parent() const -> has_decl_security_token
    {
        return detail::read_token<has_decl_security_token>(
            scope(),
            iterator(),
            composite_index::has_decl_security,
            column_offset(column_id::decl_security_parent));
    }

    auto decl_security_row::parent_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::has_decl_security,
            column_offset(column_id::decl_security_parent));
    }

    auto decl_security_row::permission_set() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::decl_security_permission_set));
    }





    auto event_map_row::parent() const -> type_def_token
    {
        return detail::read_token<type_def_token>(
            scope(),
            iterator(),
            table_id::type_def,
            column_offset(column_id::event_map_parent));
    }

    auto event_map_row::first_event() const -> event_token
    {
        return detail::read_token<event_token>(
            scope(),
            iterator(),
            table_id::event,
            column_offset(column_id::event_map_first_event));
    }

    auto event_map_row::last_event() const -> event_token
    {
        return detail::compute_last_row_token<
            table_id::event_map,
            table_id::event
        >(scope(), iterator(), &event_map_row::first_event);
    }





    auto event_row::flags() const -> event_flags
    {
        return detail::read_as<event_attribute>(
            iterator(),
            column_offset(column_id::event_flags));
    }

    auto event_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::event_name));
    }

    auto event_row::type() const -> type_def_ref_spec_token
    {
        return detail::read_token<type_def_ref_spec_token>(
            scope(),
            iterator(),
            composite_index::type_def_ref_spec,
            column_offset(column_id::event_type));
    }

    auto event_row::type_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::type_def_ref_spec,
            column_offset(column_id::event_type));
    }





    auto exported_type_row::flags() const -> type_flags
    {
        return detail::read_as<type_attribute>(
            iterator(),
            column_offset(column_id::exported_type_flags));
    }

    auto exported_type_row::type_def_id() const -> std::uint32_t
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::exported_type_type_def_id));
    }

    auto exported_type_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::exported_type_name));
    }

    auto exported_type_row::namespace_name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::exported_type_namespace_name));
    }

    auto exported_type_row::implementation() const -> implementation_token
    {
        return detail::read_token<implementation_token>(
            scope(),
            iterator(),
            composite_index::implementation,
            column_offset(column_id::exported_type_implementation));
    }

    auto exported_type_row::implementation_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::implementation,
            column_offset(column_id::exported_type_implementation));
    }





    auto field_row::flags() const -> field_flags
    {
        return detail::read_as<field_attribute>(
            iterator(),
            column_offset(column_id::field_flags));
    }

    auto field_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::field_name));
    }

    auto field_row::signature() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::field_signature));
    }





    auto field_layout_row::offset() const -> core::size_type
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::field_layout_offset));
    }

    auto field_layout_row::parent() const -> field_token
    {
        return detail::read_token<field_token>(
            scope(),
            iterator(),
            table_id::field,
            column_offset(column_id::field_layout_parent));
    }





    auto field_marshal_row::parent() const -> has_field_marshal_token
    {
        return detail::read_token<has_field_marshal_token>(
            scope(),
            iterator(),
            composite_index::has_field_marshal,
            column_offset(column_id::field_marshal_parent));
    }

    auto field_marshal_row::parent_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::has_field_marshal,
            column_offset(column_id::field_marshal_parent));
    }

    auto field_marshal_row::native_type() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::field_marshal_native_type));
    }





    auto field_rva_row::rva() const -> core::size_type
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::field_rva_rva));
    }

    auto field_rva_row::parent() const -> field_token
    {
        return detail::read_token<field_token>(
            scope(),
            iterator(),
            table_id::field,
            column_offset(column_id::field_rva_parent));
    }





    auto file_row::flags() const -> file_flags
    {
        return detail::read_as<file_attribute>(
            iterator(),
            column_offset(column_id::file_flags));
    }

    auto file_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::file_name));
    }

    auto file_row::hash_value() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::file_hash_value));
    }





    auto generic_param_row::sequence() const -> std::uint16_t
    {
        return detail::read_as<std::uint16_t>(
            iterator(),
            column_offset(column_id::generic_param_sequence));
    }

    auto generic_param_row::flags() const -> generic_parameter_flags
    {
        return detail::read_as<generic_parameter_attribute>(
            iterator(),
            column_offset(column_id::generic_param_flags));
    }

    auto generic_param_row::parent() const -> type_or_method_def_token
    {
        return detail::read_token<type_or_method_def_token>(
            scope(),
            iterator(),
            composite_index::type_or_method_def,
            column_offset(column_id::generic_param_parent));
    }

    auto generic_param_row::parent_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::type_or_method_def,
            column_offset(column_id::generic_param_parent));
    }

    auto generic_param_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::generic_param_name));
    }





    auto generic_param_constraint_row::parent() const -> generic_param_token
    {
        return detail::read_token<generic_param_token>(
            scope(),
            iterator(),
            table_id::generic_param,
            column_offset(column_id::generic_param_constraint_parent));
    }

    auto generic_param_constraint_row::constraint() const -> type_def_ref_spec_token
    {
        return detail::read_token<type_def_ref_spec_token>(
            scope(),
            iterator(),
            composite_index::type_def_ref_spec,
            column_offset(column_id::generic_param_constraint_constraint));
    }

    auto generic_param_constraint_row::constraint_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::type_def_ref_spec,
            column_offset(column_id::generic_param_constraint_constraint));
    }





    auto impl_map_row::flags() const -> pinvoke_flags
    {
        return detail::read_as<pinvoke_attribute>(
            iterator(),
            column_offset(column_id::impl_map_flags));
    }

    auto impl_map_row::member_forwarded() const -> member_forwarded_token
    {
        return detail::read_token<member_forwarded_token>(
            scope(),
            iterator(),
            composite_index::member_forwarded,
            column_offset(column_id::impl_map_member_forwarded));
    }

    auto impl_map_row::member_forwarded_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::member_forwarded,
            column_offset(column_id::impl_map_member_forwarded));
    }

    auto impl_map_row::import_name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::impl_map_import_name));
    }

    auto impl_map_row::import_scope() const -> module_ref_token
    {
        return detail::read_token<module_ref_token>(
            scope(),
            iterator(),
            table_id::module_ref,
            column_offset(column_id::impl_map_import_scope));
    }





    auto interface_impl_row::parent() const -> type_def_token
    {
        return detail::read_token<type_def_token>(
            scope(),
            iterator(),
            table_id::type_def,
            column_offset(column_id::interface_impl_parent));
    }

    auto interface_impl_row::interface_() const -> type_def_ref_spec_token
    {
        return detail::read_token<type_def_ref_spec_token>(
            scope(),
            iterator(),
            composite_index::type_def_ref_spec,
            column_offset(column_id::interface_impl_interface));
    }

    auto interface_impl_row::interface_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::type_def_ref_spec,
            column_offset(column_id::interface_impl_interface));
    }





    auto manifest_resource_row::offset() const -> core::size_type
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::manifest_resource_offset));
    }

    auto manifest_resource_row::flags() const -> manifest_resource_flags
    {
        return detail::read_as<manifest_resource_attribute>(
            iterator(),
            column_offset(column_id::manifest_resource_flags));
    }

    auto manifest_resource_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::manifest_resource_name));
    }

    auto manifest_resource_row::implementation() const -> implementation_token
    {
        return detail::read_token<implementation_token>(
            scope(),
            iterator(),
            composite_index::implementation,
            column_offset(column_id::manifest_resource_implementation));
    }

    auto manifest_resource_row::implementation_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::implementation,
            column_offset(column_id::manifest_resource_implementation));
    }





    auto member_ref_row::parent() const -> member_ref_parent_token
    {
        return detail::read_token<member_ref_parent_token>(
            scope(),
            iterator(),
            composite_index::member_ref_parent,
            column_offset(column_id::member_ref_parent));
    }

    auto member_ref_row::parent_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::member_ref_parent,
            column_offset(column_id::member_ref_parent));
    }

    auto member_ref_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::member_ref_name));
    }

    auto member_ref_row::signature() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::member_ref_signature));
    }





    auto method_def_row::rva() const -> core::size_type
    {
        return detail::read_as<std::uint32_t>(
            iterator(),
            column_offset(column_id::method_def_rva));
    }

    auto method_def_row::implementation_flags() const -> method_implementation_flags
    {
        return detail::read_as<method_implementation_attribute>(
            iterator(),
            column_offset(column_id::method_def_implementation_flags));
    }

    auto method_def_row::flags() const -> method_flags
    {
        return detail::read_as<method_attribute>(
            iterator(),
            column_offset(column_id::method_def_flags));
    }

    auto method_def_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::method_def_name));
    }

    auto method_def_row::signature() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::method_def_signature));
    }

    auto method_def_row::first_parameter() const -> param_token
    {
        return detail::read_token<param_token>(
            scope(),
            iterator(),
            table_id::param,
            column_offset(column_id::method_def_first_parameter));
    }

    auto method_def_row::last_parameter() const -> param_token
    {
        return detail::compute_last_row_token<
            table_id::method_def,
            table_id::param
        >(scope(), iterator(), &method_def_row::first_parameter);
    }





    auto method_impl_row::parent() const -> type_def_token
    {
        return detail::read_token<type_def_token>(
            scope(),
            iterator(),
            table_id::type_def,
            column_offset(column_id::method_impl_parent));
    }

    auto method_impl_row::method_body() const -> method_def_or_ref_token
    {
        return detail::read_token<method_def_or_ref_token>(
            scope(),
            iterator(),
            composite_index::method_def_or_ref,
            column_offset(column_id::method_impl_method_body));
    }

    auto method_impl_row::method_body_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::method_def_or_ref,
            column_offset(column_id::method_impl_method_body));
    }

    auto method_impl_row::method_declaration() const -> method_def_or_ref_token
    {
        return detail::read_token<method_def_or_ref_token>(
            scope(),
            iterator(),
            composite_index::method_def_or_ref,
            column_offset(column_id::method_impl_method_declaration));
    }

    auto method_impl_row::method_declaration_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::method_def_or_ref,
            column_offset(column_id::method_impl_method_declaration));
    }





    auto method_semantics_row::semantics() const -> method_semantics_flags
    {
        return detail::read_as<method_semantics_attribute>(
            iterator(),
            column_offset(column_id::method_semantics_semantics));
    }

    auto method_semantics_row::method() const -> method_def_token
    {
        return detail::read_token<method_def_token>(
            scope(),
            iterator(),
            table_id::method_def,
            column_offset(column_id::method_semantics_method));
    }

    auto method_semantics_row::parent() const -> has_semantics_token
    {
        return detail::read_token<has_semantics_token>(
            scope(),
            iterator(),
            composite_index::has_semantics,
            column_offset(column_id::method_semantics_parent));
    }

    auto method_semantics_row::parent_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::has_semantics,
            column_offset(column_id::method_semantics_parent));
    }





    auto method_spec_row::method() const -> method_def_or_ref_token
    {
        return detail::read_token<method_def_or_ref_token>(
            scope(),
            iterator(),
            composite_index::method_def_or_ref,
            column_offset(column_id::method_spec_method));
    }

    auto method_spec_row::method_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::method_def_or_ref,
            column_offset(column_id::method_spec_method));
    }

    auto method_spec_row::signature() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::method_spec_signature));
    }





    auto module_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::module_name));
    }

    auto module_row::mvid() const -> blob
    {
        return detail::read_guid_reference(
            scope(),
            iterator(),
            column_offset(column_id::module_mvid));
    }





    auto module_ref_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::module_ref_name));
    }





    auto nested_class_row::nested_class() const -> type_def_token
    {
        return detail::read_token<type_def_token>(
            scope(),
            iterator(),
            table_id::type_def,
            column_offset(column_id::nested_class_nested_class));
    }

    auto nested_class_row::enclosing_class() const -> type_def_token
    {
        return detail::read_token<type_def_token>(
            scope(),
            iterator(),
            table_id::type_def,
            column_offset(column_id::nested_class_enclosing_class));
    }





    auto param_row::flags() const -> parameter_flags
    {
        return detail::read_as<parameter_attribute>(
            iterator(),
            column_offset(column_id::param_flags));
    }

    auto param_row::sequence() const -> std::uint16_t
    {
        return detail::read_as<std::uint16_t>(
            iterator(),
            column_offset(column_id::param_sequence));
    }

    auto param_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::param_name));
    }





    auto property_row::flags() const -> property_flags
    {
        return detail::read_as<property_attribute>(
            iterator(),
            column_offset(column_id::property_flags));
    }

    auto property_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::property_name));
    }

    auto property_row::signature() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::property_signature));
    }





    auto property_map_row::parent() const -> type_def_token
    {
        return detail::read_token<type_def_token>(
            scope(),
            iterator(),
            table_id::type_def,
            column_offset(column_id::property_map_parent));
    }

    auto property_map_row::first_property() const -> property_token
    {
        return detail::read_token<property_token>(
            scope(),
            iterator(),
            table_id::property,
            column_offset(column_id::property_map_first_property));
    }

    auto property_map_row::last_property() const -> property_token
    {
        return detail::compute_last_row_token<
            table_id::property_map,
            table_id::property
        >(scope(), iterator(), &property_map_row::first_property);
    }





    auto standalone_sig_row::signature() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::standalone_sig_signature));
    }





    auto type_def_row::flags() const -> type_flags
    {
        return detail::read_as<type_attribute>(
            iterator(),
            column_offset(column_id::type_def_flags));
    }

    auto type_def_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::type_def_name));
    }

    auto type_def_row::namespace_name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::type_def_namespace_name));
    }

    auto type_def_row::extends() const -> type_def_ref_spec_token
    {
        return detail::read_token<type_def_ref_spec_token>(
            scope(),
            iterator(),
            composite_index::type_def_ref_spec,
            column_offset(column_id::type_def_extends));
    }

    auto type_def_row::extends_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::type_def_ref_spec,
            column_offset(column_id::type_def_extends));
    }

    auto type_def_row::first_field() const -> field_token
    {
        return detail::read_token<field_token>(
            scope(),
            iterator(),
            table_id::field,
            column_offset(column_id::type_def_first_field));
    }

    auto type_def_row::last_field() const -> field_token
    {
        return detail::compute_last_row_token<
            table_id::type_def,
            table_id::field
        >(scope(), iterator(), &type_def_row::first_field);
    }

    auto type_def_row::first_method() const -> method_def_token
    {
        return detail::read_token<method_def_token>(
            scope(),
            iterator(),
            table_id::method_def,
            column_offset(column_id::type_def_first_method));
    }

    auto type_def_row::last_method() const -> method_def_token
    {
        return detail::compute_last_row_token<
            table_id::type_def,
            table_id::method_def
        >(scope(), iterator(), &type_def_row::first_method);
    }





    auto type_ref_row::resolution_scope() const -> resolution_scope_token
    {
        return detail::read_token<resolution_scope_token>(
            scope(),
            iterator(),
            composite_index::resolution_scope,
            column_offset(column_id::type_ref_resolution_scope));
    }

    auto type_ref_row::resolution_scope_raw() const -> core::size_type
    {
        return detail::read_composite_index(
            scope(),
            iterator(),
            composite_index::resolution_scope,
            column_offset(column_id::type_ref_resolution_scope));
    }

    auto type_ref_row::name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::type_ref_name));
    }

    auto type_ref_row::namespace_name() const -> core::string_reference
    {
        return detail::read_string_reference(
            scope(),
            iterator(),
            column_offset(column_id::type_ref_namespace_name));
    }





    auto type_spec_row::signature() const -> blob
    {
        return detail::read_blob_reference(
            scope(),
            iterator(),
            column_offset(column_id::type_spec_signature));
    }

} }
