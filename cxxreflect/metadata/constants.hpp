
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_CONSTANTS_HPP_
#define CXXREFLECT_METADATA_CONSTANTS_HPP_

#include "cxxreflect/core/core.hpp"

namespace cxxreflect { namespace metadata {

    /// \defgroup cxxreflect_metadata_constants Metadata -> Constants
    ///
    /// Constants, enumerations, and related functions and metafunctions used by the metadata
    /// database and metadata signature parsing libraries.
    ///
    /// @{





    /// The underlying integer type of the `table_id` enumeration
    typedef core::byte integer_table_id;


    /// Identifiers for each of the tables in the metadata database
    ///
    /// The enumerator values match those specified in ECMA 335-2010 II.22, which contains the
    /// specification for the metadata logical format.
    enum class table_id : integer_table_id
    {
        module                   = 0x00,
        type_ref                 = 0x01,
        type_def                 = 0x02,
        field                    = 0x04,
        method_def               = 0x06,
        param                    = 0x08,
        interface_impl           = 0x09,
        member_ref               = 0x0a,
        constant                 = 0x0b,
        custom_attribute         = 0x0c,
        field_marshal            = 0x0d,
        decl_security            = 0x0e,
        class_layout             = 0x0f,
        field_layout             = 0x10,
        standalone_sig           = 0x11,
        event_map                = 0x12,
        event                    = 0x14,
        property_map             = 0x15,
        property                 = 0x17,
        method_semantics         = 0x18,
        method_impl              = 0x19,
        module_ref               = 0x1a,
        type_spec                = 0x1b,
        impl_map                 = 0x1c,
        field_rva                = 0x1d,
        assembly                 = 0x20,
        assembly_processor       = 0x21,
        assembly_os              = 0x22,
        assembly_ref             = 0x23,
        assembly_ref_processor   = 0x24,
        assembly_ref_os          = 0x25,
        file                     = 0x26,
        exported_type            = 0x27,
        manifest_resource        = 0x28,
        nested_class             = 0x29,
        generic_param            = 0x2a,
        method_spec              = 0x2b,
        generic_param_constraint = 0x2c
    };

    enum : core::size_type
    {
        /// An integer value one larger than the largest table identifier
        ///
        /// Note that this lies; it is not actually the count of the table identifiers because 
        /// there are some unassigned values that form holes in the list of table identifiers
        /// (e.g., there is no table that maps to the value 5).
        table_id_count = 0x2d
    };

    /// An invalid table identifier value, for use as a sentinel
    ///
    /// This object is not defined; you may not take its address.
    table_id const invalid_table_id(static_cast<table_id>(-1));


    /// An array of `size_type` objects of sufficient size that each table id is a valid array index
    typedef std::array<core::size_type, table_id_count> table_id_size_array;


    /// Tests whether `table` is actually a valid table identifier
    ///
    /// Given an arbitrary unsigned integer `table`, this function tests whether it maps to a
    /// valid table identifier.
    auto is_valid_table_id(core::size_type table) -> bool;


    /// Tests whether `table` is actually a valid table identifier
    ///
    /// Some functions may return a sentinel value (like `invalid_table_id`), which is not a valid
    /// table identifier.  This function is used to detect that situation.
    auto is_valid_table_id(table_id table) -> bool;





    template <table_id Id>
    struct integer_value_of_table_id
    {
        enum : std::underlying_type<table_id>::type
        {
            value = static_cast<std::underlying_type<table_id>::type>(Id)
        };
    };





    /// The underlying integer type of the `table_mask` enumeration
    typedef std::uint64_t integer_table_mask;


