
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Loader.hpp"

namespace CxxReflect { namespace Reflection { namespace Native {

    Loader::Loader(std::unique_ptr<cxr::package_loader> loader)
        : _loader(std::move(loader))
    {
        cxr::assert_not_null(_loader.get());
    }

    auto STDMETHODCALLTYPE Loader::FindType(HSTRING const fullName, cxrabi::IType** const type) -> HRESULT
    {
        if (type == nullptr)
            return E_INVALIDARG;

        *type = nullptr;

        if (fullName == nullptr)
            return E_INVALIDARG;

        // TODO Not yet implemented
        return S_OK;
    }

} } }
