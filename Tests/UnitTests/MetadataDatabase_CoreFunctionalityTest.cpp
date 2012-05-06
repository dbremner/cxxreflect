
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

#include "Context.hpp"
#include "CxxReflect/CxxReflect.hpp"

#include <atlbase.h>
#include <cor.h>
#include <metahost.h>

// mscoree.lib is required for CLRCreateInstance(), which we use to start up Mr. CLR.
#pragma comment(lib, "mscoree.lib")

// Disable the narrowing conversion warnings so that we compile cleanly for x64.  We aren't going
// to overflow with any of our calculations.
#pragma warning(disable: 4267)

namespace cxr
{
    using namespace CxxReflect;
    using namespace CxxReflect::Detail;
    using namespace CxxReflect::Metadata;
}

namespace CxxReflectTest { namespace {

    /// A helper to ensure that calls to CoInitializeEx and CoUninitialize stay balanced.
    class GuardedCoInitialize
    {
    public:

        GuardedCoInitialize()
        {
            if (cxr::Failed(::CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
                throw TestError(L"Failed to CoInitialize");
        }

        ~GuardedCoInitialize()
        {
            ::CoUninitialize();
        }
    };

    /// Starts the v4.0 CLR and gets the metadata dispenser from it.
    ///
    /// Note that we can't just CoCreateInstance an CLSID_CorMetaDataDispenser because it defaults
    /// to the .NET 2.0 runtime, which is not installed by default on Windows 8.  
    CComPtr<IMetaDataDispenser> GetMetaDataDispenser()
    {
        CComPtr<ICLRMetaHost> metaHost;
        int const hr0(::CLRCreateInstance(
            CLSID_CLRMetaHost,
            IID_ICLRMetaHost,
            reinterpret_cast<void**>(&metaHost)));

        if (cxr::Failed(hr0) || metaHost == nullptr)
            throw TestError(L"Failed to instantiate CLRMetaHost");

        CComPtr<ICLRRuntimeInfo> runtimeInfo;
        int const hr1(metaHost->GetRuntime(
            L"v4.0.30319",
            IID_ICLRRuntimeInfo,
            reinterpret_cast<void**>(&runtimeInfo)));

        if (cxr::Failed(hr1) || runtimeInfo == nullptr)
            throw TestError(L"Failed to get v4.0 runtime");

        CComPtr<IMetaDataDispenser> dispenser;
        int const hr2(runtimeInfo->GetInterface(
            CLSID_CorMetaDataDispenser,
            IID_IMetaDataDispenser,
            reinterpret_cast<void**>(&dispenser)));

        if (cxr::Failed(hr2) || dispenser == nullptr)
            throw TestError(L"Failed to obtain dispenser from runtime");

        return dispenser;
    }

    /// Loads an assembly using an IMetaDataDispenser and returns its IMetaDataTables interface.
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

    ULONG GetRowCount(IMetaDataTables* const corDatabase, cxr::TableId const tableId)
    {
        ULONG corRowSize(0);
        ULONG corRowCount(0);
        ULONG corColumnCount(0);
        ULONG corKeySize(0);
        char const* corTableName(nullptr);

        if (cxr::Failed(corDatabase->GetTableInfo(
            (ULONG)tableId,
            &corRowSize,
            &corRowCount,
            &corColumnCount,
            &corKeySize,
            &corTableName)))
            throw TestError(L"Failed to get table info");

        return corRowCount;
    }





    void VerifyAssemblyTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::Assembly));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            c.VerifyEquals(1u, corRowCount);

            mdToken const corToken(((ULONG)cxr::TableId::Assembly << 24) | (i + 1));

            void const*          corPublicKey(nullptr);
            ULONG                corPublicKeyLength(0);
            ULONG                corHashAlgorithm(0);
            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            ASSEMBLYMETADATA     corMetadata((ASSEMBLYMETADATA()));
            DWORD                corFlags(0);
            c.VerifySuccess(corImport->GetAssemblyProps(
                corToken,
                &corPublicKey,
                &corPublicKeyLength,
                &corHashAlgorithm,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corMetadata,
                &corFlags));

            cxr::ConstByteIterator const corPublicKeyIterator(reinterpret_cast<cxr::ConstByteIterator>(corPublicKey));
            cxr::StringReference   const corLocaleString(corMetadata.szLocale == nullptr ? L"" : corMetadata.szLocale);

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::AssemblyRow  const cxrRow(cxrDatabase.GetRow<cxr::TableId::Assembly>(cxrRowRef));

            c.VerifyRangeEquals(
                corPublicKeyIterator, corPublicKeyIterator + corPublicKeyLength,
                cxrRow.GetPublicKey().Begin(), cxrRow.GetPublicKey().End());

            c.VerifyEquals(corHashAlgorithm,                     (ULONG)cxrRow.GetHashAlgorithm());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());

            c.VerifyEquals(corMetadata.usMajorVersion,           cxrRow.GetVersion().GetMajor());
            c.VerifyEquals(corMetadata.usMinorVersion,           cxrRow.GetVersion().GetMinor());
            c.VerifyEquals(corMetadata.usBuildNumber,            cxrRow.GetVersion().GetBuild());
            c.VerifyEquals(corMetadata.usRevisionNumber,         cxrRow.GetVersion().GetRevision());
            c.VerifyEquals(corLocaleString,                      cxrRow.GetCulture());
            c.VerifyEquals(corFlags,                             cxrRow.GetFlags().GetIntegral());

