
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_FILE_HPP_
#define CXXREFLECT_REFLECTION_FILE_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/independent_handles.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"

namespace cxxreflect { namespace reflection {

    class file
    {
    public:

        file();

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

    public: // internal members

        file(assembly const& a, metadata::file_token f, core::internal_key);

    private:

        auto row() const -> metadata::file_row;

        detail::assembly_handle _assembly;
        metadata::file_token    _file;
    };

} }

#endif 
