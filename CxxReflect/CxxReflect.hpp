#ifndef CXXREFLECT_CXXREFLECT_HPP_
#define CXXREFLECT_CXXREFLECT_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Public interfaces header.  All clients should include just this header; it pulls in everything.

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Constant.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Event.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/File.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Module.hpp"
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION
#    include "CxxReflect/WindowsRuntimeCommon.hpp"
#    include "CxxReflect/WindowsRuntimeInspection.hpp"
#    include "CxxReflect/WindowsRuntimeInvocation.hpp"
#    include "CxxReflect/WindowsRuntimeLoader.hpp"
#endif

#endif
