
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_INSPECTION_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_INSPECTION_HPP_

#include "cxxreflect/windows_runtime/enumerator.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace cxxreflect { namespace windows_runtime {

    auto get_implementers(reflection::type const& interface_type)       -> std::vector<reflection::type>;
    auto get_implementers(core::string_reference interface_full_name)   -> std::vector<reflection::type>;

    auto get_implementers(core::string_reference namespace_name,
                          core::string_reference interface_simple_name) -> std::vector<reflection::type>;

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW
    template <typename Interface>
    auto get_implementers() -> std::vector<reflection::type>
    {
        core::string const full_name(Interface::typeid->FullName->Data());
        return get_implementers(full_name.c_str());
    }
    #endif





    auto get_type(core::string_reference full_name) -> reflection::type;
    auto get_type(core::string_reference namespace_name, core::string_reference simple_name) -> reflection::type;

    auto get_type_of(IInspectable* object) -> reflection::type;

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW
    template <typename T>
    auto get_type_of(T^ object) -> reflection::type
    {
        return windows_runtime::get_type_of(reinterpret_cast<IInspectable*>(object));
    }
    #endif





    auto is_default_constructible(reflection::type const&) -> bool;

    auto get_default_interface(reflection::type const&) -> reflection::type;
    auto get_guid(reflection::type const&) -> reflection::guid;
    auto get_interface_declarer(reflection::method const&) -> reflection::method;





    auto get_enumerators(reflection::type const& enumeration_type)     -> std::vector<enumerator>;
    auto get_enumerators(core::string_reference enumeration_full_name) -> std::vector<enumerator>;

    auto get_enumerators(core::string_reference namespace_name,
                         core::string_reference enumeration_simple_name) -> std::vector<enumerator>;

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW
    template <typename Enumeration>
    auto get_enumerators() -> std::vector<enumerator>
    {
        core::string const full_name(Interface::typeid->FullName->Data());
        return windows_runtime::get_enumerators(full_name.c_str());
    }
    #endif

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 
