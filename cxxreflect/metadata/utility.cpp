
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/precompiled_headers.hpp"
#include "cxxreflect/metadata/database.hpp"
#include "cxxreflect/metadata/utility.hpp"

namespace cxxreflect { namespace metadata { namespace detail { namespace {

    composite_index_size_array const composite_index_tag_size =
    {
        2, 2, 5, 1, 2, 3, 1, 1, 1, 2, 3, 2, 1
    };

} } } }

namespace cxxreflect { namespace metadata { namespace detail {

    auto compute_offset_from_rva(pe_section_header const& section, pe_rva_and_size const& rva) -> core::size_type
    {
        return rva.rva - section.virtual_address + section.raw_data_offset;
    }





    pe_section_contains_rva::pe_section_contains_rva(std::uint32_t const rva)
        : _rva(rva)
    {
    }

    auto pe_section_contains_rva::operator()(pe_section_header const& section) const -> bool
    {
        return _rva >= section.virtual_address && _rva < section.virtual_address + section.virtual_size;
    }





    auto read_pe_sections_and_cli_header(core::const_byte_cursor file) -> pe_sections_and_cli_header
    {
        // The index of the PE Header is located at index 0x3c of the DOS header
        file.seek(0x3c, core::const_byte_cursor::begin);
        
        std::uint32_t file_header_offset(0);
        file.read(&file_header_offset, 1);
        file.seek(file_header_offset, core::const_byte_cursor::begin);

        pe_file_header file_header = { 0 };
        file.read(&file_header, 1);
        if (file_header.section_count == 0 || file_header.section_count > 100)
            throw core::metadata_error(L"PE section count is out of range");

        pe_section_header_sequence sections(file_header.section_count);
        file.read(sections.data(), core::convert_integer(sections.size()));

        auto cli_header_section_it(std::find_if(
            begin(sections), end(sections),
            pe_section_contains_rva(file_header.cli_header_table.rva)));

        if (cli_header_section_it == end(sections))
            throw core::metadata_error(L"failed to locate PE file section containing CLI header");

        core::size_type cli_header_table_offset(compute_offset_from_rva(
            *cli_header_section_it,
            file_header.cli_header_table));
        
        file.seek(core::convert_integer(cli_header_table_offset), core::const_byte_cursor::begin);

        pe_cli_header cli_header = { 0 };
        file.read(&cli_header, 1);

        pe_sections_and_cli_header result;
        result.sections = std::move(sections);
        result.cli_header = cli_header;
        return result;
    }




    auto read_pe_cli_stream_headers(core::const_byte_cursor           file,
                                    pe_sections_and_cli_header const& pe_header) -> pe_cli_stream_header_sequence
    {
        auto metadata_section_it(std::find_if(
            begin(pe_header.sections),
            end(pe_header.sections),
            pe_section_contains_rva(pe_header.cli_header.metadata.rva)));

        if (metadata_section_it == end(pe_header.sections))
            throw core::metadata_error(L"failed to locate PE file section containing CLI metadata");

        core::size_type const metadata_offset(compute_offset_from_rva(
            *metadata_section_it,
            pe_header.cli_header.metadata));

        file.seek(metadata_offset, core::const_byte_cursor::begin);

        std::uint32_t magic_signature(0);
        file.read(&magic_signature, 1);
        if (magic_signature != 0x424a5342)
            throw core::metadata_error(L"magic signature does not match required value 0x424a5342");

        file.seek(8, core::const_byte_cursor::current);

        std::uint32_t version_length(0);
        file.read(&version_length, 1);
        file.seek(version_length + 2, core::const_byte_cursor::current); // Add 2 to account for unused flags

        std::uint16_t stream_count(0);
        file.read(&stream_count, 1);

        pe_cli_stream_header_sequence stream_headers((pe_cli_stream_header_sequence()));
        for (std::uint16_t i(0); i < stream_count; ++i)
        {
            pe_cli_stream_header header;
            header.metadata_offset = metadata_offset;
            file.read(&header.stream_offset, 1);
            file.read(&header.stream_size,   1);

            std::array<char, 12> current_name((std::array<char, 12>()));
            file.read(current_name.data(), core::convert_integer(current_name.size()));

            auto const handle_stream([&](char const* const name, pe_cli_stream_kind const kind, core::difference_type const rewind) -> bool
            {
                if (std::strcmp(current_name.data(), name) == 0 &&
                    stream_headers[core::as_integer(kind)].metadata_offset == 0)
                {
                    stream_headers[core::as_integer(kind)] = header;
                    file.seek(rewind, core::const_byte_cursor::current);
                    return true;
                }

                return false;
            });

            if (handle_stream("#Strings", pe_cli_stream_kind::string,       0) ||
                handle_stream("#US",      pe_cli_stream_kind::user_string, -8) ||
                handle_stream("#Blob",    pe_cli_stream_kind::blob,        -4) ||
                handle_stream("#GUID",    pe_cli_stream_kind::guid,        -4) ||
                handle_stream("#~",       pe_cli_stream_kind::table,       -8))
            {
                continue;
            }

            throw core::metadata_error(L"unknown stream name encountered");
        }

        return stream_headers;
    }





