
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "test/unit_tests/test_driver.hpp"
#include "cxxreflect/reflection/reflection.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
}

namespace cxxreflect_test { namespace {

    auto create_test_loader(context const& c) -> cxr::loader
    {
        cxr::directory_based_module_locator::directory_set directories;
        directories.insert(c.get_property(known_property::framework_path()));
        directories.insert(c.get_property(known_property::test_assemblies_path()));

        cxr::directory_based_module_locator const locator(directories);

        cxr::loader root(locator);

        root.load_assembly(cxr::module_location(c.get_property(known_property::primary_assembly_path()).c_str()));

        return root;
    }

    auto load_alpha_assembly(context const& c, cxr::loader const& root) -> cxr::assembly
    {
        cxr::module_location const location(
            (c.get_property(known_property::test_assemblies_path()) + L"\\alpha.dll").c_str());

        cxr::assembly const a(root.load_assembly(location));
        c.verify(a.is_initialized());
        return a;
    }

    auto verify_alpha_assembly_and_modules(context const& c) -> void
    {
        cxr::loader   const root(create_test_loader(c));
        cxr::assembly const a(load_alpha_assembly(c, root));

        cxr::assembly_name const name(a.name());
        c.verify_equals(name.simple_name(), L"alpha");
        c.verify_equals(name.version().major(), 1);
        c.verify_equals(name.version().minor(), 2);
        c.verify_equals(name.version().build(), 3);
        c.verify_equals(name.version().revision(), 4);
        // TODO Other name fields?

        c.verify_equals(a.referenced_assembly_count(), 1u);
        cxr::assembly_name const mscorlib_name(*a.begin_referenced_assembly_names());
        c.verify_equals(mscorlib_name.simple_name(), L"mscorlib");
        c.verify_equals(mscorlib_name.version().major(), 4);
        c.verify_equals(mscorlib_name.version().minor(), 0);
        c.verify_equals(mscorlib_name.version().build(), 0);
        c.verify_equals(mscorlib_name.version().revision(), 0);

        // cxr::public_key_token expected_token = { 0xb7, 0x7a, 0x5c, 0x56, 0x19, 0x34, 0xe0, 0x89 };
        // c.verify_equals(mscorlib_name.public_key_token(), expected_token);

        // TODO Module verification
    }

    CXXREFLECTTEST_REGISTER(reflection_alpha_assembly_and_modules, verify_alpha_assembly_and_modules);

    



    auto verify_alpha_type_visibility_accessibility(context const& c) -> void
    {
        cxr::loader   const root(create_test_loader(c));
        cxr::assembly const a(load_alpha_assembly(c, root));

        auto verify_visibility([&](cxr::type const& t, cxr::type_attribute const expected)
        {
            c.verify(t.is_initialized());
            c.verify(t.is_not_public()                 == (expected == cxr::type_attribute::not_public));
            c.verify(t.is_public()                     == (expected == cxr::type_attribute::public_));
            c.verify(t.is_nested_public()              == (expected == cxr::type_attribute::nested_public));
            c.verify(t.is_nested_private()             == (expected == cxr::type_attribute::nested_private));
            c.verify(t.is_nested_family()              == (expected == cxr::type_attribute::nested_family));
            c.verify(t.is_nested_assembly()            == (expected == cxr::type_attribute::nested_assembly));
            c.verify(t.is_nested_family_and_assembly() == (expected == cxr::type_attribute::nested_family_and_assembly));
            c.verify(t.is_nested_family_or_assembly()  == (expected == cxr::type_attribute::nested_family_or_assembly));

        });

        verify_visibility(a.find_type(L"QTrivialPrivateClass"), cxr::type_attribute::not_public);
        verify_visibility(a.find_type(L"QTrivialPublicClass"),  cxr::type_attribute::public_);

        // TODO find_type needs support for nested types (with +)
    }

    CXXREFLECTTEST_REGISTER(reflection_alpha_type_visibility_accessibility, verify_alpha_type_visibility_accessibility);





    auto verify_alpha_type_layout(context const& c) -> void
    {
        cxr::loader   const root(create_test_loader(c));
        cxr::assembly const a(load_alpha_assembly(c, root));

        auto verify_layout([&](cxr::type const& t, cxr::type_attribute const expected)
        {
            c.verify(t.is_initialized());
            c.verify(t.is_auto_layout()       == (expected == cxr::type_attribute::auto_layout));
            c.verify(t.is_explicit_layout()   == (expected == cxr::type_attribute::explicit_layout));
            c.verify(t.is_layout_sequential() == (expected == cxr::type_attribute::sequential_layout));

        });

        verify_layout(a.find_type(L"QTrivialAutoClass"),        cxr::type_attribute::auto_layout);
        verify_layout(a.find_type(L"QTrivialExplicitClass"),    cxr::type_attribute::explicit_layout);
        verify_layout(a.find_type(L"QTrivialSequentialClass"),  cxr::type_attribute::sequential_layout);
    }

    CXXREFLECTTEST_REGISTER(reflection_alpha_type_layout, verify_alpha_type_layout);





