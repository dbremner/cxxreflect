
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_LOADER_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_LOADER_HPP_

#include "Configuration.hpp"

namespace cxxreflect { namespace windows_runtime_sdk {

    class RuntimeLoader : public wrl::RuntimeClass<abi::ILoader>
    {
        InspectableClass(InterfaceName_CxxReflect_Reflection_ILoader, BaseTrust)

    public:

        RuntimeLoader(std::unique_ptr<cxr::package_loader> loader);

        virtual auto STDMETHODCALLTYPE FindNamespace(HSTRING namespaceName, abi::INamespace** value) -> HRESULT override;

        virtual auto STDMETHODCALLTYPE FindType(HSTRING fullName, abi::IType** value) -> HRESULT override;

        auto GetPackageLoader() const -> cxr::package_loader const&;

        auto GetOrCreateNamespace(cxr::string_reference const& namespaceName) -> wrl::ComPtr<RuntimeNamespace> const&;
        auto GetOrCreateType     (cxr::type             const& type         ) -> wrl::ComPtr<RuntimeType>      const&;

    private:

        std::unique_ptr<cxr::package_loader>                    _loader;

        // These are temporary; we intend to replace these with a low-latency, low-cost, lock-free
        // object pool once we have the prototype of the SDK completed.
        cxr::recursive_mutex                                    _sync;
        std::map<cxr::string, wrl::ComPtr<RuntimeNamespace>>    _namespaces;
        std::map<cxr::type,   wrl::ComPtr<RuntimeType>>         _types;
    };

} }

#endif
