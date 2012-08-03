
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_ROWS_HPP_
#define CXXREFLECT_METADATA_ROWS_HPP_

#include "cxxreflect/metadata/database.hpp"

namespace cxxreflect { namespace metadata {

    /// \defgroup cxxreflect_metadata_rows Metadata -> Rows
    ///
    /// Row types for each metadata table that decode and provide access to the columns of each
    /// table.
    ///
    /// @{





    class database;

    /// Creates an instance of a row of type `Row` from the provided scope and data pointer
    ///
    /// This serves as a common constructor for all of the row types; if Visual C++ supported
    /// inherited constructors, we could use those instead, but alas, it does not support them.
    /// This function is the only way that a row type may be constructed.
    template <typename Row>
    auto create_row(database const* const scope, core::const_byte_iterator const data) -> Row
    {
        Row row;
        row.initialize(scope, data);
        return row;
    }

    


    
    /// Base row type from which all of the other row types are derived
    ///
    /// This type contains functionality common to all row types.  Note that it is not polymorphic;
    /// it exists solely for code reuse.
    template <table_id Id>
    class base_row
    {
    public:

        typedef typename token_type_for_table_id<Id>::type token_type;

        auto is_initialized() const -> bool
        {
            return _scope.get() != nullptr && _data.get() != nullptr;
        }

        auto scope() const -> database const&
        {
            core::assert_initialized(*this);
            return *_scope.get();
        }

        auto token() const -> token_type
        {
            core::assert_initialized(*this);

            database_table  const& table(scope().tables()[Id]);
            core::size_type const  index(core::convert_integer((iterator() - table.begin()) / table.row_size()));
            return token_type(&scope(), Id, index);
        }

    protected:

        ~base_row() { }

        auto iterator() const -> core::const_byte_iterator
        {
            core::assert_initialized(*this);
            return _data.get();
        }

        auto column_offset(column_id const column) const -> core::size_type
        {
            core::assert_initialized(*this);
            return scope().tables().table_column_offset(Id, column);
        }

    private:

        template <typename Row>
        friend auto create_row(database const*, core::const_byte_iterator) -> Row;

        auto initialize(database const* const scope, core::const_byte_iterator const data) -> void
        {
            core::assert_not_null(scope);
            core::assert_not_null(data);
            core::assert_true([&]{ return !this->is_initialized(); });

            _scope.get() = scope;
            _data.get()  = data;
        }

        core::value_initialized<database const*>           _scope;
        core::value_initialized<core::const_byte_iterator> _data;
    };





    /// Represents a row in the **Assembly** table (ECMA 335-2010 II.22.2)
    class assembly_row : public base_row<table_id::assembly>
    {
    public:

        auto hash_algorithm() const -> assembly_hash_algorithm;
        auto version()        const -> four_component_version;
        auto flags()          const -> assembly_flags;
        auto public_key()     const -> blob;
        auto name()           const -> core::string_reference;
        auto culture()        const -> core::string_reference;
    };

    /// Represents a row in the **AssemblyOS** table (ECMA 335-2010 II.22.3)
    class assembly_os_row : public base_row<table_id::assembly_os>
    {
    public:

        auto platform_id()   const -> std::uint32_t;
        auto major_version() const -> std::uint32_t;
        auto minor_version() const -> std::uint32_t;
    };

    /// Represents a row in the **AssemblyProcessor** table (ECMA 335-2010 II.22.4)
    class assembly_processor_row : public base_row<table_id::assembly_processor>
    {
    public:

        auto processor() const -> std::uint32_t;
    };

    /// Represents a row in the **AssemblyRef** table (ECMA 335-2010 II.22.5)
    class assembly_ref_row : public base_row<table_id::assembly_ref>
    {
    public:

        auto version()    const -> four_component_version;
        auto flags()      const -> assembly_flags;
        auto public_key() const -> blob;
        auto name()       const -> core::string_reference;
        auto culture()    const -> core::string_reference;
        auto hash_value() const -> blob;
    };

    /// Represents a row in the **AssemblyRefOS** table (ECMA 335-2010 II.22.6)
    class assembly_ref_os_row : public base_row<table_id::assembly_ref_os>
    {
    public:

