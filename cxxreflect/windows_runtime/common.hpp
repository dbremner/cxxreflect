
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_COMMON_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_COMMON_HPP_

#include "cxxreflect/reflection/reflection.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include <atomic>
#include <future>
#include <thread>

#undef assert

struct IInspectable;

namespace cxxreflect { namespace windows_runtime {

    /// A deleter for IInspectable objects that calls IUnknown::Release()
    class inspectable_deleter
    {
    public:

        auto operator()(IInspectable* inspectable) -> void;
    };





    /// A unique_ptr specialization for IInspectable that uses `inspectable_deleter`
    typedef std::unique_ptr<IInspectable, inspectable_deleter> unique_inspectable;





    /// Exception that is thrown when dynamic invocation or instantiation fails
    ///
    /// This `runtime_error` is thrown by any of the dynamic invocation and instantiation functions
    /// if invocation fails for any reason, other than logic errors (the notable logic error is if 
    /// an invocation is attempted via a null instance, or if an instantiation function is called
    /// with an uninitialized type).
    class invocation_error : public core::runtime_error
    {
    public:

        explicit invocation_error(core::string message = L"")
            : core::runtime_error(std::move(message))
        {
        }
    };

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 

// AMDG //
