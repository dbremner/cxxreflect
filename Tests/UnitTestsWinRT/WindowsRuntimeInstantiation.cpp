
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Tests/UnitTests/Context.hpp"
#include "CxxReflect/CxxReflect.hpp"

namespace cxr
{
    using namespace CxxReflect;
    using namespace CxxReflect::WindowsRuntime;
}

namespace CxxReflectTest { namespace {

    CXXREFLECTTEST_REGISTER(WindowsRuntimeInvocation_CreateInspectableInstanceNoArguments, [](Context& c)
    {
        cxr::Type const awesomeType(cxr::GetType(L"WRLibrary.ProviderOfZero"));
        c.Verify(awesomeType.IsInitialized());

        cxr::UniqueInspectable awesomeInstance(cxr::CreateInspectableInstance(awesomeType));
        c.Verify(awesomeInstance != nullptr);
    });

    CXXREFLECTTEST_REGISTER(WindowsRuntimeInvocation_CreateObjectInstanceNoArguments, [](Context& c)
    {
        cxr::Type const awesomeType(cxr::GetType(L"WRLibrary.ProviderOfZero"));
        c.Verify(awesomeType.IsInitialized());

        Platform::Object^ awesomeInstance(cxr::CreateObjectInstance(awesomeType));
        c.Verify(awesomeInstance != nullptr);
    });

    CXXREFLECTTEST_REGISTER(WindowsRuntimeInvocation_CreateInstanceOfTNoArguments, [](Context& c)
    {
        cxr::Type const awesomeType(cxr::GetType(L"WRLibrary.ProviderOfZero"));
        c.Verify(awesomeType.IsInitialized());

        auto awesomeInstance(cxr::CreateInstance<WRLibrary::IProvideANumber>(awesomeType));
        c.Verify(awesomeInstance != nullptr);
    });

    

} }
