
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_CONFIGURATION_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_CONFIGURATION_HPP_

#include "CxxReflect.Reflection.h"

#include "cxxreflect/windows_runtime/utility.hpp"

#include <array>
#include <atomic>
#include <future>
#include <ppl.h>
#include <ppltasks.h>
#include <thread>
#include <wrl.h>
#include <wrl/async.h>
#include <wrl/client.h>
#include <wrl/wrappers/corewrappers.h>

namespace cxr
{
    using namespace cxxreflect::windows_runtime::utility;
}

namespace cxrabi
{
    using namespace ABI::CxxReflect::Reflection;
}

namespace ppl
{
    using namespace Concurrency;
}

namespace wrl
{
    using namespace Microsoft::WRL;
    using namespace Microsoft::WRL::Wrappers;

    typedef wrl::Module<Microsoft::WRL::InProc> InProcModule;
}

namespace winabi
{
    using namespace ABI::Windows::Foundation;
}

#endif
