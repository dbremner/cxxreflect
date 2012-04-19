#ifndef CXXREFLECT_WINDOWSRUNTIMECOMMON_HPP_
#define CXXREFLECT_WINDOWSRUNTIMECOMMON_HPP_

//               Copyright James P. McNellis (james@jamesmcnellis.com) 2011 - 2012.               //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Event.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/File.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

struct IInspectable;

namespace CxxReflect { namespace WindowsRuntime { namespace Internal {

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template <typename T> struct IsHat         { enum { value = false }; };
    template <typename T> struct IsHat<T^>     { enum { value = true; }; };
    template <typename T> struct AddHat        { typedef T^ Type;        };
    template <typename T> struct AddHat<T^>    {                         };
    template <typename T> struct RemoveHat     {                         };
    template <typename T> struct RemoveHat<T^> { typedef T Type;         };
    #endif

} } }

namespace CxxReflect { namespace WindowsRuntime {

    // A deleter for IInspectable objects that calls IUnknown::Release().
    class InspectableDeleter
    {
    public:

        void operator()(IInspectable* inspectable);
    };

    typedef std::unique_ptr<IInspectable, InspectableDeleter> UniqueInspectable;

} }

#endif // CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif
