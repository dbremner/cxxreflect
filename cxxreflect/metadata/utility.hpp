
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_UTILITY_HPP_
#define CXXREFLECT_METADATA_UTILITY_HPP_

#include "cxxreflect/core/core.hpp"

namespace cxxreflect { namespace metadata { namespace detail {
    
    /// \defgroup cxxreflect_metadata_database_utility Metadata -> Detail -> PE/CLI File Utilities
    ///
    /// @{





    // The PE headers and related structures are naturally aligned, so we shouldn't need any custom
    // pragmas, attributes, or directives to pack the structures.  We use static assertions to ensure
    // that there is no padding, just in case.

    /// A two-component (major, minor) PE version number
    struct pe_version
    {
        std::uint16_t major;
        std::uint16_t minor;
    };

    static_assert(sizeof(pe_version) == 4, "invalid pe_version definition");





    /// A PE relative virtual address (RVA) and the associated size (in bytes)
    struct pe_rva_and_size
    {
        std::uint32_t rva;
        std::uint32_t size;
    };

    static_assert(sizeof(pe_rva_and_size) == 8, "invalid pe_rva_and_size definition");





    /// The PE file header, from the PE/COFF specification
    struct pe_file_header
    {
        // PE Signature;
        std::uint32_t signature;

        // PE Header
        std::uint16_t machine;
        std::uint16_t section_count;
        std::uint32_t creation_timestamp;
        std::uint32_t symbol_table_pointer;
        std::uint32_t symbol_count;
        std::uint16_t optional_header_size;
        std::uint16_t characteristics;

        // PE Optional Header Standard Fields
        std::uint16_t magic;
        std::uint16_t magic_minor;
        std::uint32_t code_size;
        std::uint32_t initialized_data_size;
        std::uint32_t uninitialized_data_size;
        std::uint32_t entry_point_rva;
        std::uint32_t code_rva;
        std::uint32_t data_rva;

        // PE Optional Header Windows NT-Specific Fields
        std::uint32_t image_base;
        std::uint32_t section_alignment;
        std::uint32_t file_alignment;
        pe_version    os_version;
        pe_version    user_version;
        pe_version    subsystem_version;
        std::uint16_t reserved;
        std::uint32_t image_size;
        std::uint32_t header_size;
        std::uint32_t file_checksum;
        std::uint16_t subsystem;
        std::uint16_t dll_flags;
        std::uint32_t stack_reserve_size;
        std::uint32_t stack_commit_size;
        std::uint32_t heap_reserve_size;
        std::uint32_t heap_commit_size;
        std::uint32_t loader_flags;
        std::uint32_t data_directory_count;

        // Data Directories
        pe_rva_and_size export_table;
        pe_rva_and_size import_table;
        pe_rva_and_size resource_table;
        pe_rva_and_size exception_table;
        pe_rva_and_size certification_table;
        pe_rva_and_size base_relocation_table;
        pe_rva_and_size debug_table;
        pe_rva_and_size copyright_table;
        pe_rva_and_size global_pointer_table;
        pe_rva_and_size thread_local_storage_table;
        pe_rva_and_size load_config_table;
        pe_rva_and_size bound_import_table;
        pe_rva_and_size import_address_table;
        pe_rva_and_size delay_import_descriptor_table;
        pe_rva_and_size cli_header_table;
        pe_rva_and_size reserved_table_header;
    };

    static_assert(sizeof(pe_file_header) == 248, "invalid pe_file_header definition");





    /// A PE section header, from the PE/COFF specification
    struct pe_section_header
    {
        char          name[8];
        std::uint32_t virtual_size;
        std::uint32_t virtual_address;

        std::uint32_t raw_data_size;
        std::uint32_t raw_data_offset;

        std::uint32_t relocations_offset;
        std::uint32_t line_numbers_offset;
        std::uint16_t relocations_count;
        std::uint16_t line_numbers_count;

        std::uint32_t characteristics;
    };

    static_assert(sizeof(pe_section_header) == 40, "invalid pe_section_header definition");

    typedef std::vector<pe_section_header> pe_section_header_sequence;





    /// A CLI header from a PE file, from the CLI specification
    struct pe_cli_header
    {
        std::uint32_t   size_in_bytes;
        pe_version      runtime_version;
        pe_rva_and_size metadata;
        std::uint32_t   flags;
        std::uint32_t   entry_point_token;
        pe_rva_and_size resources;
        pe_rva_and_size strong_name_signature;
        pe_rva_and_size code_manager_table;
        pe_rva_and_size vtable_fixups;
        pe_rva_and_size export_address_table_jumps;
        pe_rva_and_size managed_native_header;
    };

    static_assert(sizeof(pe_cli_header) == 72, "invalid pe_cli_header definition");





    /// A PE four-component (major, minor, build, revision) version number
    struct pe_four_component_version
    {
        std::uint16_t major;
        std::uint16_t minor;
        std::uint16_t build;
        std::uint16_t revision;
    };

    static_assert(sizeof(pe_four_component_version) == 8, "invalid pe_four_component_version definition");





    /// Constants for the five streams that may be found in a metadata database
    enum class pe_cli_stream_kind
    {
        string      = 0x0,
        user_string = 0x1,
        blob        = 0x2,
        guid        = 0x3,
        table       = 0x4
    };





    /// Encapsulates the location and size of a CLI metadata stream in a PE file
    /// 
    /// This does not map directly to file data and has no alignment constraints
    struct pe_cli_stream_header
    {
        std::uint32_t metadata_offset;
        std::uint32_t stream_offset;
        std::uint32_t stream_size;
    };

    /// A sequence of five `pe_cli_stream_header` objects (there are five stream kinds)
    typedef std::array<pe_cli_stream_header, 5> pe_cli_stream_header_sequence;





    /// Encapsulates the set of stream headers and the overarching CLI header
    ///
    /// This does not map directly to file data and has no alignment constraints
    struct pe_sections_and_cli_header
    {
        pe_section_header_sequence sections;
        pe_cli_header              cli_header;
    };





    /// Computes the absolute offset within a PE section given the section header and an RVA
    auto compute_offset_from_rva(pe_section_header const& section, pe_rva_and_size const& rva) -> core::size_type;





    /// Function object that tests whether a given RVA is in in a particular PE section
    class pe_section_contains_rva
    {
    public:

        explicit pe_section_contains_rva(std::uint32_t rva);

        /// Returns true if the `rva` passed into the constructor is in the section `section`
        auto operator()(pe_section_header const& section) const -> bool;

    private:

        std::uint32_t _rva;
    };



    

    /// Reads the PE section headers and the CLI header from the provided file
    auto read_pe_sections_and_cli_header(core::const_byte_cursor file) -> pe_sections_and_cli_header;

    /// Reads the CLI stream headers from the provided file, given the already-read PE/CLI headers
    auto read_pe_cli_stream_headers(core::const_byte_cursor           file,
                                    pe_sections_and_cli_header const& pe_header) -> pe_cli_stream_header_sequence;





    /// @}





    /// \defgroup cxxreflect_metadata_database_utility Metadata -> Detail -> PE/CLI File Utilities
    ///
    /// @{





    template <typename T>
    auto read_as(core::const_byte_iterator const data, core::size_type const index) -> T const&
    {
        return *reinterpret_cast<T const*>(data + index);
    }





    auto read_unsigned_integer(core::const_byte_iterator pointer,
                               core::size_type           size) ->core::size_type;

    auto read_table_index(database const&           scope,
                          core::const_byte_iterator data,
                          table_id                  table,
                          core::size_type           offset) -> core::size_type;

    auto read_composite_index(database const&           scope,
                              core::const_byte_iterator data, 
                              composite_index           index,
                              core::size_type           offset) -> core::size_type;

    auto read_blob_heap_index(database const&           scope,
                              core::const_byte_iterator data,
                              core::size_type           offset) -> core::size_type;

    auto read_blob_reference(database const&           scope,
                             core::const_byte_iterator data,
                             core::size_type           offset) -> blob;

    auto read_guid_heap_index(database const&           scope,
                              core::const_byte_iterator data,
                              core::size_type           offset) -> core::size_type;

    auto read_guid_reference(database const&           scope,
                             core::const_byte_iterator data,
                             core::size_type           offset) -> blob;