    auto verify_alpha_type_semantics(context const& c) -> void
    {
        cxr::loader   const root(create_test_loader(c));
        cxr::assembly const a(load_alpha_assembly(c, root));
        
        cxr::type const class_type(a.find_type(L"QTrivialClass"));
        c.verify(class_type.is_initialized());
        c.verify(class_type.is_class());
        c.verify(!class_type.is_interface());
        c.verify(!class_type.is_value_type());
        c.verify(!class_type.is_enum());
        
        cxr::type const interface_type(a.find_type(L"QTrivialInterfaceClass"));
        c.verify(interface_type.is_initialized());
        c.verify(!interface_type.is_class());
        c.verify(interface_type.is_interface());
        c.verify(!interface_type.is_value_type());
        c.verify(!interface_type.is_enum());

        cxr::type const value_type(a.find_type(L"QTrivialValueTypeClass"));
        c.verify(value_type.is_initialized());
        c.verify(!value_type.is_class());
        c.verify(!value_type.is_interface());
        c.verify(value_type.is_value_type());
        c.verify(!value_type.is_enum());

        cxr::type const enum_type(a.find_type(L"QTrivialEnumClass"));
        c.verify(enum_type.is_initialized());
        c.verify(!enum_type.is_class());
        c.verify(!enum_type.is_interface());
        c.verify(enum_type.is_value_type());
        c.verify(enum_type.is_enum());
    }

    CXXREFLECTTEST_REGISTER(reflection_alpha_type_semantics, verify_alpha_type_semantics);





    auto verify_alpha_type_inheritance(context const& c) -> void
    {
        cxr::loader   const root(create_test_loader(c));
        cxr::assembly const a(load_alpha_assembly(c, root));

        auto verify_inheritance([&](cxr::type const& t, cxr::type_attribute const expected)
        {
            c.verify(t.is_initialized());
            c.verify(t.is_abstract() == ((expected & cxr::type_attribute::abstract_) != cxr::type_attribute()));
            c.verify(t.is_sealed()   == ((expected & cxr::type_attribute::sealed)    != cxr::type_attribute()));
        });

        verify_inheritance(a.find_type(L"QTrivialAbstractClass"),       cxr::type_attribute::abstract_);
        verify_inheritance(a.find_type(L"QTrivialSealedClass"),         cxr::type_attribute::sealed);
        verify_inheritance(a.find_type(L"QTrivialAbstractSealedClass"), cxr::type_attribute::abstract_ | cxr::type_attribute::sealed);
    }

    CXXREFLECTTEST_REGISTER(reflection_alpha_type_inheritance, verify_alpha_type_inheritance);





    auto verify_alpha_type_interoperation(context const& c) -> void
    {
        cxr::loader   const root(create_test_loader(c));
        cxr::assembly const a(load_alpha_assembly(c, root));

        auto verify_interoperation([&](cxr::type const& t, cxr::type_attribute const expected)
        {
            c.verify(t.is_initialized());
            c.verify(t.is_ansi_class()    == (expected == cxr::type_attribute::ansi_class));
            c.verify(t.is_auto_class()    == (expected == cxr::type_attribute::auto_class));
            c.verify(t.is_unicode_class() == (expected == cxr::type_attribute::unicode_class));

        });

        verify_interoperation(a.find_type(L"QTrivialAnsiClass"),     cxr::type_attribute::ansi_class);
        verify_interoperation(a.find_type(L"QTrivialAutoCharClass"), cxr::type_attribute::auto_class);
        verify_interoperation(a.find_type(L"QTrivialUnicodeClass"),  cxr::type_attribute::unicode_class);
    }

    CXXREFLECTTEST_REGISTER(reflection_alpha_type_interoperation, verify_alpha_type_interoperation);





    auto verify_alpha_type_special_handling(context const& c) -> void
    {
        cxr::loader   const root(create_test_loader(c));
        cxr::assembly const a(load_alpha_assembly(c, root));

        // FUTURE Consider exposing runtime_special_name and before_field_init

        auto verify_handling([&](cxr::type const& t, cxr::type_attribute const expected)
        {
            c.verify(t.is_initialized());
            c.verify(t.is_serializable() == ((expected & cxr::type_attribute::serializable) != cxr::type_attribute()));
            c.verify(t.is_special_name() == ((expected & cxr::type_attribute::special_name) != cxr::type_attribute()));
        });

        verify_handling(a.find_type(L"QTrivialBeforeFieldInitClass"), cxr::type_attribute());
        verify_handling(a.find_type(L"QTrivialSerializableClass"),    cxr::type_attribute::serializable);
        verify_handling(a.find_type(L"QTrivialSpecialNameClass"),     cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"QTrivialRTSpecialNameClass"),   cxr::type_attribute::special_name);
        
        verify_handling(a.find_type(L"QTrivialSpecialHandlingClass00"), cxr::type_attribute::serializable);
        verify_handling(a.find_type(L"QTrivialSpecialHandlingClass01"), cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"QTrivialSpecialHandlingClass02"), cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"QTrivialSpecialHandlingClass03"), cxr::type_attribute::serializable | cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"QTrivialSpecialHandlingClass04"), cxr::type_attribute::serializable | cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"QTrivialSpecialHandlingClass05"), cxr::type_attribute::serializable | cxr::type_attribute::special_name);
        verify_handling(a.find_type(L"QTrivialSpecialHandlingClass06"), cxr::type_attribute::serializable | cxr::type_attribute::special_name);
    }

    CXXREFLECTTEST_REGISTER(reflection_alpha_type_special_handling, verify_alpha_type_special_handling);

} }
