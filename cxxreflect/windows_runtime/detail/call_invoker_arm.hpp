
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_DETAIL_CALL_INVOKER_ARM_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_DETAIL_CALL_INVOKER_ARM_HPP_

#include "cxxreflect/reflection/reflection.hpp"

#if defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION) && CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_ARM

#include "cxxreflect/windows_runtime/detail/argument_handling.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    /// Call invoker for ARM fastcall functions
    ///
    /// TODO ARM support is not implemented; this class is provided to make things build; invoke
    /// will simply throw an exception if it is called.
    class arm_fastcall_invoker
    {
    public:

        static auto invoke(reflection::method    const& method,
                           IInspectable               * instance,
                           void                       * result,
                           variant_argument_pack const& arguments) -> core::hresult
        {
            throw core::logic_error(L"not yet implemented");
        }
    };

    typedef arm_fastcall_invoker call_invoker;


} } }

#endif // #if defined(ENABLE_WINDOWS_RUNTIME_INTEGRATION) && ARCHITECTURE == ARCHITECTURE_X86
#endif 
