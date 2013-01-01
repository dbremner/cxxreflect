
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/inspection.hpp"
#include "cxxreflect/windows_runtime/instantiation.hpp"
#include "cxxreflect/windows_runtime/loader.hpp"
#include "cxxreflect/windows_runtime/utility.hpp"
#include "cxxreflect/windows_runtime/detail/argument_handling.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_utility.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_arm.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_x64.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_x86.hpp"
#include "cxxreflect/windows_runtime/detail/overload_resolution.hpp"

#include <inspectable.h>

using namespace Microsoft::WRL;

namespace cxxreflect { namespace windows_runtime { namespace detail {

    auto create_inspectable_instance(reflection::type      const& type,
                                     variant_argument_pack const& arguments) -> unique_inspectable
    {
        core::assert_initialized(type);

        reflection::type const factory_type(global_package_loader::get().get_activation_factory_type(type));
        reflection::guid const factory_guid(get_guid(factory_type));

        auto const factory(get_activation_factory_interface(type.full_name().c_str(), factory_guid));
        if (factory == nullptr)
            throw invocation_error(L"failed to obtain activation factory for type");

        // Enumerate the candidate activation methods and perform overload resolution:
        auto const candidates(core::create_static_filtered_range(
            begin(factory_type.methods(metadata::binding_attribute::all_instance)),
            end(factory_type.methods()),
            [&](reflection::method const& m) { return m.name() == L"CreateInstance" && m.return_type() == type; }));

        overload_resolver const resolver(begin(candidates), end(candidates), arguments);

        if (!resolver.succeeded())
            throw invocation_error(L"failed to find activation method matching provided arguments");

        // Invoke the activation method to create the instance:
        ComPtr<IInspectable> new_instance;
        core::hresult result(call_invoker::invoke(
            resolver.result(),
            factory.get(),
            reinterpret_cast<void**>(new_instance.ReleaseAndGetAddressOf()),
            arguments));

        if (FAILED(result) || new_instance == nullptr)
            throw invocation_error(L"failed to create instance of type");

        return unique_inspectable(new_instance.Detach()); 
    }

} } }

namespace cxxreflect { namespace windows_runtime {

    auto create_inspectable_instance(reflection::type const& type) -> unique_inspectable
    {
        core::assert_initialized(type);

        if (!type.is_class())
            throw invocation_error(L"type is not a reference type; only reference types may be created");

        utility::smart_hstring const full_name(type.full_name().c_str());

        ComPtr<IInspectable> instance;

        core::hresult const hr(::RoActivateInstance(full_name.value(), instance.GetAddressOf()));
        if (FAILED(hr) || instance == nullptr)
            throw invocation_error(L"failed to create instance of type");

        return unique_inspectable(instance.Detach());
    }

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