    auto read_string_heap_index(database const&           scope,
                                core::const_byte_iterator data,
                                core::size_type           offset) -> core::size_type;

    auto read_string_reference(database const&           scope,
                               core::const_byte_iterator data,
                               core::size_type           offset) -> core::string_reference;

    template <typename Token>
    auto read_token(database const&           scope,
                    core::const_byte_iterator data,
                    table_id                  table,
                    core::size_type           offset) -> Token
    {
        core::size_type const index(read_table_index(scope, data, table, offset));
        if (index == core::max_size_type)
            return core::default_value();

        return Token(&scope, table, index);
    }

    typedef std::pair<core::size_type, core::size_type> tag_index_pair;

    auto decompose_composite_index(composite_index index, core::size_type value) -> tag_index_pair;

    auto compose_composite_index(composite_index index,
                                 core::size_type index_tag,
                                 core::size_type index_value) -> core::size_type;

    template <typename Token>
    auto convert_index_and_compose_row(database const& scope,
                                       composite_index index,
                                       tag_index_pair  split) -> Token
    {
        table_id const table(table_id_for(split.first, index));
        if (table == invalid_table_id)
            throw core::metadata_error(L"failed to translate composite index to table identifier");

        return Token(&scope, table, split.second);
    }

    template <typename Token>
    auto read_token(database const&           scope,
                    core::const_byte_iterator data,
                    composite_index           index,
                    core::size_type           offset) -> Token
    {
        std::uint32_t const value(read_composite_index(scope, data, index, offset));
        if (value == 0)
            return core::default_value();

        return convert_index_and_compose_row<Token>(scope, index, decompose_composite_index(index, value));
    }





    template <table_id SourceTable, table_id TargetTable, typename FirstFunction>
    auto compute_last_row_token(database                  const& scope,
                                core::const_byte_iterator const  data,
                                FirstFunction             const  first) -> typename token_type_for_table_id<TargetTable>::type
    {
        typedef typename token_type_for_table_id<TargetTable>::type ResultType;

        core::size_type const byte_offset(core::convert_integer(data - scope.tables()[SourceTable].begin()));
        core::size_type const row_size(scope.tables()[SourceTable].row_size());
        core::size_type const logical_index(byte_offset / row_size);

        core::size_type const source_table_row_count(scope.tables()[SourceTable].row_count());
        core::size_type const target_table_row_count(scope.tables()[TargetTable].row_count());
        if (logical_index + 1 == source_table_row_count)
        {
            return ResultType(&scope, TargetTable, target_table_row_count);
        }
        else
        {
            return (scope[typename token_type_for_table_id<SourceTable>::type(&scope, SourceTable, logical_index + 1)].*first)();
        }
    }

    class primary_key_strict_weak_ordering
    {
    public:

        primary_key_strict_weak_ordering(core::size_type row_size,
                                         core::size_type value_size,
                                         core::size_type value_offset);

        auto operator()(core::const_byte_iterator row_it, core::size_type target_value) const -> bool;
        auto operator()(core::size_type target_value, core::const_byte_iterator row_it) const -> bool;

    private:

        core::value_initialized<core::size_type>           _row_size;
        core::value_initialized<core::size_type>           _value_size;
        core::value_initialized<core::size_type>           _value_offset;
        core::value_initialized<core::const_byte_iterator> _last;
    };

    auto composite_index_primary_key_equal_range(unrestricted_token const& parent,
                                                 composite_index           index,
                                                 table_id                  table,
                                                 column_id                 column) -> core::const_byte_range;

    auto table_id_primary_key_equal_range(unrestricted_token const& parent,
                                          table_id                  foreign_table,
                                          table_id                  primary_table,
                                          column_id                 column) -> core::const_byte_range;

    class owning_row_strict_weak_ordering
    {
    public:

        owning_row_strict_weak_ordering(core::size_type           row_size,
                                        core::size_type           value_size,
                                        core::size_type           value_offset,
                                        core::const_byte_iterator last);

        auto operator()(core::const_byte_iterator owning_row, core::size_type owned_row) const -> bool;
        auto operator()(core::size_type owned_row, core::const_byte_iterator owning_row) const -> bool;

    private:

