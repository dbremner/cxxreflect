
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_DEBUG_HPP_
#define CXXREFLECT_METADATA_DEBUG_HPP_

#include "cxxreflect/metadata/constants.hpp"
#include "cxxreflect/metadata/database.hpp"

namespace cxxreflect { namespace metadata {

    class array_shape;
    class custom_modifier;
    class field_signature;
    class method_signature;
    class property_signature;
    class type_signature;

} }

namespace cxxreflect { namespace metadata { namespace debug {

    class realized_assembly_row
    {
    public:

        realized_assembly_row(assembly_row const&);

    private:

        assembly_hash_algorithm _hash_algorithm;
        four_component_version  _version;
        assembly_flags          _flags;
        blob                    _public_key;
        core::string_reference  _name;
        core::string_reference  _culture;
    };

    class realized_assembly_os_row
    {
    public:

        realized_assembly_os_row(assembly_os_row const&);

    private:

        std::uint32_t _platform_id;
        std::uint32_t _major_version;
        std::uint32_t _minor_version;
    };

    class realized_assembly_processor_row
    {
    public:

        realized_assembly_processor_row(assembly_processor_row const&);

    private:

        std::uint32_t _processor;
    };

    class realized_assembly_ref_row
    {
    public:

        realized_assembly_ref_row(assembly_ref_row const&);

    private:

        four_component_version _version;
        assembly_flags         _flags;
        blob                   _public_key;
        core::string_reference _name;
        core::string_reference _culture;
        blob                   _hash_value;
    };

    class realized_assembly_ref_os_row
    {
    public:

        realized_assembly_ref_os_row(assembly_ref_os_row const&);

    private:

        std::uint32_t      _platform_id;
        std::uint32_t      _major_version;
        std::uint32_t      _minor_version;
        assembly_ref_token _parent;
    };

    class realized_assembly_ref_processor_row
    {
    public:

        realized_assembly_ref_processor_row(assembly_ref_processor_row const&);

    private:

        std::uint32_t      _processor;
        assembly_ref_token _parent;
    };

    class realized_class_layout_row
    {
    public:

        realized_class_layout_row(class_layout_row const&);

    private:

        std::uint16_t  _packing_size;
        std::uint32_t  _class_size;
        type_def_token _parent;
    };

    class realized_constant_row
    {
    public:

        realized_constant_row(constant_row const&);

    private:

        element_type       _type;
        has_constant_token _parent;
        core::size_type    _parent_raw;
        blob               _value;
    };

    class realized_custom_attribute_row
    {
    public:

        realized_custom_attribute_row(custom_attribute_row const&);

    private:

        has_custom_attribute_token  _parent;
        core::size_type             _parent_raw;
        custom_attribute_type_token _type;
        core::size_type             _type_raw;
        blob                        _value;
    };

    class realized_decl_security_row
    {
    public:

        realized_decl_security_row(decl_security_row const&);

    private:

        std::uint16_t           _action;
        has_decl_security_token _parent;
        core::size_type         _parent_raw;
        blob                    _permission_set;
    };

    class realized_event_map_row
    {
    public:

        realized_event_map_row(event_map_row const&);

    private:

        type_def_token _parent;
        event_token    _first_event;
        event_token    _last_event;
    };

    class realized_event_row
    {
    public:

        realized_event_row(event_row const&);

    private:

        event_flags             _flags;
        core::string_reference  _name;
        type_def_ref_spec_token _type;
        core::size_type         _type_raw;
    };

    class realized_exported_type_row
    {
    public:

        realized_exported_type_row(exported_type_row const&);

    private:

        type_flags             _flags;
        std::uint32_t          _type_def_id;
        core::string_reference _name;
        core::string_reference _namespace_name;
        implementation_token   _implementation;
        core::size_type        _implementation_raw;
    };

    class realized_field_row
    {
    public:

        realized_field_row(field_row const&);

    private:

        field_flags            _flags;
        core::string_reference _name;
        blob                   _signature;
    };

    class realized_field_layout_row
    {
    public:

        realized_field_layout_row(field_layout_row const&);

    private:

        core::size_type _offset;
        field_token     _parent;
    };

    class realized_field_marshal_row
    {
    public:

        realized_field_marshal_row(field_marshal_row const&);

    private:

        has_field_marshal_token _parent;
        core::size_type         _parent_raw;
        blob                    _native_type;
    };

    class realized_field_rva_row
    {
    public:

        realized_field_rva_row(field_rva_row const&);

    private:

        core::size_type _rva;
        field_token     _parent;
    };

    class realized_file_row
    {
    public:

        realized_file_row(file_row const&);

    private:

        file_flags             _flags;
        core::string_reference _name;
        blob                   _hash_value;
    };

    class realized_generic_param_row
    {
    public:

        realized_generic_param_row(generic_param_row const&);

    private:

        std::uint16_t            _sequence;
        generic_parameter_flags  _flags;
        type_or_method_def_token _parent;
        core::size_type          _parent_raw;
        core::string_reference   _name;
    };

    class realized_generic_param_constraint_row
    {
    public:

        realized_generic_param_constraint_row(generic_param_constraint_row const&);

    private:

        generic_param_token     _parent;
        type_def_ref_spec_token _constraint;
        core::size_type         _constraint_raw;
    };

    class realized_impl_map_row
    {
    public:

        realized_impl_map_row(impl_map_row const&);

    private:

        pinvoke_flags          _flags;
        member_forwarded_token _member_forwarded;
        core::size_type        _member_forwarded_raw;
        core::string_reference _import_name;
        module_ref_token       _import_scope;
    };

    class realized_interface_impl_row
    {
    public:

        realized_interface_impl_row(interface_impl_row const&);

    private:

        type_def_token          _parent;
        type_def_ref_spec_token _interface;
        core::size_type         _interface_raw;
    };

    class realized_manifest_resource_row
    {
    public:

        realized_manifest_resource_row(manifest_resource_row const&);

    private:

        core::size_type         _offset;
        manifest_resource_flags _flags;
        core::string_reference  _name;
        implementation_token    _implementation;
        core::size_type         _implementation_raw;
    };

    class realized_member_ref_row
    {
    public:

        realized_member_ref_row(member_ref_row const&);

    private:

        member_ref_parent_token _parent;
        core::size_type         _parent_raw;
        core::string_reference  _name;
        blob                    _signature;
    };

    class realized_method_def_row
    {
    public:

        realized_method_def_row(method_def_row const&);

    private:

        core::size_type             _rva;
        method_implementation_flags _implementation_flags;
        method_flags                _flags;
        core::string_reference      _name;
        blob                        _signature;
        param_token                 _first_parameter;
        param_token                 _last_parameter;
    };

    class realized_method_impl_row
    {
    public:

        realized_method_impl_row(method_impl_row const&);

    private:

        type_def_token          _parent;
        method_def_or_ref_token _method_body;
        core::size_type         _method_body_raw;
        method_def_or_ref_token _method_declaration;
        core::size_type         _method_declaration_raw;
    };

    class realized_method_semantics_row
    {
    public:

        realized_method_semantics_row(method_semantics_row const&);

    private:

        method_semantics_flags _semantics;
        method_def_token       _method;
        has_semantics_token    _parent;
        core::size_type        _parent_raw;
    };

    class realized_method_spec_row
    {
    public:

        realized_method_spec_row(method_spec_row const&);

    private:

        method_def_or_ref_token _method;
        core::size_type         _method_raw;
        blob                    _signature;
    };

    class realized_module_row
    {
    public:

        realized_module_row(module_row const&);

    private:

        core::string_reference _name;
        blob                   _mvid;
    };

    class realized_module_ref_row
    {
    public:

        realized_module_ref_row(module_ref_row const&);

    private:

        core::string_reference _name;
    };

    class realized_nested_class_row
    {
    public:

        realized_nested_class_row(nested_class_row const&);

    private:

        type_def_token _nested_class;
        type_def_token _enclosing_class;
    };

    class realized_param_row
    {
    public:

        realized_param_row(param_row const&);

    private:

        parameter_flags        _flags;
        std::uint16_t          _sequence;
        core::string_reference _name;
    };

    class realized_property_row
    {
    public:

        realized_property_row(property_row const&);

    private:

        property_flags         _flags;
        core::string_reference _name;
        blob                   _signature;
    };

    class realized_property_map_row
    {
    public:

        realized_property_map_row(property_map_row const&);

    private:

        type_def_token _parent;
        property_token _first_property;
        property_token _last_property;
    };

    class realized_standalone_sig_row
    {
    public:

        realized_standalone_sig_row(standalone_sig_row const&);

    private:

        blob _signature;
    };

    class realized_type_def_row
    {
    public:

        realized_type_def_row(type_def_row const&);

    private:

        type_flags              _flags;
        core::string_reference  _name;
        core::string_reference  _namespace_name;
        type_def_ref_spec_token _extends;
        core::size_type         _extends_raw;

        field_token             _first_field;
        field_token             _last_field;

        method_def_token        _first_method;
        method_def_token        _last_method;
    };

    class realized_type_ref_row
    {
    public:

        realized_type_ref_row(type_ref_row const&);

    private:

        resolution_scope_token _resolution_scope;
        core::size_type        _resolution_scope_raw;
        core::string_reference _name;
        core::string_reference _namespace_name;
    };

    class realized_type_spec_row
    {
    public:

        realized_type_spec_row(type_spec_row const&);

    private:

        blob _signature;
    };





    auto insert_into_stream(core::base_wostream_wrapper&, assembly_attribute             ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, assembly_hash_algorithm        ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, binding_attribute              ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, calling_convention             ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, event_attribute                ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, field_attribute                ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, file_attribute                 ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, generic_parameter_attribute    ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, manifest_resource_attribute    ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, method_attribute               ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, method_implementation_attribute) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, method_semantics_attribute     ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, parameter_attribute            ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, pinvoke_attribute              ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, property_attribute             ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, signature_attribute            ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, type_attribute                 ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, element_type                   ) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, table_id                       ) -> void;





    auto insert_into_stream(core::base_wostream_wrapper&, unrestricted_token) -> void;





    auto insert_into_stream(core::base_wostream_wrapper&, array_shape        const&) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, custom_modifier    const&) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, field_signature    const&) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, method_signature   const&) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, property_signature const&) -> void;
    auto insert_into_stream(core::base_wostream_wrapper&, type_signature     const&) -> void;





    template <typename T>
    struct is_restricted_token
    {
        enum { value = false };
    };

    template <table_mask M, bool B>
    struct is_restricted_token<restricted_token<M, B>>
    {
        enum { value = true };
    };





    /// Tests if `T` is a `metadata`-namespace type for which `insert_into_stream` may be called
    ///
    /// This allows us to share the same `operator<<` operator overload (defined below) and work
    /// around the fact that we cannot use SFINAE to reject an overload due to a failure in a nested
    /// decltype.
    template <typename T>
    struct is_metadata_type
    {
        enum
        {
            value = std::is_same<T, assembly_attribute             >::value
                 || std::is_same<T, assembly_hash_algorithm        >::value
                 || std::is_same<T, binding_attribute              >::value
                 || std::is_same<T, calling_convention             >::value
                 || std::is_same<T, event_attribute                >::value
                 || std::is_same<T, field_attribute                >::value
                 || std::is_same<T, file_attribute                 >::value
                 || std::is_same<T, generic_parameter_attribute    >::value
                 || std::is_same<T, manifest_resource_attribute    >::value
                 || std::is_same<T, method_attribute               >::value
                 || std::is_same<T, method_implementation_attribute>::value
                 || std::is_same<T, method_semantics_attribute     >::value
                 || std::is_same<T, parameter_attribute            >::value
                 || std::is_same<T, pinvoke_attribute              >::value
                 || std::is_same<T, property_attribute             >::value
                 || std::is_same<T, signature_attribute            >::value
                 || std::is_same<T, type_attribute                 >::value
                 || std::is_same<T, element_type                   >::value
                 || std::is_same<T, table_id                       >::value

                 || std::is_same<T, array_shape                    >::value
                 || std::is_same<T, custom_modifier                >::value
                 || std::is_same<T, field_signature                >::value
                 || std::is_same<T, method_signature               >::value
                 || std::is_same<T, property_signature             >::value
                 || std::is_same<T, type_signature                 >::value

                 || is_restricted_token<T>::value
        };
    };





    template <typename Value>
    auto to_string(Value const& v) -> core::string
    {
        std::wostringstream os;
        core::wostream_wrapper<std::wostringstream> wrapped_os(&os);
        insert_into_stream(wrapped_os, v);
        return os.str();
    }

} } }

namespace cxxreflect { namespace metadata {

    template <typename OutputStream, typename Value>
    auto operator<<(OutputStream& os, Value const& v)
        -> typename std::enable_if<debug::is_metadata_type<Value>::value, OutputStream&>::type
    {
        core::wostream_wrapper<OutputStream> wrapped_os(&os);
        debug::insert_into_stream(wrapped_os, v);
        return os;
    }

} }

#endif
