
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Loader.hpp"
#include "LoaderFactory.hpp"

namespace CxxReflect { namespace Reflection { namespace Native {

    auto LoaderFactory::CreateLoader(IInspectable* const argument, cxrabi::LoaderFuture** const value) -> HRESULT
    {
        if (value == nullptr)
            return E_INVALIDARG;

        *value = wrl::Make<cxr::task_based_async_operation<cxrabi::ILoader>>(std::async([&]
        {
            return cxr::implicit_cast<cxrabi::ILoader*>(wrl::Make<Loader>(cxr::create_package_loader_future().get()).Detach());
        })).Detach();

        return S_OK;
    }

} } }