    auto read_unsigned_integer(core::const_byte_iterator const pointer,
                               core::size_type           const size) ->core::size_type
    {
        switch (size)
        {
        case 2:  return read_as<std::uint16_t>(pointer, 0);
        case 4:  return read_as<std::uint32_t>(pointer, 0);
        default: core::assert_fail(L"invalid integer size");
        }

        return 0;
    }

    auto read_table_index(database                  const& scope,
                          core::const_byte_iterator const  data,
                          table_id                  const  table,
                          core::size_type           const  offset) -> core::size_type
    {
        return read_unsigned_integer(data + offset, scope.tables().table_index_size(table)) - 1;
    }

    auto read_composite_index(database                  const& scope,
                              core::const_byte_iterator const  data, 
                              composite_index           const  index,
                              core::size_type           const  offset) -> core::size_type
    {
        return read_unsigned_integer(data + offset, scope.tables().composite_index_size(index));
    }

    auto read_blob_heap_index(database                  const& scope,
                              core::const_byte_iterator const  data,
                              core::size_type           const  offset) -> core::size_type
    {
        return read_unsigned_integer(data + offset, scope.tables().blob_heap_index_size());
    }

    auto read_blob_reference(database                  const& scope,
                             core::const_byte_iterator const  data,
                             core::size_type           const  offset) -> blob
    {
        return blob::compute_from_stream(
            &scope,
            read_blob_heap_index(scope, data, offset) + begin(scope.blobs()),
            end(scope.blobs()));
    }

    auto read_guid_heap_index(database                  const& scope,
                              core::const_byte_iterator const  data,
                              core::size_type           const  offset) -> core::size_type
    {
        return read_unsigned_integer(data + offset, scope.tables().guid_heap_index_size());
    }

    auto read_guid_reference(database                  const& scope,
                             core::const_byte_iterator const  data,
                             core::size_type           const  offset) -> blob
    {
        // The GUID heap index starts at 1 and counts by GUID, unlike the blob heap whose index
        // counts by byte.
        core::size_type const index(read_guid_heap_index(scope, data, offset) - 1);

        core::const_byte_iterator const first(index * 16 + begin(scope.guids()));
        return blob(&scope, first, first + 16);
    }

    auto read_string_heap_index(database                  const& scope,
                                core::const_byte_iterator const  data,
                                core::size_type           const  offset) -> core::size_type
    {
        return read_unsigned_integer(data + offset, scope.tables().string_heap_index_size());
    }

    auto read_string_reference(database                  const& scope,
                               core::const_byte_iterator const  data,
                               core::size_type           const  offset) -> core::string_reference
    {
        return scope.strings()[read_string_heap_index(scope, data, offset)];
    }

    auto decompose_composite_index(composite_index const index, core::size_type const value) -> tag_index_pair
    {
        core::size_type const tag_bits(composite_index_tag_size[core::as_integer(index)]);
        return std::make_pair(
            value & ((static_cast<core::size_type>(1u) << tag_bits) - 1),
            (value >> tag_bits) - 1
            );
    }

    auto compose_composite_index(composite_index const index,
                                 core::size_type const index_tag,
                                 core::size_type const index_value) -> core::size_type
    {
        core::size_type const tag_bits(composite_index_tag_size[core::as_integer(index)]);

        return index_tag | ((index_value + 1) << tag_bits);
    }

    primary_key_strict_weak_ordering::primary_key_strict_weak_ordering(core::size_type const row_size,
                                                                       core::size_type const value_size,
                                                                       core::size_type const value_offset)
        : _row_size(row_size), _value_size(value_size), _value_offset(value_offset)
    {
    }

    auto primary_key_strict_weak_ordering::operator()(core::const_byte_iterator const row_it,
                                                      core::size_type           const target_value) const -> bool
    {
        core::size_type const row_value(read_unsigned_integer(row_it + _value_offset.get(), _value_size.get()));

        return row_value < target_value;
    }

    auto primary_key_strict_weak_ordering::operator()(core::size_type           const target_value,
                                                      core::const_byte_iterator const row_it) const -> bool
    {
        core::size_type const row_value(read_unsigned_integer(row_it + _value_offset.get(), _value_size.get()));

        return target_value < row_value;
    }

