
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/precompiled_headers.hpp"
#include "cxxreflect/metadata/database.hpp"
#include "cxxreflect/metadata/rows.hpp"
#include "cxxreflect/metadata/utility.hpp"

namespace cxxreflect { namespace metadata { namespace {

    // These are the tag sizes for each of the composite indices.  Each element in the array
    // corresponds to the composite_index enumerator with its index value.
    composite_index_size_array const composite_index_tag_size =
    {
        2, 2, 5, 1, 2, 3, 1, 1, 1, 2, 3, 2, 1
    };

    // The `compute_{index_name}_index_size()` functions compute the size of an index.  It will
    // always be either two or four bytes.  An index value is two bytes in width if all of the
    // tables to which it can point have fewer rows than the maximum value that can be
    // represented by the index.  Each index has a tag of N bytes (see the list above) that
    // identifies the table into which it points.  
    //
    // As an example, the has_custom_attribute tag requires five bits.  So, this index can only be
    // represented by a two byte value if the number of rows in each of the tables it can reference
    // is less than 2^(16 - 5) = 2^11 = 2048.  If any table it can reference has more than 2048 rows
    // the index is represented by four-byte values.
    //
    // The `test_table_index_size` tests whether a given table can be represented in two bytes in
    // a given index.  We then aggregate the results of calling this for each table to determine
    // whether a given index is representable by two bytes.
    //
    // Easy as delicious, deceptive cake.

    auto test_table_index_size(table_id_size_array const& table_sizes, composite_index const index, table_id const table) -> bool
    {
        return table_sizes[core::as_integer(table)] < (1ull << (16 - composite_index_tag_size[core::as_integer(index)]));
    }

    auto compute_type_def_ref_spec_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::type_def_ref_spec, table_id::type_def)
            && test_table_index_size(table_sizes, composite_index::type_def_ref_spec, table_id::type_ref)
            && test_table_index_size(table_sizes, composite_index::type_def_ref_spec, table_id::type_spec) ? 2 : 4;
    }

    auto compute_has_constant_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::has_constant, table_id::field)
            && test_table_index_size(table_sizes, composite_index::has_constant, table_id::param)
            && test_table_index_size(table_sizes, composite_index::has_constant, table_id::property) ? 2 : 4;
    }

    auto compute_has_custom_attribute_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::method_def)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::field)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::type_ref)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::type_def)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::param)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::interface_impl)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::member_ref)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::module)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::property)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::event)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::standalone_sig)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::module_ref)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::type_spec)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::assembly)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::assembly_ref)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::file)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::exported_type)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::manifest_resource)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::generic_param)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::generic_param_constraint)
            && test_table_index_size(table_sizes, composite_index::has_custom_attribute, table_id::method_spec) ? 2 : 4;
    }

    auto compute_has_field_marshal_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::has_field_marshal, table_id::field)
            && test_table_index_size(table_sizes, composite_index::has_field_marshal, table_id::param) ? 2 : 4;
    }

    auto compute_has_decl_security_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::has_decl_security, table_id::type_def)
            && test_table_index_size(table_sizes, composite_index::has_decl_security, table_id::method_def)
            && test_table_index_size(table_sizes, composite_index::has_decl_security, table_id::assembly) ? 2 : 4;
    }

    auto compute_member_ref_parent_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::member_ref_parent, table_id::type_def)
            && test_table_index_size(table_sizes, composite_index::member_ref_parent, table_id::type_ref)
            && test_table_index_size(table_sizes, composite_index::member_ref_parent, table_id::module_ref)
            && test_table_index_size(table_sizes, composite_index::member_ref_parent, table_id::method_def)
            && test_table_index_size(table_sizes, composite_index::member_ref_parent, table_id::type_spec) ? 2 : 4;
    }

    auto compute_has_semantics_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::has_semantics, table_id::event)
            && test_table_index_size(table_sizes, composite_index::has_semantics, table_id::property) ? 2 : 4;
    }

    auto compute_method_def_or_ref_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::method_def_or_ref, table_id::method_def)
            && test_table_index_size(table_sizes, composite_index::method_def_or_ref, table_id::member_ref) ? 2 : 4;
    }

    auto compute_member_forwarded_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::member_forwarded, table_id::field)
            && test_table_index_size(table_sizes, composite_index::member_forwarded, table_id::method_def) ? 2 : 4;
    }

    auto compute_implementation_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::implementation, table_id::file)
            && test_table_index_size(table_sizes, composite_index::implementation, table_id::assembly_ref)
            && test_table_index_size(table_sizes, composite_index::implementation, table_id::exported_type) ? 2 : 4;
    }

    auto compute_custom_attribute_type_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::custom_attribute_type, table_id::method_def)
            && test_table_index_size(table_sizes, composite_index::custom_attribute_type, table_id::member_ref) ? 2 : 4;
    }

    auto compute_resolution_scope_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::resolution_scope, table_id::module)
            && test_table_index_size(table_sizes, composite_index::resolution_scope, table_id::module_ref)
            && test_table_index_size(table_sizes, composite_index::resolution_scope, table_id::assembly_ref)
            && test_table_index_size(table_sizes, composite_index::resolution_scope, table_id::type_ref) ? 2 : 4;
    }
    
    auto compute_type_or_method_def_index_size(table_id_size_array const& table_sizes) -> core::size_type
    {
        return test_table_index_size(table_sizes, composite_index::type_or_method_def, table_id::type_def)
            && test_table_index_size(table_sizes, composite_index::type_or_method_def, table_id::method_def) ? 2 : 4;
    }

} } }

