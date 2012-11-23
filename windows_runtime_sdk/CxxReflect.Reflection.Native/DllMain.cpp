
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Configuration.hpp"

namespace
{
    cxr::size_type const force_externals_init(cxxreflect::core::externals::initialize(cxxreflect::externals::winrt_externals()));
}

#ifndef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW

extern "C"
{
    auto WINAPI DllCanUnloadNow() -> HRESULT
    {
        return wrl::InProcModule::GetModule().GetObjectCount() == 0 ? S_OK : S_FALSE;
    }

    auto WINAPI DllGetActivationFactory(HSTRING const activatibleClassId, IActivationFactory** const factory) -> HRESULT
    {
        return wrl::InProcModule::GetModule().GetActivationFactory(activatibleClassId, factory);
    }

    auto WINAPI DllMain(HINSTANCE, DWORD, LPVOID) -> BOOL
    {
        return TRUE;
    }
}

#if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86
#    pragma comment(linker, "/EXPORT:DllGetActivationFactory=_DllGetActivationFactory@8,PRIVATE")
#    pragma comment(linker, "/EXPORT:DllCanUnloadNow=_DllCanUnloadNow@0,PRIVATE")
#endif

#endif // CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW
