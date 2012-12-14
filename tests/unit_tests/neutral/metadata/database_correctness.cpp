
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is a set of tests for nom-nom-nominal use of the CxxReflect::Metadata::Database class and
// its related classes (the row types, element reference types, streams, etc.).
//
// We load a handful of assemblies and verify two things:  first, that we correctly read all of the
// metadata tables from the assembly, without worrying about decoding the data.  This verifies that
// we can correctly find the metadata database in a PE file and that we correctly find each row in
// each table.
//
// Second, we enumerate the rows in each table and verify that we can correctly read each field of
// every row of every table.  This verifies that we correctly look up strings, GUIDs, and blobs, and
// that we correctly compute offsets and sizes for each column.

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

// TODO:  We should be able to make this work on ARM.  Currently, one of these headers is causing us
// to link against shlwapi, which is not available for ARM in the Windows SDK.  Until we fix this,
// we cannot run this test on ARM.
#if CXXREFLECT_ARCHITECTURE != CXXREFLECT_ARCHITECTURE_ARM

#include <atlbase.h>
#include <cor.h>
#include <metahost.h>

// mscoree.lib is required for CLRCreateInstance(), which we use to start up Mr. CLR.
#pragma comment(lib, "mscoree.lib")

// Disable the narrowing conversion warnings so that we compile cleanly for x64.  We aren't going
// to overflow with any of our calculations.
#pragma warning(disable: 4267)

#undef interface

namespace cxr
{
    using namespace cxxreflect;
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
}

namespace cxxreflect_test { namespace {

    /// A helper to ensure that calls to CoInitializeEx and CoUninitialize stay balanced.
    class guarded_coinitialize
    {
    public:

        guarded_coinitialize()
        {
            HRESULT const hr(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            if (FAILED(hr))
                throw test_error(L"failed to initialize");
        }

        ~guarded_coinitialize()
        {
            ::CoUninitialize();
        }
    };

    /// Starts the v4.0 CLR and gets the metadata dispenser from it.
    ///
    /// Note that we can't just CoCreateInstance an CLSID_CorMetaDataDispenser because it defaults
    /// to the .NET 2.0 runtime, which is not installed by default on Windows 8.  
    auto get_metadata_dispenser() -> CComPtr<IMetaDataDispenser>
    {
        CComPtr<ICLRMetaHost> meta_host;
        int const hr0(::CLRCreateInstance(
            CLSID_CLRMetaHost,
            IID_ICLRMetaHost,
            reinterpret_cast<void**>(&meta_host)));

        if (FAILED(hr0) || meta_host == nullptr)
            throw test_error(L"failed to load CLR host");

        CComPtr<ICLRRuntimeInfo> runtime_info;
        int const hr1(meta_host->GetRuntime(
            L"v4.0.30319",
            IID_ICLRRuntimeInfo,
            reinterpret_cast<void**>(&runtime_info)));

        if (FAILED(hr1) || runtime_info == nullptr)
            throw test_error(L"failed to get v4.0 runtime");

        CComPtr<IMetaDataDispenser> dispenser;
        int const hr2(runtime_info->GetInterface(
            CLSID_CorMetaDataDispenser,
            IID_IMetaDataDispenser,
            reinterpret_cast<void**>(&dispenser)));

        if (FAILED(hr2) || dispenser == nullptr)
            throw test_error(L"failed to obtain dispenser from runtime");

        return dispenser;
    }

    /// Loads an assembly using an IMetaDataDispenser and returns its IMetaDataTables interface.
    auto get_metadata_tables(IMetaDataDispenser* const dispenser, string const& path) -> CComPtr<IMetaDataTables>
    {
        CComPtr<IMetaDataImport> import;
        int const hr0(dispenser->OpenScope(
            path.c_str(),
            ofRead,
            IID_IMetaDataImport,
            reinterpret_cast<IUnknown**>(&import)));

        if (FAILED(hr0) || import == nullptr)
            throw test_error(L"failed to import assembly");

        CComQIPtr<IMetaDataTables, &IID_IMetaDataTables> tables(import);
        return tables;
    }

    // All of our tests require similar setup to initialize the databases.  This does that setup.
    template <typename Callable>
    auto setup_and_call(Callable&& callable, context const& c) -> void
    {
        guarded_coinitialize const init;

        string const path(c.get_property(known_property::primary_assembly_path()));

        CComPtr<IMetaDataDispenser> const md_dispenser(get_metadata_dispenser());
        CComPtr<IMetaDataTables>    const md_tables(get_metadata_tables(md_dispenser, path));

        cxr::database const cxr_database(cxr::database::create_from_file(path.c_str()));

        callable(md_tables, cxr_database, c);
    }

    auto verify_database(IMetaDataTables* const cor_database, cxr::database const& cxr_database, context const& c) -> void
    {
        // Ensure that both databases report the same number of rows:
        ULONG cor_table_count(0);
        c.verify_success(cor_database->GetNumTables(&cor_table_count));
        c.verify_equals(cor_table_count, (ULONG)cxr::table_id_count);

        for (ULONG table_index(0); table_index != cor_table_count; ++table_index)
        {
            if (!cxr::is_valid_table_id(table_index))
                continue;

            cxr::table_id const cxr_table_id(static_cast<cxr::table_id>(table_index));

            // First, verify that we compute basic properties of the table correctly:
            ULONG cor_row_size(0);
            ULONG cor_row_count(0);
            ULONG cor_column_count(0);
            ULONG cor_key_size(0);
            char const* cor_table_name(nullptr);

            c.verify_success(cor_database->GetTableInfo(
                table_index, &cor_row_size, &cor_row_count, &cor_column_count, &cor_key_size, &cor_table_name));

            cxr::database_table const& cxr_table(cxr_database.tables()[cxr_table_id]);

            if (cor_row_count > 0)
                c.verify_equals(cor_row_size, cxr_table.row_size());

            c.verify_equals(cor_row_count, cxr_table.row_count());

            // Verify that we correctly compute the offset of each column in each table:
            for (ULONG column_index(0); column_index != cor_column_count; ++column_index)
            {
                // TODO We consolidate the four version number columns into a single column.  We
                // should rework that code so that this verification works for those columns too.
                if (table_index == 0x20 || table_index == 0x23)
                    continue;

                ULONG cor_column_offset(0);
                ULONG cor_column_size(0);
                ULONG cor_column_type(0);
                char const* cor_column_name(nullptr);

                c.verify_success(cor_database->GetColumnInfo(
                    table_index,
                    column_index,
                    &cor_column_offset,
                    &cor_column_size,
                    &cor_column_type,
                    &cor_column_name));

                c.verify_equals(cor_column_offset, cxr_database.tables().table_column_offset(cxr_table_id, static_cast<cxr::column_id>(column_index)));
            }

            // Verify that we correctly read the data for each row.  To verify this, we compare the
            // byte sequences obtained from each database.
            for (ULONG row_index(0); row_index != cor_row_count; ++row_index)
            {
                std::uint8_t const* cor_row_data(nullptr);

                c.verify_success(cor_database->GetRow(table_index, row_index + 1, (void**)&cor_row_data));

                std::uint8_t const* cxr_row_data(cxr_table[row_index]);

                c.verify_range_equals(
                    cor_row_data, cor_row_data + cor_row_size,
                    cxr_row_data, cxr_row_data + cor_row_size);
            }
        }

        return;
    }
    
