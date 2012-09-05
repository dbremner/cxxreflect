
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_NAMESPACE_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_NAMESPACE_HPP_

#include "Configuration.hpp"

namespace CxxReflect { namespace Reflection { namespace Native {

    class Namespace : public wrl::RuntimeClass<cxrabi::INamespace>
    {
        InspectableClass(InterfaceName_CxxReflect_Reflection_INamespace, BaseTrust)

    public:

        Namespace(cxrabi::ILoader* loader, cxr::assembly const& assembly, cxr::string_reference const& name);
        
        virtual auto STDMETHODCALLTYPE get_MetadataFile(HSTRING* value) -> HRESULT override;
        virtual auto STDMETHODCALLTYPE get_Name(HSTRING* value) -> HRESULT override;

    private:

        cxr::weak_ref<cxrabi::ILoader> _loader;
        cxr::assembly                  _assembly;
        cxr::smart_hstring             _name;
    };

} } }

#endif