            // Note:  We don't verify the AssemblyOS and AssemblyProcessor tables because they are
            // never to be emitted into metadata, per ECMA 335 II.22.2 and II.22.3
        }
    }

    void VerifyAssemblyRefTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::AssemblyRef));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::AssemblyRef << 24) | (i + 1));

            void const*          corPublicKey(nullptr);
            ULONG                corPublicKeyLength(0);
            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            ASSEMBLYMETADATA     corMetadata((ASSEMBLYMETADATA()));
            void const*          corHashValue(nullptr);
            ULONG                corHashLength(0);
            DWORD                corFlags(0);
            c.VerifySuccess(corImport->GetAssemblyRefProps(
                corToken,
                &corPublicKey,
                &corPublicKeyLength,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corMetadata,
                &corHashValue,
                &corHashLength,
                &corFlags));

            cxr::ConstByteIterator const corPublicKeyIterator(reinterpret_cast<cxr::ConstByteIterator>(corPublicKey));
            cxr::ConstByteIterator const corHashValueIterator(reinterpret_cast<cxr::ConstByteIterator>(corHashValue));

            cxr::RowReference   const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::AssemblyRefRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::AssemblyRef>(cxrRowRef));

            c.VerifyRangeEquals(
                corPublicKeyIterator, corPublicKeyIterator + corPublicKeyLength,
                cxrRow.GetPublicKey().Begin(), cxrRow.GetPublicKey().End());

            c.VerifyRangeEquals(
                corHashValueIterator, corHashValueIterator + corPublicKeyLength,
                cxrRow.GetHashValue().Begin(), cxrRow.GetHashValue().End());

            c.VerifyEquals(cxr::StringReference(corName.data()),       cxrRow.GetName());
            c.VerifyEquals(corMetadata.usMajorVersion,                 cxrRow.GetVersion().GetMajor());
            c.VerifyEquals(corMetadata.usMinorVersion,                 cxrRow.GetVersion().GetMinor());
            c.VerifyEquals(corMetadata.usBuildNumber,                  cxrRow.GetVersion().GetBuild());
            c.VerifyEquals(corMetadata.usRevisionNumber,               cxrRow.GetVersion().GetRevision());
            c.VerifyEquals(cxr::StringReference(corMetadata.szLocale), cxrRow.GetCulture());
            c.VerifyEquals(corFlags,                                   cxrRow.GetFlags().GetIntegral());

            // Note:  We don't verify the AssemblyRefOS and AssemblyRefProcessor tables because they
            // are never to be emitted into metadata, per ECMA 335 II.22.6 and II.22.7
        }
    }

    void VerifyClassLayoutTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        // Note:  This also verifies the FieldLayout table.

        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::ClassLayout));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::ClassLayout << 24) | (i + 1));

            cxr::RowReference   const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::ClassLayoutRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::ClassLayout>(cxrRowRef));

            DWORD                         corPackSize(0);
            std::vector<COR_FIELD_OFFSET> corFieldOffsets(1000);
            ULONG                         corFieldOffsetsCount(0);
            ULONG                         corClassSize(0);
            c.VerifySuccess(corImport->GetClassLayout(
                cxrRow.GetParent().GetToken(),
                &corPackSize,
                corFieldOffsets.data(),
                corFieldOffsets.size(),
                &corFieldOffsetsCount,
                &corClassSize));

            corFieldOffsets.resize(corFieldOffsetsCount);

            c.VerifyEquals(corPackSize,  cxrRow.GetPackingSize());
            c.VerifyEquals(corClassSize, cxrRow.GetClassSize());

            std::for_each(corFieldOffsets.begin(), corFieldOffsets.end(), [&](COR_FIELD_OFFSET const& corOffset)
            {
                cxr::RowReference const cxrFieldRef(cxr::RowReference::FromToken(corOffset.ridOfField));
                cxr::FieldRow const cxrFieldRow(cxrDatabase.GetRow<cxr::TableId::Field>(cxrFieldRef));

                cxr::FieldLayoutRow const cxrFieldLayoutRow(cxr::GetFieldLayout(cxrFieldRow.GetSelfFullReference()));
                c.VerifyEquals(corOffset.ulOffset != static_cast<ULONG>(-1), cxrFieldLayoutRow.IsInitialized());

                if (cxrFieldLayoutRow.IsInitialized())
                    c.VerifyEquals(corOffset.ulOffset, cxrFieldLayoutRow.GetOffset());
            });
        }
    }

    void VerifyCustomAttributeTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::CustomAttribute));

        for (ULONG i(0); i < corRowCount; ++i)
        {
           mdToken const corToken(((ULONG)cxr::TableId::CustomAttribute << 24) | (i + 1));

            mdToken                  corParent(0);
            mdToken                  corAttributeType(0);
            void const*              corSignature(nullptr);
            ULONG                    corSignatureLength(0);
            c.VerifySuccess(corImport->GetCustomAttributeProps(
                corToken,
                &corParent,
                &corAttributeType,
                &corSignature,
                &corSignatureLength));

            cxr::RowReference       const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::CustomAttributeRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::CustomAttribute>(cxrRowRef));

            c.VerifyEquals(corParent,        cxrRow.GetParent().GetToken());
            c.VerifyEquals(corAttributeType, cxrRow.GetType().GetToken());

            cxr::ConstByteIterator const corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corSignatureLength,
                cxrRow.GetValue().Begin(), cxrRow.GetValue().End());
        }
    }

    void VerifyDeclSecurityTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::DeclSecurity));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::DeclSecurity << 24) | (i + 1));

            DWORD        corAction(0);
            void const* corPermission(nullptr);
            ULONG        corPermissionLength(0);
            c.VerifySuccess(corImport->GetPermissionSetProps(
                corToken,
                &corAction,
                &corPermission,
                &corPermissionLength));

            cxr::RowReference    const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::DeclSecurityRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::DeclSecurity>(cxrRowRef));

            c.VerifyEquals(corAction,        cxrRow.GetAction());

            cxr::ConstByteIterator const corPermissionIt(reinterpret_cast<cxr::ConstByteIterator>(corPermission));

            c.VerifyRangeEquals(
                corPermissionIt, corPermissionIt + corPermissionLength,
                cxrRow.GetPermissionSet().Begin(), cxrRow.GetPermissionSet().End());
        }
    }

    void VerifyEventTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::Event));

        for (ULONG i(0); i < corRowCount; ++i)
        {
           mdToken const corToken(((ULONG)cxr::TableId::Event << 24) | (i + 1));

            mdTypeDef                corClass(0);
            std::vector<wchar_t>     corName(1000);
            ULONG                    corNameLength(0);
            ULONG                    corFlags(0);
            mdTypeDef                corType(0);
            mdMethodDef              corAdd(0);
            mdMethodDef              corRemove(0);
            mdMethodDef              corFire(0);
            std::vector<mdMethodDef> corOtherMethods(1000);
            ULONG                    corOtherMethodsCount(0);
            c.VerifySuccess(corImport->GetEventProps(
                corToken,
                &corClass,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corFlags,
                &corType,
                &corAdd,
                &corRemove,
                &corFire,
                corOtherMethods.data(),
                corOtherMethods.size(),
                &corOtherMethodsCount));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::EventRow     const cxrRow(cxrDatabase.GetRow<cxr::TableId::Event>(cxrRowRef));

            cxr::TypeDefRow   const cxrOwnerRow(cxr::GetOwnerOfEvent(cxrRow));

            c.VerifyEquals(corClass,                             cxrOwnerRow.GetSelfReference().GetToken());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());
            c.VerifyEquals(corFlags,                             cxrRow.GetFlags().GetIntegral());
            c.VerifyEquals(corType,                              cxrRow.GetType().GetToken());

            // Verify the AddOn, RemoveOn, Fire, and Other methods for this property (this, combined
            // with the similar code to verify the Properties table, verifies the MethodSemantics):
            std::for_each(cxr::BeginMethodSemantics(cxr::FullReference(&cxrDatabase, cxrRow.GetSelfReference())),
                          cxr::EndMethodSemantics  (cxr::FullReference(&cxrDatabase, cxrRow.GetSelfReference())),
                          [&](cxr::MethodSemanticsRow const& cxrSemanticsRow)
            {
                switch (cxrSemanticsRow.GetSemantics().GetIntegral())
                {
                case cxr::MethodSemanticsAttribute::AddOn:
                    c.VerifyEquals(corAdd, cxrSemanticsRow.GetMethod().GetToken());
                    break;
                case cxr::MethodSemanticsAttribute::RemoveOn:
                    c.VerifyEquals(corRemove, cxrSemanticsRow.GetMethod().GetToken());
                    break;
                case cxr::MethodSemanticsAttribute::Fire:
                    c.VerifyEquals(corFire, cxrSemanticsRow.GetMethod().GetToken());
                case cxr::MethodSemanticsAttribute::Other:
                    c.Verify(std::find(
                        corOtherMethods.begin(),
                        corOtherMethods.end(),
                        cxrSemanticsRow.GetMethod().GetToken()) != corOtherMethods.end());
                    break;
                default:
                    c.Fail();
                    break;
                }
            });

            // Note:  This also verifies the EventMap table, by computing the owner row.
        }
    }

    void VerifyExportedTypeTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::ExportedType));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::ExportedType << 24) | (i + 1));

            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            mdToken              corImplementation(0);
            mdToken              corTypeDef(0);
            ULONG                corFlags(0);
            c.VerifySuccess(corImport->GetExportedTypeProps(
                corToken,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corImplementation,
                &corTypeDef,
                &corFlags));

            cxr::RowReference    const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::ExportedTypeRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::ExportedType>(cxrRowRef));

            cxr::String cxxTypeName(cxrRow.GetNamespace().c_str());
            if (!cxxTypeName.empty())
                cxxTypeName.push_back(L'.');
            cxxTypeName += cxrRow.GetName().c_str();

            c.VerifyEquals(cxr::StringReference(corName.data()), cxr::StringReference(cxxTypeName.c_str()));
            c.VerifyEquals(corImplementation,                    cxrRow.GetImplementation().GetToken());
            c.VerifyEquals(corTypeDef,                           cxrRow.GetTypeDefId());
            c.VerifyEquals(corFlags,                             cxrRow.GetFlags().GetIntegral());
        }
    }

    void VerifyFieldTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::Field));

        for (ULONG i(0); i < corRowCount; ++i)
        {
           mdToken const corToken(((ULONG)cxr::TableId::Field << 24) | (i + 1));

            mdTypeDef            corOwner(0);
            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            DWORD                corFlags(0);
            PCCOR_SIGNATURE      corSignature(nullptr);
            ULONG                corSignatureLength(0);
            DWORD                corElementType(0);
            UVCP_CONSTANT        corConstant(nullptr);
            ULONG                corConstantLength(0);
            c.VerifySuccess(corImport->GetFieldProps(
                corToken,
                &corOwner,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corFlags,
                &corSignature,
                &corSignatureLength,
                &corElementType,
                &corConstant,
                &corConstantLength));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::FieldRow     const cxrRow(cxrDatabase.GetRow<cxr::TableId::Field>(cxrRowRef));

            cxr::TypeDefRow   const cxrOwnerRow(cxr::GetOwnerOfField(cxrRow));

            c.VerifyEquals(corOwner,                             cxrOwnerRow.GetSelfReference().GetToken());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());
            c.VerifyEquals(corFlags,                             cxrRow.GetFlags().GetIntegral());

            cxr::ConstByteIterator const corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corSignatureLength,
                cxrRow.GetSignature().Begin(), cxrRow.GetSignature().End());

            cxr::ConstantRow cxrConstant(cxr::GetConstant(cxrRow.GetSelfFullReference()));
            c.VerifyEquals(cxrConstant.IsInitialized(), corConstant != nullptr);
            
            if (cxrConstant.IsInitialized())
            {
                c.VerifyEquals(corElementType, (DWORD)cxrConstant.GetElementType());

                cxr::ConstByteIterator const corConstantIt(reinterpret_cast<cxr::ConstByteIterator>(corConstant));
                auto const cxrDistance(cxr::Distance(cxrConstant.GetValue().Begin(), cxrConstant.GetValue().End()));

                // Note:  We cheat here and use the length obtained from the cxr value.  This is
                // because the cor length is reported as zero if the value is not a string.
                c.VerifyRangeEquals(
                    corConstantIt, corConstantIt + cxrDistance,
                    cxrConstant.GetValue().Begin(), cxrConstant.GetValue().End());
            }
        }
    }

    void VerifyFieldMarshalTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        std::for_each(cxrDatabase.Begin<cxr::TableId::FieldMarshal>(),
                      cxrDatabase.End<cxr::TableId::FieldMarshal>(),
                      [&](cxr::FieldMarshalRow const& cxrRow)
        {
            PCCOR_SIGNATURE corSignature(nullptr);
            ULONG           corSignatureLength(0);
            c.VerifySuccess(corImport->GetFieldMarshal(
                cxrRow.GetParent().GetToken(),
                &corSignature,
                &corSignatureLength));

            cxr::ConstByteIterator const corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corSignatureLength,
                cxrRow.GetNativeType().Begin(), cxrRow.GetNativeType().End());
        });
    }

    void VerifyFieldRvaTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        std::for_each(cxrDatabase.Begin<cxr::TableId::FieldRva>(),
                      cxrDatabase.End<cxr::TableId::FieldRva>(),
                      [&](cxr::FieldRvaRow const& cxrRow)
        {
            ULONG corRva(0);
            DWORD corFlags(0);
            c.VerifySuccess(corImport->GetRVA(
                cxrRow.GetParent().GetToken(),
                &corRva,
                &corFlags));

            c.VerifyEquals(corRva, cxrRow.GetRva());
        });
    }

    void VerifyFileTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::File));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::File << 24) | (i + 1));

            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            void const*          corHashValue(nullptr);
            ULONG                corHashLength(0);
            ULONG                corFlags(0);
            c.VerifySuccess(corImport->GetFileProps(
                corToken,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corHashValue,
                &corHashLength,
                &corFlags));

            cxr::ConstByteIterator const corHashIt(reinterpret_cast<cxr::ConstByteIterator>(corHashValue));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::FileRow      const cxrRow(cxrDatabase.GetRow<cxr::TableId::File>(cxrRowRef));

            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());

            c.VerifyRangeEquals(
                corHashIt, corHashIt + corHashLength,
                cxrRow.GetHashValue().Begin(), cxrRow.GetHashValue().End());

            c.VerifyEquals(corFlags,                             cxrRow.GetFlags().GetIntegral());
        }
    }

    void VerifyGenericParamTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport2> corImport(corDatabase);
        
        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::GenericParam));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::GenericParam << 24) | (i + 1));

            ULONG                corSequence(0);
            DWORD                corFlags(0);
            mdToken              corOwner(0);
            DWORD                corReserved(0);
            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            c.VerifySuccess(corImport->GetGenericParamProps(
                corToken,
                &corSequence,
                &corFlags,
                &corOwner,
                &corReserved,
                corName.data(),
                corName.size(),
                &corNameLength));

            cxr::RowReference    const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::GenericParamRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::GenericParam>(cxrRowRef));

            c.VerifyEquals(corSequence,                          cxrRow.GetSequence());
            c.VerifyEquals(corFlags,                             cxrRow.GetFlags().GetIntegral());
            c.VerifyEquals(corOwner,                             cxrRow.GetParent().GetToken());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());
        }
    }

    void VerifyGenericParamConstraintTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport2> corImport(corDatabase);
        
        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::GenericParamConstraint));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::GenericParamConstraint << 24) | (i + 1));

            mdToken corOwner(0);
            mdToken corType(0);
            c.VerifySuccess(corImport->GetGenericParamConstraintProps(
                corToken,
                &corOwner,
                &corType));

            cxr::RowReference              const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::GenericParamConstraintRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::GenericParamConstraint>(cxrRowRef));

            c.VerifyEquals(corOwner, cxrRow.GetParent().GetToken());
            c.VerifyEquals(corType,  cxrRow.GetConstraint().GetToken());
        }
    }

    void VerifyImplMapTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport2> corImport(corDatabase);
        
        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::ImplMap));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::ImplMap << 24) | (i + 1));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::ImplMapRow   const cxrRow(cxrDatabase.GetRow<cxr::TableId::ImplMap>(cxrRowRef));

            DWORD                corFlags(0);
            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            mdModuleRef          corScope(0);
            c.VerifySuccess(corImport->GetPinvokeMap(
                cxrRow.GetMemberForwarded().GetToken(),
                &corFlags,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corScope));

            c.VerifyEquals(corFlags,                             cxrRow.GetMappingFlags().GetIntegral());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetImportName());
            c.VerifyEquals(corScope,                             cxrRow.GetImportScope().GetToken());
            
        }
    }

    void VerifyInterfaceImplTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);
        
        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::InterfaceImpl));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::InterfaceImpl << 24) | (i + 1));

            mdTypeDef corClass(0);
            mdToken   corInterface(0);
            c.VerifySuccess(corImport->GetInterfaceImplProps(corToken, &corClass, &corInterface));

            cxr::RowReference     const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::InterfaceImplRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::InterfaceImpl>(cxrRowRef));

            c.VerifyEquals(corClass,     cxrRow.GetClass().GetToken());
            c.VerifyEquals(corInterface, cxrRow.GetInterface().GetToken());
        }
    }

    void VerifyManifestResourceTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::ManifestResource));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::ManifestResource << 24) | (i + 1));

            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            mdToken              corImplementation(0);
            DWORD                corOffset(0);
            DWORD                corFlags(0);
            
            c.VerifySuccess(corImport->GetManifestResourceProps(
                corToken,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corImplementation,
                &corOffset,
                &corFlags));

            cxr::RowReference        const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::ManifestResourceRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::ManifestResource>(cxrRowRef));

            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());

            if ((corImplementation & 0x00ffffff) != 0)
                c.VerifyEquals(corImplementation, cxrRow.GetImplementation().GetToken());
            else
                c.VerifyEquals(corImplementation & 0x00ffffff, cxrRow.GetImplementation().GetToken() & 0x00ffffff);

            c.VerifyEquals(corOffset, cxrRow.GetOffset());
            c.VerifyEquals(corFlags,  cxrRow.GetFlags().GetIntegral());
        }
    }

    void VerifyMemberRefTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::MemberRef));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::MemberRef << 24) | (i + 1));

            mdToken              corRefToken(0);
            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            PCCOR_SIGNATURE      corSignature(nullptr);
            ULONG                corSignatureLength(0);
            c.VerifySuccess(corImport->GetMemberRefProps(
                corToken,
                &corRefToken,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corSignature,
                &corSignatureLength));

            cxr::ConstByteIterator const corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::MemberRefRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::MemberRef>(cxrRowRef));

            c.VerifyEquals(corRefToken,                          cxrRow.GetClass().GetToken());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());
            
            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corSignatureLength,
                cxrRow.GetSignature().Begin(), cxrRow.GetSignature().End());
        }
    }

    void VerifyMethodDefTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::MethodDef));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::MethodDef << 24) | (i + 1));

            mdToken              corClass(0);
            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            ULONG                corAttributes(0);
            PCCOR_SIGNATURE      corSignature(nullptr);
            ULONG                corSignatureLength(0);
            ULONG                corRva(0);
            ULONG                corFlags(0);
            c.VerifySuccess(corImport->GetMethodProps(
                corToken,
                &corClass,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corAttributes,
                &corSignature,
                &corSignatureLength,
                &corRva,
                &corFlags));

            cxr::ConstByteIterator const corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::MethodDefRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::MethodDef>(cxrRowRef));
            
            cxr::TypeDefRow const cxrOwnerRow(cxr::GetOwnerOfMethodDef(cxrRow));

            c.VerifyEquals(corClass,                             cxrOwnerRow.GetSelfReference().GetToken());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());
            c.VerifyEquals(corAttributes,                        cxrRow.GetFlags().GetIntegral());
            c.VerifyEquals(corRva,                               cxrRow.GetRva());
            c.VerifyEquals(corFlags,                             cxrRow.GetImplementationFlags().GetIntegral());

            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corSignatureLength,
                cxrRow.GetSignature().Begin(), cxrRow.GetSignature().End());
        }
    }

    void VerifyMethodImplTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::TypeDef));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::TypeDef << 24) | (i + 1));

            HCORENUM corEnum(nullptr);
            std::vector<mdToken> corMethodBodies(1000);
            std::vector<mdToken> corMethodDecls(1000);
            ULONG corCount(0);
            c.VerifySuccess(corImport->EnumMethodImpls(
                &corEnum,
                corToken,
                corMethodBodies.data(),
                corMethodDecls.data(),
                corMethodBodies.size(),
                &corCount));

            corMethodBodies.resize(corCount);
            corMethodDecls.resize(corCount);

            std::vector<std::pair<mdToken, mdToken>> corMethods;
            std::transform(corMethodBodies.begin(), corMethodBodies.end(),
                           corMethodDecls.begin(),
                           std::back_inserter(corMethods),
                           [&](mdToken body, mdToken decl)
            {
                return std::make_pair(body, decl);
            });

            std::sort(corMethods.begin(), corMethods.end());

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::TypeDefRow   const cxrRow(cxrDatabase.GetRow<cxr::TableId::TypeDef>(cxrRowRef));

            std::vector<std::pair<mdToken, mdToken>> cxrMethods;
            std::transform(cxr::BeginMethodImpls(cxrRow.GetSelfFullReference()),
                           cxr::EndMethodImpls(cxrRow.GetSelfFullReference()),
                           std::back_inserter(cxrMethods),
                           [&](cxr::MethodImplRow const& cxrMethodRow)
            {
                return std::make_pair(
                    cxrMethodRow.GetMethodBody().GetToken(),
                    cxrMethodRow.GetMethodDeclaration().GetToken());
            });

            std::sort(cxrMethods.begin(), cxrMethods.end());

            c.VerifyRangeEquals(corMethods.begin(), corMethods.end(), cxrMethods.begin(), cxrMethods.end());   
        }
    }

    void VerifyMethodSpecTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport2, &IID_IMetaDataImport2> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::MethodSpec));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::MethodSpec << 24) | (i + 1));

            mdToken         corParent(0);
            PCCOR_SIGNATURE corSignature(nullptr);
            ULONG           corLength(0);
            c.VerifySuccess(corImport->GetMethodSpecProps(corToken, &corParent, &corSignature, &corLength));

            cxr::RowReference  const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::MethodSpecRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::MethodSpec>(cxrRowRef));

            cxr::ConstByteIterator const corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            c.VerifyEquals(corParent, cxrRow.GetMethod().GetToken());

            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corLength,
                cxrRow.GetSignature().Begin(), cxrRow.GetSignature().End());
        }
    }

    void VerifyModuleTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::Module));

        c.VerifyEquals(1u, corRowCount);

        for (ULONG i(0); i < corRowCount; ++i)
        {
           mdToken const corToken(((ULONG)cxr::TableId::Module << 24) | (i + 1));

            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            GUID                 corGuid((GUID()));
            c.VerifySuccess(corImport->GetScopeProps(
                corName.data(),
                corName.size(),
                &corNameLength,
                &corGuid));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::ModuleRow    const cxrRow(cxrDatabase.GetRow<cxr::TableId::Module>(cxrRowRef));

            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());

            c.VerifyRangeEquals(
                cxr::BeginBytes(corGuid), cxr::EndBytes(corGuid),
                cxrRow.GetMvid().Begin(), cxrRow.GetMvid().End());
        }
    }

    void VerifyModuleRefTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::ModuleRef));

        for (ULONG i(0); i < corRowCount; ++i)
        {
           mdToken const corToken(((ULONG)cxr::TableId::ModuleRef << 24) | (i + 1));

            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            c.VerifySuccess(corImport->GetModuleRefProps(
                corToken,
                corName.data(),
                corName.size(),
                &corNameLength));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::ModuleRefRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::ModuleRef>(cxrRowRef));

            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());
        }
    }

    void VerifyNestedClassTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::NestedClass));

        for (ULONG i(0); i < corRowCount; ++i)
        {
           mdToken const corToken(((ULONG)cxr::TableId::NestedClass << 24) | (i + 1));

           cxr::RowReference   const cxrRowRef(cxr::RowReference::FromToken(corToken));
           cxr::NestedClassRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::NestedClass>(cxrRowRef));

            mdTypeDef corEnclosingClass(0);
            c.VerifySuccess(corImport->GetNestedClassProps(
                cxrRow.GetNestedClass().GetToken(),
                &corEnclosingClass));

            c.VerifyEquals(corEnclosingClass, cxrRow.GetEnclosingClass().GetToken());
        }
    }

    void VerifyParamTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::Param));

        for (ULONG i(0); i < corRowCount; ++i)
        {
           mdToken const corToken(((ULONG)cxr::TableId::Param << 24) | (i + 1));

            mdMethodDef          corParentMethod(0);
            ULONG                corSequence(0);
            std::vector<wchar_t> corName(1000);
            ULONG                corNameLength(0);
            DWORD                corFlags(0);
            DWORD                corElementType(0);
            UVCP_CONSTANT        corConstant(nullptr);
            ULONG                corConstantLength(0);
            c.VerifySuccess(corImport->GetParamProps(
                corToken,
                &corParentMethod,
                &corSequence,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corFlags,
                &corElementType,
                &corConstant,
                &corConstantLength));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::ParamRow     const cxrRow(cxrDatabase.GetRow<cxr::TableId::Param>(cxrRowRef));

            cxr::MethodDefRow const cxrOwnerRow(cxr::GetOwnerOfParam(cxrRow));

            c.VerifyEquals(corParentMethod,                      cxrOwnerRow.GetSelfReference().GetToken());

            c.VerifyEquals(corSequence,                          cxrRow.GetSequence());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());
            c.VerifyEquals(corFlags,                             cxrRow.GetFlags().GetIntegral());

            cxr::ConstantRow cxrConstant(cxr::GetConstant(cxrRow.GetSelfFullReference()));
            c.VerifyEquals(cxrConstant.IsInitialized(), corConstant != nullptr);
            
            if (cxrConstant.IsInitialized())
            {
                c.VerifyEquals(corElementType, (DWORD)cxrConstant.GetElementType());

                cxr::ConstByteIterator const corConstantIt(reinterpret_cast<cxr::ConstByteIterator>(corConstant));
                auto const cxrDistance(cxr::Distance(cxrConstant.GetValue().Begin(), cxrConstant.GetValue().End()));

                // Note:  We cheat here and use the length obtained from the cxr value.  This is
                // because the cor length is reported as zero if the value is not a string.
                c.VerifyRangeEquals(
                    corConstantIt, corConstantIt + cxrDistance,
                    cxrConstant.GetValue().Begin(), cxrConstant.GetValue().End());
            }
        }
    }

    void VerifyPropertyTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::Property));

        for (ULONG i(0); i < corRowCount; ++i)
        {
           mdToken const corToken(((ULONG)cxr::TableId::Property << 24) | (i + 1));

            mdTypeDef                corClass(0);
            std::vector<wchar_t>     corName(1000);
            ULONG                    corNameLength(0);
            ULONG                    corFlags(0);
            PCCOR_SIGNATURE          corSignature(nullptr);
            ULONG                    corSignatureLength(0);
            DWORD                    corElementType(0);
            UVCP_CONSTANT            corConstant(nullptr);
            ULONG                    corConstantLength(0);
            mdMethodDef              corSetter(0);
            mdMethodDef              corGetter(0);
            std::vector<mdMethodDef> corOtherMethods(1000);
            ULONG                    corOtherMethodsCount(0);

            c.VerifySuccess(corImport->GetPropertyProps(
                corToken,
                &corClass,
                corName.data(),
                corName.size(),
                &corNameLength,
                &corFlags,
                &corSignature,
                &corSignatureLength,
                &corElementType,
                &corConstant,
                &corConstantLength,
                &corSetter,
                &corGetter,
                corOtherMethods.data(),
                corOtherMethods.size(),
                &corOtherMethodsCount));

            corOtherMethods.resize(corOtherMethodsCount);

            cxr::ConstByteIterator const corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::PropertyRow  const cxrRow(cxrDatabase.GetRow<cxr::TableId::Property>(cxrRowRef));

            cxr::TypeDefRow   const cxrOwnerRow(cxr::GetOwnerOfProperty(cxrRow));

            c.VerifyEquals(corClass,                             cxrOwnerRow.GetSelfReference().GetToken());
            c.VerifyEquals(cxr::StringReference(corName.data()), cxrRow.GetName());
            c.VerifyEquals(corFlags,                             cxrRow.GetFlags().GetIntegral());

            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corSignatureLength,
                cxrRow.GetSignature().Begin(), cxrRow.GetSignature().End());

            // Verify the Getter, Setter, and Other methods for this property (this, combined with
            // the similar code to verify the Event table, verifies the MethodSemantics table):
            std::for_each(cxr::BeginMethodSemantics(cxr::FullReference(&cxrDatabase, cxrRow.GetSelfReference())),
                          cxr::EndMethodSemantics  (cxr::FullReference(&cxrDatabase, cxrRow.GetSelfReference())),
                          [&](cxr::MethodSemanticsRow const& cxrSemanticsRow)
            {
                switch (cxrSemanticsRow.GetSemantics().GetIntegral())
                {
                case cxr::MethodSemanticsAttribute::Getter:
                    c.VerifyEquals(corGetter, cxrSemanticsRow.GetMethod().GetToken());
                    break;
                case cxr::MethodSemanticsAttribute::Setter:
                    c.VerifyEquals(corSetter, cxrSemanticsRow.GetMethod().GetToken());
                    break;
                case cxr::MethodSemanticsAttribute::Other:
                    c.Verify(std::find(
                        corOtherMethods.begin(),
                        corOtherMethods.end(),
                        cxrSemanticsRow.GetMethod().GetToken()) != corOtherMethods.end());
                    break;
                default:
                    c.Fail();
                    break;
                }
            });

            cxr::ConstantRow cxrConstant(cxr::GetConstant(cxrRow.GetSelfFullReference()));
            c.VerifyEquals(cxrConstant.IsInitialized(), corConstant != nullptr);
            
            if (cxrConstant.IsInitialized())
            {
                c.VerifyEquals(corElementType, (DWORD)cxrConstant.GetElementType());

                cxr::ConstByteIterator const corConstantIt(reinterpret_cast<cxr::ConstByteIterator>(corConstant));
                auto const cxrDistance(cxr::Distance(cxrConstant.GetValue().Begin(), cxrConstant.GetValue().End()));

                // Note:  We cheat here and use the length obtained from the cxr value.  This is
                // because the cor length is reported as zero if the value is not a string.
                c.VerifyRangeEquals(
                    corConstantIt, corConstantIt + cxrDistance,
                    cxrConstant.GetValue().Begin(), cxrConstant.GetValue().End());
            }

            // Note:  This also verifies the PropertyMap table, by checking the owner row.
        }
    }

    void VerifyPropertyMapTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        std::for_each(cxrDatabase.Begin<cxr::TableId::PropertyMap>(),
                      cxrDatabase.End<cxr::TableId::PropertyMap>(),
                      [&](cxr::PropertyMapRow const& cxrRow)
        {
            mdToken const corToken(cxrRow.GetParent().GetToken());

            HCORENUM             corEnum(nullptr);
            std::vector<mdToken> corProperties(10000);
            ULONG                corPropertyCount(0);
            c.VerifySuccess(corImport->EnumProperties(
                &corEnum,
                corToken,
                corProperties.data(),
                corProperties.size(),
                &corPropertyCount));

            cxr::ScopeGuard const cleanupEnum([&]{ corImport->CloseEnum(corEnum); });

            // We assume that there are no more than 10,000 properties of any type, for simplicity:
            c.Verify(corPropertyCount < 10000);

            // Truncate the sequence to what was actually inserted:
            corProperties.resize(corPropertyCount);
            
            // The list should be sorted, but let's sort it again anyway, to be sure:
            std::sort(begin(corProperties), end(corProperties));

            // Verify that there are no holes in the cxr range and that the begin and end match our
            // expected begin and end from cxr.
            if (corProperties.size() > 0)
            {
                for (unsigned i(1); i < corProperties.size(); ++i)
                {
                    c.VerifyEquals(corProperties[i - 1] + 1, corProperties[i]);
                }

                c.VerifyEquals(corProperties.front(), cxrRow.GetFirstProperty().GetToken());

                // Subtract 1 because cxr uses one-past-the-end, but cor only returns the elements
                c.VerifyEquals(corProperties.back(),  cxrRow.GetLastProperty().GetToken() - 1);
            }
            else
            {
                // If the cor range is empty, verify that the cxr range is empty too:
                c.VerifyEquals(cxrRow.GetFirstProperty(), cxrRow.GetLastProperty());
            }
        });
    }

    void VerifyStandaloneSigTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::StandaloneSig));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::StandaloneSig << 24) | (i + 1));

            PCCOR_SIGNATURE corSignature(nullptr);
            ULONG           corLength(0);
            c.VerifySuccess(corImport->GetSigFromToken(corToken, &corSignature, &corLength));

            cxr::RowReference     const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::StandaloneSigRow const cxrRow(cxrDatabase.GetRow<cxr::TableId::StandaloneSig>(cxrRowRef));

            cxr::ConstByteIterator const corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corLength,
                cxrRow.GetSignature().Begin(), cxrRow.GetSignature().End());
        }
    }

    void VerifyTypeDefTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::TypeDef));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::TypeDef << 24) | (i + 1));

            std::vector<wchar_t> corName(1024);
            ULONG                corNameLength(0);
            DWORD                corFlags(0);
            mdToken              corExtends(0);
            c.VerifySuccess(corImport->GetTypeDefProps(
                corToken,
                corName.data(), corName.size(), &corNameLength,
                &corFlags, &corExtends));

            cxr::StringReference const corNameString(corName.data());

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::TypeDefRow   const cxrRow(cxrDatabase.GetRow<cxr::TableId::TypeDef>(cxrRowRef));

            cxr::String cxxTypeName(cxrRow.GetNamespace().c_str());
            if (!cxxTypeName.empty())
                cxxTypeName.push_back(L'.');
            cxxTypeName += cxrRow.GetName().c_str();

            c.VerifyEquals(corNameString, cxr::StringReference(cxxTypeName.c_str()));

            c.VerifyEquals(corFlags,   cxrRow.GetFlags());
            
            if ((corExtends & 0x00ffffff) != 0)
                c.VerifyEquals(corExtends, cxrRow.GetExtends().GetToken());
            else
                c.VerifyEquals(corExtends & 0x00ffffff, cxrRow.GetExtends().GetToken() & 0x00ffffff);

            // Note that we verify field and method ownership in the Field and MethodDef table
            // verification.
        }
    }

    void VerifyTypeRefTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::TypeRef));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::TypeRef << 24) | (i + 1));

            mdToken              corScope(0);
            std::vector<wchar_t> corName(1024);
            ULONG                corNameLength(0);
            c.VerifySuccess(corImport->GetTypeRefProps(
                corToken,
                &corScope,
                corName.data(),
                corName.size(),
                &corNameLength));

            cxr::StringReference const corNameString(corName.data());

            cxr::RowReference const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::TypeRefRow   const cxrRow(cxrDatabase.GetRow<cxr::TableId::TypeRef>(cxrRowRef));

            c.VerifyEquals(corScope,      cxrRow.GetResolutionScope().GetToken());
            c.VerifyEquals(corNameString, cxrRow.GetName());
        }
    }

    void VerifyTypeSpecTable(IMetaDataTables* const corDatabase, cxr::Database const& cxrDatabase, Context& c)
    {
        CComQIPtr<IMetaDataImport, &IID_IMetaDataImport> corImport(corDatabase);

        ULONG const corRowCount(GetRowCount(corDatabase, cxr::TableId::TypeSpec));

        for (ULONG i(0); i < corRowCount; ++i)
        {
            mdToken const corToken(((ULONG)cxr::TableId::TypeSpec << 24) | (i + 1));

            PCCOR_SIGNATURE corSignature(nullptr);
            ULONG           corLength(0);
            c.VerifySuccess(corImport->GetTypeSpecFromToken(
                corToken,
                &corSignature,
                &corLength));

            cxr::ConstByteIterator corSignatureIt(reinterpret_cast<cxr::ConstByteIterator>(corSignature));

            cxr::RowReference  const cxrRowRef(cxr::RowReference::FromToken(corToken));
            cxr::TypeSpecRow   const cxrRow(cxrDatabase.GetRow<cxr::TableId::TypeSpec>(cxrRowRef));
            cxr::BlobReference const cxrSignature(cxrRow.GetSignature());

            c.VerifyRangeEquals(
                corSignatureIt, corSignatureIt + corLength,
                cxrSignature.Begin(), cxrSignature.End());
        }
    }

} }

