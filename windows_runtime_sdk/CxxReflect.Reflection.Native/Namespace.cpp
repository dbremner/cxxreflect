
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Namespace.hpp"

namespace CxxReflect { namespace Reflection { namespace Native {

    Namespace::Namespace(cxrabi::ILoader* const loader, cxr::assembly const& assembly, cxr::string_reference const& name)
        : _loader(loader), _assembly(assembly), _name(name.c_str())
    {
        cxr::assert_initialized(assembly);
        cxr::assert_not_null(name.c_str());
    }

    auto STDMETHODCALLTYPE Namespace::get_MetadataFile(HSTRING* const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            *value = cxr::smart_hstring(_assembly.location().c_str()).release();
            return S_OK;
        });
    }

    auto STDMETHODCALLTYPE Namespace::get_Name(HSTRING* const value) -> HRESULT
    {
        return cxr::call_with_runtime_convention([&]() -> HRESULT
        {
            cxr::throw_if_null_and_initialize_out_parameter(value);

            *value = cxr::smart_hstring(_name).release();
            return S_OK;
        });
    }

} } }