namespace cxxreflect { namespace metadata {

    database_stream::database_stream()
    {
    }

    database_stream::database_stream(core::const_byte_cursor       file,
                                     core::size_type         const offset,
                                     core::size_type         const n)
    {
        if (!file.can_seek(offset, core::const_byte_cursor::begin))
            throw core::metadata_error(L"unable to read metadata stream:  start index out of range");

        file.seek(offset, core::const_byte_cursor::begin);
        
        if (!file.can_read(n))
            throw core::metadata_error(L"unable to read metadata stream:  end index out of range");

        core::const_byte_iterator const it(file.get_current());
        _data = core::array_range<core::byte const>(it, it + n);
    }

    auto database_stream::begin() const -> core::const_byte_iterator
    {
        return _data.begin();
    }

    auto database_stream::end() const -> core::const_byte_iterator
    {
        return _data.end();
    }

    auto database_stream::size() const -> core::size_type
    {
        return _data.size();
    }

    auto database_stream::is_initialized() const -> bool
    {
        return _data.is_initialized();
    }

    auto database_stream::operator[](core::size_type const index) const -> core::const_byte_iterator
    {
        return range_checked_at(index, 0);
    }

    auto database_stream::range_checked_at(core::size_type const index,
                                           core::size_type const n) const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);

        if (index + n > size())
            throw core::metadata_error(L"attempted to read from beyond the end of the stream");

