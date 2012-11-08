
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/inspection.hpp"
#include "cxxreflect/windows_runtime/loader.hpp"
#include "cxxreflect/windows_runtime/utility.hpp"

namespace cxxreflect { namespace windows_runtime {

    auto get_implementers(reflection::type const& interface_type) -> std::vector<reflection::type>
    {
        return global_package_loader::get().get_implementers(interface_type);
    }

    auto get_implementers(core::string_reference const interface_full_name) -> std::vector<reflection::type>
    {
        reflection::type const interface_type(get_type(interface_full_name));
        if (!interface_type)
            throw core::runtime_error(L"failed to locate type by name");

        return get_implementers(interface_type);
    }

    auto get_implementers(core::string_reference const namespace_name,
                          core::string_reference const interface_simple_name) -> std::vector<reflection::type>
    {
        reflection::type const interface_type(get_type(namespace_name, interface_simple_name));
        if (!interface_type)
            throw core::runtime_error(L"failed to locate type by name");

        return get_implementers(interface_type);
    }

    auto get_type(core::string_reference const full_name) -> reflection::type
    {
        return global_package_loader::get().get_type(full_name);
    }

    auto get_type(core::string_reference const namespace_name,
                  core::string_reference const simple_name) -> reflection::type
    {
        return global_package_loader::get().get_type(namespace_name, simple_name);
    }

    auto get_type_of(IInspectable* const object) -> reflection::type
    {
        if (object == nullptr)
            throw core::logic_error(L"cannot get type of null inspectable object");

        utility::smart_hstring type_name;
        if (FAILED(object->GetRuntimeClassName(type_name.proxy())))
            throw core::runtime_error(L"failed to get runtime class name from inspectable object");

        if (type_name.empty())
            throw core::runtime_error(L"failed to get runtime class name from inspectable object");

        return get_type(type_name.c_str());
    }

    auto is_default_constructible(reflection::type const& type) -> bool
    {
        core::assert_initialized(type);

        metadata::binding_flags const flags(
            metadata::binding_attribute::instance |
            metadata::binding_attribute::public_);

        // TODO Consider checking the activation factory instead of the constructor definitions
        auto const first_constructor(begin(type.constructors(flags)));
        auto const last_constructor(end(type.constructors()));

        if (first_constructor == last_constructor)
            return true;

        auto const it(std::find_if(first_constructor, last_constructor, [](reflection::method const& constructor)
        {
            return constructor.parameter_count() == 0;
        }));

        return it != last_constructor;
    }

    auto get_guid(reflection::type const& type) -> reflection::guid
    {
        return global_package_loader::get().get_guid(type);
    }

    auto get_enumerators(reflection::type const& enumeration_type) -> std::vector<enumerator>
    {
        return global_package_loader::get().get_enumerators(enumeration_type);
    }

    auto get_enumerators(core::string_reference const enumeration_full_name) -> std::vector<enumerator>
    {
        reflection::type const enumeration_type(get_type(enumeration_full_name));
        if (!enumeration_type)
            throw core::runtime_error(L"failed to locate type by name");

        return get_enumerators(enumeration_type);
    }

    auto get_enumerators(core::string_reference const namespace_name,
                         core::string_reference const enumeration_simple_name) -> std::vector<enumerator>
    {
        reflection::type const enumeration_type(get_type(namespace_name, enumeration_simple_name));
        if (!enumeration_type)
            throw core::runtime_error(L"failed to locate type by name");

        return get_enumerators(enumeration_type);
    }

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
