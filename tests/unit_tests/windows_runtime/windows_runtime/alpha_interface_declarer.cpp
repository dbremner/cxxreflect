
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/windows_runtime/precompiled_headers.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
    using namespace cxxreflect::windows_runtime;
}

namespace co
{
    using namespace TestComponents::Alpha;
}

namespace cxxreflect_test { namespace unit_tests_windows_runtime {

    CXXREFLECTTEST_DEFINE_TEST(alpha_interface_declarer)
    {
        cxr::type const type_C (cxr::get_type(L"TestComponents.Alpha.InterfaceDeclarer.C" ));
        cxr::type const type_IF(cxr::get_type(L"TestComponents.Alpha.InterfaceDeclarer.IF"));
        cxr::type const type_IG(cxr::get_type(L"TestComponents.Alpha.InterfaceDeclarer.IG"));

        c.verify(type_C .is_initialized());
        c.verify(type_IF.is_initialized());
        c.verify(type_IG.is_initialized());

        cxr::method const method_C_F0(type_C.find_method(L"F0", cxr::binding_attribute::all_instance));
        cxr::method const method_C_F1(type_C.find_method(L"F1", cxr::binding_attribute::all_instance));
        cxr::method const method_C_F2(type_C.find_method(L"F2", cxr::binding_attribute::all_instance));
        cxr::method const method_C_G0(type_C.find_method(L"G0", cxr::binding_attribute::all_instance));

        c.verify(method_C_F0.is_initialized());
        c.verify(method_C_F1.is_initialized());
        c.verify(method_C_F2.is_initialized());
        c.verify(method_C_G0.is_initialized());

        cxr::method const method_IF_F0(type_IF.find_method(L"F0", cxr::binding_attribute::all_instance));
        cxr::method const method_IF_F1(type_IF.find_method(L"F1", cxr::binding_attribute::all_instance));
        cxr::method const method_IF_F2(type_IF.find_method(L"F2", cxr::binding_attribute::all_instance));

        c.verify(method_IF_F0.is_initialized());
        c.verify(method_IF_F1.is_initialized());
        c.verify(method_IF_F2.is_initialized());

        cxr::method const method_IG_G0(type_IG.find_method(L"G0", cxr::binding_attribute::all_instance));
        
        c.verify(method_IG_G0.is_initialized());

        c.verify_equals(cxr::get_interface_declarer(method_C_F0), method_IF_F0);
        c.verify_equals(cxr::get_interface_declarer(method_C_F1), method_IF_F1);
        c.verify_equals(cxr::get_interface_declarer(method_C_F2), method_IF_F2);
        c.verify_equals(cxr::get_interface_declarer(method_C_G0), method_IG_G0);

        c.verify_equals(cxr::get_interface_declarer(method_IF_F0), method_IF_F0);
        c.verify_equals(cxr::get_interface_declarer(method_IF_F1), method_IF_F1);
        c.verify_equals(cxr::get_interface_declarer(method_IF_F2), method_IF_F2);

        c.verify_equals(cxr::get_interface_declarer(method_IG_G0), method_IG_G0);
    }

} }
