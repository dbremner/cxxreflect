
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_WINDOWS_RUNTIME_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_WINDOWS_RUNTIME_HPP_

#include "cxxreflect/reflection/reflection.hpp"

#if defined(CXXREFLECT_WINDOWS_RUNTIME_UTILITY_HPP_)
#    define CXXREFLECT_WINDOWS_RUNTIME_UTILITY_HPP_INCLUDED 1
#else
#    define CXXREFLECT_WINDOWS_RUNTIME_UTILITY_HPP_INCLUDED 0
#endif

#include "cxxreflect/windows_runtime/common.hpp"
#include "cxxreflect/windows_runtime/enumerator.hpp"
#include "cxxreflect/windows_runtime/initialization.hpp"
#include "cxxreflect/windows_runtime/inspection.hpp"
#include "cxxreflect/windows_runtime/instantiation.hpp"
#include "cxxreflect/windows_runtime/invocation.hpp"
#include "cxxreflect/windows_runtime/loader.hpp"

#include "cxxreflect/windows_runtime/detail/call_invoker_x64.hpp"
#include "cxxreflect/windows_runtime/detail/call_invoker_x86.hpp"

#include "cxxreflect/windows_runtime/externals/winrt_externals.hpp"

// Ensure that we do not include the Utility header in the public interface:  It includes Windows.h
// and other Windows headers and we don't want to pollute the global and macro namespaces of library
// users if we don't have to.
#if defined(CXXREFLECT_WINDOWS_RUNTIME_UTILITY_HPP_) && CXXREFLECT_WINDOWS_RUNTIME_UTILITY_HPP_INCLUDED == 0
#    error Public CxxReflect headers should not include "cxxreflect/windows_runtime/utility.hpp"
#endif

#endif 