        core::value_initialized<core::size_type>           _row_size;
        core::value_initialized<core::size_type>           _value_size;
        core::value_initialized<core::size_type>           _value_offset;
        core::value_initialized<core::const_byte_iterator> _last;
    };

    template <table_id OwningTable, table_id OwnedTable, typename OwnedRowToken>
    auto get_owning_row(OwnedRowToken const& owned_row, column_id const column)
        -> typename row_type_for_table_id<OwningTable>::type
    {
        static_assert(table_id_for_mask<OwnedRowToken::mask>::value == OwnedTable, "Invalid arguments");
        
        core::assert_initialized(owned_row);

        core::size_type const  owned_index(owned_row.index() + 1);
        database        const& owned_scope(owned_row.scope());

        auto const first(owned_scope.stride_begin(OwningTable));
        auto const last (owned_scope.stride_end  (OwningTable));

        auto const it(core::binary_search(first, last, owned_index, owning_row_strict_weak_ordering(
            owned_scope.tables()[OwningTable].row_size(),
            owned_scope.tables().table_index_size(OwnedTable),
            owned_scope.tables().table_column_offset(OwningTable, column),
            *last)));

        if (it == last)
            throw core::metadata_error(L"failed to find owning row");


        return create_row<typename row_type_for_table_id<OwningTable>::type>(&owned_scope, *it);
    }

    /// @}





    /// \defgroup cxxreflect_metadata_signatures_utility Metadata -> Detail -> Signature Utilities
    ///
    /// These are utility functions for extracting data from a metadata signature.  There are two
    /// functions for each kind of signature value:  the functions prefixed with `read_` read the
    /// value and advance the iterator to the byte one-past-the-end of the value; the functions
    /// prefixed with `peek_` read the value but do not advance the iterator.
    ///
    /// All of these functions will throw a `metadata_error` if the end of the range `[it, last)`
    /// is reached before the complete value can be read.  Some (e.g. the functions that read an
    /// `element_type`) will also throw a `metadata_error` if the value read is malformed in some
    /// way.  
    ///
    /// @{

    auto read_sig_byte(core::const_byte_iterator& it, core::const_byte_iterator last) -> core::byte;
    auto peek_sig_byte(core::const_byte_iterator  it, core::const_byte_iterator last) -> core::byte;

    auto read_sig_compressed_int32(core::const_byte_iterator& it, core::const_byte_iterator last) -> std::int32_t;
    auto peek_sig_compressed_int32(core::const_byte_iterator  it, core::const_byte_iterator last) -> std::int32_t;

    auto read_sig_compressed_uint32(core::const_byte_iterator& it, core::const_byte_iterator last) -> std::uint32_t;
    auto peek_sig_compressed_uint32(core::const_byte_iterator  it, core::const_byte_iterator last) -> std::uint32_t;

    auto read_sig_type_def_ref_spec(core::const_byte_iterator& it, core::const_byte_iterator last) -> std::uint32_t;
    auto peek_sig_type_def_ref_spec(core::const_byte_iterator  it, core::const_byte_iterator last) -> std::uint32_t;

    auto read_sig_element_type(core::const_byte_iterator& it, core::const_byte_iterator last) -> element_type;
    auto peek_sig_element_type(core::const_byte_iterator  it, core::const_byte_iterator last) -> element_type;

    auto read_sig_pointer(core::const_byte_iterator& it, core::const_byte_iterator last) -> std::uintptr_t;
    auto peek_sig_pointer(core::const_byte_iterator  it, core::const_byte_iterator last) -> std::uintptr_t;

    template <typename T>
    auto read_sig_element(core::const_byte_iterator& it, core::const_byte_iterator const last) -> T
    {
        if (core::distance(it, last) < sizeof(T))
            throw core::metadata_error(L"invalid read:  insufficient bytes remaining");

        T value;
        core::range_checked_copy(it, it + sizeof(T), core::begin_bytes(value), core::end_bytes(value));
        it += sizeof(T);
        return value;
    }

    template <typename T>
    auto peek_sig_element(core::const_byte_iterator it, core::const_byte_iterator const last) -> T
    {
        return read_sig_element(it, last);
    }

    /// @}

} } }

#endif