    auto get_row_count(IMetaDataTables* const cor_database, cxr::table_id const table) -> ULONG
    {
        ULONG cor_row_size(0);
        ULONG cor_row_count(0);
        ULONG cor_column_count(0);
        ULONG cor_key_size(0);
        char const* cor_table_name(nullptr);

        if (FAILED(cor_database->GetTableInfo(
            static_cast<ULONG>(table),
            &cor_row_size,
            &cor_row_count,
            &cor_column_count,
            &cor_key_size,
            &cor_table_name)))
            throw test_error(L"failed to get table info");

        return cor_row_count;
    }





    auto verify_assembly_table(IMetaDataTables* const  cor_database,
                               cxr::database    const& cxr_database,
                               context          const& c) -> void
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::assembly));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            c.verify_equals(1u, cor_row_count);

            mdToken const cor_token(((ULONG)cxr::table_id::assembly << 24) | (i + 1));

            void const*          cor_public_key(nullptr);
            ULONG                cor_public_key_length(0);
            ULONG                cor_hash_algorithm(0);
            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            ASSEMBLYMETADATA     cor_metadata((ASSEMBLYMETADATA()));
            DWORD                cor_flags(0);
            c.verify_success(cor_import->GetAssemblyProps(
                cor_token,
                &cor_public_key,
                &cor_public_key_length,
                &cor_hash_algorithm,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_metadata,
                &cor_flags));

            cxr::const_byte_iterator const cor_public_key_iterator(reinterpret_cast<cxr::const_byte_iterator>(cor_public_key));
            cxr::string_reference    const cor_locale_string(cor_metadata.szLocale == nullptr ? L"" : cor_metadata.szLocale);

            cxr::assembly_token const cxr_token(&cxr_database, cor_token);
            cxr::assembly_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_range_equals(
                cor_public_key_iterator, cor_public_key_iterator + cor_public_key_length,
                begin(cxr_row.public_key()), end(cxr_row.public_key()));

