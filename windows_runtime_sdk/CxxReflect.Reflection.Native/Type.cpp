
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Loader.hpp"
#include "Namespace.hpp"
#include "Type.hpp"

namespace cxxreflect { namespace windows_runtime_sdk { namespace { 

    auto GetBooleanProperty(cxr::type const& type, bool (cxr::type::*f)() const, boolean* value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            *value = (type.*f)();
            return S_OK;
        });
    }

    template <typename String>
    auto GetStringProperty(cxr::type const& type, String(cxr::type::*f)() const, HSTRING* const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            String const& s((type.*f)());
            return ::WindowsCreateString(s.c_str(), cxr::convert_integer(s.size()), value);
        });
    }

    template <typename ResultType>
    auto GetTypeProperty(cxr::weak_ref<abi::ILoader, RuntimeLoader>& loader,
                         cxr::type const& type,
                         ResultType(cxr::type::*f)() const,
                         abi::IType** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            cxr::type const t((type.*f)());
            if (!t)
                return S_FALSE;

            *value = cxr::clone_for_return(wrl::Make<RuntimeType>(loader.resolve().Get(), t));
            return S_OK;
        });
    }

} } }

namespace cxxreflect { namespace windows_runtime_sdk {

    RuntimeType::RuntimeType(RuntimeLoader* const loader, cxr::type const& type)
        : _loader(loader), _type(type)
    {
        cxr::assert_not_null(loader);
        cxr::assert_initialized(type);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsAbstract(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_abstract, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsArray(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_array, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsByRef(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_by_ref, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsClass(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_class, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsEnum(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_enum, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsGenericType(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_generic_type, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsGenericTypeDefinition(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_generic_type_definition, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsGenericTypeInstantiation(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_generic_type_instantiation, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsGenericTypeParameter(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_generic_parameter, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_GenericTypeArguments(abi::TypeVectorView** value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            auto const arguments(_type.generic_arguments());
            *value = cxr::clone_for_return(wrl::Make<PublicGenericTypeArgumentIterator>(
                InternalGenericTypeArgumentIterator(_loader, begin(arguments)),
                InternalGenericTypeArgumentIterator(_loader, end(arguments))));
            return S_OK;
        });
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsInterface(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_interface, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsPrimitive(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_primitive, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsSealed(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_sealed, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_IsValueType(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_value_type, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_BaseType(abi::IType** const value) -> HRESULT
    {
        return GetTypeProperty(_loader, _type, &cxr::type::base_type, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_DeclaringType(abi::IType** const value) -> HRESULT
    {
        return GetTypeProperty(_loader, _type, &cxr::type::declaring_type, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_ElementType(abi::IType** const value) -> HRESULT
    {
        return GetTypeProperty(_loader, _type, &cxr::type::element_type, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_Namespace(abi::INamespace** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            *value = cxr::clone_for_return(_loader.resolve()->GetOrCreateNamespace(_type.namespace_name()));
            return S_OK;
        });
    }
        
    auto STDMETHODCALLTYPE RuntimeType::get_FullName(HSTRING* const value) -> HRESULT
    {
        return GetStringProperty(_type, &cxr::type::full_name, value);
    }

    auto STDMETHODCALLTYPE RuntimeType::get_Name(HSTRING* const value) -> HRESULT
    {
        return GetStringProperty(_type, &cxr::type::simple_name, value);
    }

    auto RuntimeType::ConstructGenericTypeArgumentType::operator()(WeakRuntimeLoaderRef                 const& loader,
                                                                   cxr::type::generic_argument_iterator const& current) const -> abi::IType*
    {
        wrl::ComPtr<RuntimeLoader> const resolved_loader(loader.resolve());
        if (resolved_loader == nullptr)
            throw cxr::logic_error(L"loader was unexpectedly destroyed");

        return cxr::clone_for_return(loader.resolve()->GetOrCreateType(*current));
    }

} }
