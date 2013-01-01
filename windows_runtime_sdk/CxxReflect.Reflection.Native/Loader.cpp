
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Loader.hpp"
#include "Namespace.hpp"
#include "Type.hpp"

namespace cxxreflect { namespace windows_runtime_sdk { namespace {

    static wrl::ComPtr<RuntimeNamespace> const nullNamespace(nullptr);

} } }

namespace cxxreflect { namespace windows_runtime_sdk {

    RuntimeLoader::RuntimeLoader(std::unique_ptr<cxr::package_loader> loader)
        : _loader(std::move(loader))
    {
        cxr::assert_not_null(_loader.get());
    }

    auto STDMETHODCALLTYPE RuntimeLoader::FindNamespace(HSTRING const namespaceName, abi::INamespace** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);
            cxr::throw_if_true(::WindowsIsStringEmpty(namespaceName) == TRUE, E_INVALIDARG);

            *value = cxr::clone_for_return(GetOrCreateNamespace(::WindowsGetStringRawBuffer(namespaceName, nullptr)));
            return S_OK;
        });
    }

    auto STDMETHODCALLTYPE RuntimeLoader::FindType(HSTRING const fullName, abi::IType** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);
            cxr::throw_if_true(::WindowsIsStringEmpty(fullName) == TRUE, E_INVALIDARG);

            cxr::type const type(_loader->get_type(::WindowsGetStringRawBuffer(fullName, nullptr)));
            cxr::throw_if_true(!type.is_initialized());

            *value = cxr::clone_for_return(wrl::Make<RuntimeType>(this, type));
            return S_OK;
        });
    }

    auto RuntimeLoader::GetPackageLoader() const -> cxr::package_loader const&
    {
        return *_loader.get();
    }

    auto RuntimeLoader::GetOrCreateNamespace(cxr::string_reference const& name) -> wrl::ComPtr<RuntimeNamespace> const&
    {
        cxr::recursive_mutex_lock const lock(_sync);

        auto const namespaceIt(_namespaces.find(name.c_str()));
        if (namespaceIt != end(_namespaces))
            return namespaceIt->second;

        cxr::assembly const& a(_loader->loader().load_assembly(_loader->locator().locate_namespace(name)));
        if (!a.is_initialized())
            return nullNamespace;

        cxr::module::type_range const& r(a.manifest_module().find_namespace(name));
        if (r.empty())
            return nullNamespace;

        return _namespaces.emplace(name.c_str(), wrl::Make<RuntimeNamespace>(this, a, r, name)).first->second;
    }

    auto RuntimeLoader::GetOrCreateType(cxr::type const& type) -> wrl::ComPtr<RuntimeType> const&
    {
        cxr::recursive_mutex_lock const lock(_sync);

        auto const typeIt(_types.find(type));
        if (typeIt != end(_types))
            return typeIt->second;

        return _types.emplace(type, wrl::Make<RuntimeType>(this, type)).first->second;
    }

} }
