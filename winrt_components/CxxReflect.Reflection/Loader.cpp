
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Loader.hpp"

#include <ppltasks.h>

namespace {

    std::mutex                         globalPackageLoaderMutex;
    std::atomic<cxrabi::LoaderFuture*> globalPackageLoader;

    template <typename F>
    auto GetOrCreateGlobalPackageLoader(F const createLoaderCallback) ->cxrabi::LoaderFuture*
    {
        cxrabi::LoaderFuture* const firstAttempt(globalPackageLoader.load());
        if (firstAttempt != nullptr)
            return firstAttempt;

        std::unique_lock<std::mutex> lock(globalPackageLoaderMutex);

        cxrabi::LoaderFuture* const secondAttempt(globalPackageLoader.load());
        if (secondAttempt != nullptr)
            return secondAttempt;

        cxrabi::LoaderFuture* const createdFuture(createLoaderCallback());
        globalPackageLoader.store(createdFuture);
        return createdFuture;
    }

    auto GetGlobalPackageLoader() -> cxrabi::LoaderFuture*
    {
        return globalPackageLoader.load();
    }

    auto GetKnownLoaderFactoryTypeName(cxrabi::LoaderType const loaderFactoryType) -> wchar_t const*
    {
        std::array<wchar_t const*, 1> const knownLoaderFactoryTypes = 
        {
            L"CxxReflect.Reflection.Native.LoaderFactory"
        };

        std::size_t const loaderFactoryTypeIndex(static_cast<std::size_t>(loaderFactoryType));
        if (loaderFactoryTypeIndex > knownLoaderFactoryTypes.size())
            return nullptr;

        return knownLoaderFactoryTypes[loaderFactoryTypeIndex];
    }

    auto CreateLoaderFuture(HSTRING const loaderFactoryTypeName) -> cxrabi::LoaderFuture*
    {
        wrl::ComPtr<cxrabi::ILoaderFactory> factory;
        HRESULT const hr0(cxr::activate_instance_and_qi(loaderFactoryTypeName, factory.GetAddressOf()));
        if (FAILED(hr0) || factory == nullptr)
            return nullptr;

        wrl::ComPtr<cxrabi::LoaderFuture> loaderFuture;
        HRESULT const hr1(factory->CreateLoader(nullptr, loaderFuture.GetAddressOf()));
        if (FAILED(hr1) || loaderFuture == nullptr)
            return nullptr;

        return loaderFuture.Detach();
    }

}

namespace CxxReflect { namespace Reflection {

    auto STDMETHODCALLTYPE LoaderStatics::get_PackageLoader(cxrabi::LoaderFuture** const value) -> HRESULT
    {
        if (value == nullptr)
            return E_INVALIDARG;

        *value = nullptr;

        wrl::ComPtr<cxrabi::LoaderFuture> loaderFuture(GetOrCreateGlobalPackageLoader([&]
        {
            return CreateLoaderFuture(wrl::HStringReference(GetKnownLoaderFactoryTypeName(cxrabi::LoaderType())).Get());
        }));

        if (loaderFuture == nullptr)
            return E_FAIL;

        *value = loaderFuture.Detach();
        return S_OK;
    }

    auto STDMETHODCALLTYPE LoaderStatics::CreateLoader(cxrabi::LoaderType const type, cxrabi::LoaderFuture** const value) -> HRESULT
    {
        if (value == nullptr)
            return E_INVALIDARG;

        *value = nullptr;

        wchar_t const* const typeName(GetKnownLoaderFactoryTypeName(type));
        if (typeName == nullptr)
            return E_INVALIDARG;

        return CreateLoaderWithTypeName(wrl::HStringReference(typeName).Get(), value);
    }

    auto STDMETHODCALLTYPE LoaderStatics::CreateLoaderWithTypeName(HSTRING typeName, cxrabi::LoaderFuture** value) -> HRESULT
    {
        if (typeName == nullptr || value == nullptr)
            return E_INVALIDARG;

        *value = CreateLoaderFuture(typeName);
        return *value != nullptr ? S_OK : E_FAIL;
    }

    auto STDMETHODCALLTYPE LoaderStatics::InitializePackageLoader(cxrabi::LoaderType const type) -> HRESULT
    {
        wchar_t const* const typeName(GetKnownLoaderFactoryTypeName(type));
        if (typeName == nullptr)
            return E_INVALIDARG;

        return InitializePackageLoaderWithTypeName(wrl::HStringReference(typeName).Get());
    }

    auto STDMETHODCALLTYPE LoaderStatics::InitializePackageLoaderWithTypeName(HSTRING const typeName) -> HRESULT
    {
        if (typeName == nullptr)
            return E_INVALIDARG;

        wrl::ComPtr<cxrabi::LoaderFuture> loaderFuture(GetOrCreateGlobalPackageLoader([&]
        {
            return CreateLoaderFuture(typeName);
        }));

        return loaderFuture != nullptr ? S_OK : E_FAIL;
    }

    auto STDMETHODCALLTYPE LoaderStatics::InitializePackageLoaderWithLoader(cxrabi::ILoader* loader) -> HRESULT
    {
        if (loader == nullptr)
            return E_INVALIDARG;

        wrl::ComPtr<cxrabi::LoaderFuture> loaderFuture(GetOrCreateGlobalPackageLoader([&]
        {
            return wrl::Make<cxr::already_completed_async_operation<cxrabi::ILoader>>(loader).Get();
        }));

        return loaderFuture != nullptr ? S_OK : E_FAIL;
    }

    auto STDMETHODCALLTYPE LoaderStatics::InitializePackageLoaderWithLoaderFuture(cxrabi::LoaderFuture* loader) -> HRESULT
    {
        if (loader == nullptr)
            return E_INVALIDARG;

        wrl::ComPtr<cxrabi::LoaderFuture> future(GetOrCreateGlobalPackageLoader([&]
        {
            return loader;
        }));

        return future != nullptr ? S_OK : E_FAIL;
    }

} }
