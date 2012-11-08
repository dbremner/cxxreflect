
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_FILE_HPP_
#define CXXREFLECT_REFLECTION_FILE_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"

namespace cxxreflect { namespace reflection {

    class file
    {
    public:

        file();
        file(assembly const& a, metadata::file_token f, core::internal_key);

        auto attributes()         const -> metadata::file_flags;
        auto name()               const -> core::string_reference;
        auto declaring_assembly() const -> assembly;
        auto contains_metadata()  const -> bool;
        auto hash_value()         const -> core::sha1_hash;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        friend auto operator==(file const&, file const&) -> bool;
        friend auto operator< (file const&, file const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(file)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(file)

    private:

        auto row() const -> metadata::file_row;

        core::checked_pointer<detail::assembly_context const> _assembly;
        metadata::file_token                                  _file;
    };

} }

#endif 
