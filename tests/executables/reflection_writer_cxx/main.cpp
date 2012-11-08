
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //





// CXXREFLECT TEST SUITE -- REFLECTION WRITER CXX
//
// This program loads an assembly using CxxReflect and dumps the types and members represented in
// the assembly to a file.  
//
// To use this program, set the input and output paths in the main() function at the bottom of this
// file, recompile, and run.
//
// TODO THIS PROGRAM IS NOT YET COMPLETE





#include "cxxreflect/cxxreflect.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::externals;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
}

namespace 
{
    cxr::binding_attribute const all_binding_flags = (cxr::binding_attribute::public_ | cxr::binding_attribute::non_public | cxr::binding_attribute::static_ | cxr::binding_attribute::instance | cxr::binding_attribute::flatten_hierarchy);

    auto is_known_problem_type(cxr::type const& t) -> bool
    {
        return false;
        //return (t.namespace_name() == L"System"                                        && t.simple_name() == L"__ComObject")
        //    || (t.namespace_name() == L"System.Runtime.Remoting.Proxies"               && t.simple_name() == L"__TransparentProxy")
        //    || (t.namespace_name() == L"System.Runtime.InteropServices.WindowsRuntime" && t.simple_name() == L"DisposableRuntimeClass")
        //    || (t.namespace_name() == L"System.StubHelpers"                            && t.simple_name() == L"HStringMarshaler");
    }

    auto write_basic_type_traits(cxr::file_handle& os, cxr::type const& t, cxr::size_type const depth) -> void
    {
        std::wstring pad(depth, L' ');
        os << pad.c_str() << L" -- Type [" << t.full_name().c_str() << L"] [$" << cxr::hex_format(t.metadata_token()) << L"]\n";
        os << pad.c_str() << L"     -- AssemblyQualifiedName [" << t.assembly_qualified_name().c_str() << L"]\n";
        os << pad.c_str() << L"     -- BaseType [" << (t.base_type() ? t.base_type().full_name().c_str() : L"NO BASE TYPE") << L"]\n";
        os << pad.c_str() << L"         -- AssemblyQualifiedName [" << (t.base_type() ? t.base_type().assembly_qualified_name().c_str() : L"NO BASE TYPE") << L"]\n";

        /* TODO
        #define F(n) (t.n() ? 1 : 0)
        os << pad.c_str() << L"     -- IsTraits [" 
           << F(is_abstract) << F(is_ansi_class) << F(is_array) << F(is_auto_class) << F(is_auto_layout) << F(is_by_ref) << F(is_class) << F(is_com_object) << L"] ["
           << F(is_contextful) << F(is_enum) << F(is_explicit_layout) << F(is_generic_parameter) << F(is_generic_type) << F(is_generic_type_definition) << F(is_import) << F(is_interface) << L"] ["
           << F(is_layout_sequential) << F(is_marshal_by_ref) << F(is_nested) << F(is_nested_assembly) << F(is_nested_family_and_assembly) << F(is_nested_family) << F(is_nested_family_or_assembly) << F(is_nested_private) << L"] ["
           << F(is_nested_public) << F(is_not_public) << F(is_pointer) << F(is_primitive) << F(is_public) << F(is_sealed) << F(is_serializable) << F(is_special_name) << L"] ["
           << F(is_unicode_class) << F(is_value_type) << F(is_visible) << L"     ]\n";
        #undef F
        */

        os << pad.c_str() << L"     -- Name [" << t.simple_name().c_str() << L"]\n";
        os << pad.c_str() << L"     -- Namespace [" << t.namespace_name().c_str() << L"]\n";
    }

    auto write(cxr::file_handle& os, cxr::assembly         const& a) -> void;
    auto write(cxr::file_handle& os, cxr::custom_attribute const& c) -> void;
    auto write(cxr::file_handle& os, cxr::field            const& f) -> void;
    auto write(cxr::file_handle& os, cxr::method           const& m) -> void;
    auto write(cxr::file_handle& os, cxr::module           const& m) -> void;
    auto write(cxr::file_handle& os, cxr::parameter        const& p) -> void;
    auto write(cxr::file_handle& os, cxr::type             const& t) -> void;

    auto write(cxr::file_handle& os, cxr::assembly const& a) -> void
    {
        os << L"Assembly [" << a.name().full_name().c_str() << L"]\n";
        os << L"!!Begin Modules\n";
        cxr::for_all(a.modules(), [&](cxr::module const& x)
        {
            os << L" -- module [" << x.name().c_str() << L"]\n";
        });
        os << L"!!End Modules\n";
        os << L"!!BeginAssemblyReferences\n";
        cxr::for_all(a.referenced_assembly_names(), [&](cxr::assembly_name const& x)
        {
            os << L" -- AssemblyName [" << x.full_name().c_str() << L"]\n";
        });
        os << L"!!EndAssemblyReferences\n";
        os << L"!!BeginTypes\n";
        cxr::for_all(a.types(), [&](cxr::type const& x)
        {
            if (is_known_problem_type(x))
                return;

            write(os, x);
        });
        os << L"!!EndTypes\n";
    }

    auto write(cxr::file_handle& os, cxr::custom_attribute const& c) -> void
    {
        // TODO os << L"     -- CustomAttribute [" << c.constructor().declaring_type().full_name().c_str() << L"]\n";

        // TODO
    }

