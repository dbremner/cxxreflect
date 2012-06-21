
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_DETAIL_CALL_INVOKER_X86_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_DETAIL_CALL_INVOKER_X86_HPP_

#include "cxxreflect/reflection/reflection.hpp"

#if defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION) && CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86

#include "cxxreflect/windows_runtime/detail/argument_handling.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    /// Frame builder that constructs an argument frame in the form required by the stdcall thunk
    class x86_argument_frame
    {
    public:

        auto begin() const -> core::const_byte_iterator;
        auto end()   const -> core::const_byte_iterator;
        auto data()  const -> core::const_byte_iterator;
        auto size()  const -> core::size_type;

        auto align_to(core::size_type alignment) -> void;

        auto push(core::const_byte_iterator first, core::const_byte_iterator last) -> void;
        
    private:

        std::vector<core::byte> _data;
    };





    /// Call invoker for x86 stdcall functions
    class x86_stdcall_invoker
    {
    public:

        static auto invoke(reflection::method    const& method,
                           IInspectable               * instance,
                           void                       * result,
                           variant_argument_pack const& arguments) -> core::hresult;

    private:

        static auto convert_and_insert(reflection::type          const& parameter_type,
                                       resolved_variant_argument const& argument,
                                       x86_argument_frame             & frame) -> void;

        template <core::size_type FrameSize>
        static auto invoke_with_frame(void const* fp, core::const_byte_iterator frame) -> core::hresult;

    };





    typedef x86_argument_frame  argument_frame;
    typedef x86_stdcall_invoker call_invoker;


} } }

#endif // #if defined(ENABLE_WINDOWS_RUNTIME_INTEGRATION) && ARCHITECTURE == ARCHITECTURE_X86
#endif 

// AMDG //
