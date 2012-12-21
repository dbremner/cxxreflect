
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Loader.hpp"
#include "LoaderFactory.hpp"

namespace cxxreflect { namespace windows_runtime_sdk {

    auto RuntimeLoaderFactory::CreateLoader(IInspectable* const argument, abi::LoaderFuture** const value) -> HRESULT
    {
        if (value == nullptr)
            return E_INVALIDARG;

        *value = wrl::Make<cxr::task_based_async_operation<abi::ILoader>>(std::async([&]() -> abi::ILoader*
        {
            return cxr::clone_for_return(wrl::Make<RuntimeLoader>(cxr::create_package_loader_future().get()));
        })).Detach();

        return S_OK;
    }

} }