    /// Masks for each of the tables in the metadata database
    ///
    /// There is a 1:1 correspondence between table identifiers and table masks.  The mask for each
    /// table has a single bit set; the bit is `1 << id`, where `id` is the identifier of the table.
    enum class table_mask : integer_table_mask
    {
        module                   = 1ull << static_cast<integer_table_mask>(table_id::module                  ),
        type_ref                 = 1ull << static_cast<integer_table_mask>(table_id::type_ref                ),
        type_def                 = 1ull << static_cast<integer_table_mask>(table_id::type_def                ),
        field                    = 1ull << static_cast<integer_table_mask>(table_id::field                   ),
        method_def               = 1ull << static_cast<integer_table_mask>(table_id::method_def              ),
        param                    = 1ull << static_cast<integer_table_mask>(table_id::param                   ),
        interface_impl           = 1ull << static_cast<integer_table_mask>(table_id::interface_impl          ),
        member_ref               = 1ull << static_cast<integer_table_mask>(table_id::member_ref              ),
        constant                 = 1ull << static_cast<integer_table_mask>(table_id::constant                ),
        custom_attribute         = 1ull << static_cast<integer_table_mask>(table_id::custom_attribute        ),
        field_marshal            = 1ull << static_cast<integer_table_mask>(table_id::field_marshal           ),
        decl_security            = 1ull << static_cast<integer_table_mask>(table_id::decl_security           ),
        class_layout             = 1ull << static_cast<integer_table_mask>(table_id::class_layout            ),
        field_layout             = 1ull << static_cast<integer_table_mask>(table_id::field_layout            ),
        standalone_sig           = 1ull << static_cast<integer_table_mask>(table_id::standalone_sig          ),
        event_map                = 1ull << static_cast<integer_table_mask>(table_id::event_map               ),
        event                    = 1ull << static_cast<integer_table_mask>(table_id::event                   ),
        property_map             = 1ull << static_cast<integer_table_mask>(table_id::property_map            ),
        property                 = 1ull << static_cast<integer_table_mask>(table_id::property                ),
        method_semantics         = 1ull << static_cast<integer_table_mask>(table_id::method_semantics        ),
        method_impl              = 1ull << static_cast<integer_table_mask>(table_id::method_impl             ),
        module_ref               = 1ull << static_cast<integer_table_mask>(table_id::module_ref              ),
        type_spec                = 1ull << static_cast<integer_table_mask>(table_id::type_spec               ),
        impl_map                 = 1ull << static_cast<integer_table_mask>(table_id::impl_map                ),
        field_rva                = 1ull << static_cast<integer_table_mask>(table_id::field_rva               ),
        assembly                 = 1ull << static_cast<integer_table_mask>(table_id::assembly                ),
        assembly_processor       = 1ull << static_cast<integer_table_mask>(table_id::assembly_processor      ),
        assembly_os              = 1ull << static_cast<integer_table_mask>(table_id::assembly_os             ),
        assembly_ref             = 1ull << static_cast<integer_table_mask>(table_id::assembly_ref            ),
        assembly_ref_processor   = 1ull << static_cast<integer_table_mask>(table_id::assembly_ref_processor  ),
        assembly_ref_os          = 1ull << static_cast<integer_table_mask>(table_id::assembly_ref_os         ),
        file                     = 1ull << static_cast<integer_table_mask>(table_id::file                    ),
        exported_type            = 1ull << static_cast<integer_table_mask>(table_id::exported_type           ),
        manifest_resource        = 1ull << static_cast<integer_table_mask>(table_id::manifest_resource       ),
        nested_class             = 1ull << static_cast<integer_table_mask>(table_id::nested_class            ),
        generic_param            = 1ull << static_cast<integer_table_mask>(table_id::generic_param           ),
        method_spec              = 1ull << static_cast<integer_table_mask>(table_id::method_spec             ),
        generic_param_constraint = 1ull << static_cast<integer_table_mask>(table_id::generic_param_constraint)
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(table_mask)

    typedef core::flags<table_mask> table_mask_flags;


    /// Computes the mask bit for a given table identifier
    auto table_mask_for(table_id table) -> table_mask;


    /// Computes the table identifier from a given mask bit
    auto table_id_for(table_mask mask) -> table_id;