namespace CxxReflectTest { namespace {

    CXXREFLECTTEST_REGISTER(MetadataDatabase_CoreFunctionalityTest, [](Context& c)
    {
        // Verifies that we correctly read the metadata table structure and that we correctly read
        // the correct sequence of bytes for each row in every table.  This does not verify that we
        // interpret the data correctly, just that we read the right data.
        SetupAndCall(VerifyDatabase, c);
    });

    #define CXXREFLECTTEST_REGISTERTABLETEST(t)                         \
        CXXREFLECTTEST_REGISTER(MetaDataDatabase_ ## t, [&](Context& c) \
        {                                                               \
            SetupAndCall(Verify ## t ## Table, c);                      \
        })

    CXXREFLECTTEST_REGISTERTABLETEST(Assembly);
    CXXREFLECTTEST_REGISTERTABLETEST(AssemblyRef);
    CXXREFLECTTEST_REGISTERTABLETEST(ClassLayout);
    CXXREFLECTTEST_REGISTERTABLETEST(CustomAttribute);
    CXXREFLECTTEST_REGISTERTABLETEST(DeclSecurity);
    CXXREFLECTTEST_REGISTERTABLETEST(Event);
    CXXREFLECTTEST_REGISTERTABLETEST(ExportedType);
    CXXREFLECTTEST_REGISTERTABLETEST(Field);
    CXXREFLECTTEST_REGISTERTABLETEST(FieldMarshal);
    CXXREFLECTTEST_REGISTERTABLETEST(FieldRva);
    CXXREFLECTTEST_REGISTERTABLETEST(File);
    CXXREFLECTTEST_REGISTERTABLETEST(GenericParam);
    CXXREFLECTTEST_REGISTERTABLETEST(GenericParamConstraint);
    CXXREFLECTTEST_REGISTERTABLETEST(ImplMap);
    CXXREFLECTTEST_REGISTERTABLETEST(InterfaceImpl);
    CXXREFLECTTEST_REGISTERTABLETEST(ManifestResource);
    CXXREFLECTTEST_REGISTERTABLETEST(MemberRef);
    CXXREFLECTTEST_REGISTERTABLETEST(MethodDef);
    CXXREFLECTTEST_REGISTERTABLETEST(MethodImpl);
    CXXREFLECTTEST_REGISTERTABLETEST(MethodSpec);
    CXXREFLECTTEST_REGISTERTABLETEST(Module);
    CXXREFLECTTEST_REGISTERTABLETEST(ModuleRef);
    CXXREFLECTTEST_REGISTERTABLETEST(NestedClass);
    CXXREFLECTTEST_REGISTERTABLETEST(Param);
    CXXREFLECTTEST_REGISTERTABLETEST(Property);
    CXXREFLECTTEST_REGISTERTABLETEST(PropertyMap);
    CXXREFLECTTEST_REGISTERTABLETEST(StandaloneSig);
    CXXREFLECTTEST_REGISTERTABLETEST(TypeDef);
    CXXREFLECTTEST_REGISTERTABLETEST(TypeRef);
    CXXREFLECTTEST_REGISTERTABLETEST(TypeSpec);

    #undef CXXREFLECTTEST_REGISTERTABLETEST

} }
