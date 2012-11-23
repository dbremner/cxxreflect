
//                            Copyright James P. McNellis 2011 - 2012.                            //
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

    CXXREFLECTTEST_DEFINE_TEST(basic_alpha_methodimpl_simple)
    {
        auto const type(cxr::get_type(L"TestComponents.Alpha.SimpleMethodImplTestDerivedClass"));
        auto const methods(type.methods(cxxreflect::metadata::binding_attribute::all_instance));

        bool saw_InterfaceFunctionShouldNotAppear(false);
        bool saw_BaseClassFunctionShouldNotAppear(false);
        bool saw_DerivedClassFunctionShouldAppear(false);

        auto const set_once([&](bool& b) { c.verify(!b); b = true; });

        cxr::for_all(methods, [&](cxr::method const& m) -> void
        {
            if (m.name() == L"InterfaceFunctionShouldNotAppear") set_once(saw_InterfaceFunctionShouldNotAppear);
            if (m.name() == L"BaseClassFunctionShouldNotAppear") set_once(saw_BaseClassFunctionShouldNotAppear);
            if (m.name() == L"DerivedClassFunctionShouldAppear") set_once(saw_DerivedClassFunctionShouldAppear);
        });

        c.verify(!saw_InterfaceFunctionShouldNotAppear);
        c.verify(!saw_BaseClassFunctionShouldNotAppear);
        c.verify(saw_DerivedClassFunctionShouldAppear );
    }

    CXXREFLECTTEST_DEFINE_TEST(basic_alpha_methodimpl_hiding)
    {
        auto const type(cxr::get_type(L"TestComponents.Alpha.HidingMethodImplTestDerivedClass"));
        auto const methods(type.methods(cxxreflect::metadata::binding_attribute::all_instance));

        bool saw_F(false);
        bool saw_G(false);
        bool saw_H(false);

        auto const set_once([&](bool& b) { c.verify(!b); b = true; });

        cxr::for_all(methods, [&](cxr::method const& m) -> void
        {
            if (m.name() == L"F") set_once(saw_F);
            if (m.name() == L"G") set_once(saw_G);
            if (m.name() == L"H") set_once(saw_H);
        });

        c.verify(!saw_F);
        c.verify(!saw_G);
        c.verify(saw_H );
    }

} }