            c.verify_equals(cor_hash_algorithm,                     static_cast<ULONG>(cxr_row.hash_algorithm()));
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());

            c.verify_equals(cor_metadata.usMajorVersion,            cxr_row.version().major());
            c.verify_equals(cor_metadata.usMinorVersion,            cxr_row.version().minor());
            c.verify_equals(cor_metadata.usBuildNumber,             cxr_row.version().build());
            c.verify_equals(cor_metadata.usRevisionNumber,          cxr_row.version().revision());
            c.verify_equals(cor_locale_string,                      cxr_row.culture());
            c.verify_equals(cor_flags,                              cxr_row.flags().integer());

            // Note:  We don't verify the AssemblyOS and AssemblyProcessor tables because they are
            // never to be emitted into metadata, per ECMA 335 II.22.2 and II.22.3
        }
    }
    
    auto verify_assembly_ref_table(IMetaDataTables* const  cor_database,
                                   cxr::database    const& cxr_database,
                                   context          const& c) -> void
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::assembly_ref));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::assembly_ref << 24) | (i + 1));

            void const*          cor_public_key(nullptr);
            ULONG                cor_public_key_length(0);
            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            ASSEMBLYMETADATA     cor_metadata((ASSEMBLYMETADATA()));
            void const*          cor_hash_value(nullptr);
            ULONG                cor_hash_length(0);
            DWORD                cor_flags(0);
            c.verify_success(cor_import->GetAssemblyRefProps(
                cor_token,
                &cor_public_key,
                &cor_public_key_length,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_metadata,
                &cor_hash_value,
                &cor_hash_length,
                &cor_flags));

            cxr::const_byte_iterator const cor_public_key_iterator(reinterpret_cast<cxr::const_byte_iterator>(cor_public_key));
            cxr::const_byte_iterator const cor_hash_value_iterator(reinterpret_cast<cxr::const_byte_iterator>(cor_hash_value));

            cxr::assembly_ref_token const cxr_token(&cxr_database, cor_token);
            cxr::assembly_ref_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_range_equals(
                cor_public_key_iterator, cor_public_key_iterator + cor_public_key_length,
                cxr_row.public_key().begin(), cxr_row.public_key().end());

            c.verify_range_equals(
                cor_hash_value_iterator, cor_hash_value_iterator + cor_hash_length,
                cxr_row.hash_value().begin(), cxr_row.hash_value().end());

            c.verify_equals(cxr::string_reference(cor_name.data()),       cxr_row.name());
            c.verify_equals(cor_metadata.usMajorVersion,                  cxr_row.version().major());
            c.verify_equals(cor_metadata.usMinorVersion,                  cxr_row.version().minor());
            c.verify_equals(cor_metadata.usBuildNumber,                   cxr_row.version().build());
            c.verify_equals(cor_metadata.usRevisionNumber,                cxr_row.version().revision());
            c.verify_equals(cxr::string_reference(cor_metadata.szLocale), cxr_row.culture());
            c.verify_equals(cor_flags,                                    cxr_row.flags().integer());

            // Note:  We don't verify the AssemblyRefOS and AssemblyRefProcessor tables because they
            // are never to be emitted into metadata, per ECMA 335 II.22.6 and II.22.7
        }
    }

    auto verify_class_layout_table(IMetaDataTables* const  cor_database,
                                   cxr::database    const& cxr_database,
                                   context          const& c) -> void
    {
        // Note:  This also verifies the field_layout table.

        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::class_layout));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::class_layout << 24) | (i + 1));

            cxr::class_layout_token const cxr_token(&cxr_database, cor_token);
            cxr::class_layout_row   const cxr_row(cxr_database[cxr_token]);

            DWORD                         cor_pack_size(0);
            std::vector<COR_FIELD_OFFSET> cor_field_offsets(1000);
            ULONG                         cor_field_offsets_count(0);
            ULONG                         cor_class_size(0);
            c.verify_success(cor_import->GetClassLayout(
                cxr_row.parent().value(),
                &cor_pack_size,
                cor_field_offsets.data(),
                cor_field_offsets.size(),
                &cor_field_offsets_count,
                &cor_class_size));

            cor_field_offsets.resize(cor_field_offsets_count);

            c.verify_equals(cor_pack_size,  cxr_row.packing_size());
            c.verify_equals(cor_class_size, cxr_row.class_size());

            cxr::for_all(cor_field_offsets, [&](COR_FIELD_OFFSET const& cor_offset)
            {
                cxr::field_token const cxr_field_token(&cxr_database, cor_offset.ridOfField);
                cxr::field_row   const cxr_field_row(cxr_database[cxr_field_token]);

                cxr::field_layout_row const cxr_field_layout_row(cxr::find_field_layout(cxr_field_token));
                c.verify_equals(cor_offset.ulOffset != static_cast<ULONG>(-1), cxr_field_layout_row.is_initialized());

                if (cxr_field_layout_row.is_initialized())
                    c.verify_equals(cor_offset.ulOffset, cxr_field_layout_row.offset());
            });
        }
    }

    auto verify_custom_attribute_table(IMetaDataTables* const  cor_database,
                                       cxr::database    const& cxr_database,
                                       context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::custom_attribute));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
           mdToken const cor_token(((ULONG)cxr::table_id::custom_attribute << 24) | (i + 1));

            mdToken                  cor_parent(0);
            mdToken                  cor_attribute_type(0);
            void const*              cor_signature(nullptr);
            ULONG                    cor_signature_length(0);
            c.verify_success(cor_import->GetCustomAttributeProps(
                cor_token,
                &cor_parent,
                &cor_attribute_type,
                &cor_signature,
                &cor_signature_length));

            cxr::custom_attribute_token const cxr_token(&cxr_database, cor_token);
            cxr::custom_attribute_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cor_parent,         cxr_row.parent().value());
            c.verify_equals(cor_attribute_type, cxr_row.type().value());

            cxr::const_byte_iterator const cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_signature_length,
                cxr_row.value().begin(), cxr_row.value().end());
        }
    }
    
    auto verify_decl_security_table(IMetaDataTables* const  cor_database,
                                    cxr::database    const& cxr_database,
                                    context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::decl_security));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::decl_security << 24) | (i + 1));

            DWORD       cor_action(0);
            void const* cor_permission(nullptr);
            ULONG       cor_permission_length(0);
            c.verify_success(cor_import->GetPermissionSetProps(
                cor_token,
                &cor_action,
                &cor_permission,
                &cor_permission_length));

            cxr::decl_security_token const cxr_token(&cxr_database, cor_token);
            cxr::decl_security_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cor_action, cxr_row.action());

            cxr::const_byte_iterator const cor_permission_it(reinterpret_cast<cxr::const_byte_iterator>(cor_permission));

            c.verify_range_equals(
                cor_permission_it, cor_permission_it + cor_permission_length,
                cxr_row.permission_set().begin(), cxr_row.permission_set().end());
        }
    }

    auto verify_event_table(IMetaDataTables* const  cor_database,
                            cxr::database    const& cxr_database,
                            context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::event));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
           mdToken const cor_token(((ULONG)cxr::table_id::event << 24) | (i + 1));

            mdTypeDef                cor_class(0);
            std::vector<wchar_t>     cor_name(1000);
            ULONG                    cor_name_length(0);
            ULONG                    cor_flags(0);
            mdTypeDef                cor_type(0);
            mdMethodDef              cor_add(0);
            mdMethodDef              cor_remove(0);
            mdMethodDef              cor_fire(0);
            std::vector<mdMethodDef> cor_other_methods(1000);
            ULONG                    cor_other_methods_count(0);
            c.verify_success(cor_import->GetEventProps(
                cor_token,
                &cor_class,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_flags,
                &cor_type,
                &cor_add,
                &cor_remove,
                &cor_fire,
                cor_other_methods.data(),
                cor_other_methods.size(),
                &cor_other_methods_count));

            cxr::event_token const cxr_token(&cxr_database, cor_token);
            cxr::event_row   const cxr_row(cxr_database[cxr_token]);

            cxr::type_def_row const cxr_owner_row(cxr::find_owner_of_event(cxr_token));

            c.verify_equals(cor_class,                              cxr_owner_row.token().value());
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());
            c.verify_equals(cor_flags,                              cxr_row.flags().integer());
            c.verify_equals(cor_type,                               cxr_row.type().value());

            // Verify the AddOn, RemoveOn, Fire, and Other methods for this property (this, combined
            // with the similar code to verify the Properties table, verifies the MethodSemantics):
            cxr::for_all(cxr::find_method_semantics(cxr_token), [&](cxr::method_semantics_row const& cxr_semantics_row)
            {
                switch (cxr_semantics_row.semantics().integer())
                {
                case cxr::method_semantics_attribute::add_on:
                    c.verify_equals(cor_add, cxr_semantics_row.method().value());
                    break;
                case cxr::method_semantics_attribute::remove_on:
                    c.verify_equals(cor_remove, cxr_semantics_row.method().value());
                    break;
                case cxr::method_semantics_attribute::fire:
                    c.verify_equals(cor_fire, cxr_semantics_row.method().value());
                case cxr::method_semantics_attribute::other:
                    c.verify(std::find(
                        cor_other_methods.begin(),
                        cor_other_methods.end(),
                        cxr_semantics_row.method().value()) != cor_other_methods.end());
                    break;
                default:
                    c.fail();
                    break;
                }
            });

            // Note:  This also verifies the EventMap table, by computing the owner row.
        }
    }

    auto verify_exported_type_table(IMetaDataTables* const  cor_database,
                                    cxr::database    const& cxr_database,
                                    context          const& c) -> void
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::exported_type));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::exported_type << 24) | (i + 1));

            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            mdToken              cor_implementation(0);
            mdToken              cor_type_def(0);
            ULONG                cor_flags(0);
            c.verify_success(cor_import->GetExportedTypeProps(
                cor_token,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_implementation,
                &cor_type_def,
                &cor_flags));

            cxr::exported_type_token const cxr_token(&cxr_database, cor_token);
            cxr::exported_type_row   const cxr_row(cxr_database[cxr_token]);

            cxr::string cxr_type_name(cxr_row.namespace_name().c_str());
            if (!cxr_type_name.empty())
                cxr_type_name.push_back(L'.');
            cxr_type_name += cxr_row.name().c_str();

            c.verify_equals(cxr::string_reference(cor_name.data()), cxr::string_reference(cxr_type_name.c_str()));
            c.verify_equals(cor_implementation,                     cxr_row.implementation().value());
            c.verify_equals(cor_type_def,                           cxr_row.type_def_id());
            c.verify_equals(cor_flags,                              cxr_row.flags().integer());
        }
    }

    auto verify_field_table(IMetaDataTables* const  cor_database,
                            cxr::database    const& cxr_database,
                            context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::field));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
           mdToken const cor_token(((ULONG)cxr::table_id::field << 24) | (i + 1));

            mdTypeDef            cor_owner(0);
            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            DWORD                cor_flags(0);
            PCCOR_SIGNATURE      cor_signature(nullptr);
            ULONG                cor_signature_length(0);
            DWORD                cor_element_type(0);
            UVCP_CONSTANT        cor_constant(nullptr);
            ULONG                cor_constant_length(0);
            c.verify_success(cor_import->GetFieldProps(
                cor_token,
                &cor_owner,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_flags,
                &cor_signature,
                &cor_signature_length,
                &cor_element_type,
                &cor_constant,
                &cor_constant_length));

            cxr::field_token  const cxr_token(&cxr_database, cor_token);
            cxr::field_row    const cxr_row(cxr_database[cxr_token]);

            cxr::type_def_row const cxr_owner_row(cxr::find_owner_of_field(cxr_token));

            c.verify_equals(cor_owner,                              cxr_owner_row.token().value());
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());
            c.verify_equals(cor_flags,                              cxr_row.flags().integer());

            cxr::const_byte_iterator const cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_signature_length,
                cxr_row.signature().begin(), cxr_row.signature().end());

            cxr::constant_row cxr_constant(cxr::find_constant(cxr_row.token()));
            c.verify_equals(cxr_constant.is_initialized(), cor_constant != nullptr);
            
            if (cxr_constant.is_initialized())
            {
                c.verify_equals(cor_element_type, (DWORD)cxr_constant.type());

                cxr::const_byte_iterator const cor_constant_it(reinterpret_cast<cxr::const_byte_iterator>(cor_constant));
                auto const cxr_distance(cxr::distance(cxr_constant.value().begin(), cxr_constant.value().end()));

                // Note:  We cheat here and use the length obtained from the cxr value.  This is
                // because the cor length is reported as zero if the value is not a string.
                c.verify_range_equals(
                    cor_constant_it, cor_constant_it + cxr_distance,
                    cxr_constant.value().begin(), cxr_constant.value().end());
            }
        }
    }

    auto verify_field_marshal_table(IMetaDataTables* const  cor_database,
                                    cxr::database    const& cxr_database,
                                    context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        cxr::for_all(cxr_database.table<cxr::table_id::field_marshal>(), [&](cxr::field_marshal_row const& cxr_row)
        {
            PCCOR_SIGNATURE cor_signature(nullptr);
            ULONG           cor_signature_length(0);
            c.verify_success(cor_import->GetFieldMarshal(
                cxr_row.parent().value(),
                &cor_signature,
                &cor_signature_length));

            cxr::const_byte_iterator const cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_signature_length,
                cxr_row.native_type().begin(), cxr_row.native_type().end());
        });
    }

    auto verify_field_rva_table(IMetaDataTables* const  cor_database,
                                cxr::database    const& cxr_database,
                                context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        cxr::for_all(cxr_database.table<cxr::table_id::field_rva>(), [&](cxr::field_rva_row const& cxr_row)
        {
            ULONG cor_rva(0);
            DWORD cor_flags(0);
            c.verify_success(cor_import->GetRVA(
                cxr_row.parent().value(),
                &cor_rva,
                &cor_flags));

            c.verify_equals(cor_rva, cxr_row.rva());
        });
    }

    auto verify_file_table(IMetaDataTables* const  cor_database,
                           cxr::database    const& cxr_database,
                           context          const& c) -> void
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::file));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::file << 24) | (i + 1));

            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            void const*          cor_hash_value(nullptr);
            ULONG                cor_hash_length(0);
            ULONG                cor_flags(0);
            c.verify_success(cor_import->GetFileProps(
                cor_token,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_hash_value,
                &cor_hash_length,
                &cor_flags));

            cxr::const_byte_iterator const cor_hash_it(reinterpret_cast<cxr::const_byte_iterator>(cor_hash_value));

            cxr::file_token const cxr_token(&cxr_database, cor_token);
            cxr::file_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());

            c.verify_range_equals(
                cor_hash_it, cor_hash_it + cor_hash_length,
                cxr_row.hash_value().begin(), cxr_row.hash_value().end());

            c.verify_equals(cor_flags, cxr_row.flags().integer());
        }
    }

    auto verify_generic_param_table(IMetaDataTables* const  cor_database,
                                    cxr::database    const& cxr_database,
                                    context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport2> cor_import(cor_database);
        
        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::generic_param));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::generic_param << 24) | (i + 1));

            ULONG                cor_sequence(0);
            DWORD                cor_flags(0);
            mdToken              cor_owner(0);
            DWORD                cor_reserved(0);
            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            c.verify_success(cor_import->GetGenericParamProps(
                cor_token,
                &cor_sequence,
                &cor_flags,
                &cor_owner,
                &cor_reserved,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length));

            cxr::generic_param_token const cxr_token(&cxr_database, cor_token);
            cxr::generic_param_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cor_sequence,                           cxr_row.sequence());
            c.verify_equals(cor_flags,                              cxr_row.flags().integer());
            c.verify_equals(cor_owner,                              cxr_row.parent().value());
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());
        }
    }

    auto verify_generic_param_constraint_table(IMetaDataTables* const  cor_database,
                                               cxr::database    const& cxr_database,
                                               context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport2> cor_import(cor_database);
        
        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::generic_param_constraint));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::generic_param_constraint << 24) | (i + 1));

            mdToken cor_owner(0);
            mdToken cor_type(0);
            c.verify_success(cor_import->GetGenericParamConstraintProps(
                cor_token,
                &cor_owner,
                &cor_type));

            cxr::generic_param_constraint_token const cxr_token(&cxr_database, cor_token);
            cxr::generic_param_constraint_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cor_owner, cxr_row.parent().value());
            c.verify_equals(cor_type,  cxr_row.constraint().value());
        }
    }

    auto verify_impl_map_table(IMetaDataTables* const  cor_database,
                               cxr::database    const& cxr_database,
                               context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport2> cor_import(cor_database);
        
        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::impl_map));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::impl_map << 24) | (i + 1));

            cxr::impl_map_token const cxr_token(&cxr_database, cor_token);
            cxr::impl_map_row   const cxr_row(cxr_database[cxr_token]);

            DWORD                cor_flags(0);
            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            mdModuleRef          cor_scope(0);
            c.verify_success(cor_import->GetPinvokeMap(
                cxr_row.member_forwarded().value(),
                &cor_flags,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_scope));

            c.verify_equals(cor_flags,                              cxr_row.flags().integer());
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.import_name());
            c.verify_equals(cor_scope,                              cxr_row.import_scope().value());
            
        }
    }

    auto verify_interface_impl_table(IMetaDataTables* const  cor_database,
                                     cxr::database    const& cxr_database,
                                     context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);
        
        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::interface_impl));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::interface_impl << 24) | (i + 1));

            mdTypeDef cor_class(0);
            mdToken   cor_interface(0);
            c.verify_success(cor_import->GetInterfaceImplProps(cor_token, &cor_class, &cor_interface));

            cxr::interface_impl_token const cxr_token(&cxr_database, cor_token);
            cxr::interface_impl_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cor_class,     cxr_row.parent().value());
            c.verify_equals(cor_interface, cxr_row.interface_().value());
        }
    }

    auto verify_manifest_resource_table(IMetaDataTables* const  cor_database,
                                        cxr::database    const& cxr_database,
                                        context          const& c) -> void
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::manifest_resource));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::manifest_resource << 24) | (i + 1));

            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            mdToken              cor_implementation(0);
            DWORD                cor_offset(0);
            DWORD                cor_flags(0);
            
            c.verify_success(cor_import->GetManifestResourceProps(
                cor_token,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_implementation,
                &cor_offset,
                &cor_flags));

            cxr::manifest_resource_token const cxr_token(&cxr_database, cor_token);
            cxr::manifest_resource_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());

            if ((cor_implementation & 0x00ffffff) != 0)
                c.verify_equals(cor_implementation, cxr_row.implementation().value());
            else
                c.verify(!cxr_row.implementation().is_initialized());

            c.verify_equals(cor_offset, cxr_row.offset());
            c.verify_equals(cor_flags,  cxr_row.flags().integer());
        }
    }

    auto verify_member_ref_table(IMetaDataTables* const  cor_database,
                                 cxr::database    const& cxr_database,
                                 context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::member_ref));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::member_ref << 24) | (i + 1));

            mdToken              cor_ref_token(0);
            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            PCCOR_SIGNATURE      cor_signature(nullptr);
            ULONG                cor_signature_length(0);
            c.verify_success(cor_import->GetMemberRefProps(
                cor_token,
                &cor_ref_token,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_signature,
                &cor_signature_length));

            cxr::const_byte_iterator const cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            cxr::member_ref_token const cxr_token(&cxr_database, cor_token);
            cxr::member_ref_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cor_ref_token,                          cxr_row.parent().value());
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());
            
            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_signature_length,
                cxr_row.signature().begin(), cxr_row.signature().end());
        }
    }

    auto verify_method_def_table(IMetaDataTables* const  cor_database,
                                 cxr::database    const& cxr_database,
                                 context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::method_def));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::method_def << 24) | (i + 1));

            mdToken              cor_class(0);
            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            ULONG                cor_attributes(0);
            PCCOR_SIGNATURE      cor_signature(nullptr);
            ULONG                cor_signature_length(0);
            ULONG                cor_rva(0);
            ULONG                cor_flags(0);
            c.verify_success(cor_import->GetMethodProps(
                cor_token,
                &cor_class,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_attributes,
                &cor_signature,
                &cor_signature_length,
                &cor_rva,
                &cor_flags));

            cxr::const_byte_iterator const cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            cxr::method_def_token const cxr_token(&cxr_database, cor_token);
            cxr::method_def_row   const cxr_row(cxr_database[cxr_token]);
            
            cxr::type_def_row const cxr_owner_row(cxr::find_owner_of_method_def(cxr_token));

            c.verify_equals(cor_class,                              cxr_owner_row.token().value());
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());
            c.verify_equals(cor_attributes,                         cxr_row.flags().integer());
            c.verify_equals(cor_rva,                                cxr_row.rva());
            c.verify_equals(cor_flags,                              cxr_row.implementation_flags().integer());

            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_signature_length,
                cxr_row.signature().begin(), cxr_row.signature().end());
        }
    }

    auto verify_method_impl_table(IMetaDataTables* const  cor_database,
                                  cxr::database    const& cxr_database,
                                  context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::type_def));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::type_def << 24) | (i + 1));

            HCORENUM cor_enum(nullptr);
            std::vector<mdToken> cor_method_bodies(1000);
            std::vector<mdToken> cor_method_decls(1000);
            ULONG cor_count(0);
            c.verify_success(cor_import->EnumMethodImpls(
                &cor_enum,
                cor_token,
                cor_method_bodies.data(),
                cor_method_decls.data(),
                cor_method_bodies.size(),
                &cor_count));

            cor_method_bodies.resize(cor_count);
            cor_method_decls.resize(cor_count);

            std::vector<std::pair<mdToken, mdToken>> cor_methods;
            std::transform(cor_method_bodies.begin(), cor_method_bodies.end(),
                           cor_method_decls.begin(),
                           std::back_inserter(cor_methods),
                           [&](mdToken body, mdToken decl)
            {
                return std::make_pair(body, decl);
            });

            std::sort(cor_methods.begin(), cor_methods.end());

            cxr::type_def_token const cxr_token(&cxr_database, cor_token);
            cxr::type_def_row   const cxr_row(cxr_database[cxr_token]);

            std::vector<std::pair<mdToken, mdToken>> cxr_methods;
            cxr::transform_all(cxr::find_method_impls(cxr_row.token()),
                               std::back_inserter(cxr_methods),
                               [&](cxr::method_impl_row const& cxr_method_row)
            {
                return std::make_pair(
                    cxr_method_row.method_body().value(),
                    cxr_method_row.method_declaration().value());
            });

            std::sort(cxr_methods.begin(), cxr_methods.end());

            c.verify_range_equals(cor_methods.begin(), cor_methods.end(), cxr_methods.begin(), cxr_methods.end());   
        }
    }

    auto verify_method_spec_table(IMetaDataTables* const  cor_database,
                                  cxr::database    const& cxr_database,
                                  context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport2> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::method_spec));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::method_spec << 24) | (i + 1));

            mdToken         cor_parent(0);
            PCCOR_SIGNATURE cor_signature(nullptr);
            ULONG           cor_length(0);
            c.verify_success(cor_import->GetMethodSpecProps(cor_token, &cor_parent, &cor_signature, &cor_length));

            cxr::method_spec_token const cxr_token(&cxr_database, cor_token);
            cxr::method_spec_row   const cxr_row(cxr_database[cxr_token]);

            cxr::const_byte_iterator const cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            c.verify_equals(cor_parent, cxr_row.method().value());

            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_length,
                cxr_row.signature().begin(), cxr_row.signature().end());
        }
    }

    auto verify_module_table(IMetaDataTables* const  cor_database,
                             cxr::database    const& cxr_database,
                             context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::module));

        c.verify_equals(1u, cor_row_count);

        for (ULONG i(0); i < cor_row_count; ++i)
        {
           mdToken const cor_token(((ULONG)cxr::table_id::module << 24) | (i + 1));

            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            GUID                 cor_guid((GUID()));
            c.verify_success(cor_import->GetScopeProps(
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_guid));

            cxr::module_token const cxr_token(&cxr_database, cor_token);
            cxr::module_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());

            c.verify_range_equals(
                cxr::begin_bytes(cor_guid), cxr::end_bytes(cor_guid),
                cxr_row.mvid().begin(), cxr_row.mvid().end());
        }
    }

    auto verify_module_ref_table(IMetaDataTables* const  cor_database,
                                 cxr::database    const& cxr_database,
                                 context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::module_ref));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
           mdToken const cor_token(((ULONG)cxr::table_id::module_ref << 24) | (i + 1));

            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            c.verify_success(cor_import->GetModuleRefProps(
                cor_token,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length));

            cxr::module_ref_token const cxr_token(&cxr_database, cor_token);
            cxr::module_ref_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());
        }
    }

    auto verify_nested_class_table(IMetaDataTables* const  cor_database,
                                   cxr::database    const& cxr_database,
                                   context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::nested_class));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
           mdToken const cor_token(((ULONG)cxr::table_id::nested_class << 24) | (i + 1));

           cxr::nested_class_token const cxr_token(&cxr_database, cor_token);
           cxr::nested_class_row   const cxr_row(cxr_database[cxr_token]);

            mdTypeDef cor_enclosing_class(0);
            c.verify_success(cor_import->GetNestedClassProps(
                cxr_row.nested_class().value(),
                &cor_enclosing_class));

            c.verify_equals(cor_enclosing_class, cxr_row.enclosing_class().value());
        }
    }

    auto verify_param_table(IMetaDataTables* const  cor_database,
                            cxr::database    const& cxr_database,
                            context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::param));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
           mdToken const cor_token(((ULONG)cxr::table_id::param << 24) | (i + 1));

            mdMethodDef          cor_parent_method(0);
            ULONG                cor_sequence(0);
            std::vector<wchar_t> cor_name(1000);
            ULONG                cor_name_length(0);
            DWORD                cor_flags(0);
            DWORD                cor_element_type(0);
            UVCP_CONSTANT        cor_constant(nullptr);
            ULONG                cor_constant_length(0);
            c.verify_success(cor_import->GetParamProps(
                cor_token,
                &cor_parent_method,
                &cor_sequence,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_flags,
                &cor_element_type,
                &cor_constant,
                &cor_constant_length));

            cxr::param_token const cxr_token(&cxr_database, cor_token);
            cxr::param_row   const cxr_row(cxr_database[cxr_token]);

            cxr::method_def_row const cxr_owner_row(cxr::find_owner_of_param(cxr_token));

            c.verify_equals(cor_parent_method,                      cxr_owner_row.token().value());

            c.verify_equals(cor_sequence,                           cxr_row.sequence());
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());
            c.verify_equals(cor_flags,                              cxr_row.flags().integer());

            cxr::constant_row cxr_constant(cxr::find_constant(cxr_row.token()));
            c.verify_equals(cxr_constant.is_initialized(), cor_constant != nullptr);
            
            if (cxr_constant.is_initialized())
            {
                c.verify_equals(cor_element_type, (DWORD)cxr_constant.type());

                cxr::const_byte_iterator const cor_constant_it(reinterpret_cast<cxr::const_byte_iterator>(cor_constant));
                auto const cxr_distance(cxr::distance(cxr_constant.value().begin(), cxr_constant.value().end()));

                // Note:  We cheat here and use the length obtained from the cxr value.  This is
                // because the cor length is reported as zero if the value is not a string.
                c.verify_range_equals(
                    cor_constant_it, cor_constant_it + cxr_distance,
                    cxr_constant.value().begin(), cxr_constant.value().end());
            }
        }
    }

    auto verify_property_table(IMetaDataTables* const  cor_database,
                               cxr::database    const& cxr_database,
                               context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::property));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
           mdToken const cor_token(((ULONG)cxr::table_id::property << 24) | (i + 1));

            mdTypeDef                cor_class(0);
            std::vector<wchar_t>     cor_name(1000);
            ULONG                    cor_name_length(0);
            ULONG                    cor_flags(0);
            PCCOR_SIGNATURE          cor_signature(nullptr);
            ULONG                    cor_signature_length(0);
            DWORD                    cor_element_type(0);
            UVCP_CONSTANT            cor_constant(nullptr);
            ULONG                    cor_constant_length(0);
            mdMethodDef              cor_setter(0);
            mdMethodDef              cor_getter(0);
            std::vector<mdMethodDef> cor_other_methods(1000);
            ULONG                    cor_other_methods_count(0);

            c.verify_success(cor_import->GetPropertyProps(
                cor_token,
                &cor_class,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_flags,
                &cor_signature,
                &cor_signature_length,
                &cor_element_type,
                &cor_constant,
                &cor_constant_length,
                &cor_setter,
                &cor_getter,
                cor_other_methods.data(),
                cor_other_methods.size(),
                &cor_other_methods_count));

            cor_other_methods.resize(cor_other_methods_count);

            cxr::const_byte_iterator const cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            cxr::property_token const cxr_token(&cxr_database, cor_token);
            cxr::property_row   const cxr_row(cxr_database[cxr_token]);

            cxr::type_def_row   const cxr_owner_row(cxr::find_owner_of_property(cxr_token));

            c.verify_equals(cor_class,                              cxr_owner_row.token().value());
            c.verify_equals(cxr::string_reference(cor_name.data()), cxr_row.name());
            c.verify_equals(cor_flags,                              cxr_row.flags().integer());

            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_signature_length,
                cxr_row.signature().begin(), cxr_row.signature().end());

            // Verify the Getter, Setter, and Other methods for this property (this, combined with
            // the similar code to verify the Event table, verifies the MethodSemantics table):
            cxr::for_all(cxr::find_method_semantics(cxr_row.token()), [&](cxr::method_semantics_row const& cxr_semantics_row)
            {
                switch (cxr_semantics_row.semantics().integer())
                {
                case cxr::method_semantics_attribute::getter:
                    c.verify_equals(cor_getter, cxr_semantics_row.method().value());
                    break;
                case cxr::method_semantics_attribute::setter:
                    c.verify_equals(cor_setter, cxr_semantics_row.method().value());
                    break;
                case cxr::method_semantics_attribute::other:
                    c.verify(std::find(
                        cor_other_methods.begin(),
                        cor_other_methods.end(),
                        cxr_semantics_row.method().value()) != cor_other_methods.end());
                    break;
                default:
                    c.fail();
                    break;
                }
            });

            cxr::constant_row cxr_constant(cxr::find_constant(cxr_row.token()));
            c.verify_equals(cxr_constant.is_initialized(), cor_constant != nullptr);
            
            if (cxr_constant.is_initialized())
            {
                c.verify_equals(cor_element_type, (DWORD)cxr_constant.type());

                cxr::const_byte_iterator const cor_constant_it(reinterpret_cast<cxr::const_byte_iterator>(cor_constant));
                auto const cxr_distance(cxr::distance(cxr_constant.value().begin(), cxr_constant.value().end()));

                // Note:  We cheat here and use the length obtained from the cxr value.  This is
                // because the cor length is reported as zero if the value is not a string.
                c.verify_range_equals(
                    cor_constant_it, cor_constant_it + cxr_distance,
                    cxr_constant.value().begin(), cxr_constant.value().end());
            }

            // Note:  This also verifies the PropertyMap table, by checking the owner row.
        }
    }

    auto verify_property_map_table(IMetaDataTables* const  cor_database,
                                   cxr::database    const& cxr_database,
                                   context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        cxr::for_all(cxr_database.table<cxr::table_id::property_map>(), [&](cxr::property_map_row const& cxr_row)
        {
            mdToken const cor_token(cxr_row.parent().value());

            HCORENUM             cor_enum(nullptr);
            std::vector<mdToken> cor_properties(10000);
            ULONG                cor_property_count(0);
            c.verify_success(cor_import->EnumProperties(
                &cor_enum,
                cor_token,
                cor_properties.data(),
                cor_properties.size(),
                &cor_property_count));

            cxr::scope_guard const cleanup_enum([&]{ cor_import->CloseEnum(cor_enum); });

            // We assume that there are no more than 10,000 properties of any type, for simplicity:
            c.verify(cor_property_count < 10000);

            // Truncate the sequence to what was actually inserted:
            cor_properties.resize(cor_property_count);
            
            // The list should be sorted, but let's sort it again anyway, to be sure:
            std::sort(begin(cor_properties), end(cor_properties));

            // Verify that there are no holes in the cxr range and that the begin and end match our
            // expected begin and end from cxr.
            if (cor_properties.size() > 0)
            {
                for (unsigned i(1); i < cor_properties.size(); ++i)
                {
                    c.verify_equals(cor_properties[i - 1] + 1, cor_properties[i]);
                }

                c.verify_equals(cor_properties.front(), cxr_row.first_property().value());

                // Subtract 1 because cxr uses one-past-the-end, but cor only returns the elements
                c.verify_equals(cor_properties.back(),  cxr_row.last_property().value() - 1);
            }
            else
            {
                // If the cor range is empty, verify that the cxr range is empty too:
                c.verify_equals(cxr_row.first_property(), cxr_row.last_property());
            }
        });
    }

    auto verify_standalone_sig_table(IMetaDataTables* const  cor_database,
                                     cxr::database    const& cxr_database,
                                     context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::standalone_sig));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::standalone_sig << 24) | (i + 1));

            PCCOR_SIGNATURE cor_signature(nullptr);
            ULONG           cor_length(0);
            c.verify_success(cor_import->GetSigFromToken(cor_token, &cor_signature, &cor_length));

            cxr::standalone_sig_token const cxr_token(&cxr_database, cor_token);
            cxr::standalone_sig_row   const cxr_row(cxr_database[cxr_token]);

            cxr::const_byte_iterator const cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_length,
                cxr_row.signature().begin(), cxr_row.signature().end());
        }
    }

    auto verify_type_def_table(IMetaDataTables* const  cor_database,
                               cxr::database    const& cxr_database,
                               context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::type_def));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::type_def << 24) | (i + 1));

            std::vector<wchar_t> cor_name(1024);
            ULONG                cor_name_length(0);
            DWORD                cor_flags(0);
            mdToken              cor_extends(0);
            c.verify_success(cor_import->GetTypeDefProps(
                cor_token,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length,
                &cor_flags,
                &cor_extends));

            cxr::string_reference const cor_name_string(cor_name.data());

            cxr::type_def_token const cxr_token(&cxr_database, cor_token);
            cxr::type_def_row   const cxr_row(cxr_database[cxr_token]);

            cxr::string cxr_type_name(cxr_row.namespace_name().c_str());
            if (!cxr_type_name.empty())
                cxr_type_name.push_back(L'.');
            cxr_type_name += cxr_row.name().c_str();

            c.verify_equals(cor_name_string, cxr::string_reference(cxr_type_name.c_str()));

            c.verify_equals(cor_flags, cxr_row.flags());
            
            if ((cor_extends & 0x00ffffff) != 0)
                c.verify_equals(cor_extends, cxr_row.extends().value());
            else
                c.verify(cxr_row.is_initialized());

            // Note that we verify field and method ownership in the Field and MethodDef table
            // verification.
        }
    }

    auto verify_type_ref_table(IMetaDataTables* const  cor_database,
                               cxr::database    const& cxr_database,
                               context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::type_ref));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::type_ref << 24) | (i + 1));

            mdToken              cor_scope(0);
            std::vector<wchar_t> cor_name(1024);
            ULONG                cor_name_length(0);
            c.verify_success(cor_import->GetTypeRefProps(
                cor_token,
                &cor_scope,
                cor_name.data(),
                cor_name.size(),
                &cor_name_length));

            cxr::string_reference const cor_name_string(cor_name.data());

            cxr::type_ref_token const cxr_token(&cxr_database, cor_token);
            cxr::type_ref_row   const cxr_row(cxr_database[cxr_token]);

            c.verify_equals(cor_scope,       cxr_row.resolution_scope().value());
            c.verify_equals(cor_name_string, cxr_row.name());
        }
    }

    auto verify_type_spec_table(IMetaDataTables* const  cor_database,
                                cxr::database    const& cxr_database,
                                context          const& c) -> void
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> cor_import(cor_database);

        ULONG const cor_row_count(get_row_count(cor_database, cxr::table_id::type_spec));

        for (ULONG i(0); i < cor_row_count; ++i)
        {
            mdToken const cor_token(((ULONG)cxr::table_id::type_spec << 24) | (i + 1));

            PCCOR_SIGNATURE cor_signature(nullptr);
            ULONG           cor_length(0);
            c.verify_success(cor_import->GetTypeSpecFromToken(
                cor_token,
                &cor_signature,
                &cor_length));

            cxr::const_byte_iterator cor_signature_it(reinterpret_cast<cxr::const_byte_iterator>(cor_signature));

            cxr::type_spec_token const cxr_token(&cxr_database, cor_token);
            cxr::type_spec_row   const cxr_row(cxr_database[cxr_token]);
            cxr::blob            const cxr_signature(cxr_row.signature());

            c.verify_range_equals(
                cor_signature_it, cor_signature_it + cor_length,
                cxr_signature.begin(), cxr_signature.end());
        }
    }

} }

