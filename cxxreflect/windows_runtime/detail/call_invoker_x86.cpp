
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#if defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION) && CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86

#include "cxxreflect/windows_runtime/inspection.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_utility.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_x86.hpp"
#include "cxxreflect/windows_runtime/detail/overload_resolution.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    auto x86_argument_frame::begin() const -> core::const_byte_iterator
    {
        return _data.data();
    }

    auto x86_argument_frame::end() const -> core::const_byte_iterator
    {
        return  _data.data() + _data.size();
    }

    auto x86_argument_frame::data() const -> core::const_byte_iterator
    {
        return _data.data();
    }

    auto x86_argument_frame::size() const -> core::size_type
    {
        return _data.size();
    }

    auto x86_argument_frame::align_to(core::size_type const alignment) -> void
    {
        if (_data.empty())
            return;

        core::size_type const bytes_to_insert(alignment - (_data.size() % alignment));
        _data.resize(_data.size() + bytes_to_insert);
    }

    auto x86_argument_frame::push(core::const_byte_iterator const first, core::const_byte_iterator const last) -> void
    {
        _data.insert(_data.end(), first, last);
    }





    auto x86_stdcall_invoker::invoke(reflection::method    const& method,
                                     IInspectable               * instance,
                                     void                       * result,
                                     variant_argument_pack const& arguments) -> core::hresult
    {
        // We can only call a method defined by an interface implemented by the runtime type, so
        // we re-resolve the method against the interfaces of its declaring type.  If it has
        // already been resolved to an interface method, this is a no-op transformation.
        reflection::method const interface_method(find_matching_interface_method(method));
        if (!interface_method)
            throw invocation_error(L"failed to find interface that defines method");

        // Next, we need to compute the vtable slot of the method and QI to get the correct
        // interface pointer in order to obtain the function pointer.
        core::size_type const method_slot(compute_method_slot_index(interface_method));
        auto const interface_pointer(query_interface(instance, interface_method.declaring_type()));
            
        // We compute the function pointer from the vtable.  '6' is the well-known offset of all
        // Windows Runtime interface methods (IUnknown has three functions, and IInspectable has
        // an additional three functions).
        void const* const fp(compute_function_pointer(interface_pointer.get(), method_slot + 6));

        // We construct the argument frame, by converting each argument to the correct type and
        // appending it to an array.  In stdcall, arguments are pushed onto the stack left-to-right.
        // Because the stack is upside-down (i.e., it grows top-to-bottom), we push the arguments
        // into our argument frame right-to-left.
        x86_argument_frame frame;

        // Every function is called via an interface pointer.  That is always the first argument:
        void const* const raw_interface_pointer(interface_pointer.get());
        frame.push(core::begin_bytes(raw_interface_pointer), core::end_bytes(raw_interface_pointer));

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
            throw invocation_error(L"method arity does not match argument count");
        }

        if (method.return_type() != get_type(L"Platform", L"Void"))
        {
            frame.push(core::begin_bytes(result), core::end_bytes(result));
        }
        else if (result != nullptr)
        {
            throw core::logic_error(L"attempted to call a void-returning function with a result pointer");
        }
            
        // Due to promotion and padding, all argument frames should have a size divisible by 4.
        // In order to avoid writing inline assembly to move the arguments frame onto the stack
        // and issue the call instruction, we have a set of function template instantiations
        // that handle invocation for us.
        switch (frame.size())
        {
        case  4: return invoke_with_frame< 4>(fp, frame.data());
        case  8: return invoke_with_frame< 8>(fp, frame.data());
        case 12: return invoke_with_frame<12>(fp, frame.data());
        case 16: return invoke_with_frame<16>(fp, frame.data());
        case 20: return invoke_with_frame<20>(fp, frame.data());
        case 24: return invoke_with_frame<24>(fp, frame.data());
        case 28: return invoke_with_frame<28>(fp, frame.data());
        case 32: return invoke_with_frame<32>(fp, frame.data());
        case 36: return invoke_with_frame<36>(fp, frame.data());
        case 40: return invoke_with_frame<40>(fp, frame.data());
        case 44: return invoke_with_frame<44>(fp, frame.data());
        case 48: return invoke_with_frame<48>(fp, frame.data());
        case 52: return invoke_with_frame<52>(fp, frame.data());
        case 56: return invoke_with_frame<56>(fp, frame.data());
        case 60: return invoke_with_frame<60>(fp, frame.data());
        case 64: return invoke_with_frame<64>(fp, frame.data());
        }

        // If we hit this, we just need to add additional cases above.
        throw core::logic_error(L"size of requested frame is out of range");
    }

    auto x86_stdcall_invoker::convert_and_insert(reflection::type          const& parameter_type,
                                                 resolved_variant_argument const& argument,
                                                 x86_argument_frame             & frame) -> void
    {
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
        {
            std::int32_t const value(convert_to_i4(argument));
            frame.push(core::begin_bytes(value), core::end_bytes(value));
            break;
        }

        case metadata::element_type::i8:
        {
            std::int64_t const value(convert_to_i8(argument));
            frame.push(core::begin_bytes(value), core::end_bytes(value));
            break;
        }

        case metadata::element_type::u1:
        case metadata::element_type::u2:
        case metadata::element_type::u4:
        {
            std::uint32_t const value(convert_to_u4(argument));
            frame.push(core::begin_bytes(value), core::end_bytes(value));
            break;
        }

        case metadata::element_type::u8:
        {
            std::uint64_t const value(convert_to_u8(argument));
            frame.push(core::begin_bytes(value), core::end_bytes(value));
            break;
        }

        case metadata::element_type::r4:
        {
            float const value(convert_to_r4(argument));
            frame.push(core::begin_bytes(value), core::end_bytes(value));
            break;
        }

        case metadata::element_type::r8:
        {
            double const value(convert_to_r8(argument));
            frame.push(core::begin_bytes(value), core::end_bytes(value));
            break;
        }

        case metadata::element_type::class_type:
        {
            IInspectable* const value(convert_to_interface(argument, get_guid(parameter_type)));
            frame.push(core::begin_bytes(value), core::end_bytes(value));
            break;
        }

        case metadata::element_type::value_type:
        {
            throw core::logic_error(L"not yet implemented");
            break;
        }

        default:
        {
            throw core::logic_error(L"not yet implemented");
        }
        }
    }

    template <core::size_type FrameSize>
    auto x86_stdcall_invoker::invoke_with_frame(void const* fp, core::const_byte_iterator frame) -> core::hresult
    {
        struct frame_type { core::byte _value[FrameSize]; };
        typedef core::hresult (__stdcall* method_signature)(frame_type);

        frame_type const*     const typed_frame(reinterpret_cast<frame_type const*>(frame));
        method_signature const typed_fp(reinterpret_cast<method_signature>(fp));

        // Here we go!
        return typed_fp(*typed_frame);
    }

} } }

#endif // #if defined(ENABLE_WINDOWS_RUNTIME_INTEGRATION) && ARCHITECTURE == ARCHITECTURE_X86

// AMDG //
