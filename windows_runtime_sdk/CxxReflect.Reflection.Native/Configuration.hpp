
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_CONFIGURATION_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_CONFIGURATION_

#include "CxxReflect.Reflection.h"
#include "CxxReflect.Reflection.Native.h"

#include "cxxreflect/cxxreflect.hpp"
#include "cxxreflect/windows_runtime/externals/winrt_externals.hpp"
#include "cxxreflect/windows_runtime/utility.hpp"

#include <ppl.h>
#include <ppltasks.h>
#include <wrl.h>
#include <wrl/async.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

// C4373:  "virtual function overrides, previous versions of the compiler did not override when
// parameters only differed by const/volatile qualifiers."  This is all new code and this is what we
// want the compiler to do.
#pragma warning(disable: 4373)

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
    using namespace cxxreflect::windows_runtime;
    using namespace cxxreflect::windows_runtime::utility;

    typedef cxxreflect::core::size_type size_type;
}

namespace abi
{
    using namespace ABI::CxxReflect::Reflection;
    using namespace ABI::CxxReflect::Reflection::Native;
}

namespace ppl
{
    using namespace Concurrency;
}

namespace wrl
{
    using namespace Microsoft::WRL;

    typedef wrl::Module<Microsoft::WRL::InProc> InProcModule;
}

namespace win
{
    using namespace ABI::Windows::Foundation;
    using namespace ABI::Windows::Foundation::Collections;
}

namespace cxxreflect { namespace windows_runtime_sdk {

    class RuntimeConstant;
    class RuntimeEvent;
    class RuntimeLoader;
    class RuntimeLoaderFactory;
    class RuntimeMethod;
    class RuntimeNamespace;
    class RuntimeParameter;
    class RuntimeProperty;
    class RuntimeType;

    template <typename ForwardIterator>
    class RuntimeIterator;

    template <typename RandomAccessIterator>
    class RuntimeVectorView;

    typedef cxr::weak_ref<abi::ILoader, RuntimeLoader> WeakRuntimeLoaderRef;

    // When we instantiate the generic collection types, we need to transform between the ABI type
    // and the actual runtime class type (because the type of the generic interface uses the runtime
    // class type but the parameters of all of the functions use the ABI type). We have sufficiently
    // few types that we can just list the mapping here.

    template <typename T> struct ConvertToRuntimeClass     { typedef T                                        Type; };
    template <typename T> struct ConvertToRuntimeClass<T*> { typedef typename ConvertToRuntimeClass<T>::Type* Type; };

    template <> struct ConvertToRuntimeClass<abi::IConstant > { typedef abi::Constant  Type; };
    template <> struct ConvertToRuntimeClass<abi::IEvent    > { typedef abi::Event     Type; };
    template <> struct ConvertToRuntimeClass<abi::IMethod   > { typedef abi::Method    Type; };
    template <> struct ConvertToRuntimeClass<abi::INamespace> { typedef abi::Namespace Type; };
    template <> struct ConvertToRuntimeClass<abi::IParameter> { typedef abi::Parameter Type; };
    template <> struct ConvertToRuntimeClass<abi::IProperty > { typedef abi::Property  Type; };
    template <> struct ConvertToRuntimeClass<abi::IType     > { typedef abi::Type      Type; };

} }

#endif
