
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

    CXXREFLECTTEST_DEFINE_TEST(alpha_default_interface)
    {
        cxr::type const type_TestClass        (cxr::get_type(L"TestComponents.Alpha.DefaultInterface.TestClass"        ));
        cxr::type const type_TestEnum         (cxr::get_type(L"TestComponents.Alpha.DefaultInterface.TestEnum"         ));
        cxr::type const type_TestStruct       (cxr::get_type(L"TestComponents.Alpha.DefaultInterface.TestStruct"       ));
        cxr::type const type_IDefaultInterface(cxr::get_type(L"TestComponents.Alpha.DefaultInterface.IDefaultInterface"));
        cxr::type const type_IOtherInterface  (cxr::get_type(L"TestComponents.Alpha.DefaultInterface.IOtherInterface"  ));

        c.verify(type_TestClass        .is_initialized());
        c.verify(type_TestEnum         .is_initialized());
        c.verify(type_TestStruct       .is_initialized());
        c.verify(type_IDefaultInterface.is_initialized());
        c.verify(type_IOtherInterface  .is_initialized());

        cxr::type const di_TestClass        (cxr::get_default_interface(type_TestClass        ));
        cxr::type const di_TestEnum         (cxr::get_default_interface(type_TestEnum         ));
        cxr::type const di_TestStruct       (cxr::get_default_interface(type_TestStruct       ));
        cxr::type const di_IDefaultInterface(cxr::get_default_interface(type_IDefaultInterface));
        cxr::type const di_IOtherInterface  (cxr::get_default_interface(type_IOtherInterface  ));

        c.verify_equals(di_TestClass,         type_IDefaultInterface);
        c.verify_equals(di_TestEnum,          cxr::type()           );
        c.verify_equals(di_TestStruct,        cxr::type()           );
        c.verify_equals(di_IDefaultInterface, type_IDefaultInterface);
        c.verify_equals(di_IOtherInterface,   type_IOtherInterface  );
    }

} }
