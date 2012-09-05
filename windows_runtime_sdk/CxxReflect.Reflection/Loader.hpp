
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_LOADER_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_LOADER_HPP_

#include "Configuration.hpp"

namespace CxxReflect { namespace Reflection {

    class LoaderStatics : public wrl::ActivationFactory<cxrabi::ILoaderStatics>
    {
        InspectableClassStatic(RuntimeClass_CxxReflect_Reflection_Loader, BaseTrust)

    public:

        virtual auto STDMETHODCALLTYPE get_PackageLoader(cxrabi::LoaderFuture** value) -> HRESULT override;

        // CreateLoader
        virtual auto STDMETHODCALLTYPE CreateLoader            (cxrabi::LoaderType type,     cxrabi::LoaderFuture** value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE CreateLoaderWithTypeName(HSTRING            typeName, cxrabi::LoaderFuture** value) -> HRESULT override;

        // InitializePackageLoader
        virtual auto STDMETHODCALLTYPE InitializePackageLoader                (cxrabi::LoaderType    type    ) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE InitializePackageLoaderWithTypeName    (HSTRING               typeName) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE InitializePackageLoaderWithLoader      (cxrabi::ILoader*      loader  ) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE InitializePackageLoaderWithLoaderFuture(cxrabi::LoaderFuture* loader  ) -> HRESULT override;
    };

    ActivatableStaticOnlyFactory(LoaderStatics);

} }

#endif
