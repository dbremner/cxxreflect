
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_LOADERFACTORY_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_LOADERFACTORY_HPP_

#include "Configuration.hpp"

namespace CxxReflect { namespace Reflection { namespace Native {

    class LoaderFactory : public wrl::RuntimeClass<cxrabi::ILoaderFactory>
    {
        InspectableClass(RuntimeClass_CxxReflect_Reflection_Native_LoaderFactory, BaseTrust)

    public:

        virtual auto STDMETHODCALLTYPE CreateLoader(IInspectable* argument, cxrabi::LoaderFuture** value) -> HRESULT override;

    };

    ActivatableClass(LoaderFactory)

} } }

#endif
