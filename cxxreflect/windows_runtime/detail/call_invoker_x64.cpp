
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#if defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION) && CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64

#include "cxxreflect/windows_runtime/inspection.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_utility.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_x64.hpp"
#include "cxxreflect/windows_runtime/detail/overload_resolution.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    auto x64_fastcall_invoker::invoke(reflection::method    const& method,
                                      IInspectable               * instance,
                                      void                       * result,
                                      variant_argument_pack const& arguments) -> core::hresult
    {
        // We can only call a method defined by an interface implemented by the runtime type, so
        // we re-resolve the method against the interfaces of its declaring type.  If it has
        // already been resolved to an interface method, this is a no-op transformation.
        reflection::method const interface_method(find_matching_interface_method(method));
        if (!interface_method)
            throw core::runtime_error(L"failed to find interface that defines method.");

        // Next, we need to compute the vtable slot of the method and QI to get the correct
        // interface pointer in order to obtain the function pointer.
        core::size_type const method_slot(compute_method_slot_index(interface_method));
        auto const interface_pointer(query_interface(instance, interface_method.declaring_type()));
            
        // We compute the function pointer from the vtable.  '6' is the well-known offset of all
        // Windows Runtime interface methods (IUnknown has three functions, and IInspectable has
        // an additional three functions).
        void const* fp(compute_function_pointer(interface_pointer.get(), method_slot + 6));

        // We construct the argument frame, by converting each argument to the correct type and
        // appending it to the frame, with basic type information for determining enregistration.
        x64_argument_frame frame;

        // Every function is called via an interface pointer.  That is always the first argument:
        void const* const raw_interface_pointer(interface_pointer.get());
        frame.push(raw_interface_pointer);

        // Next, we iterate over the arguments and parameters, convert each argument to the correct
        // parameter type, and push the argument into the frame:
        auto p_it(method.begin_parameters());
        auto a_it(arguments.begin());
        for (; p_it != method.end_parameters() && a_it != arguments.end(); ++p_it, ++a_it)
        {
            convert_and_insert(p_it->parameter_type(), arguments.resolve(*a_it), frame);
        }

        if (p_it != method.end_parameters() || a_it != arguments.end())
        {
            throw core::runtime_error(L"method arity does not match argument count");
        }

        // All calls to runtime classes use the COM calling convention of returning an HRESULT error
        // code.  If there is a "return value," it appears as a by-pointer parameter at the end of
        // the parameter list.  
        if (method.return_type() != get_type(L"Platform", L"Void"))
        {
            frame.push(result);
        }
        else if (result != nullptr)
        {
            throw core::runtime_error(L"attempted to call a void-returning function with a result pointer");
        }
            
        return cxxreflect_windows_runtime_x64_fastcall_thunk(fp, frame.arguments(), frame.types(), frame.count());
    }

    auto x64_fastcall_invoker::convert_and_insert(reflection::type          const& parameter_type,
                                                  resolved_variant_argument const& argument,
                                                  x64_argument_frame             & frame) -> void
    {
        core::assert_initialized(parameter_type);

        switch (compute_overload_element_type(parameter_type))
        {
        case metadata::element_type::boolean:
        {
            throw core::logic_error(L"not yet implemented");
            break;
        }

        case metadata::element_type::character:
        {
            throw core::logic_error(L"not yet implemented");
            break;
        }

        case metadata::element_type::i1:
        case metadata::element_type::i2:
        case metadata::element_type::i4:
        case metadata::element_type::i8:
        {
            std::int64_t const value(convert_to_i8(argument));
            frame.push(value);
            break;
        }

        case metadata::element_type::u1:
        case metadata::element_type::u2:
        case metadata::element_type::u4:
        case metadata::element_type::u8:
        {
            std::uint64_t const value(convert_to_u8(argument));
            frame.push(value);
            break;
        }

        case metadata::element_type::r4:
        {
            float const value(convert_to_r4(argument));
            frame.push(value);
            break;
        }

        case metadata::element_type::r8:
        {
            double const value(convert_to_r8(argument));
            frame.push(value);
            break;
        }

        case metadata::element_type::class_type:
        {
            throw core::logic_error(L"not yet implemented");
            IInspectable* const value(convert_to_interface(argument, get_guid(parameter_type)));
            frame.push(value);
            break;
        }

        case metadata::element_type::value_type:
        {
            throw core::logic_error(L"not yet implemented");
            break;
        }

        default:
        {
            core::assert_fail(L"unreachable code");
            break;
        }
        }
    }

} } }

#endif // #if defined(ENABLE_WINDOWS_RUNTIME_INTEGRATION) && ARCHITECTURE == ARCHITECTURE_X64

// AMDG //
