
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

namespace cxxreflect_test { namespace {

    auto verify_property(context               const& c,
                         cxr::property         const& p,
                         cxr::type             const& expected_declarer,
                         cxr::string_reference const& expected_type,
                         cxr::method           const& expected_get,
                         cxr::method           const& expected_set) -> void
    {
        c.verify(p.is_initialized());
        c.verify_equals(p.declaring_type(), expected_declarer);
        c.verify_equals(p.property_type().simple_name(), expected_type);

        if (expected_get.is_initialized())
        {
            c.verify(p.get_method().is_initialized());
            c.verify_equals(p.get_method(), expected_get);
            c.verify(p.can_read());
        }
        else
        {
            c.verify(!p.get_method().is_initialized());
            c.verify(!p.can_read());
        }

        if (expected_set.is_initialized())
        {
            c.verify(p.set_method().is_initialized());
            c.verify_equals(p.set_method(), expected_set);
            c.verify(p.can_write());
        }
        else
        {
            c.verify(!p.set_method().is_initialized());
            c.verify(!p.can_write());
        }
    }

    auto find_method(cxr::type const& reflected_type, cxr::type const& declaring_type, cxr::string_reference const& method_name)
        -> cxr::method
    {
        auto const methods(reflected_type.methods(cxr::binding_attribute::all_instance));
        auto const it(cxr::find_if(methods, [&](cxr::method const& m)
        {
            return m.name() == method_name && m.declaring_type() == declaring_type;
        }));

        if (it == end(methods))
            throw test_error(L"unexpectedly could not find method");

        return *it;
    }

    auto toggle(context const& c, bool& b) -> void
    {
        c.verify(!b);
        b = true;
    }

} }

namespace cxxreflect_test {

    CXXREFLECTTEST_DEFINE_BETA_TEST(reflection_basic_beta_membership_properties_simple)
    {
        cxr::type const t(beta.find_type(L"", L"MPropertySimple"));
        c.verify(t.is_initialized());

        auto const& properties(t.properties(cxr::binding_attribute::all_instance));

        bool saw_RW(false), saw_R(false), saw_W(false);

        cxr::for_all(properties, [&](cxr::property const& p) -> void
        {
            c.verify(p.is_initialized());
            c.verify_equals(p.declaring_type(), t);
            c.verify_equals(p.reflected_type(), t);

            if (p.name() == L"RW")
            {
                toggle(c, saw_RW);
                verify_property(c, p, t, L"Int32", 
                    t.find_method(L"get_RW", cxr::binding_attribute::all_instance),
                    t.find_method(L"set_RW", cxr::binding_attribute::all_instance));
            }
            else if (p.name() == L"R")
            {
                toggle(c, saw_R);
                verify_property(c, p, t, L"Int32", 
                    t.find_method(L"get_R", cxr::binding_attribute::all_instance),
                    cxr::method());
            }
            else if (p.name() == L"W")
            {
                toggle(c, saw_W);
                verify_property(c, p, t, L"Int32", 
                    cxr::method(),
                    t.find_method(L"set_W", cxr::binding_attribute::all_instance));
            }
            else
            {
                c.fail(L"encountered unexpected property");
            }
        });

        c.verify(saw_RW && saw_R && saw_W);
    }

    CXXREFLECTTEST_DEFINE_BETA_TEST(reflection_basic_beta_membership_properties_derived)
    {
        // Simple derived (hidden) property check
        {
            cxr::type const bt(beta.find_type(L"", L"MPropertySimpleBase"));
            cxr::type const dt(beta.find_type(L"", L"MPropertySimpleDerived"));
            c.verify(bt.is_initialized());
            c.verify(dt.is_initialized());

            bool saw_BP(false), saw_DP(false);

            cxr::for_all(dt.properties(cxr::binding_attribute::all_instance), [&](cxr::property const& p) -> void
            {
                c.verify(p.is_initialized());
                c.verify_equals(p.reflected_type(), dt);

                if (p.name() == L"P" && p.declaring_type() == bt)
                {
                    toggle(c, saw_BP);
                    verify_property(c, p, bt, L"Int32",
                        find_method(dt, bt, L"get_P"),
                        find_method(dt, bt, L"set_P"));
                }
                else if (p.name() == L"P" && p.declaring_type() == dt)
                {
                    toggle(c, saw_DP);
                    verify_property(c, p, dt, L"Int32",
                        find_method(dt, dt, L"get_P"),
                        find_method(dt, dt, L"set_P"));
                }
                else c.fail(L"encountered unexpected property");
            });

            c.verify(saw_BP && saw_DP);
        }

        // Virtual derived (overridden) property check
        {
            cxr::type const t(beta.find_type(L"", L"MPropertyVirtualDerived"));
            c.verify(t.is_initialized());

            bool saw_P(false);

            std::vector<cxr::property> ps(begin(t.properties(cxr::binding_attribute::all_instance)), end(t.properties(cxr::binding_attribute::all_instance)));

            cxr::for_all(t.properties(cxr::binding_attribute::all_instance), [&](cxr::property const& p) -> void
            {
                c.verify(p.is_initialized());
                c.verify_equals(p.reflected_type(), t);

                if (p.name() == L"P" && p.declaring_type() == t)
                {
                    toggle(c, saw_P);
                    verify_property(c, p, t, L"Int32",
                        find_method(t, t, L"get_P"),
                        find_method(t, t, L"set_P"));
                }
                else c.fail(L"encountered unexpected property");
            });

            c.verify(saw_P);
        }
    }

}