        auto platform_id()   const -> std::uint32_t;
        auto major_version() const -> std::uint32_t;
        auto minor_version() const -> std::uint32_t;
        auto parent()        const -> assembly_ref_token;
    };

    /// Represents a row in the **AssemblyRef** Processor table (ECMA 335-2010 II.22.7)
    class assembly_ref_processor_row : public base_row<table_id::assembly_ref_processor>
    {
    public:

        auto processor() const -> std::uint32_t;
        auto parent()    const -> assembly_ref_token;
    };

    /// Represents a row in the **ClassLayout** table (ECMA 335-2010 II.22.8)
    class class_layout_row : public base_row<table_id::class_layout>
    {
    public:

        auto packing_size() const -> std::uint16_t;
        auto class_size()   const -> std::uint32_t;
        auto parent()       const -> type_def_token;
    };

    /// Represents a row in the **Constant** table (ECMA 335-2010 II.22.9)
    class constant_row : public base_row<table_id::constant>
    {
    public:

        /// Gets the `ElementType` of the value pointed to by `GetValue()`
        ///
        /// In ECMA 335-2010, this is called the "Type" field.
        auto type()         const -> element_type;
        auto parent()       const -> has_constant_token;
        auto parent_raw()   const -> core::size_type;
        auto value()        const -> blob;
    };

    /// Represents a row in the **CustomAttribute** table (ECMA 335-2010 II.22.10)
    class custom_attribute_row : public base_row<table_id::custom_attribute>
    {
    public:

        auto parent()     const -> has_custom_attribute_token;
        auto parent_raw() const -> core::size_type;
        auto type()       const -> custom_attribute_type_token;
        auto type_raw()   const -> core::size_type;
        auto value()      const -> blob;
    };

    /// Represents a row in the **DeclSecurity** table (ECMA 335-2010 II.22.11)
    ///
    /// \note This column is referred to elsewhere as *PermissionSet* (e.g., see its value in the
    ///       **HasCustomAttribute** composite index.
    class decl_security_row : public base_row<table_id::decl_security>
    {
    public:

        auto action()         const -> std::uint16_t;
        auto parent()         const -> has_decl_security_token;
        auto parent_raw()     const -> core::size_type;
        auto permission_set() const -> blob;
    };

    /// Represents a row in the **EventMap** table (ECMA 335-2010 II.22.12)
    class event_map_row : public base_row<table_id::event_map>
    {
    public:

        auto parent()      const -> type_def_token;
        auto first_event() const -> event_token;
        auto last_event()  const -> event_token;
    };

    /// Represents a row in the **Event** table (ECMA 335-2010 II.22.13)
    class event_row : public base_row<table_id::event>
    {
    public:

        auto flags()    const -> event_flags;
        auto name()     const -> core::string_reference;
        auto type()     const -> type_def_ref_spec_token;
        auto type_raw() const -> core::size_type;
    };

    /// Represents a row in the **ExportedType** table (ECMA 335-2010 II.22.14)
    class exported_type_row : public base_row<table_id::exported_type>
    {
    public:

        auto flags()              const -> type_flags;
        auto type_def_id()        const -> std::uint32_t;
        auto name()               const -> core::string_reference;
        auto namespace_name()     const -> core::string_reference;
        auto implementation()     const -> implementation_token;
        auto implementation_raw() const -> core::size_type;
    };

    /// Represents a row in the **Field** table (ECMA 335-2010 II.22.15)
    class field_row : public base_row<table_id::field>
    {
    public:

        auto flags()     const -> field_flags;
        auto name()      const -> core::string_reference;
        auto signature() const -> blob;
    };

    /// Represents a row in the **FieldLayout** table (ECMA 335-2010 II.22.16)
    class field_layout_row : public base_row<table_id::field_layout>
    {
    public:

        auto offset() const -> core::size_type;
        auto parent() const -> field_token;
    };

    /// Represents a row in the **FieldMarshal** table (ECMA 335-2010 II.22.17)
    class field_marshal_row : public base_row<table_id::field_marshal>
    {
    public:

