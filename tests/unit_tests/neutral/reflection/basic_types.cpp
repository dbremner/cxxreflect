
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

#define CXXREFLECTTEST_DEFINE_BETA_TEST(name)                                                                             \
    auto CXXREFLECTTEST_CONCATENATE(name, _)(context const& c, cxr::loader root, cxr::assembly beta) -> void;             \
    auto name(context const& c) -> void                                                                                   \
    {                                                                                                                     \
        cxr::loader_root const root(create_test_loader(c));                                                               \
                                                                                                                          \
        cxr::assembly const beta(load_beta_assembly(c, root));                                                            \
        c.verify(beta.is_initialized());                                                                                  \
        CXXREFLECTTEST_CONCATENATE(name, _)(c, root.get(), beta);                                                         \
    }                                                                                                                     \
    CXXREFLECTTEST_REGISTER(name);                                                                                        \
    auto CXXREFLECTTEST_CONCATENATE(name, _)(context const& c, cxr::loader const root, cxr::assembly const beta) -> void

namespace cxxreflect_test {

    static auto create_test_loader(context const& c) -> cxr::loader_root
    {
        cxr::search_path_module_locator::search_path_sequence paths;
        paths.push_back(c.get_property(known_property::test_assemblies_path()));
        paths.push_back(c.get_property(known_property::framework_path()));

        cxr::search_path_module_locator const locator(paths);

        cxr::loader_root root(cxr::create_loader_root(locator, cxr::default_loader_configuration()));

        root.get().load_assembly(cxr::module_location(
            c.get_property(known_property::primary_assembly_path()).c_str()));

        return root;
    }

    static auto load_beta_assembly(context const& c, cxr::loader_root const& root) -> cxr::assembly
    {
        cxr::module_location const location(
            (c.get_property(known_property::test_assemblies_path()) + L"\\beta.dll").c_str());

        cxr::assembly const a(root.get().load_assembly(location));
        c.verify(a.is_initialized());
        return a;
    }

    CXXREFLECTTEST_DEFINE_BETA_TEST(aggregation_beta_types_enumerate_types)
    {
        c.verify(beta.find_type(L"", L"ZArgument").is_initialized());
        c.verify(beta.find_type(L"", L"ZReturn").is_initialized());
        c.verify(beta.find_type(L"", L"ZBaseInterface").is_initialized());
        c.verify(beta.find_type(L"", L"ZBase").is_initialized());
    }

}