        return _data.begin() + index;
    }





    database_table::database_table()
    {
    }

    database_table::database_table(core::const_byte_iterator const data,
                                   core::size_type           const row_size,
                                   core::size_type           const row_count,
                                   bool                      const is_sorted)
        : _data(data), _row_size(row_size), _row_count(row_count), _is_sorted(is_sorted)
    {
        core::assert_not_null(data);
        core::assert_true([&]{ return row_size != 0 && row_count != 0; });
    }

    auto database_table::begin() const -> core::const_byte_iterator
    {
        // Note:  it's okay if _data is nullptr; if it is, then begin() == end(), so the table is
        // considered to be empty.  Thus, we don't assert initialized here.
        return _data.get();
    }

    auto database_table::end() const -> core::const_byte_iterator
    {
        core::assert_true([&]{ return _data.get() != nullptr || _row_count.get() * _row_size.get() == 0; });

        // Note:  it's okay if _data is nullptr; if it is, then begin() == end() so the table is
        // considered to be empty.  Thus, we don't assert initialized here.
        return _data.get() + _row_count.get() * _row_size.get();
    }

    auto database_table::is_sorted() const -> bool
    {
        return _is_sorted.get();
    }

    auto database_table::row_count() const -> core::size_type
    {
        return _row_count.get();
    }

    auto database_table::row_size() const -> core::size_type
    {
        return _row_size.get();
    }

    auto database_table::operator[](core::size_type const index) const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);

        if (index >= row_count())
            throw core::metadata_error(L"attempted to read past end of table");

        return _data.get() + _row_size.get() * index;
    }

    auto database_table::is_initialized() const -> bool
    {
        return _data.get() != nullptr;
    }





    database_table_collection::database_table_collection()
    {
    }

    database_table_collection::database_table_collection(database_stream const& stream)
        : _stream(stream)
    {
        std::bitset<8> heap_sizes(_stream.read_as<std::uint8_t>(6));
        _string_heap_index_size.get() = heap_sizes.test(0) ? 4 : 2;
        _guid_heap_index_size.get()   = heap_sizes.test(1) ? 4 : 2;
        _blob_heap_index_size.get()   = heap_sizes.test(2) ? 4 : 2;

        _valid_bits.get()  = _stream.read_as<std::uint64_t>(8);
        _sorted_bits.get() = _stream.read_as<std::uint64_t>(16);

        core::size_type index(24);
        for (unsigned x(0); x < table_id_count; ++x)
        {
            if (!_valid_bits.get().test(x))
                continue;

            if (!is_valid_table_id(x))
                throw core::metadata_error(L"metadata table presence vector has invalid bits set");

            _row_counts.get()[x] = _stream.read_as<std::uint32_t>(index);
            index += 4;
        }

        compute_composite_index_sizes();
        compute_table_row_sizes();

        for (unsigned x(0); x < table_id_count; ++x)
        {
            if (!_valid_bits.get().test(x) || _row_counts.get()[x] == 0)
                continue;

            _tables.get()[x] = database_table(_stream[index],
                                              _row_sizes.get()[x],
                                              _row_counts.get()[x],
                                              _sorted_bits.get().test(x));
            index += _row_sizes.get()[x] * _row_counts.get()[x];
        }
    }

    auto database_table_collection::operator[](table_id const table) const -> database_table const&
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return is_valid_table_id(table); });

        return _tables.get()[core::as_integer(table)];
    }

    auto database_table_collection::table_index_size(table_id const table) const -> core::size_type
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return is_valid_table_id(table); });
        return _row_counts.get()[core::as_integer(table)] < (1 << 16) ? 2 : 4;
    }

    auto database_table_collection::composite_index_size(composite_index const index) const -> core::size_type
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return is_valid_composite_index(index); });
        return _composite_index_sizes.get()[core::as_integer(index)];
    }

    auto database_table_collection::string_heap_index_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return _string_heap_index_size.get();
    }

    auto database_table_collection::guid_heap_index_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return _guid_heap_index_size.get();
    }

    auto database_table_collection::blob_heap_index_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return _blob_heap_index_size.get();
    }

    auto database_table_collection::table_column_offset(table_id  const table,
                                                        column_id const column) const -> core::size_type
    {
        core::assert_initialized(*this);
        core::assert_true([&]() -> bool
        {
            if (core::as_integer(column) >= maximum_column_count)
                return false;

            return core::as_integer(column) == 0
                || _column_offsets.get()[core::as_integer(table)][core::as_integer(column)] != 0;
        });

        return _column_offsets.get()[core::as_integer(table)][core::as_integer(column)];
    }

    auto database_table_collection::is_initialized() const -> bool
    {
        return _stream.is_initialized();
    }

    auto database_table_collection::compute_composite_index_sizes() -> void
    {
        #define CXXREFLECT_GENERATE(x)                                           \
            _composite_index_sizes.get()[core::as_integer(composite_index::x)] = \
                compute_ ## x ## _index_size(_row_counts.get())
        
        CXXREFLECT_GENERATE(type_def_ref_spec    );
        CXXREFLECT_GENERATE(has_constant         );
        CXXREFLECT_GENERATE(has_custom_attribute );
        CXXREFLECT_GENERATE(has_field_marshal    );
        CXXREFLECT_GENERATE(has_decl_security    );
        CXXREFLECT_GENERATE(member_ref_parent    );
        CXXREFLECT_GENERATE(has_semantics        );
        CXXREFLECT_GENERATE(method_def_or_ref    );
        CXXREFLECT_GENERATE(member_forwarded     );
        CXXREFLECT_GENERATE(implementation       );
        CXXREFLECT_GENERATE(custom_attribute_type);
        CXXREFLECT_GENERATE(resolution_scope     );
        CXXREFLECT_GENERATE(type_or_method_def   );

        #undef CXXREFLECT_GENERATE
    }

    auto database_table_collection::compute_table_row_sizes() -> void
    {
        // First, we build up the _column_offsets table, which will contain the offsets of each
        // column in each table in this database.  The offset of the column one-past-the-end of
        // the last column of a table is the size of the whole table.
        //
        // Note that the offset of the initial column is always zero (for obvious reasons).  When we
        // call set_column_index for a given column, we provide it with the zero-based index of the
        // column and the size of the column at the previous index.  We accumulate the size of the
        // row as we set column indices.
        //
        // Another way to look at it is this:  each call to set_column_index provides the one-based
        // index of a column and its size.  Either way you look at it, the math is the same.
        //
        // Note that we've chosen to use integer column identifiers instead of the named constants
        // from the column_id enumeration to make it more easily verifiable that we're setting the
        // column sizes in order.  This is important since each index depends on the index of the
        // previous column.
        auto const set_column_index([&](table_id const table, core::size_type const column, core::size_type const size)
        {
            _column_offsets.get()[core::as_integer(table)][column] = _column_offsets.get()[core::as_integer(table)][column - 1] + size;
        });

        set_column_index(table_id::assembly, 1, 4);
        set_column_index(table_id::assembly, 2, 8);
        set_column_index(table_id::assembly, 3, 4);
        set_column_index(table_id::assembly, 4, blob_heap_index_size());
        set_column_index(table_id::assembly, 5, string_heap_index_size());
        set_column_index(table_id::assembly, 6, string_heap_index_size());

        set_column_index(table_id::assembly_os, 1, 4);
        set_column_index(table_id::assembly_os, 2, 4);
        set_column_index(table_id::assembly_os, 3, 4);

        set_column_index(table_id::assembly_processor, 1, 4);
        
        set_column_index(table_id::assembly_ref, 1, 8);
        set_column_index(table_id::assembly_ref, 2, 4);
        set_column_index(table_id::assembly_ref, 3, blob_heap_index_size());
        set_column_index(table_id::assembly_ref, 4, string_heap_index_size());
        set_column_index(table_id::assembly_ref, 5, string_heap_index_size());
        set_column_index(table_id::assembly_ref, 6, blob_heap_index_size());

        set_column_index(table_id::assembly_ref_os, 1, 4);
        set_column_index(table_id::assembly_ref_os, 2, 4);
        set_column_index(table_id::assembly_ref_os, 3, 4);
        set_column_index(table_id::assembly_ref_os, 4, table_index_size(table_id::assembly_ref));

        set_column_index(table_id::assembly_ref_processor, 1, 4);
        set_column_index(table_id::assembly_ref_processor, 2, table_index_size(table_id::assembly_ref));

        set_column_index(table_id::class_layout, 1, 2);
        set_column_index(table_id::class_layout, 2, 4);
        set_column_index(table_id::class_layout, 3, table_index_size(table_id::type_def));

        set_column_index(table_id::constant, 1, 2);
        set_column_index(table_id::constant, 2, composite_index_size(composite_index::has_constant));
        set_column_index(table_id::constant, 3, blob_heap_index_size());

        set_column_index(table_id::custom_attribute, 1, composite_index_size(composite_index::has_custom_attribute));
        set_column_index(table_id::custom_attribute, 2, composite_index_size(composite_index::custom_attribute_type));
        set_column_index(table_id::custom_attribute, 3, blob_heap_index_size());

        set_column_index(table_id::decl_security, 1, 2);
        set_column_index(table_id::decl_security, 2, composite_index_size(composite_index::has_decl_security));
        set_column_index(table_id::decl_security, 3, blob_heap_index_size());

        set_column_index(table_id::event_map, 1, table_index_size(table_id::type_def));
        set_column_index(table_id::event_map, 2, table_index_size(table_id::event));

        set_column_index(table_id::event, 1, 2);
        set_column_index(table_id::event, 2, string_heap_index_size());
        set_column_index(table_id::event, 3, composite_index_size(composite_index::type_def_ref_spec));

        set_column_index(table_id::exported_type, 1, 4);
        set_column_index(table_id::exported_type, 2, 4);
        set_column_index(table_id::exported_type, 3, string_heap_index_size());
        set_column_index(table_id::exported_type, 4, string_heap_index_size());
        set_column_index(table_id::exported_type, 5, composite_index_size(composite_index::implementation));

        set_column_index(table_id::field, 1, 2);
        set_column_index(table_id::field, 2, string_heap_index_size());
        set_column_index(table_id::field, 3, blob_heap_index_size());

        set_column_index(table_id::field_layout, 1, 4);
        set_column_index(table_id::field_layout, 2, table_index_size(table_id::field));

        set_column_index(table_id::field_marshal, 1, composite_index_size(composite_index::has_field_marshal));
        set_column_index(table_id::field_marshal, 2, blob_heap_index_size());

        set_column_index(table_id::field_rva, 1, 4);
        set_column_index(table_id::field_rva, 2, table_index_size(table_id::field));

        set_column_index(table_id::file, 1, 4);
        set_column_index(table_id::file, 2, string_heap_index_size());
        set_column_index(table_id::file, 3, blob_heap_index_size());

        set_column_index(table_id::generic_param, 1, 2);
        set_column_index(table_id::generic_param, 2, 2);
        set_column_index(table_id::generic_param, 3, composite_index_size(composite_index::type_or_method_def));
        set_column_index(table_id::generic_param, 4, string_heap_index_size());

        set_column_index(table_id::generic_param_constraint, 1, table_index_size(table_id::generic_param));
        set_column_index(table_id::generic_param_constraint, 2, composite_index_size(composite_index::type_def_ref_spec));

        set_column_index(table_id::impl_map, 1, 2);
        set_column_index(table_id::impl_map, 2, composite_index_size(composite_index::member_forwarded));
        set_column_index(table_id::impl_map, 3, string_heap_index_size());
        set_column_index(table_id::impl_map, 4, table_index_size(table_id::module_ref));

        set_column_index(table_id::interface_impl, 1, table_index_size(table_id::type_def));
        set_column_index(table_id::interface_impl, 2, composite_index_size(composite_index::type_def_ref_spec));

        set_column_index(table_id::manifest_resource, 1, 4);
        set_column_index(table_id::manifest_resource, 2, 4);
        set_column_index(table_id::manifest_resource, 3, string_heap_index_size());
        set_column_index(table_id::manifest_resource, 4, composite_index_size(composite_index::implementation));

        set_column_index(table_id::member_ref, 1, composite_index_size(composite_index::member_ref_parent));
        set_column_index(table_id::member_ref, 2, string_heap_index_size());
        set_column_index(table_id::member_ref, 3, blob_heap_index_size());

        set_column_index(table_id::method_def, 1, 4);
        set_column_index(table_id::method_def, 2, 2);
        set_column_index(table_id::method_def, 3, 2);
        set_column_index(table_id::method_def, 4, string_heap_index_size());
        set_column_index(table_id::method_def, 5, blob_heap_index_size());
        set_column_index(table_id::method_def, 6, table_index_size(table_id::param));

        set_column_index(table_id::method_impl, 1, table_index_size(table_id::type_def));
        set_column_index(table_id::method_impl, 2, composite_index_size(composite_index::method_def_or_ref));
        set_column_index(table_id::method_impl, 3, composite_index_size(composite_index::method_def_or_ref));

        set_column_index(table_id::method_semantics, 1, 2);
        set_column_index(table_id::method_semantics, 2, table_index_size(table_id::method_def));
        set_column_index(table_id::method_semantics, 3, composite_index_size(composite_index::has_semantics));

        set_column_index(table_id::method_spec, 1, composite_index_size(composite_index::method_def_or_ref));
        set_column_index(table_id::method_spec, 2, blob_heap_index_size());

        set_column_index(table_id::module, 1, 2);
        set_column_index(table_id::module, 2, string_heap_index_size());
        set_column_index(table_id::module, 3, guid_heap_index_size());
        set_column_index(table_id::module, 4, guid_heap_index_size());
        set_column_index(table_id::module, 5, guid_heap_index_size());

        set_column_index(table_id::module_ref, 1, string_heap_index_size());

        set_column_index(table_id::nested_class, 1, table_index_size(table_id::type_def));
        set_column_index(table_id::nested_class, 2, table_index_size(table_id::type_def));

        set_column_index(table_id::param, 1, 2);
        set_column_index(table_id::param, 2, 2);
        set_column_index(table_id::param, 3, string_heap_index_size());

        set_column_index(table_id::property, 1, 2);
        set_column_index(table_id::property, 2, string_heap_index_size());
        set_column_index(table_id::property, 3, blob_heap_index_size());

        set_column_index(table_id::property_map, 1, table_index_size(table_id::type_def));
        set_column_index(table_id::property_map, 2, table_index_size(table_id::property));

        set_column_index(table_id::standalone_sig, 1, blob_heap_index_size());

        set_column_index(table_id::type_def, 1, 4);
        set_column_index(table_id::type_def, 2, string_heap_index_size());
        set_column_index(table_id::type_def, 3, string_heap_index_size());
        set_column_index(table_id::type_def, 4, composite_index_size(composite_index::type_def_ref_spec));
        set_column_index(table_id::type_def, 5, table_index_size(table_id::field));
        set_column_index(table_id::type_def, 6, table_index_size(table_id::method_def));

        set_column_index(table_id::type_ref, 1, composite_index_size(composite_index::resolution_scope));
        set_column_index(table_id::type_ref, 2, string_heap_index_size());
        set_column_index(table_id::type_ref, 3, string_heap_index_size());

        set_column_index(table_id::type_spec, 1, blob_heap_index_size());

        // Finally, compute the complete row sizes:
        std::transform(begin(_column_offsets.get()), end(_column_offsets.get()), begin(_row_sizes.get()), 
        [](column_offset_sequence const& x) -> core::size_type
        {
            auto const it(std::find_if(x.rbegin(), x.rend(), [](core::size_type n) { return n != 0; }));
            return it != x.rend() ? *it : 0;
        });
    }





    database_string_collection::database_string_collection()
    {
    }

    database_string_collection::database_string_collection(database_stream&& stream)
        : _stream(std::move(stream))
    {
    }

    database_string_collection::database_string_collection(database_string_collection&& other)
        : _stream(std::move(other._stream)),
          _buffer(std::move(other._buffer)),
          _index (std::move(other._index ))
    {
    }

    auto database_string_collection::operator=(database_string_collection&& other) -> database_string_collection&
    {
        _stream = std::move(other._stream);
        _buffer = std::move(other._buffer);
        _index  = std::move(other._index);
        return *this;
    }

    auto database_string_collection::operator[](core::size_type const index) const -> core::string_reference
    {
        core::assert_initialized(*this);

        // TODO We can easily break this work up into smaller chunks if this becomes contentious.
        // We can easily do the UTF8->UTF16 conversion outside of any locks.
        auto const lock(_sync.lock());

        auto const existing_it(_index.find(index));
        if (existing_it != end(_index))
            return existing_it->second;

        char const* const pointer(_stream.reinterpret_as<char>(index));
        int const required(core::externals::compute_utf16_length_of_utf8_string(pointer));

        auto const range(_buffer.allocate(required));
        if (!core::externals::convert_utf8_to_utf16(pointer, begin(range), required))
            throw core::metadata_error(L"Failed to convert UTF8 to UTF16");

        return _index.insert(std::make_pair(
            index,
            core::string_reference(begin(range), end(range) - 1)
        )).first->second;
    }

    auto database_string_collection::is_initialized() const -> bool
    {
        return _stream.is_initialized();
    }





    auto database::create_from_file(core::string_reference const path) -> database
    {
        core::file_handle const file(path.c_str(), core::file_mode::read | core::file_mode::binary);
        return database(core::externals::map_file(file.handle()));
    }

    database::database(file_range&& file)
        : _file(std::move(file))
    {
        core::const_byte_cursor const cursor(_file.begin(), _file.end());

        auto const cli_header(detail::read_pe_sections_and_cli_header(cursor));
        auto const stream_headers(detail::read_pe_cli_stream_headers(cursor, cli_header));
        for (std::size_t i(0); i < stream_headers.size(); ++i)
        {
            if (stream_headers[i].metadata_offset == 0)
                continue;

            database_stream new_stream(
                cursor,
                stream_headers[i].metadata_offset + stream_headers[i].stream_offset,
                stream_headers[i].stream_size);

            switch (static_cast<detail::pe_cli_stream_kind>(i))
            {
            case detail::pe_cli_stream_kind::string:
                _strings = database_string_collection(std::move(new_stream));
                break;

            case detail::pe_cli_stream_kind::user_string:
                // We do not use the userstrings stream for metadata
                break;

            case detail::pe_cli_stream_kind::blob:
                _blobs = std::move(new_stream);
                break;

            case detail::pe_cli_stream_kind::guid:
                _guids = std::move(new_stream);
                break;

            case detail::pe_cli_stream_kind::table:
                _tables = database_table_collection(std::move(new_stream));
                break;

            default:
                throw core::metadata_error(L"unexpected stream kind value");
            }
        }
    }

    database::database(database&& other)
        : _blobs  (std::move(other._blobs  )),
          _guids  (std::move(other._guids  )),
          _strings(std::move(other._strings)),
          _tables (std::move(other._tables )),
          _file   (std::move(other._file   ))
    {
    }

    auto database::operator=(database&& other) -> database&
    {
        swap(other);
        return *this;
    }

    auto database::swap(database& other) -> void
    {
        std::swap(_blobs,   other._blobs  );
        std::swap(_guids,   other._guids  );
        std::swap(_strings, other._strings);
        std::swap(_tables,  other._tables );
        std::swap(_file,    other._file   );
    }

    auto database::stride_begin(table_id const table) const -> core::stride_iterator
    {
        core::assert_initialized(*this);
        return core::stride_iterator(_tables[table].begin(), _tables[table].row_size());
    }

    auto database::stride_end(table_id const table) const -> core::stride_iterator
    {
        core::assert_initialized(*this);
        return core::stride_iterator(_tables[table].end(), _tables[table].row_size());
    }

    auto database::tables() const -> database_table_collection const&
    {
        core::assert_initialized(*this);
        return _tables;
    }

    auto database::strings() const -> database_string_collection const&
    {
        core::assert_initialized(*this);
        return _strings;
    }

    auto database::blobs() const -> database_stream const&
    {
        core::assert_initialized(*this);
        return _blobs;
    }

    auto database::guids() const -> database_stream const&
    {
        core::assert_initialized(*this);
        return _guids;
    }

    auto database::is_initialized() const -> bool
    {
        return _blobs.is_initialized()
            && _guids.is_initialized()
            && _strings.is_initialized()
            && _tables.is_initialized();
    }

    auto operator==(database const& lhs, database const& rhs) -> bool
    {
        return &lhs == &rhs;
    }

    auto operator<(database const& lhs, database const& rhs) -> bool
    {
        return std::less<database const*>()(&lhs, &rhs);
    }

} }

// AMDG //