    template <table_mask Id>
    struct integer_value_of_mask
    {
        enum : std::underlying_type<table_mask>::type
        {
            value = static_cast<std::underlying_type<table_mask>::type>(Id)
        };
    };





    typedef core::byte      composite_index_integer;
    typedef core::size_type composite_index_key;

    /// Identifiers for each of the composite indices used in a metadata database
    ///
    /// A composite index is used when a field may refer to a row in one of several possible tables.
    /// The enumerator values match those specified in ECMA 335-2010 II.24.2.6, which contains the
    /// specification for each of the composite indices.
    enum class composite_index
    {
        type_def_ref_spec      = 0x00,
        has_constant           = 0x01,
        has_custom_attribute   = 0x02,
        has_field_marshal      = 0x03,
        has_decl_security      = 0x04,
        member_ref_parent      = 0x05,
        has_semantics          = 0x06,
        method_def_or_ref      = 0x07,
        member_forwarded       = 0x08,
        implementation         = 0x09,
        custom_attribute_type  = 0x0a,
        resolution_scope       = 0x0b,
        type_or_method_def     = 0x0c
    };


    enum : core::size_type
    {
        /// A value one larger than the largest `CompositeIndex` enumerator value
        ///
        /// This is also the number of `CompositeIndex` enumerators because there are no unused
        /// values (holes) in the enumeration.
        composite_index_count = 0x0d,
    };


    /// An array of `size_type` objects of sufficient size that each index id is a valid array indexs
    typedef std::array<core::size_type, composite_index_count> composite_index_size_array;


    /// Tests whether `index` is actually a valid composite index identifier
    ///
    /// Given an arbitrary unsigned integer `index`, this function tests whether it maps to a
    /// valid composite index identifier.
    auto is_valid_composite_index(core::size_type index) -> bool;


    /// Tests whether `index` is actually a valid composite index identifier
    auto is_valid_composite_index(composite_index index) -> bool;


    /// Converts an index key to the identifier of the table it represents
    auto table_id_for(composite_index_key key, composite_index index) -> table_id;
    
    
    /// Converts a table identifier to the key that represents it in the specified index
    auto index_key_for(table_id table, composite_index index) -> composite_index_key;





    /// Column identifiers for each column in each database table
    ///
    /// The columns of different tables are unrelated, so there is only loose type checking when
    /// using this enumeration type.  It's really just a collection of constants to avoid repeating
    /// the column numbers all over the place.
    enum class column_id : core::size_type
    {
        assembly_hash_algorithm = 0,
        assembly_version        = 1,
        assembly_flags          = 2,
        assembly_public_key     = 3,
        assembly_name           = 4,
        assembly_culture        = 5,

        assembly_os_platform_id   = 0,
        assembly_os_major_version = 1,
        assembly_os_minor_version = 2,

        assembly_processor_processor = 0,

        assembly_ref_version     = 0,
        assembly_ref_flags       = 1,
        assembly_ref_public_key  = 2,
        assembly_ref_name        = 3,
        assembly_ref_culture     = 4,
        assembly_ref_hash_value  = 5,

        assembly_ref_os_platform_id   = 0,
        assembly_ref_os_major_version = 1,
        assembly_ref_os_minor_version = 2,
        assembly_ref_os_parent        = 3,

        assembly_ref_processor_processor = 0,
        assembly_ref_processor_parent    = 1,

        class_layout_packing_size = 0,
        class_layout_class_size   = 1,
        class_layout_parent       = 2,

        constant_type   = 0,
        constant_parent = 1,
        constant_value  = 2,

        custom_attribute_parent = 0,
        custom_attribute_type   = 1,
        custom_attribute_value  = 2,

        decl_security_action         = 0,
        decl_security_parent         = 1,
        decl_security_permission_set = 2,

        event_map_parent      = 0,
        event_map_first_event = 1,

        event_flags = 0,
        event_name  = 1,
        event_type  = 2,

