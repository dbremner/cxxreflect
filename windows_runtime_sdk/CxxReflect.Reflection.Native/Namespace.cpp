
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Collections.hpp"
#include "Loader.hpp"
#include "Namespace.hpp"
#include "Type.hpp"

namespace cxxreflect { namespace windows_runtime_sdk {

    RuntimeNamespace::RuntimeNamespace(RuntimeLoader*          const  loader,
                                       cxr::assembly           const& assembly,
                                       cxr::module::type_range const& types,
                                       cxr::string_reference   const& name)
        : _loader(loader), _assembly(assembly), _name(name.c_str())
    {
        cxr::assert_not_null(loader);

        WeakRuntimeLoaderRef const weak_loader(loader);
        _types = wrl::Make<PublicTypeIterator>(
            InternalTypeIterator(weak_loader, begin(types)),
            InternalTypeIterator(weak_loader, end(types)));
    }

    auto STDMETHODCALLTYPE RuntimeNamespace::get_Loader(abi::ILoader** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            *value = _loader.resolve().Detach();
            return S_OK;
        });
    }

    auto STDMETHODCALLTYPE RuntimeNamespace::get_MetadataFile(HSTRING* const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            *value = cxr::smart_hstring(_assembly.location().c_str()).release();
            return S_OK;
        });
    }

    auto STDMETHODCALLTYPE RuntimeNamespace::get_Name(HSTRING* const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            *value = cxr::smart_hstring(_name).release();
            return S_OK;
        });
    }

    auto STDMETHODCALLTYPE RuntimeNamespace::get_Types(abi::TypeVectorView** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            *value = cxr::clone_for_return(wrl::ComPtr<PublicTypeIterator>(_types));
            return S_OK;
        });
    }

    auto RuntimeNamespace::ConstructType::operator()(WeakRuntimeLoaderRef       const& loader,
                                                     cxr::module::type_iterator const& current) const -> abi::IType*
    {
        wrl::ComPtr<RuntimeLoader> const resolved_loader(loader.resolve());
        if (resolved_loader == nullptr)
            throw cxr::logic_error(L"loader was unexpectedly destroyed");

        return cxr::clone_for_return(loader.resolve()->GetOrCreateType(*current));
    }

} }