        auto parent()      const -> has_field_marshal_token;
        auto parent_raw()  const -> core::size_type;
        auto native_type() const -> blob;
    };

    /// Represents a row in the **FieldRVA** table (ECMA 335-2010 II.22.18)
    class field_rva_row : public base_row<table_id::field_rva>
    {
    public:

        auto rva()    const -> core::size_type;
        auto parent() const -> field_token;
    };

    /// Represents a row in the **File** table (ECMA 335-2010 II.22.19)
    class file_row : public base_row<table_id::file>
    {
    public:

        auto flags()      const -> file_flags;
        auto name()       const -> core::string_reference;
        auto hash_value() const -> blob;
    };

    /// Represents a row in the **GenericParam** table (ECMA 335-2010 II.22.20)
    class generic_param_row : public base_row<table_id::generic_param>
    {
    public:

        auto sequence()   const -> std::uint16_t;
        auto flags()      const -> generic_parameter_flags;
        auto parent()     const -> type_or_method_def_token;
        auto parent_raw() const -> core::size_type;
        auto name()       const -> core::string_reference;
    };

    /// Represents a row in the **GenericParamConstraint** table (ECMA 335-2010 II.22.21)
    class generic_param_constraint_row : public base_row<table_id::generic_param_constraint>
    {
    public:

        auto parent()         const -> generic_param_token;
        auto constraint()     const -> type_def_ref_spec_token;
        auto constraint_raw() const -> core::size_type;
    };

    /// Represents a row in the **ImplMap** table (ECMA 335-2010 II.22.22)
    class impl_map_row : public base_row<table_id::impl_map>
    {
    public:

        auto flags()                const -> pinvoke_flags;
        auto member_forwarded()     const -> member_forwarded_token;
        auto member_forwarded_raw() const -> core::size_type;
        auto import_name()          const -> core::string_reference;
        auto import_scope()         const -> module_ref_token;
    };

    /// Represents a row in the **InterfaceImpl** table (ECMA 335-2010 II.22.23)
    class interface_impl_row : public base_row<table_id::interface_impl>
    {
    public:

        auto parent()        const -> type_def_token;
        auto interface_()    const -> type_def_ref_spec_token;
        auto interface_raw() const -> core::size_type;
    };

    /// Represents a row in the **ManifestResource** table (ECMA 335-2010 II.22.24)
    class manifest_resource_row : public base_row<table_id::manifest_resource>
    {
    public:
        
        auto offset()             const -> core::size_type;
        auto flags()              const -> manifest_resource_flags;
        auto name()               const -> core::string_reference;
        auto implementation()     const -> implementation_token;
        auto implementation_raw() const -> core::size_type;
    };

    /// Represents a row in the **MemberRef** table (ECMA 335-2010 II.22.25)
    class member_ref_row : public base_row<table_id::member_ref>
    {
    public:

        auto parent()     const -> member_ref_parent_token;
        auto parent_raw() const -> core::size_type;
        auto name()       const -> core::string_reference;
        auto signature()  const -> blob;
    };

    /// Represents a row in the **MethodDef** table (ECMA 335-2010 II.22.26)
    class method_def_row : public base_row<table_id::method_def>
    {
    public:

        auto rva()                  const -> core::size_type;
        auto implementation_flags() const -> method_implementation_flags;
        auto flags()                const -> method_flags;
        auto name()                 const -> core::string_reference;
        auto signature()            const -> blob;

        auto first_parameter()      const -> param_token;
        auto last_parameter()       const -> param_token;
    };

    /// Represents a row in the **MethodImpl** table (ECMA 335-2010 II.22.27)
    class method_impl_row : public base_row<table_id::method_impl>
    {
    public:

        /// Gets a reference to the **TypeDef** that owns this **MethodImpl** row
        ///
        /// This column is the primary key.  The table is sorted by this column's value.  Note that
        /// in ECMA 335-2010, this is called the "Class" field.
        auto parent()                 const -> type_def_token;

        auto method_body()            const -> method_def_or_ref_token;
        auto method_body_raw()        const -> core::size_type;
        auto method_declaration()     const -> method_def_or_ref_token;
        auto method_declaration_raw() const -> core::size_type;
    };