        exported_type_flags          = 0,
        exported_type_type_def_id    = 1,
        exported_type_name           = 2,
        exported_type_namespace_name = 3,
        exported_type_implementation = 4,

        field_flags     = 0,
        field_name      = 1,
        field_signature = 2,

        field_layout_offset = 0,
        field_layout_parent = 1,

        field_marshal_parent      = 0,
        field_marshal_native_type = 1,

        field_rva_rva    = 0,
        field_rva_parent = 1,

        file_flags      = 0,
        file_name       = 1,
        file_hash_value = 2,

        generic_param_sequence = 0,
        generic_param_flags    = 1,
        generic_param_parent   = 2,
        generic_param_name     = 3,

        generic_param_constraint_parent     = 0,
        generic_param_constraint_constraint = 1,
        
        impl_map_flags            = 0,
        impl_map_member_forwarded = 1,
        impl_map_import_name      = 2,
        impl_map_import_scope     = 3,

        interface_impl_parent    = 0,
        interface_impl_interface = 1,

        manifest_resource_offset         = 0,
        manifest_resource_flags          = 1,
        manifest_resource_name           = 2,
        manifest_resource_implementation = 3,

        member_ref_parent    = 0,
        member_ref_name      = 1,
        member_ref_signature = 2,

        method_def_rva                  = 0,
        method_def_implementation_flags = 1,
        method_def_flags                = 2,
        method_def_name                 = 3,
        method_def_signature            = 4,
        method_def_first_parameter      = 5,

        method_impl_parent             = 0,
        method_impl_method_body        = 1,
        method_impl_method_declaration = 2,

        method_semantics_semantics = 0,
        method_semantics_method    = 1,
        method_semantics_parent    = 2,

        method_spec_method    = 0,
        method_spec_signature = 1,

        module_name = 1,
        module_mvid = 2,

        module_ref_name = 0,

        nested_class_nested_class    = 0,
        nested_class_enclosing_class = 1,

        param_flags    = 0,
        param_sequence = 1,
        param_name     = 2,

        property_flags     = 0,
        property_name      = 1,
        property_signature = 2,

        property_map_parent         = 0,
        property_map_first_property = 1,

        standalone_sig_signature = 0,

        type_def_flags          = 0,
        type_def_name           = 1,
        type_def_namespace_name = 2,
        type_def_extends        = 3,
        type_def_first_field    = 4,
        type_def_first_method   = 5,

        type_ref_resolution_scope = 0,
        type_ref_name             = 1,
        type_ref_namespace_name   = 2,

        type_spec_signature = 0
    };





    enum class assembly_attribute : std::uint32_t
    {
        public_key                    = 0x0001,
        retargetable                  = 0x0100,
        disable_jit_compile_optimizer = 0x4000,
        enable_jit_compile_tracking   = 0x8000,

        /// A mask for the content type flags of the Assembly flags
        ///
        /// The content type flags are not part of ECMA 335-2010.  They are used to differentiate
        /// between Windows Runtime metadata and ordinary CLI metadata.  The flags are found in the
        /// Windows 8.0 SDK header corhdr.h, in the CorAssemblyFlags enumeration.
        content_type_mask             = 0x0E00,

        /// The "default" content type, used for ordinary CLI metadata
        default_content_type          = 0x0000,

        /// The Windows Runtime content type, used for Windows Runtime metadata (.winmd files)
        windows_runtime_content_type  = 0x0200
        
    };


    enum class assembly_hash_algorithm : std::uint32_t
    {
        none     = 0x0000,
        md5      = 0x8003,
        sha1     = 0x8004
    };


    // The subset of System.Reflection.BindingFlags that are useful for reflection-only
    enum class binding_attribute : std::uint32_t
    {
        default_                       = 0x00000000,
        ignore_case                    = 0x00000001,
        declared_only                  = 0x00000002,
        instance                       = 0x00000004,
        static_                        = 0x00000008,
        public_                        = 0x00000010,
        non_public                     = 0x00000020,
        flatten_hierarchy              = 0x00000040,

        all_instance                   = instance | public_ | non_public,
        all_static                     = static_  | public_ | non_public,

        internal_use_only_mask         = 0x10000000,
        internal_use_only_constructor  = 0x10000001
    };


    enum class calling_convention : std::uint8_t
    {
        standard      = 0x00,
        varargs       = 0x05,
        has_this      = 0x20,
        explicit_this = 0x40
    };


    enum class event_attribute : std::uint16_t
    {
        special_name         = 0x0200,
        runtime_special_name = 0x0400
    };


    enum class field_attribute : std::uint16_t
    {
        field_access_mask    = 0x0007,
        member_access_mask   = 0x0007,

        compiler_controlled  = 0x0000,
        private_             = 0x0001,
        family_and_assembly  = 0x0002,
        assembly             = 0x0003,
        family               = 0x0004,
        family_or_assembly   = 0x0005,
        public_              = 0x0006,

        static_              = 0x0010,
        init_only            = 0x0020,
        literal              = 0x0040,
        not_serialized       = 0x0080,
        special_name         = 0x0200,

        pinvoke_impl         = 0x2000,

        runtime_special_name = 0x0400,
        has_field_marshal    = 0x1000,
        has_default          = 0x8000,
        has_field_rva        = 0x0100
    };


    enum class file_attribute : std::uint32_t
    {
        contains_metadata    = 0x0000,
        contains_no_metadata = 0x0001
    };


    enum class generic_parameter_attribute : std::uint16_t
    {
        variance_mask                      = 0x0003,
        none                               = 0x0000,
        covariant                          = 0x0001,
        contravariant                      = 0x0002,

        special_constraint_mask            = 0x001c,
        reference_type_constraint          = 0x0004,
        non_nullable_value_type_constraint = 0x0008,
        default_constructor_constraint     = 0x0010
    };


    enum class manifest_resource_attribute : std::uint32_t
    {
        visibility_mask = 0x0007,
        public_         = 0x0001,
        private_        = 0x0002
    };


    enum class method_attribute : std::uint16_t
    {
        member_access_mask      = 0x0007,
        compiler_controlled     = 0x0000,
        private_                = 0x0001,
        family_and_assembly     = 0x0002,
        assembly                = 0x0003,
        family                  = 0x0004,
        family_or_assembly      = 0x0005,
        public_                 = 0x0006,

        static_                 = 0x0010,
        final                   = 0x0020,
        virtual_                = 0x0040,
        hide_by_sig             = 0x0080,

        vtable_layout_mask      = 0x0100,
        reuse_slot              = 0x0000,
        new_slot                = 0x0100,

        strict                  = 0x0200,
        abstract                = 0x0400,
        special_name            = 0x0800,

        pinvoke_impl            = 0x2000,
        runtime_special_name    = 0x1000,
        has_security            = 0x4000,
        require_security_object = 0x8000
    };


    enum class method_implementation_attribute : std::uint16_t
    {
        code_type_mask   = 0x0003,
        il               = 0x0000,
        native           = 0x0001,
        runtime          = 0x0003,

        managed_mask     = 0x0004,
        unmanaged        = 0x0004,
        managed          = 0x0000,

        forward_ref      = 0x0010,
        preserve_sig     = 0x0080,
        internal_call    = 0x1000,
        synchronized     = 0x0020,
        no_inlining      = 0x0008,
        no_optimization  = 0x0040
    };


    enum class method_semantics_attribute : std::uint16_t
    {
        setter    = 0x0001,
        getter    = 0x0002,
        other     = 0x0004,
        add_on    = 0x0008,
        remove_on = 0x0010,
        fire      = 0x0020
    };


    enum class parameter_attribute : std::uint16_t
    {
        in                = 0x0001,
        out               = 0x0002,
        optional          = 0x0010,
        has_default       = 0x1000,
        has_field_marshal = 0x2000
    };


    enum class pinvoke_attribute : std::uint16_t
    {
        no_mangle                        = 0x0001,

        character_set_mask               = 0x0006,
        character_set_mask_not_specified = 0x0000,
        character_set_mask_ansi          = 0x0002,
        character_set_mask_unicode       = 0x0004,
        character_set_mask_auto          = 0x0006,

        supports_last_error              = 0x0040,

        calling_convention_mask          = 0x0700,
        calling_convention_platform_api  = 0x0100,
        calling_convention_cdecl         = 0x0200,
        calling_convention_stdcall       = 0x0300,
        calling_convention_thiscall      = 0x0400,
        calling_convention_fastcall      = 0x0500
    };


    enum class property_attribute : std::uint16_t
    {
        special_name         = 0x0200,
        runtime_special_name = 0x0400,
        has_default          = 0x1000
    };


    enum class signature_attribute : std::uint8_t
    {
        has_this                    = 0x20,
        explicit_this               = 0x40,

        calling_convention_mask     = 0x0f,
        calling_convention_default  = 0x00,
        calling_convention_cdecl    = 0x01,
        calling_convention_stdcall  = 0x02,
        calling_convention_thiscall = 0x03,
        calling_convention_fastcall = 0x04,
        calling_convention_varargs  = 0x05,

        field                       = 0x06,
        local                       = 0x07,
        property_                   = 0x08,

        generic_                    = 0x10,

        sentinel                    = 0x41
    };


    enum class type_attribute : std::uint32_t
    {
        visibility_mask            = 0x00000007,
        not_public                 = 0x00000000,
        public_                    = 0x00000001,
        nested_public              = 0x00000002,
        nested_private             = 0x00000003,
        nested_family              = 0x00000004,
        nested_assembly            = 0x00000005,
        nested_family_and_assembly = 0x00000006,
        nested_family_or_assembly  = 0x00000007,

        layout_mask                = 0x00000018,
        auto_layout                = 0x00000000,
        sequential_layout          = 0x00000008,
        explicit_layout            = 0x00000010,

        class_semantics_mask       = 0x00000020,
        class_                     = 0x00000000,
        interface                  = 0x00000020,

        abstract_                  = 0x00000080,
        sealed                     = 0x00000100,
        special_name               = 0x00000400,

        import                     = 0x00001000,
        serializable               = 0x00002000,

        string_format_mask         = 0x00030000,
        ansi_class                 = 0x00000000,
        unicode_class              = 0x00010000,
        auto_class                 = 0x00020000,
        custom_format_class        = 0x00030000,
        custom_string_format_mask  = 0x00c00000,

        before_field_init          = 0x00100000,

