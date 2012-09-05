
//                            Copyright James P. McNellis 2011 - 2012.                            //
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

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
    using namespace cxxreflect::windows_runtime;
    using namespace cxxreflect::windows_runtime::utility;

    typedef cxxreflect::core::size_type size_type;
}

namespace cxrabi
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

namespace winabi
{
    using namespace ABI::Windows::Foundation;
    using namespace ABI::Windows::Foundation::Collections;
}

#endif
