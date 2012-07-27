
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_EXTERNALS_WIN32_HPP_
#define CXXREFLECT_CORE_EXTERNALS_WIN32_HPP_

#include "cxxreflect/core/standard_library.hpp"
#include "cxxreflect/core/string.hpp"
#include "cxxreflect/core/utility.hpp"

namespace cxxreflect { namespace externals {

    class base_win32_externals
    {
    public:

        auto compute_utf16_length_of_utf8_string(char const* const source) const -> unsigned;

        auto convert_utf8_to_utf16(char const* const source,
                                   wchar_t*    const target,
                                   unsigned    const length) const -> bool;

        auto open_file(wchar_t const* const file_name, wchar_t const* const mode) const -> FILE*;

        auto map_file(FILE* const file) const -> core::unique_byte_array;

    protected:

        ~base_win32_externals();

    };

    class win32_externals : public base_win32_externals
    {
    public:

        auto compute_sha1_hash(core::const_byte_iterator first, core::const_byte_iterator last) const -> core::sha1_hash;

        auto compute_canonical_uri(wchar_t const* const path_or_uri) const -> core::string;

        auto file_exists(wchar_t const* const file_path) const -> bool;
    };

} }

#endif