namespace cxxreflect_test {

    CXXREFLECTTEST_REGISTER_NAMED(metadata_database_fundamental_functionality_test, [](context const& c)
    {
        // Verifies that we correctly read the metadata table structure and that we correctly read
        // the correct sequence of bytes for each row in every table.  This does not verify that we
        // interpret the data correctly, just that we read the right data.
        setup_and_call(verify_database, c);
    });
    
    #define CXXREFLECTTEST_REGISTER_TABLE_TEST(t)                                           \
        CXXREFLECTTEST_REGISTER_NAMED(metadata_database_tables_ ## t, [&](context const& c) \
        {                                                                                   \
            setup_and_call(verify_ ## t ## _table, c);                                      \
        })
    
    CXXREFLECTTEST_REGISTER_TABLE_TEST(assembly);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(assembly_ref);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(class_layout);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(custom_attribute);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(decl_security);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(event);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(exported_type);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(field);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(field_marshal);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(field_rva);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(file);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(generic_param);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(generic_param_constraint);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(impl_map);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(interface_impl);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(manifest_resource);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(member_ref);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(method_def);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(method_impl);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(method_spec);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(module);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(module_ref);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(nested_class);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(param);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(property);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(property_map);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(standalone_sig);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(type_def);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(type_ref);
    CXXREFLECTTEST_REGISTER_TABLE_TEST(type_spec);

    #undef CXXREFLECTTEST_REGISTER_TABLE_TEST
    
}

#endif
