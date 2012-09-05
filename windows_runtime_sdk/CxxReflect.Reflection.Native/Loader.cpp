
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "IterableUtility.hpp"
#include "Loader.hpp"
#include "Namespace.hpp"

namespace CxxReflect { namespace Reflection { namespace Native {

    Loader::Loader(std::unique_ptr<cxr::package_loader> loader)
        : _loader(std::move(loader))
    {
        cxr::assert_not_null(_loader.get());
    }

    auto STDMETHODCALLTYPE Loader::FindNamespace(HSTRING const namespaceName, cxrabi::INamespace** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value, E_INVALIDARG);
            cxr::throw_if_true(::WindowsIsStringEmpty(namespaceName) == TRUE, E_INVALIDARG);

            cxr::module_location const location(
                _loader->locator().find_metadata_for_namespace(
                    ::WindowsGetStringRawBuffer(namespaceName, nullptr)));

            if (!location.is_initialized())
                return S_FALSE;

            cxr::assembly const assembly(_loader->loader().load_assembly(location));

            *value = wrl::Make<Namespace>(this, assembly, ::WindowsGetStringRawBuffer(namespaceName, nullptr)).Detach();
            return S_OK;
        });
    }

    auto STDMETHODCALLTYPE Loader::FindType(HSTRING const fullName, cxrabi::IType** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value, E_INVALIDARG);
            cxr::throw_if_true(::WindowsIsStringEmpty(fullName) == TRUE, E_INVALIDARG);

            return S_OK; // TODO Not yet implemented
        });
    }

    auto STDMETHODCALLTYPE Loader::GetTypes(cxrabi::ITypeIterable** const value) -> HRESULT
    {
        // *value = wrl::Make<Detail::Iterable<cxrabi::IType*>>().Detach();
        return S_OK;
    }

} } }
