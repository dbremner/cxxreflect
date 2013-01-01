
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_LOADERFACTORY_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_LOADERFACTORY_HPP_

#include "Configuration.hpp"

namespace cxxreflect { namespace windows_runtime_sdk {

    class RuntimeLoaderFactory : public wrl::RuntimeClass<abi::ILoaderFactory>
    {
        InspectableClass(RuntimeClass_CxxReflect_Reflection_Native_LoaderFactory, BaseTrust)

    public:

        virtual auto STDMETHODCALLTYPE CreateLoader(IInspectable* argument, abi::LoaderFuture** value) -> HRESULT override;

    };

    ActivatableClass(RuntimeLoaderFactory)

} }

#endif