        runtime_special_name       = 0x00000800,
        has_security               = 0x00040000,
        is_type_forwarder          = 0x00200000
    };


    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(assembly_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(binding_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(event_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(field_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(file_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(generic_parameter_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(manifest_resource_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(method_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(method_implementation_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(method_semantics_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(parameter_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(pinvoke_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(property_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(signature_attribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(type_attribute)


    typedef core::flags<assembly_attribute>              assembly_flags;
    typedef core::flags<binding_attribute>               binding_flags;
    typedef core::flags<event_attribute>                 event_flags;
    typedef core::flags<field_attribute>                 field_flags;
    typedef core::flags<file_attribute>                  file_flags;
    typedef core::flags<generic_parameter_attribute>     generic_parameter_flags;
    typedef core::flags<manifest_resource_attribute>     manifest_resource_flags;
    typedef core::flags<method_attribute>                method_flags;
    typedef core::flags<method_implementation_attribute> method_implementation_flags;
    typedef core::flags<method_semantics_attribute>      method_semantics_flags;
    typedef core::flags<parameter_attribute>             parameter_flags;
    typedef core::flags<pinvoke_attribute>               pinvoke_flags;
    typedef core::flags<property_attribute>              property_flags;
    typedef core::flags<signature_attribute>             signature_flags;
    typedef core::flags<type_attribute>                  type_flags;





    /// Identifiers for each of the CLI element types
    enum class element_type : core::byte
    {
        end                           = 0x00,
        void_type                     = 0x01,
        boolean                       = 0x02,
        character                     = 0x03,
        i1                            = 0x04,
        u1                            = 0x05,
        i2                            = 0x06,
        u2                            = 0x07,
        i4                            = 0x08,
        u4                            = 0x09,
        i8                            = 0x0a,
        u8                            = 0x0b,
        r4                            = 0x0c,
        r8                            = 0x0d,
        string                        = 0x0e,
        ptr                           = 0x0f,
        by_ref                        = 0x10,
        value_type                    = 0x11,
        class_type                    = 0x12,
        var                           = 0x13,
        array                         = 0x14,
        generic_inst                  = 0x15,
        typed_by_ref                  = 0x16,

        i                             = 0x18,
        u                             = 0x19,
        fn_ptr                        = 0x1b,
        object                        = 0x1c,

        concrete_element_type_max     = 0x1d,

        sz_array                      = 0x1d,
        mvar                          = 0x1e,

        custom_modifier_required      = 0x1f,
        custom_modifier_optional      = 0x20,

        internal                      = 0x21,
        modifier                      = 0x40,
        sentinel                      = 0x41,
        pinned                        = 0x45,

        type                          = 0x50,
        custom_attribute_boxed_object = 0x51,
        custom_attribute_field        = 0x53,
        custom_attribute_property     = 0x54,
        custom_attribute_enum         = 0x55,

        /// For internal use only
        ///
        /// This is not a real ElementType and it will never be found in metadata read from a
        /// database.  This faux ElementType is used when a signature is instantiated with types
        /// that are defined in or referenced from a database other than the database in which the
        /// uninstantiated signature is located.
        ///
        /// The cross-module type reference is composed of both a TypeDefOrSpec and a pointer to the
        /// database in which it is to be resolved.
        cross_module_type_reference   = 0x5f,
    };


    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(element_type);





    auto is_valid_element_type           (core::byte   id  ) -> bool;
    auto is_type_element_type            (core::byte   id  ) -> bool;
    auto is_custom_modifier_element_type (core::byte   id  ) -> bool;
    auto is_integer_element_type         (element_type type) -> bool;
    auto is_signed_integer_element_type  (element_type type) -> bool;
    auto is_unsigned_integer_element_type(element_type type) -> bool;
    auto is_real_element_type            (element_type type) -> bool;
    auto is_numeric_element_type         (element_type type) -> bool;





    template <table_mask Mask> struct table_id_for_mask;
    template <table_mask Mask> struct row_type_for_mask;
    template <table_mask Mask> struct token_type_for_mask;

    template <table_id Id> struct mask_for_table_id;
    template <table_id Id> struct row_type_for_table_id;
    template <table_id Id> struct token_type_for_table_id; 

    template <typename Row> struct table_id_for_row_type;
    template <typename Row> struct mask_for_row_type;

    

    

    /// \cond CXXREFLECT_DOXYGEN_FALSE
    // This macro specializes each of the above metafunctions for a table type; we invoke it for
    // each table to generate all of the required specializations.

    template <table_mask Mask, bool WithArithmetic = false>
    class restricted_token;

    template <table_id Id>
    class row_iterator;

    #define CXXREFLECT_GENERATE(t)                                                      \
    class t ## _row;                                                                    \
                                                                                        \
    template <>                                                                         \
    struct table_id_for_row_type<t ## _row>                                             \
    {                                                                                   \
        static const table_id value = table_id::t;                                      \
    };                                                                                  \
                                                                                        \
    template <>                                                                         \
    struct table_id_for_mask<table_mask::t>                                             \
    {                                                                                   \
        static const table_id value = table_id::t;                                      \
    };                                                                                  \
                                                                                        \
    template <>                                                                         \
    struct mask_for_row_type<t ## _row>                                                 \
    {                                                                                   \
        static const table_mask value = table_mask::t;                                  \
    };                                                                                  \
                                                                                        \
    template <>                                                                         \
    struct mask_for_table_id<table_id::t>                                               \
    {                                                                                   \
        static const table_mask value = table_mask::t;                                  \
    };                                                                                  \
                                                                                        \
    template <>                                                                         \
    struct row_type_for_table_id<table_id::t>                                           \
    {                                                                                   \
        typedef t ## _row type;                                                         \
    };                                                                                  \
                                                                                        \
    template <>                                                                         \
    struct row_type_for_mask<table_mask::t>                                             \
    {                                                                                   \
        typedef t ## _row type;                                                         \
    };                                                                                  \
                                                                                        \
    template <>                                                                         \
    struct token_type_for_table_id<table_id::t>                                         \
    {                                                                                   \
        typedef restricted_token<mask_for_table_id<table_id::t>::value> type;           \
    };                                                                                  \
                                                                                        \
    template <>                                                                         \
    struct token_type_for_mask<table_mask::t>                                           \
    {                                                                                   \
        typedef restricted_token<table_mask::t> type;                                   \
    };                                                                                  \
                                                                                        \
    typedef restricted_token<table_mask::t> t ## _token;                                \
                                                                                        \
    typedef row_iterator<table_id::t> t ## _row_iterator;                               \
    typedef std::pair<t ## _row_iterator, t ## _row_iterator> t ## _row_iterator_pair

    CXXREFLECT_GENERATE(assembly                );
    CXXREFLECT_GENERATE(assembly_os             );
    CXXREFLECT_GENERATE(assembly_processor      );
    CXXREFLECT_GENERATE(assembly_ref            );
    CXXREFLECT_GENERATE(assembly_ref_os         );
    CXXREFLECT_GENERATE(assembly_ref_processor  );
    CXXREFLECT_GENERATE(class_layout            );
    CXXREFLECT_GENERATE(constant                );
    CXXREFLECT_GENERATE(custom_attribute        );
    CXXREFLECT_GENERATE(decl_security           );
    CXXREFLECT_GENERATE(event_map               );
    CXXREFLECT_GENERATE(event                   );
    CXXREFLECT_GENERATE(exported_type           );
    CXXREFLECT_GENERATE(field                   );
    CXXREFLECT_GENERATE(field_layout            );
    CXXREFLECT_GENERATE(field_marshal           );
    CXXREFLECT_GENERATE(field_rva               );
    CXXREFLECT_GENERATE(file                    );
    CXXREFLECT_GENERATE(generic_param           );
    CXXREFLECT_GENERATE(generic_param_constraint);
    CXXREFLECT_GENERATE(impl_map                );
    CXXREFLECT_GENERATE(interface_impl          );
    CXXREFLECT_GENERATE(manifest_resource       );
    CXXREFLECT_GENERATE(member_ref              );
    CXXREFLECT_GENERATE(method_def              );
    CXXREFLECT_GENERATE(method_impl             );
    CXXREFLECT_GENERATE(method_semantics        );
    CXXREFLECT_GENERATE(method_spec             );
    CXXREFLECT_GENERATE(module                  );
    CXXREFLECT_GENERATE(module_ref              );
    CXXREFLECT_GENERATE(nested_class            );
    CXXREFLECT_GENERATE(param                   );
    CXXREFLECT_GENERATE(property                );
    CXXREFLECT_GENERATE(property_map            );
    CXXREFLECT_GENERATE(standalone_sig          );
    CXXREFLECT_GENERATE(type_def                );
    CXXREFLECT_GENERATE(type_ref                );
    CXXREFLECT_GENERATE(type_spec               );

    #undef CXXREFLECT_GENERATE

    /// \endcond





    /// @}

    using std::begin;
    using std::end;

} }

#endif

// AMDG //
