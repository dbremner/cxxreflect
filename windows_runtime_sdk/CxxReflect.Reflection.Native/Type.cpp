
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Type.hpp"

namespace CxxReflect { namespace Reflection { namespace Native { namespace { 

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

    auto GetTypeProperty(cxr::weak_ref<cxrabi::ILoader>& loader,
                         cxr::type const& type,
                         cxr::type(cxr::type::*f)() const,
                         cxrabi::IType** const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            cxr::type const t((type.*f)());
            if (!t)
                return S_FALSE;

            *value = wrl::Make<Type>(loader.resolve().Get(), t).Detach();
            return S_OK;
        });
    }

} } } }

namespace CxxReflect { namespace Reflection { namespace Native {

    Type::Type(cxrabi::ILoader* const loader, cxr::type const& type)
        : _loader(loader), _type(type)
    {
        cxr::assert_not_null(loader);
        cxr::assert_initialized(type);
    }

    auto STDMETHODCALLTYPE Type::get_IsAbstract(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_abstract, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsArray(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_array, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsByRef(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_by_ref, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsClass(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_class, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsEnum(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_enum, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsGenericParameter(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_generic_parameter, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsGenericType(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_generic_type, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsGenericTypeDefinition(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_generic_type_definition, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsInterface(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_interface, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsPrimitive(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_primitive, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsSealed(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_sealed, value);
    }

    auto STDMETHODCALLTYPE Type::get_IsValueType(boolean* const value) -> HRESULT
    {
        return GetBooleanProperty(_type, &cxr::type::is_value_type, value);
    }

    auto STDMETHODCALLTYPE Type::get_BaseType(cxrabi::IType** const value) -> HRESULT
    {
        return GetTypeProperty(_loader, _type, &cxr::type::base_type, value);
    }

    auto STDMETHODCALLTYPE Type::get_DeclaringType(cxrabi::IType** const value) -> HRESULT
    {
        return GetTypeProperty(_loader, _type, &cxr::type::declaring_type, value);
    }

    auto STDMETHODCALLTYPE Type::get_ElementType(cxrabi::IType** const value) -> HRESULT
    {
        return GetTypeProperty(_loader, _type, &cxr::type::element_type, value);
    }

    auto STDMETHODCALLTYPE Type::get_Namespace(cxrabi::INamespace** const value) -> HRESULT
    {
        return E_NOTIMPL;
    }
        
    auto STDMETHODCALLTYPE Type::get_FullName(HSTRING* const value) -> HRESULT
    {
        return GetStringProperty(_type, &cxr::type::full_name, value);
    }

    auto STDMETHODCALLTYPE Type::get_Name(HSTRING* const value) -> HRESULT
    {
        return GetStringProperty(_type, &cxr::type::simple_name, value);
    }

} } }
