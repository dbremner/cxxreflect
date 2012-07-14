
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //





// CXXREFLECT TEST SUITE -- METADATA WRITER CXX
//
// This program loads an assembly using CxxReflect and dumps the metadata tables to a file.  It does
// not report any intertable relationship data; rather, it dumps each table independently.  For a
// higher-level metadata view, including intertable relationships and ownership information, use the
// reflection_writer_cxx program, which uses the CxxReflect reflection APIs.
//
// To use this program, set the input and output paths in the main() function at the bottom of this
// file, recompile, and run.





#include "cxxreflect/cxxreflect.hpp"
#include "cxxreflect/metadata/debug.hpp"

#define PP_CONCAT_(x, y) x ## y
#define PP_CONCAT(x, y) PP_CONCAT_(x, y)

#define PP_STRING_(x) # x
#define PP_STRING(x) PP_STRING_(x)

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::externals;
    using namespace cxxreflect::metadata;
}

namespace cxxreflect { namespace metadata {

    class as_hex
    {
    public:

        template <typename T>
        as_hex(T x) : _x((unsigned)x) { }

        friend auto operator<<(cxr::file_handle& os, as_hex const& o) -> cxr::file_handle&
        {
            os << L"0x" << cxr::hex_format(o._x);
            return os;
        }

    private:

        unsigned _x;
    };

    class as_bytes
    {
    public:

        as_bytes(cxr::blob x) : _x(x) { }

        friend auto operator<<(cxr::file_handle& os, as_bytes const& o) -> cxr::file_handle&
        {
            bool first(true);
            std::for_each(begin(o._x), end(o._x), [&](cxr::byte const v)
            {
                if (!first) os << L" ";
                std::array<wchar_t, 3> r((std::array<wchar_t, 3>()));
                std::swprintf(r.data(), r.size(), L"%02x", (unsigned)v);
                os << r.data();
                first = false;
            });
            return os;
        }

    private:

        cxr::blob _x;
    };

