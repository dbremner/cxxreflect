
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/inspection.hpp"
#include "cxxreflect/windows_runtime/utility.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_utility.hpp"
#include "cxxreflect/windows_runtime/detail/runtime_utility.hpp"

#include <roapi.h>
#include <rometadata.h>
#include <rometadataapi.h>
#include <rometadataresolution.h>
#include <wrl/client.h>

#include <windows.applicationModel.h>
#include <windows.foundation.h>
#include <windows.storage.h>

using namespace Microsoft::WRL;

namespace cxxreflect { namespace windows_runtime { namespace detail { namespace {


} } } }

namespace cxxreflect { namespace windows_runtime { namespace detail {

    auto compute_function_pointer(void const* const instance, unsigned const slot) -> void const*
    {
        core::assert_not_null(instance);

        // There are two levels of indirection to get to the function pointer:
        //
        //                  object            vtable
        //               +----------+      +----------+
        // instance ---> | vptr     | ---> | slot 0   |
        //               |~~~~~~~~~~|      | slot 1   |
        //                                 | slot 2   |
        //                                 |~~~~~~~~~~|
        //
        // This is fundamentally unsafe, so be very careful when calling. :-)
        return (*reinterpret_cast<void const* const* const*>(instance))[slot];
    }

    auto compute_method_slot_index(reflection::method const& method) -> core::size_type
    {
        core::assert_initialized(method);

        // TODO We should really add this as a member function on 'method'.  We can trivially compute
        // it by determining the index of the method in its element context table.  We should also
        // only scan declared methods, not all methods.
        reflection::type const type(method.reflected_type());

        core::size_type slot_index(0);
        for (auto it(type.begin_methods(metadata::binding_attribute::all_instance)); it != type.end_methods(); ++it)
        {
            if (*it == method)
                break;

            ++slot_index;
        }

        // TODO Error checking?
        return slot_index;
    }

    auto find_matching_interface_method(reflection::method const& runtime_type_method) -> reflection::method
    {
        core::assert_initialized(runtime_type_method);

        // TODO Reconsider the way that we find methods originally; it is absurd to go through all
        // of this work just to re-resolve a method.

        metadata::binding_flags const flags(
            metadata::binding_attribute::public_ |
            metadata::binding_attribute::instance);

        reflection::type const runtime_type(runtime_type_method.reflected_type());
        if (runtime_type.is_interface())
            return runtime_type_method;

        for (auto if_it(runtime_type.begin_interfaces()); if_it != runtime_type.end_interfaces(); ++if_it)
        {
            for (auto method_it(if_it->begin_methods(flags)); method_it != if_it->end_methods(); ++method_it)
            {
                // TODO This does not handle the ImplMap. Do we have to do that for WinRT?
                if (method_it->name() != runtime_type_method.name())
                    continue;

                if (method_it->return_type() != runtime_type_method.return_type())
                    continue;

                if (!core::range_checked_equal(
                        method_it->begin_parameters(), method_it->end_parameters(),
                        runtime_type_method.begin_parameters(), runtime_type_method.end_parameters()))
                    continue;

                return *method_it;
            }
        }

        return reflection::method();
    }

    auto get_activation_factory_interface(core::string_reference const& type_full_name,
                                          reflection::guid       const& interface_guid) -> unique_inspectable
    {
        core::assert_true([&]{ return !type_full_name.empty() && interface_guid != reflection::guid::empty; });

        utility::smart_hstring const type_full_name_hstring(type_full_name.c_str());

        ComPtr<IInspectable> factory;
        core::hresult const hr(::RoGetActivationFactory(
            type_full_name_hstring.value(),
            to_com_guid(interface_guid),
            reinterpret_cast<void**>(factory.GetAddressOf())));

        if (FAILED(hr) || factory == nullptr)
            throw core::runtime_error(L"failed to get requested activation factory interface");

        return unique_inspectable(factory.Detach());
    }

    auto query_interface(IInspectable* const instance, reflection::type const& interface_type) -> unique_inspectable
    {
        core::assert_true([&]{ return instance != nullptr && interface_type.is_interface(); });

        reflection::guid const interface_guid(get_guid(interface_type));
        
        ComPtr<IInspectable> interface_pointer;
        utility::throw_on_failure(instance->QueryInterface(
            to_com_guid(interface_guid),
            reinterpret_cast<void**>(interface_pointer.GetAddressOf())));

        return unique_inspectable(interface_pointer.Detach());
    }

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
