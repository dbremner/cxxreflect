
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

    auto get_default_interface(reflection::type const& type) -> reflection::type
    {
        core::assert_initialized(type);

        if (type.is_interface())
            return type;

        if (type.is_value_type())
            return reflection::type();

        metadata::type_def_or_signature const context(type.context(core::internal_key()));
        core::assert_true([&]{ return context.is_token(); });

        auto const interface_impls(metadata::find_interface_impls(context.as_token()));
        auto const default_interface_impl(core::find_if(interface_impls, [&](metadata::interface_impl_row const& impl) -> bool
        {
            auto const custom_attributes(metadata::find_custom_attributes(impl.token()));
            auto const default_attribute(core::find_if(custom_attributes, [&](metadata::custom_attribute_row const& attribute) -> bool
            {
                metadata::custom_attribute_type_token const attribute_ctor(attribute.type());
                switch (attribute_ctor.table())
                {
                case metadata::table_id::method_def:
                {
                    metadata::method_def_token const real_attribute_ctor(attribute_ctor.as<metadata::method_def_token>());
                    metadata::type_def_row     const attribute_type(metadata::find_owner_of_method_def(real_attribute_ctor));

                    return attribute_type.namespace_name() == L"Windows.Foundation.Metadata"
                        && attribute_type.name() == L"DefaultAttribute";
                }
                case metadata::table_id::member_ref:
                {
                    metadata::member_ref_token const unresolved_attribute_ctor(attribute_ctor.as<metadata::member_ref_token>());

                    metadata::type_resolver const& resolver(reflection::detail::loader_context::from(context.scope()));
                    metadata::field_or_method_def_token const resolved_attribute_ctor(resolver.resolve_member(unresolved_attribute_ctor));
                    metadata::method_def_token const real_attribute_ctor(resolved_attribute_ctor.as<metadata::method_def_token>());
                    metadata::type_def_row const attribute_type(metadata::find_owner_of_method_def(real_attribute_ctor));

                    ::OutputDebugString(attribute_type.namespace_name().c_str());
                    ::OutputDebugString(L"/");
                    ::OutputDebugString(attribute_type.name().c_str());
                    ::OutputDebugString(L"\n");

                    return attribute_type.namespace_name() == L"Windows.Foundation.Metadata"
                        && attribute_type.name() == L"DefaultAttribute";
                }
                default:
                {
                    core::assert_unreachable();
                }
                }
                return false; // TODO
            }));

            return default_attribute != end(custom_attributes);
        }));

        if (default_interface_impl == end(interface_impls))
            return reflection::type();

        return reflection::type(default_interface_impl->interface_(), core::internal_key());
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
