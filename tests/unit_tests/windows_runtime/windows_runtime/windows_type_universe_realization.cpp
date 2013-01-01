
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

namespace {

    typedef std::set<cxr::string> visited_types_set;

    auto visit(visited_types_set& v, cxr::constant         const& c) -> void;
    auto visit(visited_types_set& v, cxr::custom_attribute const& a) -> void;
    auto visit(visited_types_set& v, cxr::method           const& m) -> void;
    auto visit(visited_types_set& v, cxr::parameter        const& p) -> void;
    auto visit(visited_types_set& v, cxr::type             const& t) -> void;

    auto visit(visited_types_set& v, cxr::constant const& c) -> void
    {
        if (!c.is_initialized())
            return;

        c.get_kind();
    }

    auto visit(visited_types_set& v, cxr::custom_attribute const& a) -> void
    {
        if (!a.is_initialized())
            return;

        visit(v, a.constructor());
    }

    auto visit(visited_types_set& v, cxr::method const& m) -> void
    {
        if (!m.is_initialized())
            return;

        m.attributes();
        m.calling_convention();
        // NYI m.contains_generic_parameters();
        m.context(cxr::internal_key());
        
        cxr::for_all(m.custom_attributes(), [&](cxr::custom_attribute const& a) { visit(v, a); });

        m.declaring_module();
        visit(v, m.declaring_type());

        m.is_abstract();
        m.is_assembly();
        m.is_constructor();
        m.is_family();
        m.is_family_and_assembly();
        m.is_family_or_assembly();
        m.is_final();
        m.is_generic_method();
        m.is_generic_method_definition();
        m.is_hide_by_signature();
        m.is_private();
        m.is_public();
        m.is_special_name();
        m.is_static();
        m.is_virtual();

        m.metadata_token();
        m.name();

        cxr::for_all(m.parameters(), [&](cxr::parameter const& p) { visit(v, p); });

        m.parameter_count();

        visit(v, m.reflected_type());

        visit(v, m.return_parameter());
        visit(v, m.return_type());
    }
    
    auto visit(visited_types_set& v, cxr::parameter const& p) -> void
    {
        if (!p.is_initialized())
            return;

        p.attributes();
        cxr::for_all(p.custom_attributes(), [&](cxr::custom_attribute const& a) { visit(v, a); });
        p.declaring_method();
        visit(v, p.default_value());
        p.is_in();
        // NYI p.is_lcid();
        p.is_optional();
        p.is_out();
        // NYI p.is_ret_val();
        p.metadata_token();
        p.name();
        visit(v, p.parameter_type());
        p.position();
    }

    auto visit(visited_types_set& v, cxr::type const& t) -> void
    {
        if (!t.is_initialized() || v.count(t.assembly_qualified_name()) != 0)
            return;

        v.insert(t.assembly_qualified_name());

        t.attributes();
        visit(v, t.base_type());
        
        cxr::for_all(t.constructors(cxr::binding_attribute::all_instance), [&](cxr::method const& m) { visit(v, m); });
        cxr::for_all(t.custom_attributes(), [&](cxr::custom_attribute const& a) { visit(v, a); });
        visit(v, t.declaring_type());
        t.defining_assembly();
        t.defining_module();
        visit(v, t.element_type());
        // NYI t.events()
        // NYI t.fields()
        t.full_name();
        cxr::for_all(t.interfaces(), [&](cxr::type const& i) { visit(v, i); });
        t.is_abstract();
        t.is_array();
        t.is_by_ref();
        t.is_class();
        t.is_com_object();
        t.is_contextful();
        t.is_enum();
        t.is_generic_parameter();
        t.is_generic_type();
        t.is_generic_type_definition();
        t.is_generic_type_instantiation();
        t.is_import();
        t.is_interface();
        t.is_marshal_by_ref();
        t.is_nested();
        t.is_pointer();
        t.is_primitive();
        t.is_sealed();
        t.is_serializable();
        t.is_special_name();
        t.is_value_type();
        t.is_visible();
        t.layout();
        t.metadata_token();
        cxr::for_all(t.methods(), [&](cxr::method const& m) { visit(v, m); });
        t.namespace_name();
        cxr::for_all(t.optional_custom_modifiers(), [&](cxr::type const& m) { visit(v, m); });
        t.primary_name();
        // NYI t.properties()
        cxr::for_all(t.required_custom_modifiers(), [&](cxr::type const& m) { visit(v, m); });
        t.simple_name();
        t.string_format();
        t.visibility();
    }

}

namespace cxxreflect_test { namespace unit_tests_windows_runtime {

    CXXREFLECTTEST_DEFINE_TEST(windows_type_universe_realization)
    {
        auto const& root(cxr::global_package_loader::get().loader());

        cxr::package_module_locator::path_map const assemblies(cxr::global_package_loader::get().locator().metadata_files());

        visited_types_set visited_types;
        cxr::for_all(assemblies, [&](std::pair<cxr::string const, cxr::string> const& name_path_pair)
        {
            if (!cxr::starts_with(name_path_pair.first.c_str(), L"windows"))
                return;

            cxr::assembly const& a(root.load_assembly(cxr::module_location(name_path_pair.second.c_str())));

            cxr::for_all(a.types(), [&](cxr::type const& t) { visit(visited_types, t); });
        });
    }

} }
