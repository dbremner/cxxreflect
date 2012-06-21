
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_DETAIL_PACKAGE_GRAPH_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_DETAIL_PACKAGE_GRAPH_HPP_

#include "cxxreflect/reflection/reflection.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

typedef struct _GUID GUID;

namespace cxxreflect { namespace windows_runtime { namespace detail {

    auto compute_canonical_uri(core::string path) -> core::string;

    auto current_package_root() -> core::string;

    auto enumerate_package_metadata_files(core::string_reference package_root) -> std::vector<core::string>;

    auto remove_rightmost_type_name_component(core::string& type_name) -> void;

    auto to_com_guid(reflection::guid const& cxx_guid) -> GUID;
    auto to_cxx_guid(GUID             const& com_guid) -> reflection::guid;

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 

// AMDG //
