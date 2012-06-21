
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_DETAIL_CALL_INVOKER_UTILITY_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_DETAIL_CALL_INVOKER_UTILITY_HPP_

#include "cxxreflect/windows_runtime/common.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace cxxreflect { namespace windows_runtime { namespace detail {

    auto compute_function_pointer(void const* instance, unsigned slot) -> void const*;

    auto compute_method_slot_index(reflection::method const&) -> core::size_type;

    auto find_matching_interface_method(reflection::method const&) -> reflection::method;

    auto get_activation_factory_interface(core::string_reference const& type_full_name,
                                          reflection::guid       const& interface_guid) -> unique_inspectable;

    auto query_interface(IInspectable* instance, reflection::type const& interface_type) -> unique_inspectable;

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 

// AMDG //