    /// Represents a row in the **MethodSemantics** table (ECMA 335-2010 II.22.28)
    class method_semantics_row : public base_row<table_id::method_semantics>
    {
    public:

        auto semantics()   const -> method_semantics_flags;
        auto method()      const -> method_def_token;

        /// Gets a reference to the **Event** or **Property** that owns this **MethodSemantics** row
        ///
        /// Note that in ECMA 335-2010, this is called the "Association" field.  We have named it
        /// "Parent" for consistency with other tables in the database.
        auto parent()      const -> has_semantics_token;
        auto parent_raw()  const -> core::size_type;
    };

    /// Represents a row in the **MethodSpec** table (ECMA 335-2010 II.22.29)
    class method_spec_row : public base_row<table_id::method_spec>
    {
    public:

        auto method()     const -> method_def_or_ref_token;
        auto method_raw() const -> core::size_type;
        auto signature()  const -> blob;
    };

    /// Represents a row in the **Module** table (ECMA 335-2010 II.22.30)
    class module_row : public base_row<table_id::module>
    {
    public:

        auto name() const -> core::string_reference;
        auto mvid() const -> blob;
    };

    /// Represents a row in the **ModuleRef** table (ECMA 335-2010 II.22.31)
    class module_ref_row : public base_row<table_id::module_ref>
    {
    public:

        auto name() const -> core::string_reference;
    };

    /// Represents a row in the **NestedClass** table (ECMA 335-2010 II.22.32)
    class nested_class_row : public base_row<table_id::nested_class>
    {
    public:

        auto nested_class()    const -> type_def_token;
        auto enclosing_class() const -> type_def_token;
    };

    /// Represents a row in the **Param** table (ECMA 335-2010 II.22.33)
    class param_row : public base_row<table_id::param>
    {
    public:

        auto flags()    const -> parameter_flags;
        auto sequence() const -> std::uint16_t;
        auto name()     const -> core::string_reference;
    };

    /// Represents a row in the **Property** table (ECMA 335-2010 II.22.34)
    class property_row : public base_row<table_id::property>
    {
    public:

        auto flags()     const -> property_flags;
        auto name()      const -> core::string_reference;
        auto signature() const -> blob;
    };

    /// Represents a row in the **PropertyMap** table (ECMA 335-2010 II.22.35)
    class property_map_row : public base_row<table_id::property_map>
    {
    public:

        /// Gets a reference to the **TypeDef** that owns this **PropertyMap** row
        auto parent()         const -> type_def_token;

        auto first_property() const -> property_token;
        auto last_property()  const -> property_token;
    };

    /// Represents a row in the **StandaloneSig** table (ECMA 335-2010 II.22.36)
    class standalone_sig_row : public base_row<table_id::standalone_sig>
    {
    public:

        auto signature() const -> blob;
    };

    /// Represents a row in the **TypeDef** table (ECMA 335-2010 II.22.37)
    class type_def_row : public base_row<table_id::type_def>
    {
    public:

        auto flags()          const -> type_flags;
        auto name()           const -> core::string_reference;
        auto namespace_name() const -> core::string_reference;
        auto extends()        const -> type_def_ref_spec_token;
        auto extends_raw()    const -> core::size_type;

        auto first_field()    const -> field_token;
        auto last_field()     const -> field_token;

        auto first_method()   const -> method_def_token;
        auto last_method()    const -> method_def_token;
    };

    /// Represents a row in the **TypeRef** table (ECMA 335-2010 II.22.38)
    class type_ref_row : public base_row<table_id::type_ref>
    {
    public:

        auto resolution_scope()     const -> resolution_scope_token;
        auto resolution_scope_raw() const -> core::size_type;
        auto name()                 const -> core::string_reference;
        auto namespace_name()       const -> core::string_reference;
    };

    /// Represents a row in the **TypeSpec** table (ECMA 335-2010 II.22.39)
    class type_spec_row : public base_row<table_id::type_spec>
    {
    public:

        auto signature() const -> blob;
    };





    /// @}

} }

#endif
