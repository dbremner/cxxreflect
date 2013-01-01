
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

namespace cxxreflect_test {

    CXXREFLECTTEST_DEFINE_BETA_TEST(reflection_basic_beta_membership_properties_simple)
    {
        cxr::type const t(beta.find_type(L"", L"MPropertySimple"));
        c.verify(t.is_initialized());

        auto const& properties(t.properties(cxr::binding_attribute::all_instance));

        bool saw_RW(false);
        bool saw_R (false);
        bool saw_W (false);

        cxr::for_all(properties, [&](cxr::property const& p) -> void
        {
            c.verify(p.is_initialized());
            c.verify_equals(p.declaring_type(), t);
            c.verify_equals(p.declaring_type(), t);

            if (p.name() == L"RW")
            {
                c.verify(!saw_RW);
                saw_RW = true;

                cxr::type const pt(p.property_type());
                c.verify(pt.is_initialized());
                c.verify_equals(pt.simple_name(), L"Int32");

                cxr::method const get(p.get_method());
                cxr::method const set(p.set_method());

                c.verify(get.is_initialized());
                c.verify(set.is_initialized());

                c.verify_equals(get, t.find_method(L"get_RW", cxr::binding_attribute::all_instance));
                c.verify_equals(set, t.find_method(L"set_RW", cxr::binding_attribute::all_instance));
            }
            else if (p.name() == L"R")
            {
                c.verify(!saw_R);
                saw_R = true;

                cxr::type const pt(p.property_type());
                c.verify(pt.is_initialized());
                c.verify_equals(pt.simple_name(), L"Int32");

                cxr::method const get(p.get_method());
                cxr::method const set(p.set_method());

                c.verify(get.is_initialized());
                c.verify(!set.is_initialized());

                c.verify_equals(get, t.find_method(L"get_R", cxr::binding_attribute::all_instance));
            }
            else if (p.name() == L"W")
            {
                c.verify(!saw_W);
                saw_W = true;

                cxr::type const pt(p.property_type());
                c.verify(pt.is_initialized());
                c.verify_equals(pt.simple_name(), L"Int32");

                cxr::method const get(p.get_method());
                cxr::method const set(p.set_method());

                c.verify(!get.is_initialized());
                c.verify(set.is_initialized());

                c.verify_equals(set, t.find_method(L"set_W", cxr::binding_attribute::all_instance));
            }
            else
            {
                c.fail(L"encountered unexpected property");
            }
        });

        c.verify(saw_RW);
        c.verify(saw_R);
        c.verify(saw_W);
    }

}