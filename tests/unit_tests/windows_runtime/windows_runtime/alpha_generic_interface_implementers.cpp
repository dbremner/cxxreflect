
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

    CXXREFLECTTEST_DEFINE_TEST(alpha_generic_interface_tests_iterable_implementer)
    {
        {
            cxr::type const t(cxr::get_type(L"TestComponents.Alpha.GenericInterfaceImplementations.IterableObject"));
            c.verify(t.is_initialized());

            // Find the generic interface and examine it:
            bool saw_iiterable(false);
            cxr::for_all(t.interfaces(), [&](cxr::type const& i)
            {
                i.methods(cxr::binding_attribute::all_instance);
                if (i.primary_name() == L"IIterable`1")
                {
                    c.verify(!saw_iiterable);
                    c.verify_equals(cxr::distance(i.generic_arguments()), 1u);
                    c.verify_equals((*i.generic_arguments().begin()).full_name(), L"Platform.Object");
                    saw_iiterable = true;
                }

                t.methods(cxr::binding_attribute::all_instance);
            });
            c.verify(saw_iiterable);

            // Verify that we can realize the methods without asserting:
            t.methods(cxr::binding_attribute::all_instance);
        }

        {
            cxr::type const t(cxr::get_type(L"TestComponents.Alpha.GenericInterfaceImplementations.IIterablePair"));
            c.verify(t.is_initialized());

            cxr::for_all(t.interfaces(), [&](cxr::type const& i)
            {
                i.methods(cxr::binding_attribute::all_instance);
                if (i.is_generic_type_instantiation())
                    cxr::for_all(i.generic_arguments(), [&](cxr::type const&) { });
            });
            t.methods(cxr::binding_attribute::all_instance);
        }

        {
            cxr::type const t(cxr::get_type(L"TestComponents.Alpha.GenericInterfaceImplementations.VectorViewObject"));
            c.verify(t.is_initialized());

            cxr::for_all(t.interfaces(), [&](cxr::type const& i)
            {
                i.methods(cxr::binding_attribute::all_instance);
                if (i.is_generic_type_instantiation())
                    cxr::for_all(i.generic_arguments(), [&](cxr::type const&) { });
            });
            t.methods(cxr::binding_attribute::all_instance);
        }
    }

} }
