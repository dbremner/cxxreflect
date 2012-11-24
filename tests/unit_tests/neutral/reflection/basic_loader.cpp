
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

namespace cxr
{
    using namespace cxxreflect;
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
}

namespace cxxreflect_test {

    template <typename Configuration>
    static auto create_test_loader(context const& c, Configuration configuration) -> cxr::loader_root
    {
        cxr::search_path_module_locator::search_path_sequence paths;
        paths.push_back(c.get_property(known_property::test_assemblies_path()));
        paths.push_back(c.get_property(known_property::framework_path()));

        cxr::search_path_module_locator const locator(paths);

        cxr::loader_root root(cxr::create_loader_root(locator, configuration));

        root.get().load_assembly(cxr::module_location(
            c.get_property(known_property::primary_assembly_path()).c_str()));

        return root;
    }

    static auto create_test_loader(context const& c) -> cxr::loader_root
    {
        return create_test_loader(c, cxr::default_loader_configuration());
    }

    static auto load_alpha_assembly(context const& c, cxr::loader_root const& root) -> cxr::assembly
    {
        cxr::module_location const location(
            (c.get_property(known_property::test_assemblies_path()) + L"\\alpha.dll").c_str());

        cxr::assembly const a(root.get().load_assembly(location));
        c.verify(a.is_initialized());
        return a;
    }

    // Verify that we can create a loader and load an assembly.
    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_loader_test)
    {
        cxr::loader_root const root(create_test_loader(c));
        c.verify(root.is_initialized());

        cxr::assembly const a(load_alpha_assembly(c, root));
        c.verify(a.is_initialized());
        c.verify(a.owning_loader() == root.get());

        cxr::type const t(a.find_type(L"", L"QTrivialPublicClass"));
        c.verify(t.is_initialized());
        c.verify_equals(t.namespace_name(), L"");
        c.verify_equals(t.simple_name(), L"QTrivialPublicClass");
    }

    // Verify that loader_configuration::is_filtered_type is respected when we search for types and
    // enumerate types.
    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_loader_configuration_is_filtered_type)
    {
        class test_loader_configuration
            : public cxr::loader_configuration_public_types_filter_policy,
              public cxr::loader_configuration_system_system_namespace_policy
        {
        };

        cxr::loader_root const root(create_test_loader(c, test_loader_configuration()));
        c.verify(root.is_initialized());

        cxr::assembly const a(load_alpha_assembly(c, root));
        c.verify(a.is_initialized());

        cxr::module const m(a.manifest_module());
        c.verify(m.is_initialized());

        // Verify that the known public type is discoverable...
        cxr::type const t0(m.find_type(L"", L"QTrivialPublicClass"));
        c.verify(t0.is_initialized());

        // Verify that the known private type is not discoverable...
        cxr::type const t1(m.find_type(L"", L"QTrivialPrivateClass"));
        c.verify(!t1.is_initialized());

        // Verify that all discoverable types pass the filter check:
        cxr::for_all(m.types(), [&](cxr::type const& tx)
        {
            c.verify(!test_loader_configuration().is_filtered_type(tx.context(cxr::internal_key()).as_token()));
        });
    }

    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_loader_methods)
    {
        cxr::loader_root const root(create_test_loader(c));
        c.verify(root.is_initialized());

        cxr::assembly const a(load_alpha_assembly(c, root));
        c.verify(a.is_initialized());
        c.verify(a.owning_loader() == root.get());

        cxr::type const t(a.find_type(L"", L"QTrivialTypeMethodChecks"));

        auto& context = cxxreflect::reflection::detail::loader_context::from(t.context(cxr::internal_key()).scope());
        auto membership = context.get_membership(t.context(cxr::internal_key()));
        auto range = membership.get_methods();

        for (auto const& m : membership.get_methods())
        {
            cxr::string_reference name = row_from(m->member_token()).name();
        }
    }

}