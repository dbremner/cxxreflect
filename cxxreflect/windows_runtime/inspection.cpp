
//                            Copyright James P. McNellis 2011 - 2013.                            //
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

        metadata::binding_flags const flags(metadata::binding_attribute::instance | metadata::binding_attribute::public_);

        auto const constructors(type.constructors(flags));
        auto const it(core::find_if(constructors, [](reflection::method const& constructor)
        {
            return constructor.parameter_count() == 0;
        }));

        return it != end(constructors);
    }

    auto get_default_interface(reflection::type const& type) -> reflection::type
    {
        core::assert_initialized(type);

        // An interface is its own default interface:
        if (type.is_interface())
            return type;

        // A value type implements no interfaces and thus has no default interface:
        if (type.is_value_type())
            return reflection::type();

        // A reference type (runtime class type) has a default interface, which we must compute by
        // examining its InterfaceImpls.  Here we go...
        metadata::type_def_or_signature const context(type.context(core::internal_key()));
        core::assert_true([&]{ return context.is_token(); });

        auto const interface_impls(metadata::find_interface_impls(context.as_token()));
        auto const default_interface_impl(core::find_if(interface_impls, [&](metadata::interface_impl_row const& impl) -> bool
        {
            auto const custom_attributes(metadata::find_custom_attributes(impl.token()));
            auto const default_attribute(core::find_if(custom_attributes, [&](metadata::custom_attribute_row const& attribute) -> bool
            {
                metadata::custom_attribute_type_token const attribute_ctor(attribute.type());
                metadata::type_def_row const attribute_type([&]() -> metadata::type_def_row
                {
                    switch (attribute_ctor.table())
                    {
                    case metadata::table_id::method_def:
                    {
                        return metadata::find_owner_of_method_def(attribute_ctor.as<metadata::method_def_token>());
                    }
                    case metadata::table_id::member_ref:
                    {
                        metadata::type_resolver const& resolver(reflection::detail::loader_context::from(context.scope()));

                        // PERF:  We don't need to fully resolve the member here; instead, we only
                        // need to get the parent of the member, which may be a type reference.
                        // Since type names are unique; we just have to compare the name of the
                        // referenced type to the name of the DefaultAttribute type.
                        metadata::member_ref_token          const unresolved_ctor(attribute_ctor.as<metadata::member_ref_token>());
                        metadata::field_or_method_def_token const resolved_ctor  (resolver.resolve_member(unresolved_ctor));

                        return metadata::find_owner_of_method_def(resolved_ctor.as<metadata::method_def_token>());
                    }
                    default:
                    {
                        core::assert_unreachable();
                    }
                    }
                }());
                
                return attribute_type.namespace_name() == L"Windows.Foundation.Metadata" && attribute_type.name() == L"DefaultAttribute";
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

    auto get_interface_declarer(reflection::method const& method) -> reflection::method
    {
        core::assert_initialized(method);

        typedef reflection::detail::method_traits                                 method_traits;
        typedef metadata::token_with_arithmetic<metadata::method_def_token>::type method_def_token_a;

        // If the method was already obtained via reflection on an interface type, we can return it:
        if (method.reflected_type().is_interface())
            return method;

        method_def_token_a           const method_context(method.context(core::internal_key()).member_token());
        method_traits::override_slot const override_slot (method_traits::compute_override_slot(method_context));

        metadata::type_def_row const declaring_type_row   (metadata::find_owner_of_method_def(override_slot.declared_method()));
        method_def_token_a     const first_declared_method(declaring_type_row.first_method());
        method_def_token_a     const last_declared_method (declaring_type_row.last_method());
        
        core::assert_true([&]
        {
            return first_declared_method <= override_slot.declared_method()
                && override_slot.declared_method() < last_declared_method;
        });

        core::size_type const declaring_type_method_index(override_slot.declared_method() - first_declared_method);

        reflection::type const interface_type(override_slot.declaring_type(), core::internal_key());
        auto const methods(interface_type.methods(metadata::binding_attribute::all_instance));

        core::assert_true([&]{ return declaring_type_method_index < core::distance(begin(methods), end(methods)); });

        return *(std::next(methods.begin(), declaring_type_method_index));
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
