
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_HIERARCHY_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_HIERARCHY_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    auto is_system_assembly(assembly_context const& source_assembly) -> bool;
    auto is_system_module(module_context const& source_module) -> bool;
    auto is_system_database(metadata::database const& source_database) -> bool;

    auto is_system_type(metadata::type_def_token const& source_type, metadata::element_type        target_system_type) -> bool;
    auto is_system_type(metadata::type_def_token const& source_type, core::string_reference const& target_simple_name) -> bool;

    auto is_derived_from_system_type(metadata::type_def_token const& source_type,
                                     metadata::element_type          target_system_type,
                                     bool                            include_self) -> bool;

    auto is_derived_from_system_type(metadata::type_def_token const& source_type,
                                     core::string_reference   const& target_simple_name,
                                     bool                            include_self) -> bool;

} } }

#endif
