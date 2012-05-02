
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Basic functionality tests for the Metadata::Database and related classes.

#include "Context.hpp"
#include "CxxReflect/CxxReflect.hpp"

#include <atlbase.h>
#include <cor.h>
#include <metahost.h>

#if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86

namespace cxr
{
    using namespace CxxReflect;
    using namespace CxxReflect::Detail;
    using namespace CxxReflect::Metadata;
}

namespace CxxReflectTest { namespace {

    // A helper to ensure calls to CoInitializeEx and CoUninitialize stay balanced.
    class GuardedCoInitialize
    {
    public:

        GuardedCoInitialize()  { _shouldUninitialize = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED) >= 0; }
        ~GuardedCoInitialize() { if (_shouldUninitialize) { ::CoUninitialize(); }                            }

    private:

        bool _shouldUninitialize;
    };

    CComPtr<IMetaDataDispenser> GetMetaDataDispenser()
    {
        CComPtr<IMetaDataDispenser> dispenser;
        int const hr(::CoCreateInstance(
            CLSID_CorMetaDataDispenser,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_IMetaDataDispenser,
            reinterpret_cast<void**>(&dispenser)));

        if (hr < 0 || dispenser == nullptr)
            throw TestError(L"Failed to create CLR Metadata Dispenser");

        return dispenser;
    }

    CComPtr<IMetaDataTables> GetMetaDataTables(IMetaDataDispenser* const dispenser, String const& assemblyPath)
    {
        CComPtr<IMetaDataImport> import;
        int const hr0(dispenser->OpenScope(
            assemblyPath.c_str(),
            ofRead,
            IID_IMetaDataImport,
            reinterpret_cast<IUnknown**>(&import)));

        if (hr0 < 0 || import == nullptr)
            throw TestError(L"Failed to import assembly");

        CComQIPtr<IMetaDataTables, &IID_IMetaDataTables> tables(import);
        return tables;
    }

    // All of our tests require similar setup to initialize the databases.  This does that setup.
    template <typename TCallable>
    void SetupAndCall(TCallable&& callable, Context& c)
    {
        GuardedCoInitialize const coInitialize;

        String const assemblyPath(c.GetProperty(KnownProperty::PrimaryAssemblyPath));

        CComPtr<IMetaDataDispenser> const mdDispenser(GetMetaDataDispenser());
        CComPtr<IMetaDataTables>    const mdTables(GetMetaDataTables(mdDispenser, assemblyPath));

        cxr::Database const cxrDatabase(cxr::Database::CreateFromFile(assemblyPath.c_str()));

        callable(mdTables, cxrDatabase, c);
    }





    void VerifyDatabase(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        // Ensure that both databases report the same number of rows:
        ULONG corTableCount(0);
        c.VerifySuccess(corDatabase->GetNumTables(&corTableCount));
        c.VerifyEquals(corTableCount, (ULONG)cxr::TableIdCount);

        for (ULONG tableIndex(0); tableIndex != corTableCount; ++tableIndex)
        {
            if (!cxr::IsValidTableId(tableIndex))
                continue;

            cxr::TableId const tableId(static_cast<cxr::TableId>(tableIndex));

            // First, verify that we compute basic properties of the table correctly:
            ULONG corRowSize(0);
            ULONG corRowCount(0);
            ULONG corColumnCount(0);
            ULONG corKeySize(0);
            char const* corTableName(nullptr);

            c.VerifySuccess(corDatabase->GetTableInfo(
                tableIndex, &corRowSize, &corRowCount, &corColumnCount, &corKeySize, &corTableName));

            cxr::Table const& cxrTable(cxrDatabase.GetTables().GetTable(tableId));

            if (corRowCount > 0)
                c.VerifyEquals(corRowSize, cxrTable.GetRowSize());

            c.VerifyEquals(corRowCount, cxrTable.GetRowCount());

            // Verify that we correctly compute the offset of each column in each table:
            for (ULONG columnIndex(0); columnIndex != corColumnCount; ++columnIndex)
            {
                // TODO We consolidate the four version number columns into a single column.  We
                // should rework that code so that this verification works for those columns too.
                if (tableIndex == 0x20 || tableIndex == 0x23)
                    continue;

                ULONG corColumnOffset(0);
                ULONG corColumnSize(0);
                ULONG corColumnType(0);
                char const* corColumnName(nullptr);

                c.VerifySuccess(corDatabase->GetColumnInfo(
                    tableIndex, columnIndex, &corColumnOffset, &corColumnSize, &corColumnType, &corColumnName));

                c.VerifyEquals(corColumnOffset, cxrDatabase.GetTables().GetTableColumnOffset(tableId, columnIndex));
            }

            // Verify that we correctly read the data for each row.  To verify this, we compare the
            // byte sequences obtained from each database.
            for (ULONG rowIndex(0); rowIndex != corRowCount; ++rowIndex)
            {
                cxr::ConstByteIterator corRowData(nullptr);

                c.VerifySuccess(corDatabase->GetRow(tableIndex, rowIndex + 1, (void**)&corRowData));

                cxr::ConstByteIterator cxrRowData(cxrTable.At(rowIndex));

                c.VerifyRangeEquals(corRowData, corRowData + corRowSize, cxrRowData, cxrRowData + corRowSize);
            }
        }

        return;
    }

    void VerifyInterfaceImpl(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);
        
        ULONG corRowSize(0);
        ULONG corRowCount(0);
        ULONG corColumnCount(0);
        ULONG corKeySize(0);
        char const* corTableName(nullptr);

        c.VerifySuccess(corDatabase->GetTableInfo(
            (ULONG)cxr::TableId::InterfaceImpl, &corRowSize, &corRowCount, &corColumnCount, &corKeySize, &corTableName));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdInterfaceImpl const ifToken(((ULONG)cxr::TableId::InterfaceImpl << 24) | (i + 1));

            mdTypeDef corIfClass(0);
            mdToken   corIfInterface(0);
            c.VerifySuccess(corImport->GetInterfaceImplProps(ifToken, &corIfClass, &corIfInterface));

            cxr::RowReference     const cxrIfRowRef(cxr::RowReference::FromToken(ifToken));
            cxr::InterfaceImplRow const cxrIfRow(cxrDatabase.GetRow<cxr::TableId::InterfaceImpl>(cxrIfRowRef));

            c.VerifyEquals(corIfClass,     cxrIfRow.GetClass().GetToken());
            c.VerifyEquals(corIfInterface, cxrIfRow.GetInterface().GetToken());
        }
    }

    // TODO Tests for the rest of the metadata tables.

} }

namespace CxxReflectTest { namespace {

    CXXREFLECTTEST_REGISTER(MetadataDatabase_CoreFunctionalityTest, [](Context& c)
    {
        // Verifies that we correctly read the metadata table structure and that we correctly read
        // the correct sequence of bytes for each row in every table.  This does not verify that we
        // interpret the data correctly, just that we read the right data.
        SetupAndCall(VerifyDatabase, c);
    });

    CXXREFLECTTEST_REGISTER(MetaDataDatabase_InterfaceImplTest, [&](Context& c)
    {
        // Verifies that we interpret the data from the InterfaceImpl table correctly.
        SetupAndCall(VerifyInterfaceImpl, c);
    });

} }

#endif // CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86