    auto write(cxr::file_handle& os, cxr::field const& f) -> void
    {
        os << L"     -- Field [" << f.name().c_str() << L"] [$" << cxr::hex_format(f.metadata_token()) << L"]\n";

        #define F(n) (f.n() ? 1 : 0)
        os << L"         -- Attributes [" << cxr::hex_format(f.attributes().integer()) << L"]\n";
        os << L"         -- Declaring Type [" << f.declaring_type().full_name().c_str() << L"]\n";
        os << L"         -- IsTraits "
           << L"[" << F(is_assembly) << F(is_family) << F(is_family_and_assembly) << F(is_family_or_assembly) << F(is_init_only) << F(is_literal) << F(is_not_serialized) << F(is_pinvoke_impl) << L"] "
           << L"[" << F(is_private) << F(is_public) << F(is_special_name) << F(is_static) << L"    ]\n";
        #undef F
        // TODO
    }

    auto write(cxr::file_handle& os, cxr::method const& m) -> void
    {
        os << L"     -- Method [" << m.name().c_str() << L"] [$" << cxr::hex_format(m.metadata_token()) << L"]\n";
        os << L"        !!BeginParameters\n";
        cxr::for_all(m.parameters(), [&](cxr::parameter const& p)
        {
            write(os, p);
        });
        os << L"        !!EndParameters\n";

        // TODO
    }

    auto write(cxr::file_handle& os, cxr::module const& m) -> void
    {
        // TODO
    }

    auto write(cxr::file_handle& os, cxr::parameter const& p) -> void
    {
        os << L"         -- [" << p.name().c_str()
           << L"] [$" << cxr::hex_format(p.metadata_token())
           << L"] [" << p.parameter_type().full_name().c_str() << ((p.is_out() && !p.parameter_type().is_by_ref() && !p.parameter_type().is_array() && !p.parameter_type().is_pointer() && !p.is_in() && p.parameter_type().full_name() != L"System.Text.StringBuilder" && !p.parameter_type().full_name().empty()) ? L"&" : L"") << L"]\n";

        // TODO Not yet implemented feature:  we do not correctly handle uninstantiated generic parameters.
        if (p.parameter_type().full_name().size() > 0)
            write_basic_type_traits(os, p.parameter_type(), 12);
        // TODO
    }

    auto write(cxr::file_handle& os, cxr::type const& t) -> void
    {
        write_basic_type_traits(os, t, 0);

        os << L"    !!BeginInterfaces\n";
        std::vector<cxr::type> all_interfaces(begin(t.interfaces()), end(t.interfaces()));
        std::sort(all_interfaces.begin(), all_interfaces.end(), cxr::metadata_token_less_than_comparer());
        std::for_each(all_interfaces.begin(), all_interfaces.end(), [&](cxr::type const& it)
        {
            os << L"     -- Interface [" << it.full_name().c_str() << L"] [$" << cxr::hex_format(it.metadata_token()) << L"]\n";
        });
        os << L"    !!EndInterfaces\n";
        /*
        os << L"    !!BeginCustomAttributes\n";
        std::vector<cxr::custom_attribute> all_custom_attributes(t.begin_custom_attributes(), t.end_custom_attributes());
        std::sort(all_custom_attributes.begin(),
                  all_custom_attributes.end(),
                  cxr::metadata_token_less_than_comparer([](cxr::custom_attribute const& c)
        {
            return c.constructor().declaring_type().metadata_token();
        }));
        std::for_each(all_custom_attributes.begin(), all_custom_attributes.end(), [&](cxr::custom_attribute const& c)
        {
            write(os, c);
        });
        os << L"    !!EndCustomAttributes\n";
        */
        os << L"    !!BeginConstructors\n";
        std::vector<cxr::method> all_constructors(begin(t.constructors(all_binding_flags)), end(t.constructors()));
        std::sort(all_constructors.begin(), all_constructors.end(), cxr::metadata_token_less_than_comparer());
        std::for_each(all_constructors.begin(), all_constructors.end(), [&](cxr::method const& m)
        {
            write(os, m);
        });
        os << L"    !!EndConstructors\n";

        os << L"    !!BeginMethods\n";
        std::vector<cxr::method> all_methods(begin(t.methods(all_binding_flags)), end(t.methods()));
        std::sort(all_methods.begin(), all_methods.end(), cxr::metadata_token_less_than_comparer());
        std::for_each(all_methods.begin(), all_methods.end(), [&](cxr::method const& m)
        {
            write(os, m);
        });
        os << L"    !!EndMethods\n";
        os << L"    !!BeginFields\n";
        std::vector<cxr::field> all_fields(begin(t.fields(all_binding_flags)), end(t.fields()));
        std::sort(all_fields.begin(), all_fields.end(), cxr::metadata_token_less_than_comparer());
        std::for_each(all_fields.begin(), all_fields.end(), [&](cxr::field const& f)
        {
            write(os, f);
        });
        os << L"    !!EndFields\n";

        // TODO
    }
}

auto main() -> int
{
    auto const start(std::chrono::high_resolution_clock::now());
    cxr::externals::initialize(cxr::win32_externals());

    cxr::string const input_path(L"c:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");
    cxr::string const output_path(L"c:\\jm\\reflection_writer_cxx.txt");

    cxr::search_path_module_locator::search_path_sequence directories;
    directories.push_back(L"c:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319");

    cxr::loader_root const loader(cxr::create_loader_root(cxr::search_path_module_locator(directories), cxr::default_loader_configuration()));

    cxr::assembly a(loader.get().load_assembly(input_path.c_str()));

    cxr::file_handle os(output_path.c_str(), cxr::file_mode::write);
    write(os, a);

    auto const end(std::chrono::high_resolution_clock::now());

    std::chrono::milliseconds duration = std::chrono::duration_cast<std::chrono::milliseconds>((end - start));

    os << L"\n" << (unsigned)duration.count() << L"\n";


    return 0;
}