    auto composite_index_primary_key_equal_range(unrestricted_token const& parent,
                                                 composite_index    const  index,
                                                 table_id           const  table,
                                                 column_id          const  column) -> core::const_byte_range
    {
        core::size_type const index_tag(index_key_for(parent.table(), index));
        if (index_tag == -1)
            throw core::logic_error(L"invalid argument:  parent is not from an allowed table for this index");

        core::size_type const index_value(compose_composite_index(index, index_tag, parent.index()));

        auto const first(parent.scope().stride_begin(table));
        auto const last (parent.scope().stride_end  (table));

        auto const range(core::equal_range(first, last, index_value, primary_key_strict_weak_ordering(
            parent.scope().tables()[table].row_size(),
            parent.scope().tables().composite_index_size(index),
            parent.scope().tables().table_column_offset(table, column))));

        if (range.first == range.second)
            return core::const_byte_range();

        return core::const_byte_range(*range.first, *range.second);
    }

    auto table_id_primary_key_equal_range(unrestricted_token const& parent,
                                          table_id           const  foreign_table,
                                          table_id           const  primary_table,
                                          column_id          const  column) -> core::const_byte_range
    {
        core::size_type const index_value(parent.index() + 1);

        auto const first(parent.scope().stride_begin(primary_table));
        auto const last (parent.scope().stride_end  (primary_table));

        auto const range(core::equal_range(first, last, index_value, primary_key_strict_weak_ordering(
            parent.scope().tables()[primary_table].row_size(),
            parent.scope().tables().table_index_size(foreign_table),
            parent.scope().tables().table_column_offset(primary_table, column))));

        if (range.first == range.second)
            return core::const_byte_range();

        return core::const_byte_range(*range.first, *range.second);
    }

    owning_row_strict_weak_ordering::owning_row_strict_weak_ordering(core::size_type           const row_size,
                                                                     core::size_type           const value_size,
                                                                     core::size_type           const value_offset,
                                                                     core::const_byte_iterator const last)
        : _row_size(row_size), _value_size(value_size), _value_offset(value_offset), _last(last)
    {
    }

    auto owning_row_strict_weak_ordering::operator()(core::const_byte_iterator const owning_row,
                                                     core::size_type           const owned_row) const -> bool
    {
        core::const_byte_iterator const next_row(owning_row + _row_size.get());
        if (next_row == _last.get())
            return false;

        core::size_type const owned_range_last(read_unsigned_integer(
            next_row + _value_offset.get(),
            _value_size.get()));

        return owned_range_last <= owned_row;
    }

    auto owning_row_strict_weak_ordering::operator()(core::size_type           const owned_row,
                                                     core::const_byte_iterator const owning_row) const -> bool
    {
        core::size_type const owned_range_first(read_unsigned_integer(
            owning_row + _value_offset.get(),
            _value_size.get()));

        return owned_row < owned_range_first;
    }

} } }

namespace cxxreflect { namespace metadata { namespace detail { namespace {

    core::const_character_iterator const iterator_read_unexpected_end(L"unexpectedly reached end of range");

    struct compressed_int_bytes
    {
        std::array<core::byte, 4> Bytes;
        core::size_type           Count;

        compressed_int_bytes()
            : Bytes(), Count()
        {
        }
    };

    auto read_sig_compressed_int_bytes(core::const_byte_iterator&      it,
                                       core::const_byte_iterator const last) -> compressed_int_bytes
    {
        compressed_int_bytes result;

        if (it == last)
            throw core::metadata_error(iterator_read_unexpected_end);

        // Note:  we've manually unrolled this for performance.  Thank you, Mr. Profiler.
        result.Bytes[0] = *it++;
        if ((result.Bytes[0] & 0x80) == 0)
        {
            result.Count = 1;
            return result;
        }
        else if ((result.Bytes[0] & 0x40) == 0)
        {
            result.Count = 2;
            result.Bytes[1] = result.Bytes[0];
            result.Bytes[1] ^= 0x80;

            if (last - it < 1)
                throw core::metadata_error(iterator_read_unexpected_end);

            result.Bytes[0] = *it++;
            return result;
        }
        else if ((result.Bytes[0] & 0x20) == 0)
        {
            result.Count = 4;
            result.Bytes[3] = result.Bytes[0];
            result.Bytes[3] ^= 0xC0;

            if (last - it < 3)
                throw core::metadata_error(iterator_read_unexpected_end);

            result.Bytes[2] = *it++;
            result.Bytes[1] = *it++;
            result.Bytes[0] = *it++;
            return result;
        }
        else
        {
            throw core::metadata_error(iterator_read_unexpected_end);
        }
    }

  

    auto is_custom_modifier_element_type(core::byte const value) -> bool
    {
        return value == element_type::custom_modifier_optional
            || value == element_type::custom_modifier_required;
    }

} } } }

namespace cxxreflect { namespace metadata { namespace detail {

    auto read_sig_byte(core::const_byte_iterator& it, core::const_byte_iterator const last) -> core::byte
    {
        if (it == last)
            throw core::metadata_error(iterator_read_unexpected_end);

        return *it++;
    }

    auto peek_sig_byte(core::const_byte_iterator it, core::const_byte_iterator const last) -> core::byte
    {
        return read_sig_byte(it, last);
    }

    auto read_sig_compressed_int32(core::const_byte_iterator& it, core::const_byte_iterator const last) -> std::int32_t
    {
        compressed_int_bytes bytes(read_sig_compressed_int_bytes(it, last));

        bool const lsbSet((bytes.Bytes[bytes.Count - 1] & 0x01) != 0);

        switch (bytes.Count)
        {
        case 1:
        {
            std::uint8_t p(bytes.Bytes[0]);
            p >>= 1;
            lsbSet ? (p |= 0xFFFFFF80) : (p &= 0x0000007F);
            return *reinterpret_cast<std::int8_t*>(&p);
        }
        case 2:
        {
            std::uint32_t p(*reinterpret_cast<std::uint16_t*>(&bytes.Bytes[0]));
            p >>= 1;
            lsbSet ? (p |= 0xFFFFE000) : (p &= 0x00001FFF);
            return *reinterpret_cast<std::int16_t*>(&p);
        }
        case 4:
        {
            std::uint32_t p(*reinterpret_cast<std::uint32_t*>(&bytes.Bytes[0]));
            p >>= 1;
            lsbSet ? (p |= 0xF0000000) : (p &= 0x0FFFFFFF);
            return *reinterpret_cast<std::int32_t*>(&p);
        }
        default:
        {
            core::assert_fail(L"it is impossible to get here");
            return 0;
        }
        }
    }

    auto peek_sig_compressed_int32(core::const_byte_iterator it, core::const_byte_iterator const last) -> std::int32_t
    {
        return read_sig_compressed_int32(it, last);
    }

    auto read_sig_compressed_uint32(core::const_byte_iterator& it, core::const_byte_iterator const last) -> std::uint32_t
    {
        compressed_int_bytes const bytes(read_sig_compressed_int_bytes(it, last));

        switch (bytes.Count)
        {
        case 1:  return *reinterpret_cast<std::uint8_t  const*>(&bytes.Bytes[0]);
        case 2:  return *reinterpret_cast<std::uint16_t const*>(&bytes.Bytes[0]);
        case 4:  return *reinterpret_cast<std::uint32_t const*>(&bytes.Bytes[0]);
        default: core::assert_fail(L"it is impossible to get here"); return 0;
        }
    }

    auto peek_sig_compressed_uint32(core::const_byte_iterator it, core::const_byte_iterator const last) -> std::uint32_t
    {
        return read_sig_compressed_uint32(it, last);
    }

    auto read_sig_type_def_ref_spec(core::const_byte_iterator& it, core::const_byte_iterator const last) -> std::uint32_t
    {
        std::uint32_t const token_value(read_sig_compressed_uint32(it, last));
        std::uint32_t const token_type(token_value & 0x03);

        switch (token_type)
        {
        case 0x00: return (token_value >> 2) | (core::as_integer(table_id::type_def)  << 24);
        case 0x01: return (token_value >> 2) | (core::as_integer(table_id::type_ref)  << 24);
        case 0x02: return (token_value >> 2) | (core::as_integer(table_id::type_spec) << 24);
        default:   throw core::metadata_error(L"unexpected table id in type def/ref/spec encoded");
        }
    }

    auto peek_sig_type_def_ref_spec(core::const_byte_iterator it, core::const_byte_iterator const last) -> std::uint32_t
    {
        return read_sig_type_def_ref_spec(it, last);
    }

    auto read_sig_element_type(core::const_byte_iterator& it, core::const_byte_iterator const last) -> element_type
    {
        core::byte const value(read_sig_byte(it, last));
        if (!is_valid_element_type(value))
            throw core::metadata_error(L"unexpected element type");

        return static_cast<element_type>(value);
    }

    auto peek_sig_element_type(core::const_byte_iterator it, core::const_byte_iterator const last) -> element_type
    {
        return read_sig_element_type(it, last);
    }

    auto read_sig_pointer(core::const_byte_iterator& it, core::const_byte_iterator const last) -> std::uintptr_t
    {
        if (core::distance(it, last) < sizeof(std::uintptr_t))
            throw core::metadata_error(iterator_read_unexpected_end);

        std::uintptr_t value(0);
        core::range_checked_copy(it, it + sizeof(std::uintptr_t), core::begin_bytes(value), core::end_bytes(value));
        it += sizeof(std::uintptr_t);
        return value;
    }

    auto peek_sig_pointer(core::const_byte_iterator it, core::const_byte_iterator const last) -> std::uintptr_t
    {
        return read_sig_pointer(it, last);
    }

} } }
