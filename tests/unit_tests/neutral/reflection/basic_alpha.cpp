
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
}

namespace cxxreflect_test {

    static auto create_test_loader(context const& c) -> cxr::loader_root
    {
        cxr::search_path_module_locator::search_path_sequence directories;
        directories.push_back(c.get_property(known_property::framework_path()));
        directories.push_back(c.get_property(known_property::test_assemblies_path()));

        cxr::search_path_module_locator const locator(directories);

        cxr::loader_root root(cxr::create_loader_root(locator, cxr::default_loader_configuration()));

        root.get().load_assembly(cxr::module_location(c.get_property(known_property::primary_assembly_path()).c_str()));

        return root;
    }

    static auto load_alpha_assembly(context const& c, cxr::loader_root const& root) -> cxr::assembly
    {
        cxr::module_location const location(
            (c.get_property(known_property::test_assemblies_path()) + L"\\alpha.dll").c_str());

        cxr::assembly const a(root.get().load_assembly(location));
        c.verify(a.is_initialized());
        return a;
    }

    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_alpha_assembly_and_modules)
    {
        cxr::loader_root const root(create_test_loader(c));
        cxr::assembly    const a(load_alpha_assembly(c, root));

        cxr::assembly_name const name(a.name());
        c.verify_equals(name.simple_name(), L"alpha");
        c.verify_equals(name.version().major(), 1);
        c.verify_equals(name.version().minor(), 2);
        c.verify_equals(name.version().build(), 3);
        c.verify_equals(name.version().revision(), 4);
        // TODO Other name fields?

        c.verify_equals(a.referenced_assembly_names().size(), 2u);

        bool found_mscorlib(false);
        bool found_nonexistent(false);

        cxr::for_all(a.referenced_assembly_names(), [&](cxr::assembly_name const& name)
        {
            if (name.simple_name() == L"mscorlib")
            {
                c.verify(!found_mscorlib);
                found_mscorlib = true;
                c.verify_equals(name.version().major(), 4);
                c.verify_equals(name.version().minor(), 0);
                c.verify_equals(name.version().build(), 0);
                c.verify_equals(name.version().revision(), 0);
            }
            else if (name.simple_name() == L"nonexistent")
            {
                c.verify(!found_nonexistent);
                found_nonexistent = true;
                c.verify_equals(name.version().major(), 1);
                c.verify_equals(name.version().minor(), 2);
                c.verify_equals(name.version().build(), 3);
                c.verify_equals(name.version().revision(), 4);
            }
            else
            {
                c.fail(L"unexpected referenced assembly");
            }
        });

        c.verify(found_mscorlib);
        c.verify(found_nonexistent);

        // cxr::public_key_token expected_token = { 0xb7, 0x7a, 0x5c, 0x56, 0x19, 0x34, 0xe0, 0x89 };
        // c.verify_equals(mscorlib_name.public_key_token(), expected_token);

        // TODO Module verification
    }

    



    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_alpha_type_visibility_accessibility)
    {
        cxr::loader_root const root(create_test_loader(c));
        cxr::assembly    const a(load_alpha_assembly(c, root));

        auto verify_visibility([&](cxr::type const& t, cxr::type_visibility const expected)
        {
            c.verify(t.is_initialized());
            c.verify_equals(t.visibility(), expected);
        });

        verify_visibility(a.find_type(L"", L"QTrivialPrivateClass"), cxr::type_visibility::not_public);
        verify_visibility(a.find_type(L"", L"QTrivialPublicClass"),  cxr::type_visibility::public_);

        // TODO find_type needs support for nested types (with +)
    }





    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_alpha_type_layout)
    {
        cxr::loader_root const root(create_test_loader(c));
        cxr::assembly    const a(load_alpha_assembly(c, root));

        auto verify_layout([&](cxr::type const& t, cxr::type_layout const expected)
        {
            c.verify(t.is_initialized());
            c.verify_equals(t.layout(), expected);
        });

        verify_layout(a.find_type(L"", L"QTrivialAutoClass"),        cxr::type_layout::auto_layout);
        verify_layout(a.find_type(L"", L"QTrivialExplicitClass"),    cxr::type_layout::explicit_layout);
        verify_layout(a.find_type(L"", L"QTrivialSequentialClass"),  cxr::type_layout::sequential_layout);
    }





    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_alpha_type_semantics)
    {
        cxr::loader_root const root(create_test_loader(c));
        cxr::assembly    const a(load_alpha_assembly(c, root));
        
        cxr::type const class_type(a.find_type(L"", L"QTrivialClass"));
        c.verify(class_type.is_initialized());
        c.verify(class_type.is_class());
        c.verify(!class_type.is_interface());
        c.verify(!class_type.is_value_type());
        c.verify(!class_type.is_enum());
        
        cxr::type const interface_type(a.find_type(L"", L"QTrivialInterfaceClass"));
        c.verify(interface_type.is_initialized());
        c.verify(!interface_type.is_class());
        c.verify(interface_type.is_interface());
        c.verify(!interface_type.is_value_type());
        c.verify(!interface_type.is_enum());

        cxr::type const value_type(a.find_type(L"", L"QTrivialValueTypeClass"));
        c.verify(value_type.is_initialized());
        c.verify(!value_type.is_class());
        c.verify(!value_type.is_interface());
        c.verify(value_type.is_value_type());
        c.verify(!value_type.is_enum());

        cxr::type const enum_type(a.find_type(L"", L"QTrivialEnumClass"));
        c.verify(enum_type.is_initialized());
        c.verify(!enum_type.is_class());
        c.verify(!enum_type.is_interface());
        c.verify(enum_type.is_value_type());
        c.verify(enum_type.is_enum());
    }





    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_alpha_type_inheritance)
    {
        cxr::loader_root const root(create_test_loader(c));
        cxr::assembly    const a(load_alpha_assembly(c, root));

        auto verify_inheritance([&](cxr::type const& t, cxr::type_attribute const expected)
        {
            c.verify(t.is_initialized());
            c.verify(t.is_abstract() == ((expected & cxr::type_attribute::abstract_) != cxr::type_attribute()));
            c.verify(t.is_sealed()   == ((expected & cxr::type_attribute::sealed)    != cxr::type_attribute()));
        });

        verify_inheritance(a.find_type(L"", L"QTrivialAbstractClass"),       cxr::type_attribute::abstract_);
        verify_inheritance(a.find_type(L"", L"QTrivialSealedClass"),         cxr::type_attribute::sealed);
        verify_inheritance(a.find_type(L"", L"QTrivialAbstractSealedClass"), cxr::type_attribute::abstract_ | cxr::type_attribute::sealed);
    }





    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_alpha_type_interoperation)
    {
        cxr::loader_root const root(create_test_loader(c));
        cxr::assembly    const a(load_alpha_assembly(c, root));

        auto verify_interoperation([&](cxr::type const& t, cxr::type_string_format const expected)
        {
            c.verify(t.is_initialized());
            c.verify_equals(t.string_format(), expected);

        });

        verify_interoperation(a.find_type(L"", L"QTrivialAnsiClass"),     cxr::type_string_format::ansi_string_format);
        verify_interoperation(a.find_type(L"", L"QTrivialAutoCharClass"), cxr::type_string_format::auto_string_format);
        verify_interoperation(a.find_type(L"", L"QTrivialUnicodeClass"),  cxr::type_string_format::unicode_string_format);
    }





    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_alpha_type_special_handling)
    {
        cxr::loader_root const root(create_test_loader(c));
        cxr::assembly    const a(load_alpha_assembly(c, root));

        // FUTURE Consider exposing runtime_special_name and before_field_init

        auto verify_handling([&](cxr::type const& t, cxr::type_attribute const expected)
        {
            c.verify(t.is_initialized());
            c.verify(t.is_serializable() == ((expected & cxr::type_attribute::serializable) != cxr::type_attribute()));
            c.verify(t.is_special_name() == ((expected & cxr::type_attribute::special_name) != cxr::type_attribute()));
        });

        verify_handling(a.find_type(L"", L"QTrivialBeforeFieldInitClass"), cxr::type_attribute());
        verify_handling(a.find_type(L"", L"QTrivialSerializableClass"),    cxr::type_attribute::serializable);
        verify_handling(a.find_type(L"", L"QTrivialSpecialNameClass"),     cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"", L"QTrivialRTSpecialNameClass"),   cxr::type_attribute::special_name);
        
        verify_handling(a.find_type(L"", L"QTrivialSpecialHandlingClass00"), cxr::type_attribute::serializable);
        verify_handling(a.find_type(L"", L"QTrivialSpecialHandlingClass01"), cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"", L"QTrivialSpecialHandlingClass02"), cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"", L"QTrivialSpecialHandlingClass03"), cxr::type_attribute::serializable | cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"", L"QTrivialSpecialHandlingClass04"), cxr::type_attribute::serializable | cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"", L"QTrivialSpecialHandlingClass05"), cxr::type_attribute::serializable | cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"", L"QTrivialSpecialHandlingClass06"), cxr::type_attribute::serializable | cxr::type_attribute::special_name);
    }





    CXXREFLECTTEST_DEFINE_TEST(reflection_basic_alpha_custom_modifiers)
    {
        cxr::loader_root const root(create_test_loader(c));
        cxr::assembly    const a(load_alpha_assembly(c, root));

        cxr::type const t(a.find_type(L"", L"QClassWithCustomModifiers"));
        c.verify(t.is_initialized());

        cxr::method const m(t.find_method(L"F", cxr::binding_attribute::all_instance));
        c.verify(m.is_initialized());
        c.verify_equals(cxr::distance(begin(m.parameters()), end(m.parameters())), 1u);

        cxr::parameter const p(*begin(m.parameters()));
        c.verify_equals(p.name(), L"arg");
        
        cxr::type const pt(p.parameter_type());
        c.verify(pt.is_initialized());
        c.verify(pt.is_pointer());

        cxr::type const pte(pt.element_type());
        c.verify(pte.is_initialized());
        c.verify_equals(pte.simple_name(), L"Boolean");

        {
            auto const modopts(pte.optional_custom_modifiers());
            c.verify_equals(cxr::distance(begin(modopts), end(modopts)), 1u);
            c.verify_equals(begin(modopts)->full_name(), L"System.UInt32");
       
            auto const modreqs(pte.required_custom_modifiers());
            c.verify_equals(cxr::distance(begin(modopts), end(modopts)), 1u);
            c.verify_equals(begin(modreqs)->full_name(), L"System.UInt64");
        }

        {
            auto const modopts(pt.optional_custom_modifiers());
            c.verify_equals(cxr::distance(begin(modopts), end(modopts)), 1u);
            c.verify_equals(begin(modopts)->full_name(), L"System.Int32");
       
            auto const modreqs(pt.required_custom_modifiers());
            c.verify_equals(cxr::distance(begin(modopts), end(modopts)), 1u);
            c.verify_equals(begin(modreqs)->full_name(), L"System.Int64");
        }
    }

}
