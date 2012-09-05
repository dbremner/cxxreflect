
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_TYPE_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_TYPE_HPP_

#include "Configuration.hpp"

namespace CxxReflect { namespace Reflection { namespace Native {

    class Type : public wrl::RuntimeClass<cxrabi::IType>
    {
        InspectableClass(InterfaceName_CxxReflect_Reflection_INamespace, BaseTrust)

    public:

        Type(cxrabi::ILoader* loader, cxr::type const& type);
        
        virtual auto STDMETHODCALLTYPE get_IsAbstract             (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsArray                (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsByRef                (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsClass                (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsEnum                 (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsGenericParameter     (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsGenericType          (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsGenericTypeDefinition(boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsInterface            (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsPrimitive            (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsSealed               (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsValueType            (boolean* value) -> HRESULT override;

        virtual auto STDMETHODCALLTYPE get_BaseType     (cxrabi::IType** value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_DeclaringType(cxrabi::IType** value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_ElementType  (cxrabi::IType** value) -> HRESULT override;

        virtual auto STDMETHODCALLTYPE get_Namespace(cxrabi::INamespace** value) -> HRESULT override;
        
        virtual auto STDMETHODCALLTYPE get_FullName(HSTRING* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_Name    (HSTRING* value) -> HRESULT override;

    private:

        cxr::weak_ref<cxrabi::ILoader> _loader;
        cxr::type                      _type;
    };

} } }

#endif