    auto operator<<(cxr::file_handle& os, cxr::assembly_row const& r) -> cxr::file_handle&
    {
        os << L"//  * hash_algorithm [" << as_hex(r.hash_algorithm()) << L"] [" <<  r.hash_algorithm() << L"]\n";
        os << L"//  * version ["
               << r.version().major() << L"."
               << r.version().minor() << L"."
               << r.version().build() << L"."
               << r.version().revision()
           << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * public_key [" << as_bytes(r.public_key()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * culture [" << r.culture().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_os_row const& r) -> cxr::file_handle&
    {
        os << L"//  * platform_id [" << r.platform_id() << L"]\n";
        os << L"//  * major_version [" << r.major_version() << L"]\n";
        os << L"//  * minor_version [" << r.minor_version() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_processor_row const& r) -> cxr::file_handle&
    {
        os << L"//  * processor [" << r.processor() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_row const& r) -> cxr::file_handle&
    {
        os << L"//  * version ["
               << r.version().major() << L"."
               << r.version().minor() << L"."
               << r.version().build() << L"."
               << r.version().revision()
           << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * public_key [" << as_bytes(r.public_key()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * culture [" << r.culture().c_str() << L"]\n";
        os << L"//  * hash_value [" << as_bytes(r.hash_value()) << L"]\n";
        os << L"// \n";
        return os;
    }


    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_os_row const& r) -> cxr::file_handle&
    {
        os << L"//  * platform_id [" << r.platform_id() << L"]\n";
        os << L"//  * major_version [" << r.major_version() << L"]\n";
        os << L"//  * minor_version [" << r.minor_version() << L"]\n";
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_processor_row const& r) -> cxr::file_handle&
    {
        os << L"//  * processor [" << r.processor() << L"]\n";
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::class_layout_row const& r) -> cxr::file_handle&
    {
        os << L"//  * packing_size [" << r.packing_size() << L"]\n";
        os << L"//  * class_size [" << r.class_size() << L"]\n";
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::constant_row const& r) -> cxr::file_handle&
    {
        os << L"//  * type [" << r.type() << L"]\n";
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * value [" << as_bytes(r.value()) << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::custom_attribute_row const& r) -> cxr::file_handle&
    {
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * type [" << r.type() << L"]\n";
        os << L"//  * value [" << as_bytes(r.value()) << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::decl_security_row const& r) -> cxr::file_handle&
    {
        os << L"//  * action [" << r.action() << L"]\n";
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * permission_set [" << as_bytes(r.permission_set()) << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::event_map_row const& r) -> cxr::file_handle&
    {
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * events ["
               << r.first_event() << L" ~ "
               << r.last_event()
           << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::event_row const& r) -> cxr::file_handle&
    {
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * type [" << r.type() << L"]\n";
        os << L"// \n";
        return os;
    }
    
    auto operator<<(cxr::file_handle& os, cxr::exported_type_row const& r) -> cxr::file_handle&
    {
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * type_def_id [" << r.type_def_id() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * namespace_name [" << r.namespace_name().c_str() << L"]\n";
        os << L"//  * implementation [" << r.implementation() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_row const& r) -> cxr::file_handle&
    {
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        os << L"//    " << r.signature().as<cxr::field_signature>() << L"\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_layout_row const& r) -> cxr::file_handle&
    {
        os << L"//  * offset [" << r.offset() << L"]\n"; 
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_marshal_row const& r) -> cxr::file_handle&
    {
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * native_type [" << as_bytes(r.native_type()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_rva_row const& r) -> cxr::file_handle&
    {
        os << L"//  * rva [" << as_hex(r.rva()) << L"]\n";
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::file_row const& r) -> cxr::file_handle&
    {
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * hash_value [" << as_bytes(r.hash_value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::generic_param_row const& r) -> cxr::file_handle&
    {
        os << L"//  * sequence [" << r.sequence() << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::generic_param_constraint_row const& r) -> cxr::file_handle&
    {
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * constraint [" << r.constraint() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::impl_map_row const& r) -> cxr::file_handle&
    {
        os << L"//  * mapping_flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * member_forwarded [" << r.member_forwarded() << L"]\n";
        os << L"//  * import_name [" << r.import_name().c_str() << L"]\n";
        os << L"//  * import_scope [" << r.import_scope() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::interface_impl_row const& r) -> cxr::file_handle&
    {
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * interface [" << r.interface() << L"]\n";
        os << L"// \n";
        return os;
    }
    
    auto operator<<(cxr::file_handle& os, cxr::manifest_resource_row const& r) -> cxr::file_handle&
    {
        os << L"//  * offset [" << as_hex(r.offset()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        if (r.implementation().is_initialized())
            os << L"//  * implementation [" << r.implementation() << L"]\n";
        else
            os << L"//  * implementation [<none>]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::member_ref_row const& r) -> cxr::file_handle&
    {
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        // TODO Determine signature type
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_def_row const& r) -> cxr::file_handle&
    {
        os << L"//  * rva [" << as_hex(r.rva()) << L"]\n";
        os << L"//  * implementation_flags [" << as_hex(r.implementation_flags().integer()) << L"] [" << r.implementation_flags().enumerator() << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        os << L"//    " << r.signature().as<cxr::method_signature>() << L"\n";
        os << L"//  * parameters ["
               << r.first_parameter() << L" ~ "
               << r.last_parameter()
           << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_impl_row const& r) -> cxr::file_handle&
    {
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * method_body [" << r.method_body() << L"]\n";
        os << L"//  * method_declaration [" << r.method_declaration() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_semantics_row const& r) -> cxr::file_handle&
    {
        os << L"//  * semantics [" << as_hex(r.semantics().integer()) << L"] [" << r.semantics().enumerator() << L"]\n";
        os << L"//  * method [" << r.method() << L"]\n";
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"// \n";
        return os;
    }
    
    auto operator<<(cxr::file_handle& os, cxr::method_spec_row const& r) -> cxr::file_handle&
    {
        os << L"//  * method [" << r.method() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        os << L"//    " << r.signature().as<cxr::type_signature>() << L"\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::module_row const& r) -> cxr::file_handle&
    {
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * mvid [" << as_bytes(r.mvid()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::module_ref_row const& r) -> cxr::file_handle&
    {
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::nested_class_row const& r) -> cxr::file_handle&
    {
        os << L"//  * nested_class [" << r.nested_class() << L"]\n";
        os << L"//  * enclosing_class [" << r.enclosing_class() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::param_row const& r) -> cxr::file_handle&
    {
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * sequence [" << r.sequence() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::property_row const& r) -> cxr::file_handle&
    {
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        os << L"//    " << r.signature().as<cxr::property_signature>() << L"\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::property_map_row const& r) -> cxr::file_handle&
    {
        os << L"//  * parent [" << r.parent() << L"]\n";
        os << L"//  * properties ["
               << r.first_property() << L" ~ "
               << r.last_property()
           << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::standalone_sig_row const& r) -> cxr::file_handle&
    {
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::type_def_row const& r) -> cxr::file_handle&
    {
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * namespace_name [" << r.namespace_name().c_str() << L"]\n";
        if (r.extends().is_initialized())
            os << L"//  * extends [" << r.extends() << L"]\n";
        else
            os << L"//  * extends [<none>]\n";
        os << L"//  * fields ["
               << r.first_field() << L" ~ "
               << r.last_field()
           << L"]\n";
        os << L"//  * methods ["
               << r.first_method() << L" ~ "
               << r.last_method()
           << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::type_ref_row const& r) -> cxr::file_handle&
    {
        os << L"//  * resolution_scope [" << r.resolution_scope() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * namespace_name [" << r.namespace_name().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::type_spec_row const& r) -> cxr::file_handle&
    {
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        os << L"//    " << r.signature().as<cxr::type_signature>() << L"\n";
        os << L"// \n";
        return os;
    }

} }

namespace {

    template <cxr::table_id Id>
    auto write_table(cxr::file_handle        & os,
                     cxr::database      const& scope,
                     cxr::string        const& table_name) -> cxr::file_handle&
    {
        typedef typename cxr::row_type_for_table_id<Id>::type row_type;

        os << L"\n";
        os << L"\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"// TABLE [" << table_name.c_str() << L"]\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        std::for_each(scope.begin<Id>(), scope.end<Id>(), [&](decltype(*scope.begin<Id>()) x)
        {
            os << L"// " << x.token() << L"\n";
            os << x;
        });

        return os;
    }
}

auto main() -> int
{
    cxr::externals::initialize(cxr::win32_externals());

    cxr::string const input_path(L"c:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");
    cxr::string const output_path(L"c:\\jm\\metadata_writer_cxx.txt");

    cxr::database scope(cxr::database::create_from_file(input_path.c_str()));

    cxr::file_handle of(output_path.c_str(), cxr::file_mode::write);

    #define WRITE_TABLE(x) write_table<cxr::table_id::x>(of, scope, PP_CONCAT(L, PP_STRING(x)))

    WRITE_TABLE(assembly);
    WRITE_TABLE(assembly_os);
    WRITE_TABLE(assembly_processor);
    WRITE_TABLE(assembly_ref);
    WRITE_TABLE(assembly_ref_os);
    WRITE_TABLE(assembly_ref_processor);
    WRITE_TABLE(class_layout);
    WRITE_TABLE(constant);
    WRITE_TABLE(custom_attribute);
    WRITE_TABLE(decl_security);
    WRITE_TABLE(event_map);
    WRITE_TABLE(event);
    WRITE_TABLE(exported_type);
    WRITE_TABLE(field);
    WRITE_TABLE(field_layout);
    WRITE_TABLE(field_marshal);
    WRITE_TABLE(field_rva);
    WRITE_TABLE(file);
    WRITE_TABLE(generic_param);
    WRITE_TABLE(generic_param_constraint);
    WRITE_TABLE(impl_map);
    WRITE_TABLE(interface_impl);
    WRITE_TABLE(manifest_resource);
    WRITE_TABLE(member_ref);
    WRITE_TABLE(method_def);
    WRITE_TABLE(method_impl);
    WRITE_TABLE(method_semantics);
    WRITE_TABLE(method_spec);
    WRITE_TABLE(module);
    WRITE_TABLE(module_ref);
    WRITE_TABLE(nested_class);
    WRITE_TABLE(param);
    WRITE_TABLE(property);
    WRITE_TABLE(property_map);
    WRITE_TABLE(standalone_sig);
    WRITE_TABLE(type_def);
    WRITE_TABLE(type_ref);
    WRITE_TABLE(type_spec);

    #undef WRITE_TABLE

    return 0;
}

// AMDG //
