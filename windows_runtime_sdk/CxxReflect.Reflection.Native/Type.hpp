
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_TYPE_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_TYPE_HPP_

#include "Configuration.hpp"
#include "Collections.hpp"

namespace cxxreflect { namespace windows_runtime_sdk {

    class RuntimeType final : public wrl::RuntimeClass<abi::IType>
    {
        InspectableClass(InterfaceName_CxxReflect_Reflection_INamespace, BaseTrust)

    public:

        RuntimeType(RuntimeLoader* loader, cxr::type const& type);
        
        virtual auto STDMETHODCALLTYPE get_IsAbstract (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsArray    (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsByRef    (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsClass    (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsEnum     (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsInterface(boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsPrimitive(boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsSealed   (boolean* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsValueType(boolean* value) -> HRESULT override;

        virtual auto STDMETHODCALLTYPE get_IsGenericType             (boolean*              value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsGenericTypeDefinition   (boolean*              value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsGenericTypeInstantiation(boolean*              value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_IsGenericTypeParameter    (boolean*              value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_GenericTypeArguments      (abi::TypeVectorView** value) -> HRESULT override;

        virtual auto STDMETHODCALLTYPE get_BaseType     (abi::IType** value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_DeclaringType(abi::IType** value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_ElementType  (abi::IType** value) -> HRESULT override;

        virtual auto STDMETHODCALLTYPE get_Namespace(abi::INamespace** value) -> HRESULT override;
        
        virtual auto STDMETHODCALLTYPE get_FullName(HSTRING* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_Name    (HSTRING* value) -> HRESULT override;

    private:

        struct ConstructGenericTypeArgumentType
        {
            auto operator()(WeakRuntimeLoaderRef const& loader, cxr::type::generic_argument_iterator const& current) const
                -> abi::IType*;
        };

        typedef cxr::instantiating_iterator<
            cxr::type::generic_argument_iterator,
            abi::IType*,
            WeakRuntimeLoaderRef,
            ConstructGenericTypeArgumentType,
            cxr::identity_transformer,
            std::forward_iterator_tag
        > InternalGenericTypeArgumentIterator;

        typedef RuntimeVectorView<InternalGenericTypeArgumentIterator> PublicGenericTypeArgumentIterator;

        cxr::weak_ref<abi::ILoader, RuntimeLoader> _loader;
        cxr::type                                  _type;
    };

} }

#endif
