
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_HIERARCHY_UTILITY_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_HIERARCHY_UTILITY_HPP_

#include "cxxreflect/metadata/metadata.hpp"

#include "cxxreflect/reflection/assembly_name.hpp"
#include "cxxreflect/reflection/detail/element_contexts.hpp"
#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/loader_configuration.hpp"
#include "cxxreflect/reflection/module_locator.hpp"
#include "cxxreflect/reflection/detail/type_policy.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    auto is_system_assembly(assembly_context const& a) -> bool;
    auto is_system_assembly(assembly const& a) -> bool;

    auto is_system_module(module_context const& m) -> bool;
    auto is_system_module(module const& m) -> bool;

    auto is_system_type(loader_context           const& root,
                        metadata::type_def_token const& t,
                        core::string_reference          simple_name) -> bool;

    auto is_system_type(type_def_with_module const& t, core::string_reference simple_name) -> bool;
    auto is_system_type(type const& t, core::string_reference simple_name) -> bool;
    
    auto is_derived_from_system_type(type_def_with_module const& t, metadata::element_type system_type, bool include_self) -> bool;
    auto is_derived_from_system_type(type_def_with_module const& t, core::string_reference simple_name, bool include_self) -> bool;
    auto is_derived_from_system_type(type const& t, metadata::element_type system_type, bool include_self) -> bool;
    auto is_derived_from_system_type(type const& t, core::string_reference simple_name, bool include_self) -> bool;

} } }

#endif
