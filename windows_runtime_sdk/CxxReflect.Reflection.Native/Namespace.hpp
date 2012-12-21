
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_NAMESPACE_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_NAMESPACE_HPP_

#include "Configuration.hpp"
#include "Collections.hpp"

namespace cxxreflect { namespace windows_runtime_sdk {

    class RuntimeNamespace final : public wrl::RuntimeClass<abi::INamespace>
    {
        InspectableClass(InterfaceName_CxxReflect_Reflection_INamespace, BaseTrust)

    public:

        RuntimeNamespace(RuntimeLoader*                 loader,
                         cxr::assembly           const& assembly,
                         cxr::module::type_range const& types,
                         cxr::string_reference   const& name);
        
        virtual auto STDMETHODCALLTYPE get_Loader(abi::ILoader** value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_MetadataFile(HSTRING* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_Name(HSTRING* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_Types(abi::TypeVectorView** value) -> HRESULT override;

    private:

        struct ConstructType
        {
            auto operator()(WeakRuntimeLoaderRef const& loader, cxr::module::type_iterator const& current) const -> abi::IType*;
        };

        typedef cxr::instantiating_iterator<
            cxr::module::type_iterator,
            abi::IType*,
            WeakRuntimeLoaderRef,
            ConstructType
        > InternalTypeIterator;

        typedef RuntimeVectorView<InternalTypeIterator> PublicTypeIterator;

        WeakRuntimeLoaderRef            _loader;
        cxr::assembly                   _assembly;
        cxr::smart_hstring              _name;
        wrl::ComPtr<PublicTypeIterator> _types;
    };

} }

#endif
