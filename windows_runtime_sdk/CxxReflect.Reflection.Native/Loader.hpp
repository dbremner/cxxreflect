
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_LOADER_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_LOADER_HPP_

#include "Configuration.hpp"

namespace CxxReflect { namespace Reflection { namespace Native {

    class Loader : public wrl::RuntimeClass<cxrabi::ILoader>
    {
        InspectableClass(InterfaceName_CxxReflect_Reflection_ILoader, BaseTrust)

    public:

        Loader(std::unique_ptr<cxr::package_loader> loader);

        virtual auto STDMETHODCALLTYPE FindNamespace(HSTRING namespaceName, cxrabi::INamespace** value) -> HRESULT override;

        virtual auto STDMETHODCALLTYPE FindType(HSTRING fullName, cxrabi::IType** value) -> HRESULT override;

        virtual auto STDMETHODCALLTYPE GetTypes(cxrabi::ITypeIterable** value) -> HRESULT override;

    private:

        std::unique_ptr<cxr::package_loader> _loader;
    };

} } }

#endif
