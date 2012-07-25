
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/file.hpp"

namespace cxxreflect { namespace reflection {

    file::file()
    {
    }

    file::file(assembly const& a, metadata::file_token const f, core::internal_key)
        : _assembly(a), _file(f)
    {
        core::assert_initialized(a);
        core::assert_initialized(f);
    }

    auto file::attributes() const -> metadata::file_flags
    {
        core::assert_initialized(*this);

        return row().flags();
    }

    auto file::name() const -> core::string_reference
    {
        core::assert_initialized(*this);

        return row().name();
    }

    auto file::declaring_assembly() const -> assembly
    {
        core::assert_initialized(*this);

        return _assembly.realize();
    }

    auto file::contains_metadata() const -> bool
    {
        core::assert_initialized(*this);

        return !row().flags().is_set(metadata::file_attribute::contains_metadata);
    }

    auto file::hash_value() const -> core::sha1_hash
    {
        core::assert_initialized(*this);

        metadata::blob const value(row().hash_value());

        core::sha1_hash result((core::sha1_hash()));
        core::range_checked_copy(begin(value), end(value), core::begin_bytes(result), core::end_bytes(result));
        return result;
    }

    auto file::is_initialized() const -> bool
    {
        return _assembly.is_initialized() && _file.is_initialized();
    }

    auto file::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(file const& lhs, file const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._file == rhs._file;
    }

    auto operator<(file const& lhs, file const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._file < rhs._file;
    }

    auto file::row() const -> metadata::file_row
    {
        core::assert_initialized(*this);

        return row_from(_file);
    }

} }
