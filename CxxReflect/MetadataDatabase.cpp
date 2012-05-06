
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect { namespace Metadata { namespace { namespace Private {

    // The PE headers and related structures are naturally aligned, so we shouldn't need any custom
    // pragmas, attributes, or directives to pack the structures.  We use static assertions to ensure
    // that there is no padding, just in case.

    struct PeVersion
    {
        std::uint16_t _major;
        std::uint16_t _minor;
    };

    static_assert(sizeof(PeVersion) == 4, "Invalid PeVersion Definition");

    struct PeRvaAndSize
    {
        std::uint32_t _rva;
        std::uint32_t _size;
    };

    static_assert(sizeof(PeRvaAndSize) == 8, "Invalid PeRvaAndSize Definition");

    struct PeFileHeader
    {
        // PE Signature;
        std::uint32_t _signature;

        // PE Header
        std::uint16_t _machine;
        std::uint16_t _sectionCount;
        std::uint32_t _creationTimestamp;
        std::uint32_t _symbolTablePointer;
        std::uint32_t _symbolCount;
        std::uint16_t _optionalHeaderSize;
        std::uint16_t _characteristics;

        // PE Optional Header Standard Fields
        std::uint16_t _magic;
        std::uint16_t _majorMinor;
        std::uint32_t _codeSize;
        std::uint32_t _initializedDataSize;
        std::uint32_t _uninitializedDataSize;
        std::uint32_t _entryPointRva;
        std::uint32_t _codeRva;
        std::uint32_t _dataRva;

        // PE Optional Header Windows NT-Specific Fields
        std::uint32_t _imageBase;
        std::uint32_t _sectionAlignment;
        std::uint32_t _fileAlignment;
        PeVersion     _osVersion;
        PeVersion     _userVersion;
        PeVersion     _subsystemVersion;
        std::uint16_t _reserved;
        std::uint32_t _imageSize;
        std::uint32_t _headerSize;
        std::uint32_t _fileChecksum;
        std::uint16_t _subsystem;
        std::uint16_t _dllFlags;
        std::uint32_t _stackReserveSize;
        std::uint32_t _stackCommitSize;
        std::uint32_t _heapReserveSize;
        std::uint32_t _heapCommitSize;
        std::uint32_t _loaderFlags;
        std::uint32_t _dataDirectoryCount;

        // Data Directories
        PeRvaAndSize  _exportTable;
        PeRvaAndSize  _importTable;
        PeRvaAndSize  _resourceTable;
        PeRvaAndSize  _exceptionTable;
        PeRvaAndSize  _certificateTable;
        PeRvaAndSize  _baseRelocationTable;
        PeRvaAndSize  _debugTable;
        PeRvaAndSize  _copyrightTable;
        PeRvaAndSize  _globalPointerTable;
        PeRvaAndSize  _threadLocalStorageTable;
        PeRvaAndSize  _loadConfigTable;
        PeRvaAndSize  _boundImportTable;
        PeRvaAndSize  _importAddressTable;
        PeRvaAndSize  _delayImportDescriptorTable;
        PeRvaAndSize  _cliHeaderTable;
        PeRvaAndSize  _reservedTableHeader;
    };

    static_assert(sizeof(PeFileHeader) == 248, "Invalid PeFileHeader Definition");

    struct PeSectionHeader
    {
        char          _name[8];
        std::uint32_t _virtualSize;
        std::uint32_t _virtualAddress;

        std::uint32_t _rawDataSize;
        std::uint32_t _rawDataOffset;

        std::uint32_t _relocationsOffset;
        std::uint32_t _lineNumbersOffset;
        std::uint16_t _relocationsCount;
        std::uint16_t _lineNumbersCount;

        std::uint32_t _characteristics;
    };

    static_assert(sizeof(PeSectionHeader) == 40, "Invalid PeSectionHeader Definition");

    typedef std::vector<PeSectionHeader> PeSectionHeaderSequence;

    struct PeCliHeader
    {
        std::uint32_t _sizeInBytes;
        PeVersion     _runtimeVersion;
        PeRvaAndSize  _metadata;
        std::uint32_t _flags;
        std::uint32_t _entryPointToken;
        PeRvaAndSize  _resources;
        PeRvaAndSize  _strongNameSignature;
        PeRvaAndSize  _codeManagerTable;
        PeRvaAndSize  _vtableFixups;
        PeRvaAndSize  _exportAddressTableJumps;
        PeRvaAndSize  _managedNativeHeader;
    };

    static_assert(sizeof(PeCliHeader) == 72, "Invalid PeCliHeader Definition");

    enum class PeCliStreamKind
    {
        StringStream     = 0x0,
        UserStringStream = 0x1,
        BlobStream       = 0x2,
        GuidStream       = 0x3,
        TableStream      = 0x4
    };

    // This does not map directly to file data and has no alignment constraints
    struct PeCliStreamHeader
    {
        std::uint32_t   _metadataOffset;
        std::uint32_t   _streamOffset;
        std::uint32_t   _streamSize;
    };

    typedef std::array<PeCliStreamHeader, 5> PeCliStreamHeaderSequence;

    SizeType ComputeOffsetFromRva(PeSectionHeader section, PeRvaAndSize rvaAndSize)
    {
        return rvaAndSize._rva - section._virtualAddress + section._rawDataOffset;
    }

    struct RawFourComponentVersion
    {
        std::uint16_t _major;
        std::uint16_t _minor;
        std::uint16_t _build;
        std::uint16_t _revision;
    };

    static_assert(sizeof(RawFourComponentVersion) == 8, "Invalid FourComponentVersion Definition");

    struct PeSectionsAndCliHeader
    {
        PeSectionHeaderSequence _sections;
        PeCliHeader             _cliHeader;
    };

    // Predicate that tests whether a PE section contains a specified RVA
    class PeSectionContainsRva
    {
    public:

        explicit PeSectionContainsRva(std::uint32_t rva)
            : _rva(rva)
        {
        }

        bool operator()(PeSectionHeader const& section) const
        {
            return _rva >= section._virtualAddress
                && _rva < section._virtualAddress + section._virtualSize;
        }

    private:

        std::uint32_t _rva;
    };

    PeSectionsAndCliHeader ReadPeSectionsAndCliHeader(Detail::ConstByteCursor file)
    {
        // The index of the PE Header is located at index 0x3c of the DOS header
        file.Seek(0x3c, Detail::ConstByteCursor::Begin);
        
        std::uint32_t fileHeaderOffset(0);
        file.Read(&fileHeaderOffset, 1);
        file.Seek(fileHeaderOffset, Detail::ConstByteCursor::Begin);

        PeFileHeader fileHeader = { 0 };
        file.Read(&fileHeader, 1);
        if (fileHeader._sectionCount == 0 || fileHeader._sectionCount > 100)
            throw MetadataReadError(L"PE section count is out of range");

        PeSectionHeaderSequence sections(fileHeader._sectionCount);
        file.Read(sections.data(), static_cast<SizeType>(sections.size()));

        auto cliHeaderSectionIt(std::find_if(
            sections.begin(), sections.end(),
            PeSectionContainsRva(fileHeader._cliHeaderTable._rva)));

        if (cliHeaderSectionIt == sections.end())
            throw MetadataReadError(L"Failed to locate PE file section containing CLI header");

        SizeType cliHeaderTableOffset(ComputeOffsetFromRva(
            *cliHeaderSectionIt,
            fileHeader._cliHeaderTable));

        file.Seek(static_cast<Detail::ConstByteCursor::DifferenceType>(cliHeaderTableOffset),
                  Detail::ConstByteCursor::Begin);

        PeCliHeader cliHeader = { 0 };
        file.Read(&cliHeader, 1);

        PeSectionsAndCliHeader result;
        result._sections = std::move(sections);
        result._cliHeader = cliHeader;
        return result;
    }

    PeCliStreamHeaderSequence ReadPeCliStreamHeaders(Detail::ConstByteCursor        file,
                                                     PeSectionsAndCliHeader  const& peHeader)
    {
        auto metadataSectionIt(std::find_if(
            peHeader._sections.begin(),
            peHeader._sections.end(),
            PeSectionContainsRva(peHeader._cliHeader._metadata._rva)));

        if (metadataSectionIt == peHeader._sections.end())
            throw MetadataReadError(L"Failed to locate PE file section containing CLI metadata");

        SizeType metadataOffset(ComputeOffsetFromRva(
            *metadataSectionIt,
            peHeader._cliHeader._metadata));

        file.Seek(metadataOffset, Detail::ConstByteCursor::Begin);

        std::uint32_t magicSignature(0);
        file.Read(&magicSignature, 1);
        if (magicSignature != 0x424a5342)
            throw MetadataReadError(L"Magic signature does not match required value 0x424a5342");

        file.Seek(8, Detail::ConstByteCursor::Current);

        std::uint32_t versionLength(0);
        file.Read(&versionLength, 1);
        file.Seek(versionLength + 2, Detail::ConstByteCursor::Current); // Add 2 to account for unused flags

        std::uint16_t streamCount(0);
        file.Read(&streamCount, 1);

        PeCliStreamHeaderSequence streamHeaders = { 0 };
        for (std::uint16_t i(0); i < streamCount; ++i)
        {
            PeCliStreamHeader header;
            header._metadataOffset = metadataOffset;
            file.Read(&header._streamOffset, 1);
            file.Read(&header._streamSize,   1);

            std::array<char, 12> currentName = { 0 };
            file.Read(currentName.data(), static_cast<SizeType>(currentName.size()));

            #define CXXREFLECT_GENERATE(name, id, reset)                                        \
                if (std::strcmp(currentName.data(), name) == 0 &&                               \
                    streamHeaders[Detail::AsInteger(PeCliStreamKind::id)]._metadataOffset == 0) \
                {                                                                               \
                    streamHeaders[Detail::AsInteger(PeCliStreamKind::id)] = header;             \
                    file.Seek(reset, Detail::ConstByteCursor::Current);                         \
                    used = true;                                                                \
                }

            bool used(false);
            CXXREFLECT_GENERATE("#Strings", StringStream,      0);
            CXXREFLECT_GENERATE("#US",      UserStringStream, -8);
            CXXREFLECT_GENERATE("#Blob",    BlobStream,       -4);
            CXXREFLECT_GENERATE("#GUID",    GuidStream,       -4);
            CXXREFLECT_GENERATE("#~",       TableStream,      -8);
            if (!used)
                throw MetadataReadError(L"Unknown stream name encountered");

            #undef CXXREFLECT_GENERATE
        }

        return streamHeaders;
    }





    CompositeIndexSizeArray const CompositeIndexTagSize =
    {
        2, 2, 5, 1, 2, 3, 1, 1, 1, 2, 3, 2, 1
    };

    #define CXXREFLECT_GENERATE(x, y)                                                \
        (tableSizes[Detail::AsInteger(TableId::y)] <                                 \
        (1ull << (16 - CompositeIndexTagSize[Detail::AsInteger(CompositeIndex::x)])))

    SizeType ComputeTypeDefOrRefIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(TypeDefOrRef, TypeDef)
            && CXXREFLECT_GENERATE(TypeDefOrRef, TypeRef)
            && CXXREFLECT_GENERATE(TypeDefOrRef, TypeSpec) ? 2 : 4;
    }

    SizeType ComputeHasConstantIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasConstant, Field)
            && CXXREFLECT_GENERATE(HasConstant, Param)
            && CXXREFLECT_GENERATE(HasConstant, Property) ? 2 : 4;
    }

    SizeType ComputeHasCustomAttributeIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasCustomAttribute, MethodDef)
            && CXXREFLECT_GENERATE(HasCustomAttribute, Field)
            && CXXREFLECT_GENERATE(HasCustomAttribute, TypeRef)
            && CXXREFLECT_GENERATE(HasCustomAttribute, TypeDef)
            && CXXREFLECT_GENERATE(HasCustomAttribute, Param)
            && CXXREFLECT_GENERATE(HasCustomAttribute, InterfaceImpl)
            && CXXREFLECT_GENERATE(HasCustomAttribute, MemberRef)
            && CXXREFLECT_GENERATE(HasCustomAttribute, Module)
            && CXXREFLECT_GENERATE(HasCustomAttribute, Property)
            && CXXREFLECT_GENERATE(HasCustomAttribute, Event)
            && CXXREFLECT_GENERATE(HasCustomAttribute, StandaloneSig)
            && CXXREFLECT_GENERATE(HasCustomAttribute, ModuleRef)
            && CXXREFLECT_GENERATE(HasCustomAttribute, TypeSpec)
            && CXXREFLECT_GENERATE(HasCustomAttribute, Assembly)
            && CXXREFLECT_GENERATE(HasCustomAttribute, AssemblyRef)
            && CXXREFLECT_GENERATE(HasCustomAttribute, File)
            && CXXREFLECT_GENERATE(HasCustomAttribute, ExportedType)
            && CXXREFLECT_GENERATE(HasCustomAttribute, ManifestResource)
            && CXXREFLECT_GENERATE(HasCustomAttribute, GenericParam)
            && CXXREFLECT_GENERATE(HasCustomAttribute, GenericParamConstraint)
            && CXXREFLECT_GENERATE(HasCustomAttribute, MethodSpec) ? 2 : 4;
    }

    SizeType ComputeHasFieldMarshalIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasFieldMarshal, Field)
            && CXXREFLECT_GENERATE(HasFieldMarshal, Param) ? 2 : 4;
    }

    SizeType ComputeHasDeclSecurityIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasDeclSecurity, TypeDef)
            && CXXREFLECT_GENERATE(HasDeclSecurity, MethodDef)
            && CXXREFLECT_GENERATE(HasDeclSecurity, Assembly) ? 2 : 4;
    }

    SizeType ComputeMemberRefParentIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(MemberRefParent, TypeDef)
            && CXXREFLECT_GENERATE(MemberRefParent, TypeRef)
            && CXXREFLECT_GENERATE(MemberRefParent, ModuleRef)
            && CXXREFLECT_GENERATE(MemberRefParent, MethodDef)
            && CXXREFLECT_GENERATE(MemberRefParent, TypeSpec) ? 2 : 4;
    }

    SizeType ComputeHasSemanticsIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasSemantics, Event)
            && CXXREFLECT_GENERATE(HasSemantics, Property) ? 2 : 4;
    }

    SizeType ComputeMethodDefOrRefIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(MethodDefOrRef, MethodDef)
            && CXXREFLECT_GENERATE(MethodDefOrRef, MemberRef) ? 2 : 4;
    }

    SizeType ComputeMemberForwardedIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(MemberForwarded, Field)
            && CXXREFLECT_GENERATE(MemberForwarded, MethodDef) ? 2 : 4;
    }

    SizeType ComputeImplementationIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(Implementation, File)
            && CXXREFLECT_GENERATE(Implementation, AssemblyRef)
            && CXXREFLECT_GENERATE(Implementation, ExportedType) ? 2 : 4;
    }

    SizeType ComputeCustomAttributeTypeIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(CustomAttributeType, MethodDef)
            && CXXREFLECT_GENERATE(CustomAttributeType, MemberRef) ? 2 : 4;
    }

    SizeType ComputeResolutionScopeIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(ResolutionScope, Module)
            && CXXREFLECT_GENERATE(ResolutionScope, ModuleRef)
            && CXXREFLECT_GENERATE(ResolutionScope, AssemblyRef)
            && CXXREFLECT_GENERATE(ResolutionScope, TypeRef) ? 2 : 4;
    }

    SizeType ComputeTypeOrMethodDefIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(TypeOrMethodDef, TypeDef)
            && CXXREFLECT_GENERATE(TypeOrMethodDef, MethodDef) ? 2 : 4;
    }

    #undef CXXREFLECT_GENERATE





    template <typename T>
    T const& ReadAs(ConstByteIterator const data, SizeType const index)
    {
        return *reinterpret_cast<T const*>(data + index);
    }





    std::uint32_t ReadTableIndex(Database          const& database,
                                 ConstByteIterator const  data,
                                 TableId           const  table,
                                 SizeType          const  offset)
    {
        // Table indexes are one-based, to allow zero to be used as a null reference value.  In
        // order to simplify row offset calculations, we subtract one from all table indices to
        // make them zero-based, with -1 (0xffffffff) representing a null reference.
        switch (database.GetTables().GetTableIndexSize(table))
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset) - 1;
        case 4:  return ReadAs<std::uint32_t>(data, offset) - 1;
        default: Detail::AssertFail(L"Invalid table index size");
        }

        return 0;
    }

    SizeType ReadCompositeIndex(Database            const& database,
                                ConstByteIterator   const  data,
                                CompositeIndex      const  index,
                                SizeType            const  offset)
    {
        switch (database.GetTables().GetCompositeIndexSize(index))
        {
            case 2:  return ReadAs<std::uint16_t>(data, offset);
            case 4:  return ReadAs<std::uint32_t>(data, offset);
            default: Detail::AssertFail(L"Invalid composite index size");
        }
        
        return 0;
    }

    SizeType ReadBlobHeapIndex(Database const& database, ConstByteIterator const data, SizeType const offset)
    {
        switch (database.GetTables().GetBlobHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset);
        case 4:  return ReadAs<std::uint32_t>(data, offset);
        default: Detail::AssertFail(L"Invalid blob heap index size");
        }

        return 0;
    }

    BlobReference ReadBlobReference(Database const& database, ConstByteIterator const data, SizeType const offset)
    {
        return BlobReference::ComputeFromStream(
            ReadBlobHeapIndex(database, data, offset) + database.GetBlobs().Begin(),
            database.GetBlobs().End());
    }

    SizeType ReadGuidHeapIndex(Database const& database, ConstByteIterator const data, SizeType const offset)
    {
        switch (database.GetTables().GetGuidHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset);
        case 4:  return ReadAs<std::uint32_t>(data, offset);
        default: Detail::AssertFail(L"Invalid blob heap index size");
        }

        return 0;
    }

    BlobReference ReadGuidReference(Database const& database, ConstByteIterator const data, SizeType const offset)
    {
        // The GUID heap index starts at 1 and counts by GUID, unlike the blob heap whose index
        // counts by byte.
        SizeType const index(ReadGuidHeapIndex(database, data, offset) - 1);

        ConstByteIterator const first(index * 16 + database.GetGuids().Begin());
        return BlobReference(first, first + 16);
    }

    SizeType ReadStringHeapIndex(Database const& database, ConstByteIterator const data, SizeType const offset)
    {
        switch (database.GetTables().GetStringHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset);
        case 4:  return ReadAs<std::uint32_t>(data, offset);
        default: Detail::AssertFail(L"Invalid string heap index size");
        }

        return 0;
    }

    StringReference ReadStringReference(Database const& database, ConstByteIterator const data, SizeType const offset)
    {
        return database.GetString(ReadStringHeapIndex(database, data, offset));
    }

    RowReference ReadRowReference(Database          const& database,
                                  ConstByteIterator const  data,
                                  TableId           const  table,
                                  SizeType          const  offset)
    {
        return RowReference(table, ReadTableIndex(database, data, table, offset));
    }





    typedef std::pair<std::uint32_t, std::uint32_t> TagIndexPair;

    TagIndexPair SplitCompositeIndex(CompositeIndex const index, std::uint32_t const value)
    {
        std::uint32_t const tagBits(CompositeIndexTagSize[Detail::AsInteger(index)]);
        return std::make_pair(
            value & ((static_cast<std::uint32_t>(1u) << tagBits) - 1),
            (value >> tagBits) - 1
            );
    }

    RowReference ConvertIndexAndComposeRow(CompositeIndex const index, TagIndexPair const split)
    {
        TableId const tableId(GetTableIdFromCompositeIndexKey(split.first, index));
        if (tableId == static_cast<TableId>(-1))
            throw MetadataReadError(L"Failed to translate CompositeIndex to TableId");

        return RowReference(tableId, split.second);
    }

    RowReference ReadRowReference(Database          const& database,
                                  ConstByteIterator const  data,
                                  CompositeIndex    const  index,
                                  SizeType          const  offset)
    {
        std::uint32_t const value(ReadCompositeIndex(database, data, index, offset));
        if (value == 0)
            return RowReference();

        return ConvertIndexAndComposeRow(index, SplitCompositeIndex(index, value));
    }





    template <TableId TSourceId, TableId TTargetId, typename TFirstFunction>
    RowReference ComputeLastRowReference(Database          const& database,
                                         ConstByteIterator const  data,
                                         TFirstFunction    const  first)
    {
        SizeType const byteOffset(static_cast<SizeType>(data - database.GetTables().GetTable(TSourceId).Begin()));
        SizeType const rowSize(database.GetTables().GetTable(TSourceId).GetRowSize());
        SizeType const logicalIndex(byteOffset / rowSize);

        SizeType const sourceTableRowCount(database.GetTables().GetTable(TSourceId).GetRowCount());
        SizeType const targetTableRowCount(database.GetTables().GetTable(TTargetId).GetRowCount());
        if (logicalIndex + 1 == sourceTableRowCount)
        {
            return RowReference(TTargetId, targetTableRowCount);
        }
        else
        {
            return (database.GetRow<TSourceId>(logicalIndex + 1).*first)();
        }
    }





    /// A strict weak ordering for owning rows.
    ///
    /// This comparison function object provides a strict-weak ordering over a range of rows to find
    /// the row in an owning table that owns the row in an owned-record table.  For example, given a
    /// MethodDef row, it can be used to search over a range of TypeDef rows for the TypeDef that
    /// owns the MethodDef.
    ///
    /// Note that the lists in an owned-row table (like MethodDef or Field) are guaranteed to be
    /// sorted by the owning row, so the methods for TypeDef 1 will be followed immediately by those
    /// for TypeDef 2, and so on. This is not explicitly specified in ECMA 335-2010, but it is necessarily
    /// the case due to the way that the lists are specified.
    template
    <
        typename TOwningRow,
        RowReference (TOwningRow::*FFirst)() const,
        RowReference (TOwningRow::*FLast)() const
    >
    class ElementListStrictWeakOrdering
    {
    public:

        template <typename TOwnedRow>
        bool operator()(TOwningRow const& owningRow, TOwnedRow const& ownedRow) const
        {
            Detail::Assert([&]{ return owningRow.IsInitialized(); });
            Detail::Assert([&]{ return ownedRow.IsInitialized();  });

            RowReference const rangeLast((owningRow.*FLast)());
            
            Detail::Assert([&]{ return rangeLast.GetTable() == ownedRow.GetSelfReference().GetTable(); });
            return rangeLast <= ownedRow.GetSelfReference();
        }

        template <typename TOwnedRow>
        bool operator()(TOwnedRow const& ownedRow, TOwningRow const& owningRow) const
        {
            Detail::Assert([&]{ return ownedRow.IsInitialized();  });
            Detail::Assert([&]{ return owningRow.IsInitialized(); });
            
            RowReference const rangeFirst((owningRow.*FFirst)());
            
            Detail::Assert([&]{ return rangeFirst.GetTable() == ownedRow.GetSelfReference().GetTable(); });
            return ownedRow.GetSelfReference() < rangeFirst;
        }
    };

    



    template
    <
        typename TOwningRow,
        typename TOwnedRow,
        RowReference (TOwningRow::*FFirst)() const,
        RowReference (TOwningRow::*FLast)() const
    >
    TOwningRow GetOwningRow(TOwnedRow const& ownedRow)
    {
        Detail::Assert([&]{ return ownedRow.IsInitialized(); });

        typedef Private::ElementListStrictWeakOrdering<TOwningRow, FFirst, FLast> ComparerType;
        TableId const OwningTableId(static_cast<TableId>(RowTypeToTableId<TOwningRow>::Value));

        auto const it(Detail::BinarySearch(
            ownedRow.GetDatabase().Begin<OwningTableId>(),
            ownedRow.GetDatabase().End<OwningTableId>(),
            ownedRow,
            ComparerType()));

        if (it == ownedRow.GetDatabase().End<OwningTableId>())
            throw MetadataReadError(L"Failed to locate owning row");

        return ownedRow.GetDatabase().GetRow<OwningTableId>(it.GetReference());
    }

} } } }

namespace CxxReflect { namespace Metadata {

    bool IsValidTableId(SizeType const value)
    {
        static std::array<Byte, 0x40> const mask =
        {
            1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        return value < mask.size() && mask[value] == 1;
    }





    TableId GetTableIdFromCompositeIndexKey(SizeType const key, CompositeIndex const index)
    {
        switch (index)
        {
        case CompositeIndex::CustomAttributeType:
            switch (key)
            {
            case 2:  return TableId::MethodDef;
            case 3:  return TableId::MemberRef;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::HasConstant:
            switch (key)
            {
            case 0:  return TableId::Field;
            case 1:  return TableId::Param;
            case 2:  return TableId::Property;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::HasCustomAttribute:
            switch (key)
            {
            case  0:  return TableId::MethodDef;
            case  1:  return TableId::Field;
            case  2:  return TableId::TypeRef;
            case  3:  return TableId::TypeDef;
            case  4:  return TableId::Param;
            case  5:  return TableId::InterfaceImpl;
            case  6:  return TableId::MemberRef;
            case  7:  return TableId::Module;
            case  8:  return TableId::DeclSecurity;
            case  9:  return TableId::Property;
            case 10:  return TableId::Event;
            case 11:  return TableId::StandaloneSig;
            case 12:  return TableId::ModuleRef;
            case 13:  return TableId::TypeSpec;
            case 14:  return TableId::Assembly;
            case 15:  return TableId::AssemblyRef;
            case 16:  return TableId::File;
            case 17:  return TableId::ExportedType;
            case 18:  return TableId::ManifestResource;
            case 19:  return TableId::GenericParam;
            case 20:  return TableId::GenericParamConstraint;
            case 21:  return TableId::MethodSpec;
            default:  return static_cast<TableId>(-1);
            }

        case CompositeIndex::HasDeclSecurity:
            switch (key)
            {
            case 0:  return TableId::TypeDef;
            case 1:  return TableId::MethodDef;
            case 2:  return TableId::Assembly;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::HasFieldMarshal:
            switch (key)
            {
            case 0:  return TableId::Field;
            case 1:  return TableId::Param;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::HasSemantics:
            switch (key)
            {
            case 0:  return TableId::Event;
            case 1:  return TableId::Property;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::Implementation:
            switch (key)
            {
            case 0:  return TableId::File;
            case 1:  return TableId::AssemblyRef;
            case 2:  return TableId::ExportedType;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::MemberForwarded:
            switch (key)
            {
            case 0:  return TableId::Field;
            case 1:  return TableId::MethodDef;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::MemberRefParent:
            switch (key)
            {
            case 0:  return TableId::TypeDef;
            case 1:  return TableId::TypeRef;
            case 2:  return TableId::ModuleRef;
            case 3:  return TableId::MethodDef;
            case 4:  return TableId::TypeSpec;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::MethodDefOrRef:
            switch (key)
            {
            case 0:  return TableId::MethodDef;
            case 1:  return TableId::MemberRef;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::ResolutionScope:
            switch (key)
            {
            case 0:  return TableId::Module;
            case 1:  return TableId::ModuleRef;
            case 2:  return TableId::AssemblyRef;
            case 3:  return TableId::TypeRef;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::TypeDefOrRef:
            switch (key)
            {
            case 0:  return TableId::TypeDef;
            case 1:  return TableId::TypeRef;
            case 2:  return TableId::TypeSpec;
            default: return static_cast<TableId>(-1);
            }

        case CompositeIndex::TypeOrMethodDef:
            switch (key)
            {
            case 0:  return TableId::TypeDef;
            case 1:  return TableId::MethodDef;
            default: return static_cast<TableId>(-1);
            }

        default:
            return static_cast<TableId>(-1);
        }
    }

    SizeType GetCompositeIndexKeyFromTableId(TableId const tableId, CompositeIndex const index)
    {
        switch (index)
        {
        case CompositeIndex::CustomAttributeType:
            switch (tableId)
            {
            case TableId::MethodDef: return 2;
            case TableId::MemberRef: return 3;
            default:                 return static_cast<SizeType>(-1);
            }

        case CompositeIndex::HasConstant:
            switch (tableId)
            {
            case TableId::Field:    return 0;
            case TableId::Param:    return 1;
            case TableId::Property: return 2;
            default:                return static_cast<SizeType>(-1);
            }

        case CompositeIndex::HasCustomAttribute:
            switch (tableId)
            {
            case TableId::MethodDef:              return  0;
            case TableId::Field:                  return  1;
            case TableId::TypeRef:                return  2;
            case TableId::TypeDef:                return  3;
            case TableId::Param:                  return  4;
            case TableId::InterfaceImpl:          return  5;
            case TableId::MemberRef:              return  6;
            case TableId::Module:                 return  7;
            case TableId::DeclSecurity:           return  8;
            case TableId::Property:               return  9;
            case TableId::Event:                  return 10;
            case TableId::StandaloneSig:          return 11;
            case TableId::ModuleRef:              return 12;
            case TableId::TypeSpec:               return 13;
            case TableId::Assembly:               return 14;
            case TableId::AssemblyRef:            return 15;
            case TableId::File:                   return 16;
            case TableId::ExportedType:           return 17;
            case TableId::ManifestResource:       return 18;
            case TableId::GenericParam:           return 19;
            case TableId::GenericParamConstraint: return 20;
            case TableId::MethodSpec:             return 21;
            default:                              return static_cast<SizeType>(-1);
            }

        case CompositeIndex::HasDeclSecurity:
            switch (tableId)
            {
            case TableId::TypeDef:   return 0;
            case TableId::MethodDef: return 1;
            case TableId::Assembly:  return 2;
            default:                 return static_cast<SizeType>(-1);
            }

        case CompositeIndex::HasFieldMarshal:
            switch (tableId)
            {
            case TableId::Field: return 0;
            case TableId::Param: return 1;
            default:             return static_cast<SizeType>(-1);
            }

        case CompositeIndex::HasSemantics:
            switch (tableId)
            {
            case TableId::Event:    return 0;
            case TableId::Property: return 1;
            default:                return static_cast<SizeType>(-1);
            }

        case CompositeIndex::Implementation:
            switch (tableId)
            {
            case TableId::File:         return 0;
            case TableId::AssemblyRef:  return 1;
            case TableId::ExportedType: return 2;
            default:                    return static_cast<SizeType>(-1);
            }

        case CompositeIndex::MemberForwarded:
            switch (tableId)
            {
            case TableId::Field:     return 0;
            case TableId::MethodDef: return 1;
            default:                 return static_cast<SizeType>(-1);
            }

        case CompositeIndex::MemberRefParent:
            switch (tableId)
            {
            case TableId::TypeDef:   return 0;
            case TableId::TypeRef:   return 1;
            case TableId::ModuleRef: return 2;
            case TableId::MethodDef: return 3;
            case TableId::TypeSpec:  return 4;
            default:                 return static_cast<SizeType>(-1);
            }

        case CompositeIndex::MethodDefOrRef:
            switch (tableId)
            {
            case TableId::MethodDef: return 0;
            case TableId::MemberRef: return 1;
            default:                 return static_cast<SizeType>(-1);
            }

        case CompositeIndex::ResolutionScope:
            switch (tableId)
            {
            case TableId::Module:      return 0;
            case TableId::ModuleRef:   return 1;
            case TableId::AssemblyRef: return 2;
            case TableId::TypeRef:     return 3;
            default:                   return static_cast<SizeType>(-1);
            }

        case CompositeIndex::TypeDefOrRef:
            switch (tableId)
            {
            case TableId::TypeDef:  return 0;
            case TableId::TypeRef:  return 1;
            case TableId::TypeSpec: return 2;
            default:                return static_cast<SizeType>(-1);
            }

        case CompositeIndex::TypeOrMethodDef:
            switch (tableId)
            {
            case TableId::TypeDef:   return 0;
            case TableId::MethodDef: return 1;
            default:                 return static_cast<SizeType>(-1);
            }

        default:
            return static_cast<SizeType>(-1);
        }
    }





    RowReference::RowReference()
        : _value(InvalidValue)
    {
    }

    RowReference::RowReference(TableId const tableId, SizeType const index)
        : _value(ComposeValue(tableId, index))
    {
    }

    TableId RowReference::GetTable() const
    {
        return static_cast<TableId>((_value.Get() & ValueTableIdMask) >> ValueIndexBits);
    }

    SizeType RowReference::GetIndex() const
    {
        return _value.Get() & ValueIndexMask;
    }

    RowReference::ValueType RowReference::GetValue() const
    {
        return _value.Get();
    }

    RowReference::TokenType RowReference::GetToken() const
    {
        return _value.Get() + 1;
    }

    bool RowReference::IsInitialized() const
    {
        return _value.Get() != InvalidValue;
    }

    bool operator==(RowReference const& lhs, RowReference const& rhs)
    {
        return lhs._value.Get() == rhs._value.Get();
    }

    bool operator<(RowReference const& lhs, RowReference const& rhs)
    {
        return lhs._value.Get() < rhs._value.Get();
    }

    RowReference::ValueType RowReference::ComposeValue(TableId const tableId, SizeType const index)
    {
        Detail::Assert([&]{ return IsValidTableId(Detail::AsInteger(tableId));           });
        Detail::Assert([&]{ return Detail::AsInteger(tableId) < (1 << ValueTableIdBits); });
        Detail::Assert([&]{ return index < ValueIndexMask;                               });

        ValueType const tableIdValue(Detail::AsInteger(tableId) & (ValueTableIdMask >> ValueIndexBits));

        ValueType const tableIdComponent(tableIdValue << ValueIndexBits);
        ValueType const indexComponent(index & ValueIndexMask);

        return tableIdComponent | indexComponent;
    }

    RowReference RowReference::FromToken(TokenType const token)
    {
        RowReference result;

        // If the token is a null reference, we will return the default-constructed object.
        if ((token & ValueIndexMask) != 0)
            result._value.Get() = token - 1;

        return result;
    }





    BlobReference::BlobReference()
    {
    }

    BlobReference::BlobReference(ConstByteIterator const first, ConstByteIterator const last)
        : _first(first), _last(last)
    {
        AssertInitialized();
    }

    BlobReference::BlobReference(BaseSignature const& signature)
        : _first(signature.BeginBytes()), _last(signature.EndBytes())
    {
        AssertInitialized();
    }

    ConstByteIterator BlobReference::Begin() const
    {
        return _first.Get();
    }

    ConstByteIterator BlobReference::End() const
    {
        return _last.Get();
    }

    bool BlobReference::IsInitialized() const
    {
        return _first.Get() != nullptr && _last.Get() != nullptr;
    }

    void BlobReference::AssertInitialized() const
    {
        Detail::Assert([&] { return IsInitialized(); });
    }

    bool operator==(BlobReference const& lhs, BlobReference const& rhs)
    {
        return std::equal_to<ConstByteIterator>()(lhs.Begin(), rhs.Begin());
    }

    bool operator<(BlobReference const& lhs, BlobReference const& rhs)
    {
        return std::less<ConstByteIterator>()(lhs.Begin(), rhs.Begin());
    }

    BlobReference BlobReference::ComputeFromStream(ConstByteIterator const first, ConstByteIterator const last)
    {
        if (first == last)
            throw MetadataReadError(L"Invalid blob");

        Byte initialByte(*first);
        SizeType blobSizeBytes(0);
        switch (initialByte >> 5)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            blobSizeBytes = 1;
            initialByte &= 0x7f;
            break;

        case 4:
        case 5:
            blobSizeBytes = 2;
            initialByte &= 0x3f;
            break;

        case 6:
            blobSizeBytes = 4;
            initialByte &= 0x1f;
            break;

        case 7:
        default:
            throw MetadataReadError(L"Invalid blob");
        }

        if (Detail::Distance(first, last) < blobSizeBytes)
            throw MetadataReadError(L"Invalid blob");

        SizeType blobSize(initialByte);
        for (unsigned i(1); i < blobSizeBytes; ++ i)
            blobSize = (blobSize << 8) + *(first + i);

        if (Detail::Distance(first, last) < blobSizeBytes + blobSize)
            throw MetadataReadError(L"Invalid blob");

        return BlobReference(first + blobSizeBytes, first + blobSizeBytes + blobSize);
    }





    BaseElementReference::BaseElementReference()
        : _first(nullptr),
          _size(InvalidElementSentinel)
    {
    }

    BaseElementReference::BaseElementReference(RowReference const& reference)
        : _index(reference.GetToken()),
          _size(TableKindBit)
    {
        AssertInitialized();
    }

    BaseElementReference::BaseElementReference(BlobReference const& reference)
        : _first(reference.Begin()),
          _size((reference.End() ? 0 : (reference.End() - reference.Begin()) & ~KindMask) | BlobKindBit)
    {
        AssertInitialized();
    }

    bool BaseElementReference::IsRowReference() const
    {
        return IsInitialized() && (_size.Get() & KindMask) == TableKindBit;
    }

    bool BaseElementReference::IsBlobReference() const
    {
        return IsInitialized() && (_size.Get() & KindMask) == BlobKindBit;
    }

    bool BaseElementReference::IsInitialized() const
    {
        return _size.Get() != InvalidElementSentinel;
    }

    void BaseElementReference::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    RowReference BaseElementReference::AsRowReference() const
    {
        Detail::Assert([&]{ return IsRowReference(); });
        return RowReference::FromToken(_index);
    }

    BlobReference BaseElementReference::AsBlobReference() const
    {
        Detail::Assert([&]{ return IsBlobReference(); });
        return BlobReference(_first, _size.Get() ? (_first + _size.Get()) : nullptr);
    }

    bool operator==(BaseElementReference const& lhs, BaseElementReference const& rhs)
    {
        if (lhs.IsBlobReference() != rhs.IsBlobReference())
            return false;

        return lhs.IsBlobReference()
            ? (lhs._first == rhs._first)
            : (lhs._index == rhs._index);
    }

    bool operator<(BaseElementReference const& lhs, BaseElementReference const& rhs)
    {
        // Arbitrarily order all blob references before row references for sorting purposes:
        if (lhs.IsBlobReference() != rhs.IsBlobReference())
            return lhs.IsBlobReference();

        return lhs.IsBlobReference()
            ? (lhs._first < rhs._first)
            : (lhs._index < rhs._index);
    }





    ElementReference::ElementReference()
    {
    }

    ElementReference::ElementReference(RowReference const& reference)
        : BaseElementReference(reference)
    {
    }

    ElementReference::ElementReference(BlobReference const& reference)
        : BaseElementReference(reference)
    {
    }





    FullReference::FullReference()
    {
    }

    FullReference::FullReference(Database const* const database, RowReference const& reference)
        : BaseElementReference(reference),
          _database(database)
    {
        Detail::AssertNotNull(database);
        AssertInitialized();
    }

    FullReference::FullReference(Database const* const database, BlobReference const& reference)
        : BaseElementReference(reference),
          _database(database)
    {
        Detail::AssertNotNull(database);
        AssertInitialized();
    }

    FullReference::FullReference(Database const* const database, ElementReference const& reference)
        : BaseElementReference(reference.IsRowReference()
              ? BaseElementReference(reference.AsRowReference())
              : BaseElementReference(reference.AsBlobReference())),
          _database(database)
    {
        Detail::AssertNotNull(database);
        AssertInitialized();
    }

    Database const& FullReference::GetDatabase() const
    {
        Detail::AssertNotNull(_database.Get());
        return *_database.Get();
    }

    bool operator==(FullReference const& lhs, FullReference const& rhs)
    {
        if (lhs.GetDatabase() != rhs.GetDatabase())
            return false;

        BaseElementReference const& lhsBase(lhs);
        BaseElementReference const& rhsBase(rhs);

        return lhsBase == rhsBase;
    }

    bool operator<(FullReference const& lhs, FullReference const& rhs)
    {
        if (lhs.GetDatabase() < rhs.GetDatabase())
            return true;

        if (lhs.GetDatabase() > rhs.GetDatabase())
            return false;

        BaseElementReference const& lhsBase(lhs);
        BaseElementReference const& rhsBase(rhs);

        return lhsBase < rhsBase;
    }





    FourComponentVersion::FourComponentVersion()
    {
    }

    FourComponentVersion::FourComponentVersion(Component const major,
                                               Component const minor,
                                               Component const build,
                                               Component const revision)
        : _major(major), _minor(minor), _build(build), _revision(revision)
    {
    }

    FourComponentVersion::Component FourComponentVersion::GetMajor() const
    {
        return _major.Get();
    }

    FourComponentVersion::Component FourComponentVersion::GetMinor() const
    {
        return _minor.Get();
    }

    FourComponentVersion::Component FourComponentVersion::GetBuild() const
    {
        return _build.Get();
    }

    FourComponentVersion::Component FourComponentVersion::GetRevision() const
    {
        return _revision.Get();
    }





    Stream::Stream()
    {
    }

    Stream::Stream(Detail::ConstByteCursor file,
                   SizeType const metadataOffset,
                   SizeType const streamOffset,
                   SizeType const streamSize)
    {
        SizeType const metadataStart(metadataOffset + streamOffset);

        if (!file.CanSeek(metadataStart, Detail::ConstByteCursor::Begin))
            throw MetadataReadError(L"Unable to read metadata stream:  start index out of range");

        file.Seek(metadataOffset + streamOffset, Detail::ConstByteCursor::Begin);

        if (!file.CanRead(streamSize))
            throw MetadataReadError(L"Unable to read metadata stream:  end index out of range");

        ConstByteIterator const it(file.GetCurrent());
        _data = ConstByteRange(it, it + streamSize);
    }

    ConstByteIterator Stream::Begin() const
    {
        return _data.Begin();
    }

    ConstByteIterator Stream::End() const
    {
        return _data.End();
    }

    SizeType Stream::Size() const
    {
        return Detail::Distance(_data.Begin(), _data.End());
    }

    bool Stream::IsInitialized() const
    {
        return _data.IsInitialized();
    }

    void Stream::AssertInitialized() const
    {
        Detail::Assert([&] { return IsInitialized(); });
    }

    ConstByteIterator Stream::At(SizeType const index) const
    {
        return RangeCheckedAt(index, 0);
    }

    ConstByteIterator Stream::RangeCheckedAt(SizeType const index, SizeType const size) const
    {
        AssertInitialized();

        if (index + size > Size())
            throw MetadataReadError(L"Attempted to read from beyond the end of the stream");

        return _data.Begin() + index;
    }





    Table::Table()
    {
    }

    Table::Table(ConstByteIterator const data, SizeType const rowSize, SizeType const rowCount, bool const isSorted)
        : _data(data), _rowSize(rowSize), _rowCount(rowCount), _isSorted(isSorted)
    {
        Detail::AssertNotNull(data);
        Detail::Assert([&]{ return rowSize != 0 && rowCount != 0; });
    }

    ConstByteIterator Table::Begin() const
    {
        // Note:  it's okay if _data is nullptr; if it is, then Begin() == End(), so the table is
        // considered to be empty.  Thus, we don't AssertInitialized() here.
        return _data.Get();
    }

    ConstByteIterator Table::End() const
    {
        // Note:  it's okay if _data is nullptr; if it is, then Begin() == End(), so the table is
        // considered to be empty.  Thus, we don't AssertInitialized() here.
        Detail::Assert([&]{ return _data.Get() != nullptr || _rowCount.Get() * _rowSize.Get() == 0; });
        return _data.Get() + _rowCount.Get() * _rowSize.Get();
    }

    bool Table::IsSorted() const
    {
        return _isSorted.Get();
    }

    SizeType Table::GetRowCount() const
    {
        return _rowCount.Get();
    }

    SizeType Table::GetRowSize()  const
    {
        return _rowSize.Get();
    }

    ConstByteIterator Table::At(SizeType const index) const
    {
        AssertInitialized();

        if (index >= GetRowCount())
            throw MetadataReadError(L"Attempted to read row at index greater than number of rows");

        return _data.Get() + _rowSize.Get() * index;
    }

    bool Table::IsInitialized() const
    {
        return _data.Get() != nullptr;
    }

    void Table::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }





    TableCollection::TableCollection()
    {
    }

    TableCollection::TableCollection(Stream const& stream)
        : _stream(stream)
    {
        std::bitset<8> heapSizes(_stream.ReadAs<std::uint8_t>(6));
        _state.Get()._stringHeapIndexSize = heapSizes.test(0) ? 4 : 2;
        _state.Get()._guidHeapIndexSize   = heapSizes.test(1) ? 4 : 2;
        _state.Get()._blobHeapIndexSize   = heapSizes.test(2) ? 4 : 2;

        _state.Get()._validBits  = _stream.ReadAs<std::uint64_t>(8);
        _state.Get()._sortedBits = _stream.ReadAs<std::uint64_t>(16);

        SizeType index(24);
        for (unsigned x(0); x < TableIdCount; ++x)
        {
            if (!_state.Get()._validBits.test(x))
                continue;

            if (!IsValidTableId(x))
                throw MetadataReadError(L"Metadata table presence vector has invalid bits set");

            _state.Get()._rowCounts[x] = _stream.ReadAs<std::uint32_t>(index);
            index += 4;
        }

        ComputeCompositeIndexSizes();
        ComputeTableRowSizes();

        for (unsigned x(0); x < TableIdCount; ++x)
        {
            if (!_state.Get()._validBits.test(x) || _state.Get()._rowCounts[x] == 0)
                continue;

            _state.Get()._tables[x] = Table(_stream.At(index),
                                            _state.Get()._rowSizes[x],
                                            _state.Get()._rowCounts[x],
                                            _state.Get()._sortedBits.test(x));
            index += _state.Get()._rowSizes[x] * _state.Get()._rowCounts[x];
        }
    }

    void TableCollection::ComputeCompositeIndexSizes()
    {
        #define CXXREFLECT_GENERATE(x)                                                \
            _state.Get()._compositeIndexSizes[Detail::AsInteger(CompositeIndex::x)] = \
                Private::Compute ## x ## IndexSize(_state.Get()._rowCounts)
        
        CXXREFLECT_GENERATE(TypeDefOrRef       );
        CXXREFLECT_GENERATE(HasConstant        );
        CXXREFLECT_GENERATE(HasCustomAttribute );
        CXXREFLECT_GENERATE(HasFieldMarshal    );
        CXXREFLECT_GENERATE(HasDeclSecurity    );
        CXXREFLECT_GENERATE(MemberRefParent    );
        CXXREFLECT_GENERATE(HasSemantics       );
        CXXREFLECT_GENERATE(MethodDefOrRef     );
        CXXREFLECT_GENERATE(MemberForwarded    );
        CXXREFLECT_GENERATE(Implementation     );
        CXXREFLECT_GENERATE(CustomAttributeType);
        CXXREFLECT_GENERATE(ResolutionScope    );
        CXXREFLECT_GENERATE(TypeOrMethodDef    );

        #undef CXXREFLECT_GENERATE
    }

    void TableCollection::ComputeTableRowSizes()
    {
        #define CXXREFLECT_GENERATE(x, n, o)                                      \
            _state.Get()._columnOffsets[Detail::AsInteger(TableId::x)][n] =       \
            _state.Get()._columnOffsets[Detail::AsInteger(TableId::x)][n - 1] + o

        CXXREFLECT_GENERATE(Assembly, 1, 4);
        CXXREFLECT_GENERATE(Assembly, 2, 8);
        CXXREFLECT_GENERATE(Assembly, 3, 4);
        CXXREFLECT_GENERATE(Assembly, 4, GetBlobHeapIndexSize());
        CXXREFLECT_GENERATE(Assembly, 5, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(Assembly, 6, GetStringHeapIndexSize());

        CXXREFLECT_GENERATE(AssemblyOs, 1, 4);
        CXXREFLECT_GENERATE(AssemblyOs, 2, 4);
        CXXREFLECT_GENERATE(AssemblyOs, 3, 4);

        CXXREFLECT_GENERATE(AssemblyProcessor, 1, 4);
        
        CXXREFLECT_GENERATE(AssemblyRef, 1, 8);
        CXXREFLECT_GENERATE(AssemblyRef, 2, 4);
        CXXREFLECT_GENERATE(AssemblyRef, 3, GetBlobHeapIndexSize());
        CXXREFLECT_GENERATE(AssemblyRef, 4, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(AssemblyRef, 5, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(AssemblyRef, 6, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(AssemblyRefOs, 1, 4);
        CXXREFLECT_GENERATE(AssemblyRefOs, 2, 4);
        CXXREFLECT_GENERATE(AssemblyRefOs, 3, 4);
        CXXREFLECT_GENERATE(AssemblyRefOs, 4, GetTableIndexSize(TableId::AssemblyRef));

        CXXREFLECT_GENERATE(AssemblyRefProcessor, 1, 4);
        CXXREFLECT_GENERATE(AssemblyRefProcessor, 2, GetTableIndexSize(TableId::AssemblyRef));

        CXXREFLECT_GENERATE(ClassLayout, 1, 2);
        CXXREFLECT_GENERATE(ClassLayout, 2, 4);
        CXXREFLECT_GENERATE(ClassLayout, 3, GetTableIndexSize(TableId::TypeDef));

        CXXREFLECT_GENERATE(Constant, 1, 2);
        CXXREFLECT_GENERATE(Constant, 2, GetCompositeIndexSize(CompositeIndex::HasConstant));
        CXXREFLECT_GENERATE(Constant, 3, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(CustomAttribute, 1, GetCompositeIndexSize(CompositeIndex::HasCustomAttribute));
        CXXREFLECT_GENERATE(CustomAttribute, 2, GetCompositeIndexSize(CompositeIndex::CustomAttributeType));
        CXXREFLECT_GENERATE(CustomAttribute, 3, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(DeclSecurity, 1, 2);
        CXXREFLECT_GENERATE(DeclSecurity, 2, GetCompositeIndexSize(CompositeIndex::HasDeclSecurity));
        CXXREFLECT_GENERATE(DeclSecurity, 3, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(EventMap, 1, GetTableIndexSize(TableId::TypeDef));
        CXXREFLECT_GENERATE(EventMap, 2, GetTableIndexSize(TableId::Event));

        CXXREFLECT_GENERATE(Event, 1, 2);
        CXXREFLECT_GENERATE(Event, 2, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(Event, 3, GetCompositeIndexSize(CompositeIndex::TypeDefOrRef));

        CXXREFLECT_GENERATE(ExportedType, 1, 4);
        CXXREFLECT_GENERATE(ExportedType, 2, 4);
        CXXREFLECT_GENERATE(ExportedType, 3, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(ExportedType, 4, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(ExportedType, 5, GetCompositeIndexSize(CompositeIndex::Implementation));

        CXXREFLECT_GENERATE(Field, 1, 2);
        CXXREFLECT_GENERATE(Field, 2, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(Field, 3, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(FieldLayout, 1, 4);
        CXXREFLECT_GENERATE(FieldLayout, 2, GetTableIndexSize(TableId::Field));

        CXXREFLECT_GENERATE(FieldMarshal, 1, GetCompositeIndexSize(CompositeIndex::HasFieldMarshal));
        CXXREFLECT_GENERATE(FieldMarshal, 2, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(FieldRva, 1, 4);
        CXXREFLECT_GENERATE(FieldRva, 2, GetTableIndexSize(TableId::Field));

        CXXREFLECT_GENERATE(File, 1, 4);
        CXXREFLECT_GENERATE(File, 2, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(File, 3, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(GenericParam, 1, 2);
        CXXREFLECT_GENERATE(GenericParam, 2, 2);
        CXXREFLECT_GENERATE(GenericParam, 3, GetCompositeIndexSize(CompositeIndex::TypeOrMethodDef));
        CXXREFLECT_GENERATE(GenericParam, 4, GetStringHeapIndexSize());

        CXXREFLECT_GENERATE(GenericParamConstraint, 1, GetTableIndexSize(TableId::GenericParam));
        CXXREFLECT_GENERATE(GenericParamConstraint, 2, GetCompositeIndexSize(CompositeIndex::TypeDefOrRef));

        CXXREFLECT_GENERATE(ImplMap, 1, 2);
        CXXREFLECT_GENERATE(ImplMap, 2, GetCompositeIndexSize(CompositeIndex::MemberForwarded));
        CXXREFLECT_GENERATE(ImplMap, 3, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(ImplMap, 4, GetTableIndexSize(TableId::ModuleRef));

        CXXREFLECT_GENERATE(InterfaceImpl, 1, GetTableIndexSize(TableId::TypeDef));
        CXXREFLECT_GENERATE(InterfaceImpl, 2, GetCompositeIndexSize(CompositeIndex::TypeDefOrRef));

        CXXREFLECT_GENERATE(ManifestResource, 1, 4);
        CXXREFLECT_GENERATE(ManifestResource, 2, 4);
        CXXREFLECT_GENERATE(ManifestResource, 3, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(ManifestResource, 4, GetCompositeIndexSize(CompositeIndex::Implementation));

        CXXREFLECT_GENERATE(MemberRef, 1, GetCompositeIndexSize(CompositeIndex::MemberRefParent));
        CXXREFLECT_GENERATE(MemberRef, 2, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(MemberRef, 3, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(MethodDef, 1, 4);
        CXXREFLECT_GENERATE(MethodDef, 2, 2);
        CXXREFLECT_GENERATE(MethodDef, 3, 2);
        CXXREFLECT_GENERATE(MethodDef, 4, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(MethodDef, 5, GetBlobHeapIndexSize());
        CXXREFLECT_GENERATE(MethodDef, 6, GetTableIndexSize(TableId::Param));

        CXXREFLECT_GENERATE(MethodImpl, 1, GetTableIndexSize(TableId::TypeDef));
        CXXREFLECT_GENERATE(MethodImpl, 2, GetCompositeIndexSize(CompositeIndex::MethodDefOrRef));
        CXXREFLECT_GENERATE(MethodImpl, 3, GetCompositeIndexSize(CompositeIndex::MethodDefOrRef));

        CXXREFLECT_GENERATE(MethodSemantics, 1, 2);
        CXXREFLECT_GENERATE(MethodSemantics, 2, GetTableIndexSize(TableId::MethodDef));
        CXXREFLECT_GENERATE(MethodSemantics, 3, GetCompositeIndexSize(CompositeIndex::HasSemantics));

        CXXREFLECT_GENERATE(MethodSpec, 1, GetCompositeIndexSize(CompositeIndex::MethodDefOrRef));
        CXXREFLECT_GENERATE(MethodSpec, 2, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(Module, 1, 2);
        CXXREFLECT_GENERATE(Module, 2, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(Module, 3, GetGuidHeapIndexSize());
        CXXREFLECT_GENERATE(Module, 4, GetGuidHeapIndexSize());
        CXXREFLECT_GENERATE(Module, 5, GetGuidHeapIndexSize());

        CXXREFLECT_GENERATE(ModuleRef, 1, GetStringHeapIndexSize());

        CXXREFLECT_GENERATE(NestedClass, 1, GetTableIndexSize(TableId::TypeDef));
        CXXREFLECT_GENERATE(NestedClass, 2, GetTableIndexSize(TableId::TypeDef));

        CXXREFLECT_GENERATE(Param, 1, 2);
        CXXREFLECT_GENERATE(Param, 2, 2);
        CXXREFLECT_GENERATE(Param, 3, GetStringHeapIndexSize());

        CXXREFLECT_GENERATE(Property, 1, 2);
        CXXREFLECT_GENERATE(Property, 2, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(Property, 3, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(PropertyMap, 1, GetTableIndexSize(TableId::TypeDef));
        CXXREFLECT_GENERATE(PropertyMap, 2, GetTableIndexSize(TableId::Property));

        CXXREFLECT_GENERATE(StandaloneSig, 1, GetBlobHeapIndexSize());

        CXXREFLECT_GENERATE(TypeDef, 1, 4);
        CXXREFLECT_GENERATE(TypeDef, 2, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(TypeDef, 3, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(TypeDef, 4, GetCompositeIndexSize(CompositeIndex::TypeDefOrRef));
        CXXREFLECT_GENERATE(TypeDef, 5, GetTableIndexSize(TableId::Field));
        CXXREFLECT_GENERATE(TypeDef, 6, GetTableIndexSize(TableId::MethodDef));

        CXXREFLECT_GENERATE(TypeRef, 1, GetCompositeIndexSize(CompositeIndex::ResolutionScope));
        CXXREFLECT_GENERATE(TypeRef, 2, GetStringHeapIndexSize());
        CXXREFLECT_GENERATE(TypeRef, 3, GetStringHeapIndexSize());

        CXXREFLECT_GENERATE(TypeSpec, 1, GetBlobHeapIndexSize());

        #undef CXXREFLECT_GENERATE

        // Finally, compute the complete row sizes:
        std::transform(_state.Get()._columnOffsets.begin(),
                       _state.Get()._columnOffsets.end(),
                       _state.Get()._rowSizes.begin(),
                       [](ColumnOffsetSequence const& x) -> SizeType
        {
            auto const it(std::find_if(x.rbegin(), x.rend(), [](SizeType n) { return n != 0; }));
            return it != x.rend() ? *it : 0;
        });
    }

    SizeType TableCollection::GetCompositeIndexSize(CompositeIndex const index) const
    {
        AssertInitialized();

        return _state.Get()._compositeIndexSizes[Detail::AsInteger(index)];
    }

    SizeType TableCollection::GetStringHeapIndexSize() const
    {
        AssertInitialized();

        return _state.Get()._stringHeapIndexSize;
    }

    SizeType TableCollection::GetGuidHeapIndexSize() const
    {
        AssertInitialized();

        return _state.Get()._guidHeapIndexSize;
    }

    SizeType TableCollection::GetBlobHeapIndexSize() const
    {
        AssertInitialized();

        return _state.Get()._blobHeapIndexSize;
    }

    SizeType TableCollection::GetTableColumnOffset(TableId const tableId, SizeType const column) const
    {
        AssertInitialized();
        Detail::Assert([&]
        {
            return column < MaximumColumnCount
                && (column == 0 || _state.Get()._columnOffsets[Detail::AsInteger(tableId)][column] != 0);
        },
        L"Invalid column identifier");

        return _state.Get()._columnOffsets[Detail::AsInteger(tableId)][column];
    }

    Table const& TableCollection::GetTable(TableId const tableId) const
    {
        AssertInitialized();

        return _state.Get()._tables[Detail::AsInteger(tableId)];
    }

    SizeType TableCollection::GetTableIndexSize(TableId const tableId) const
    {
        AssertInitialized();

        return _state.Get()._rowCounts[Detail::AsInteger(tableId)] < (1 << 16) ? 2 : 4;
    }

    bool TableCollection::IsInitialized() const
    {
        return _stream.IsInitialized();
    }

    void TableCollection::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }





    /// Transforms UTF-8 strings to UTF-16, with string caching.
    ///
    /// This class is pimpl'ed because it depends on the C++ <mutex>, which cannot be included in
    /// the public interface headers.
    class StringCollectionCache
    {
    public:

        StringCollectionCache()
        {
        }

        explicit StringCollectionCache(Stream&& stream)
            : _stream(std::move(stream))
        {
        }

        StringReference At(SizeType const index)
        {
            // TODO We can easily break this work up into smaller chunks if this becomes contentious.
            // We can easily do the UTF8->UTF16 conversion outside of any locks.
            Lock const lock(_sync);

            auto const existingIt(_index.find(index));
            if (existingIt != _index.end())
                return existingIt->second;

            char const* pointer(_stream.ReinterpretAs<char>(index));
            int const required(Externals::ComputeUtf16LengthOfUtf8String(pointer));

            auto const range(_buffer.Allocate(required));
            if (!Externals::ConvertUtf8ToUtf16(pointer, range.Begin(), required))
                throw MetadataReadError(L"Failed to convert UTF8 to UTF16");

            return _index.insert(std::make_pair(
                index,
                StringReference(range.Begin(), range.End() - 1)
            )).first->second;
        }

        bool IsInitialized() const
        {
            return _stream.IsInitialized();
        }

    private:

        typedef Detail::LinearArrayAllocator<Character, (1 << 16)> Allocator;
        typedef std::map<SizeType, StringReference>                StringMap;
        typedef std::mutex                                         Mutex;
        typedef std::lock_guard<Mutex>                             Lock;

        StringCollectionCache(StringCollectionCache const&);
        StringCollectionCache& operator=(StringCollectionCache const&);

        void AssertInitialized() const
        {
            Detail::Assert([&]{ return IsInitialized(); });
        }

        Stream            _stream;
        Allocator mutable _buffer;  // Stores the transformed UTF-16 strings
        StringMap mutable _index;   // Maps string heap indices into indices in the buffer
        Mutex     mutable _sync;
    };


    StringCollection::StringCollection()
    {
    }

    StringCollection::StringCollection(Stream&& stream)
        : _cache(new StringCollectionCache(std::move(stream)))
    {
    }

    StringCollection::StringCollection(StringCollection&& other)
        : _cache(std::move(other._cache))
    {
    }

    StringCollection& StringCollection::operator=(StringCollection&& other)
    {
        _cache = std::move(other._cache);
        return *this;
    }

    StringCollection::~StringCollection()
    {
        // User-defined destructor required for pimpl'ed unique_ptr
    }

    StringReference StringCollection::At(SizeType const index) const
    {
        return _cache->At(index);
    }

    bool StringCollection::IsInitialized() const
    {
        return _cache != nullptr && _cache->IsInitialized();
    }

    void StringCollection::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }





    Database Database::CreateFromFile(StringReference const path)
    {
        Detail::FileHandle const fileHandle(path.c_str(), Detail::FileMode::Read | Detail::FileMode::Binary);
        return Database(Externals::MapFile(fileHandle.GetHandle()));
    }

    Database::Database(Detail::FileRange&& file)
        : _file(std::move(file))
    {
        Detail::ConstByteCursor const cursor(_file.Begin(), _file.End());

        auto const peSectionsAndCliHeader(Private::ReadPeSectionsAndCliHeader(cursor));
        auto const streamHeaders(Private::ReadPeCliStreamHeaders(cursor, peSectionsAndCliHeader));
        for (std::size_t i(0); i < streamHeaders.size(); ++i)
        {
            if (streamHeaders[i]._metadataOffset == 0)
                continue;

            Stream newStream(
                cursor,
                streamHeaders[i]._metadataOffset,
                streamHeaders[i]._streamOffset,
                streamHeaders[i]._streamSize);

            switch (static_cast<Private::PeCliStreamKind>(i))
            {
            case Private::PeCliStreamKind::StringStream:
                _strings = StringCollection(std::move(newStream));
                break;

            case Private::PeCliStreamKind::UserStringStream:
                // We do not use the userstrings stream for metadata
                break;

            case Private::PeCliStreamKind::BlobStream:
                _blobStream = std::move(newStream);
                break;

            case Private::PeCliStreamKind::GuidStream:
                _guidStream = std::move(newStream);
                break;

            case Private::PeCliStreamKind::TableStream:
                _tables = TableCollection(std::move(newStream));
                break;

            default:
                throw MetadataReadError(L"Unexpected stream kind value");
            }
        }
    }

    Database::Database(Database&& other)
        : _blobStream(std::move(other._blobStream)),
          _guidStream(std::move(other._guidStream)),
          _strings   (std::move(other._strings   )),
          _tables    (std::move(other._tables    )),
          _file      (std::move(other._file      ))
    {
    }

    Database& Database::operator=(Database&& other)
    {
        Swap(other);
        return *this;
    }

    void Database::Swap(Database& other)
    {
        std::swap(_blobStream, other._blobStream);
        std::swap(_guidStream, other._guidStream);
        std::swap(_strings,    other._strings   );
        std::swap(_tables,     other._tables    );
        std::swap(_file,       other._file      );
    }

    template <TableId TId>
    RowIterator<TId> Database::Begin() const
    {
        AssertInitialized();

        return RowIterator<TId>(this, 0);
    }

    template <TableId TId>
    RowIterator<TId> Database::End() const
    {
        AssertInitialized();

        return RowIterator<TId>(this, _tables.GetTable(TId).GetRowCount());
    }

    template <TableId TId>
    typename TableIdToRowType<TId>::Type Database::GetRow(SizeType const index) const
    {
        typedef typename TableIdToRowType<TId>::Type ReturnType;

        AssertInitialized();

        return CreateRow<ReturnType>(this, _tables.GetTable(TId).At(index));
    }

    template <TableId TId>
    typename TableIdToRowType<TId>::Type Database::GetRow(RowReference const& reference) const
    {
        typedef typename TableIdToRowType<TId>::Type ReturnType;

        AssertInitialized();
        Detail::Assert([&]{ return reference.GetTable() == TId; });

        return CreateRow<ReturnType>(this, _tables.GetTable(TId).At(reference.GetIndex()));
    }

    template <TableId TId>
    typename TableIdToRowType<TId>::Type Database::GetRow(BaseElementReference const& reference) const
    {
        typedef typename TableIdToRowType<TId>::Type ReturnType;

        AssertInitialized();
        Detail::Assert([&]{ return reference.AsRowReference().GetTable() == TId; });

        return CreateRow<ReturnType>(this, _tables.GetTable(TId).At(reference.AsRowReference().GetIndex()));
    }

    StringReference Database::GetString(SizeType const index) const
    {
        AssertInitialized();

        return StringReference(_strings.At(index));
    }

    TableCollection const& Database::GetTables() const
    {
        AssertInitialized();

        return _tables;
    }

    StringCollection const& Database::GetStrings() const
    {
        AssertInitialized();

        return _strings;
    }

    Stream const& Database::GetBlobs() const
    {
        AssertInitialized();

        return _blobStream;
    }

    Stream const& Database::GetGuids() const
    {
        AssertInitialized();
        
        return _guidStream;
    }

    bool Database::IsInitialized() const
    {
        return _blobStream.IsInitialized()
            && _guidStream.IsInitialized()
            && _strings.IsInitialized()
            && _tables.IsInitialized();
    }

    void Database::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    bool operator==(Database const& lhs, Database const& rhs)
    {
        return &lhs == &rhs;
    }

    bool operator<(Database const& lhs, Database const& rhs)
    {
        return std::less<Database const*>()(&lhs, &rhs);
    }

    // Generate explicit instantiations of the Database class's member functions:
    #define CXXREFLECT_GENERATE(t)                                                         \
        template RowIterator<TableId::t> Database::Begin<TableId::t>() const;              \
        template RowIterator<TableId::t> Database::End<TableId::t>()   const;              \
        template t ## Row Database::GetRow<TableId::t>(SizeType                   ) const; \
        template t ## Row Database::GetRow<TableId::t>(RowReference         const&) const; \
        template t ## Row Database::GetRow<TableId::t>(BaseElementReference const&) const;

    CXXREFLECT_GENERATE(Assembly              );
    CXXREFLECT_GENERATE(AssemblyOs            );
    CXXREFLECT_GENERATE(AssemblyProcessor     );
    CXXREFLECT_GENERATE(AssemblyRef           );
    CXXREFLECT_GENERATE(AssemblyRefOs         );
    CXXREFLECT_GENERATE(AssemblyRefProcessor  );
    CXXREFLECT_GENERATE(ClassLayout           );
    CXXREFLECT_GENERATE(Constant              );
    CXXREFLECT_GENERATE(CustomAttribute       );
    CXXREFLECT_GENERATE(DeclSecurity          );
    CXXREFLECT_GENERATE(EventMap              );
    CXXREFLECT_GENERATE(Event                 );
    CXXREFLECT_GENERATE(ExportedType          );
    CXXREFLECT_GENERATE(Field                 );
    CXXREFLECT_GENERATE(FieldLayout           );
    CXXREFLECT_GENERATE(FieldMarshal          );
    CXXREFLECT_GENERATE(FieldRva              );
    CXXREFLECT_GENERATE(File                  );
    CXXREFLECT_GENERATE(GenericParam          );
    CXXREFLECT_GENERATE(GenericParamConstraint);
    CXXREFLECT_GENERATE(ImplMap               );
    CXXREFLECT_GENERATE(InterfaceImpl         );
    CXXREFLECT_GENERATE(ManifestResource      );
    CXXREFLECT_GENERATE(MemberRef             );
    CXXREFLECT_GENERATE(MethodDef             );
    CXXREFLECT_GENERATE(MethodImpl            );
    CXXREFLECT_GENERATE(MethodSemantics       );
    CXXREFLECT_GENERATE(MethodSpec            );
    CXXREFLECT_GENERATE(Module                );
    CXXREFLECT_GENERATE(ModuleRef             );
    CXXREFLECT_GENERATE(NestedClass           );
    CXXREFLECT_GENERATE(Param                 );
    CXXREFLECT_GENERATE(Property              );
    CXXREFLECT_GENERATE(PropertyMap           );
    CXXREFLECT_GENERATE(StandaloneSig         );
    CXXREFLECT_GENERATE(TypeDef               );
    CXXREFLECT_GENERATE(TypeRef               );
    CXXREFLECT_GENERATE(TypeSpec              );

    #undef CXXREFLECT_GENERATE

    




    AssemblyHashAlgorithm AssemblyRow::GetHashAlgorithm() const
    {
        return Private::ReadAs<AssemblyHashAlgorithm>(GetIterator(), GetColumnOffset(0));
    }

    FourComponentVersion AssemblyRow::GetVersion() const
    {
        Private::RawFourComponentVersion const version(
            Private::ReadAs<Private::RawFourComponentVersion>(GetIterator(), GetColumnOffset(1)));
        return FourComponentVersion(version._major, version._minor, version._build, version._revision);
    }

    AssemblyFlags AssemblyRow::GetFlags() const
    {
        return Private::ReadAs<AssemblyAttribute>(GetIterator(), GetColumnOffset(2));
    }

    BlobReference AssemblyRow::GetPublicKey() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(3));
    }

    StringReference AssemblyRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(4));
    }

    StringReference AssemblyRow::GetCulture() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(5));
    }

    std::uint32_t AssemblyOsRow::GetOsPlatformId()   const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    std::uint32_t AssemblyOsRow::GetOsMajorVersion() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(1));
    }

    std::uint32_t AssemblyOsRow::GetOsMinorVersion() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(2));
    }

    std::uint32_t AssemblyProcessorRow::GetProcessor() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    FourComponentVersion AssemblyRefRow::GetVersion() const
    {
        Private::RawFourComponentVersion const version(
            Private::ReadAs<Private::RawFourComponentVersion>(GetIterator(), GetColumnOffset(0)));
        return FourComponentVersion(version._major, version._minor, version._build, version._revision);
    }

    AssemblyFlags AssemblyRefRow::GetFlags() const
    {
        return Private::ReadAs<AssemblyAttribute>(GetIterator(), GetColumnOffset(1));
    }

    BlobReference AssemblyRefRow::GetPublicKey() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    StringReference AssemblyRefRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(3));
    }

    StringReference AssemblyRefRow::GetCulture() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(4));
    }

    BlobReference AssemblyRefRow::GetHashValue() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(5));
    }

    std::uint32_t AssemblyRefOsRow::GetOsPlatformId()   const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    std::uint32_t AssemblyRefOsRow::GetOsMajorVersion() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(1));
    }

    std::uint32_t AssemblyRefOsRow::GetOsMinorVersion() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(2));
    }

    RowReference AssemblyRefOsRow::GetAssemblyRef() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::AssemblyRef, GetColumnOffset(3));
    }

    std::uint32_t AssemblyRefProcessorRow::GetProcessor() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    RowReference AssemblyRefProcessorRow::GetAssemblyRef() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::AssemblyRef, GetColumnOffset(1));
    }

    std::uint16_t ClassLayoutRow::GetPackingSize() const
    {
        return Private::ReadAs<std::uint16_t>(GetIterator(), GetColumnOffset(0));
    }

    std::uint32_t ClassLayoutRow::GetClassSize() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(1));
    }

    RowReference ClassLayoutRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::TypeDef, GetColumnOffset(2));
    }

    ElementType ConstantRow::GetElementType() const
    {
        Byte const type(Private::ReadAs<Byte>(GetIterator(), GetColumnOffset(0)));
        if (!IsValidElementType(type))
            throw MetadataReadError(L"Constant row contains invalid element type.");

        return static_cast<ElementType>(type);
    }

    RowReference ConstantRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::HasConstant, GetColumnOffset(1));
    }

    BlobReference ConstantRow::GetValue() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    RowReference CustomAttributeRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::HasCustomAttribute, GetColumnOffset(0));
    }

    RowReference CustomAttributeRow::GetType() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::CustomAttributeType, GetColumnOffset(1));
    }

    BlobReference CustomAttributeRow::GetValue() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    std::uint16_t DeclSecurityRow::GetAction() const
    {
        return Private::ReadAs<std::uint16_t>(GetIterator(), GetColumnOffset(0));
    }

    RowReference DeclSecurityRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::HasDeclSecurity, GetColumnOffset(1));
    }

    BlobReference DeclSecurityRow::GetPermissionSet() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    RowReference EventMapRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::TypeDef, GetColumnOffset(0));
    }

    RowReference EventMapRow::GetFirstEvent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::Event, GetColumnOffset(1));
    }

    RowReference EventMapRow::GetLastEvent() const
    {
        return Private::ComputeLastRowReference<
            TableId::EventMap,
            TableId::Event
        >(GetDatabase(), GetIterator(), &EventMapRow::GetFirstEvent);
    }

    EventFlags EventRow::GetFlags() const
    {
        return Private::ReadAs<EventAttribute>(GetIterator(), GetColumnOffset(0));
    }

    StringReference EventRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    RowReference EventRow::GetType() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::TypeDefOrRef, GetColumnOffset(2));
    }

    TypeFlags ExportedTypeRow::GetFlags() const
    {
        return Private::ReadAs<TypeAttribute>(GetIterator(), GetColumnOffset(0));
    }

    std::uint32_t ExportedTypeRow::GetTypeDefId() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(1));
    }

    StringReference ExportedTypeRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    StringReference ExportedTypeRow::GetNamespace() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(3));
    }

    RowReference ExportedTypeRow::GetImplementation() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::Implementation, GetColumnOffset(4));
    }

    FieldFlags FieldRow::GetFlags() const
    {
        return Private::ReadAs<FieldAttribute>(GetIterator(), GetColumnOffset(0));
    }

    StringReference FieldRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    BlobReference FieldRow::GetSignature() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    SizeType FieldLayoutRow::GetOffset() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    RowReference FieldLayoutRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::Field, GetColumnOffset(1));
    }

    RowReference FieldMarshalRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::HasFieldMarshal, GetColumnOffset(0));
    }

    BlobReference FieldMarshalRow::GetNativeType() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    SizeType FieldRvaRow::GetRva() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    RowReference FieldRvaRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::Field, GetColumnOffset(1));
    }

    FileFlags FileRow::GetFlags() const
    {
        return Private::ReadAs<FileAttribute>(GetIterator(), GetColumnOffset(0));
    }

    StringReference FileRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    BlobReference FileRow::GetHashValue() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    std::uint16_t GenericParamRow::GetSequence() const
    {
        return Private::ReadAs<std::uint16_t>(GetIterator(), GetColumnOffset(0));
    }

    GenericParameterFlags GenericParamRow::GetFlags() const
    {
        return Private::ReadAs<GenericParameterAttribute>(GetIterator(), GetColumnOffset(1));
    }

    RowReference GenericParamRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::TypeOrMethodDef, GetColumnOffset(2));
    }

    StringReference GenericParamRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(3));
    }

    RowReference GenericParamConstraintRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::GenericParam, GetColumnOffset(0));
    }

    RowReference GenericParamConstraintRow::GetConstraint() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::TypeDefOrRef, GetColumnOffset(1));
    }

    PInvokeFlags ImplMapRow::GetMappingFlags() const
    {
        return Private::ReadAs<PInvokeAttribute>(GetIterator(), GetColumnOffset(0));
    }

    RowReference ImplMapRow::GetMemberForwarded() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::MemberForwarded, GetColumnOffset(1));
    }

    StringReference ImplMapRow::GetImportName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    RowReference ImplMapRow::GetImportScope() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::ModuleRef, GetColumnOffset(3));
    }

    RowReference InterfaceImplRow::GetClass() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::TypeDef, GetColumnOffset(0));
    }

    RowReference InterfaceImplRow::GetInterface() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::TypeDefOrRef, GetColumnOffset(1));
    }

    SizeType ManifestResourceRow::GetOffset() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    ManifestResourceFlags ManifestResourceRow::GetFlags() const
    {
        return Private::ReadAs<ManifestResourceAttribute>(GetIterator(), GetColumnOffset(1));
    }

    StringReference ManifestResourceRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    RowReference ManifestResourceRow::GetImplementation() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::Implementation, GetColumnOffset(3));
    }

    RowReference MemberRefRow::GetClass() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::MemberRefParent, GetColumnOffset(0));
    }

    StringReference MemberRefRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    BlobReference MemberRefRow::GetSignature() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    SizeType MethodDefRow::GetRva() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    MethodImplementationFlags MethodDefRow::GetImplementationFlags() const
    {
        return Private::ReadAs<MethodImplementationAttribute>(GetIterator(), GetColumnOffset(1));
    }

    MethodFlags MethodDefRow::GetFlags() const
    {
        return Private::ReadAs<MethodAttribute>(GetIterator(), GetColumnOffset(2));
    }

    StringReference MethodDefRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(3));
    }

    BlobReference MethodDefRow::GetSignature() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(4));
    }

    RowReference MethodDefRow::GetFirstParameter() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::Param, GetColumnOffset(5));
    }

    RowReference MethodDefRow::GetLastParameter() const
    {
        return Private::ComputeLastRowReference<
            TableId::MethodDef,
            TableId::Param
        >(GetDatabase(), GetIterator(), &MethodDefRow::GetFirstParameter);
    }

    RowReference MethodImplRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::TypeDef, GetColumnOffset(0));
    }

    RowReference MethodImplRow::GetMethodBody() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::MethodDefOrRef, GetColumnOffset(1));
    }

    RowReference MethodImplRow::GetMethodDeclaration() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::MethodDefOrRef, GetColumnOffset(2));
    }

    MethodSemanticsFlags MethodSemanticsRow::GetSemantics() const
    {
        return Private::ReadAs<MethodSemanticsAttribute>(GetIterator(), GetColumnOffset(0));
    }

    RowReference MethodSemanticsRow::GetMethod() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::MethodDef, GetColumnOffset(1));
    }

    RowReference MethodSemanticsRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::HasSemantics, GetColumnOffset(2));
    }

    RowReference MethodSpecRow::GetMethod() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::MethodDefOrRef, GetColumnOffset(0));
    }

    BlobReference MethodSpecRow::GetSignature() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    StringReference ModuleRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    BlobReference ModuleRow::GetMvid() const
    {
        return Private::ReadGuidReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    StringReference ModuleRefRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(0));
    }

    RowReference NestedClassRow::GetNestedClass() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::TypeDef, GetColumnOffset(0));
    }

    RowReference NestedClassRow::GetEnclosingClass() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::TypeDef, GetColumnOffset(1));
    }

    ParameterFlags ParamRow::GetFlags() const
    {
        return Private::ReadAs<ParameterAttribute>(GetIterator(), GetColumnOffset(0));
    }

    std::uint16_t ParamRow::GetSequence() const
    {
        return Private::ReadAs<std::uint16_t>(GetIterator(), GetColumnOffset(1));
    }

    StringReference ParamRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    PropertyFlags PropertyRow::GetFlags() const
    {
        return Private::ReadAs<PropertyAttribute>(GetIterator(), GetColumnOffset(0));
    }

    StringReference PropertyRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    BlobReference PropertyRow::GetSignature() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    RowReference PropertyMapRow::GetParent() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::TypeDef, GetColumnOffset(0));
    }

    RowReference PropertyMapRow::GetFirstProperty() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::Property, GetColumnOffset(1));
    }

    RowReference PropertyMapRow::GetLastProperty() const
    {
        return Private::ComputeLastRowReference<
            TableId::PropertyMap,
            TableId::Property
        >(GetDatabase(), GetIterator(), &PropertyMapRow::GetFirstProperty);
    }

    BlobReference StandaloneSigRow::GetSignature() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(0));
    }

    TypeFlags TypeDefRow::GetFlags() const
    {
        return Private::ReadAs<TypeAttribute>(GetIterator(), GetColumnOffset(0));
    }

    StringReference TypeDefRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    StringReference TypeDefRow::GetNamespace() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    RowReference TypeDefRow::GetExtends() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::TypeDefOrRef, GetColumnOffset(3));
    }

    RowReference TypeDefRow::GetFirstField() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::Field, GetColumnOffset(4));
    }

    RowReference TypeDefRow::GetLastField() const
    {
        return Private::ComputeLastRowReference<
            TableId::TypeDef,
            TableId::Field
        >(GetDatabase(), GetIterator(), &TypeDefRow::GetFirstField);
    }

    RowReference TypeDefRow::GetFirstMethod() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::MethodDef, GetColumnOffset(5));
    }

    RowReference TypeDefRow::GetLastMethod() const
    {
        return Private::ComputeLastRowReference<
            TableId::TypeDef,
            TableId::MethodDef
        >(GetDatabase(), GetIterator(), &TypeDefRow::GetFirstMethod);
    }

    RowReference TypeRefRow::GetResolutionScope() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::ResolutionScope, GetColumnOffset(0));
    }

    StringReference TypeRefRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    StringReference TypeRefRow::GetNamespace() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(2));
    }

    BlobReference TypeSpecRow::GetSignature() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(0));
    }





    CompositeIndexPrimaryKeyStrictWeakOrdering::CompositeIndexPrimaryKeyStrictWeakOrdering(CompositeIndex const index)
        : _index(index)
    {
        Detail::Assert([&]{ return index != static_cast<CompositeIndex>(-1); });
    }

    bool CompositeIndexPrimaryKeyStrictWeakOrdering::operator()(RowReference const& lhs, RowReference const& rhs) const
    {
        if (lhs.GetIndex() < rhs.GetIndex())
            return true;

        if (lhs.GetIndex() > rhs.GetIndex())
            return false;

        SizeType const lhsIndexKey(GetCompositeIndexKeyFromTableId(lhs.GetTable(), _index.Get()));
        SizeType const rhsIndexKey(GetCompositeIndexKeyFromTableId(rhs.GetTable(), _index.Get()));

        // If we've already gotten this far, we've successfully converted the index value to a
        // table identifier, so the reverse should work equally well.  Check only in debug:
        Detail::Assert([&]{ return lhsIndexKey != -1 && rhsIndexKey != -1; });

        return lhsIndexKey < rhsIndexKey;
    }

    TableIdPrimeryKeyStrictWeakOrdering::TableIdPrimeryKeyStrictWeakOrdering(TableId const tableId)
        : _tableId(tableId)
    {
    }

    bool TableIdPrimeryKeyStrictWeakOrdering::operator()(RowReference const& lhs, RowReference const& rhs) const
    {
        Detail::Assert([&]{ return lhs.GetTable() == rhs.GetTable(); });

        if (lhs.GetIndex() < rhs.GetIndex())
            return true;

        return false;
    }




    
    TypeDefRow GetOwnerOfEvent(EventRow const& eventRow)
    {
        EventMapRow const mapRow(Private::GetOwningRow<
            EventMapRow, EventRow, &EventMapRow::GetFirstEvent, &EventMapRow::GetLastEvent
        >(eventRow));

        return eventRow.GetDatabase().GetRow<TableId::TypeDef>(mapRow.GetParent());
    }

    
    TypeDefRow GetOwnerOfMethodDef(MethodDefRow const& methodDef)
    {
        return Private::GetOwningRow<
            TypeDefRow, MethodDefRow, &TypeDefRow::GetFirstMethod, &TypeDefRow::GetLastMethod
        >(methodDef);
    }

    TypeDefRow GetOwnerOfField(FieldRow const& field)
    {
        return Private::GetOwningRow<
            TypeDefRow, FieldRow, &TypeDefRow::GetFirstField, &TypeDefRow::GetLastField
        >(field);
    }

    TypeDefRow GetOwnerOfProperty(PropertyRow const& propertyRow)
    {
        PropertyMapRow const mapRow(Private::GetOwningRow<
            PropertyMapRow, PropertyRow, &PropertyMapRow::GetFirstProperty, &PropertyMapRow::GetLastProperty
        >(propertyRow));

        return propertyRow.GetDatabase().GetRow<TableId::TypeDef>(mapRow.GetParent());
    }

    MethodDefRow GetOwnerOfParam(ParamRow const& param)
    {
        return Private::GetOwningRow<
            MethodDefRow, ParamRow, &MethodDefRow::GetFirstParameter, &MethodDefRow::GetLastParameter
        >(param);
    }





    ConstantRow GetConstant(FullReference const& parent)
    {
        if (!parent.IsRowReference())
            throw LogicError(L"Invalid argument:  parent must be a row reference");

        RowReference const& parentRow(parent.AsRowReference());

        if (GetCompositeIndexKeyFromTableId(parentRow.GetTable(), CompositeIndex::HasConstant) == -1)
            throw LogicError(L"Invalid argument:  parent is not from an allowed table for this index");

        auto const range(Detail::EqualRange(
            parent.GetDatabase().Begin<TableId::Constant>(),
            parent.GetDatabase().End<TableId::Constant>(),
            parentRow,
            CompositeIndexPrimaryKeyStrictWeakOrdering(CompositeIndex::HasConstant)));

        // Not every row has a constant value:
        if (range.first == range.second)
            return ConstantRow();

        if (Detail::Distance(range.first, range.second) != 1)
            throw MetadataReadError(L"Constant table has non-unique parent index");

        return *range.first;
    }

    FieldLayoutRow GetFieldLayout(FullReference const& parent)
    {
        if (!parent.IsRowReference())
            throw LogicError(L"Invalid argument:  parent must be a row reference");

        RowReference const& parentRow(parent.AsRowReference());

        if (parentRow.GetTable() != TableId::Field)
            throw LogicError(L"Invalid argument:  parent is not from an allowed table for this query");

        auto const range(Detail::EqualRange(
            parent.GetDatabase().Begin<TableId::FieldLayout>(),
            parent.GetDatabase().End<TableId::FieldLayout>(),
            parentRow,
            TableIdPrimeryKeyStrictWeakOrdering(TableId::Field)));

        // Not every row has a constant value:
        if (range.first == range.second)
            return FieldLayoutRow();

        if (Detail::Distance(range.first, range.second) != 1)
            throw MetadataReadError(L"FieldLayout table has non-unique parent index");

        return *range.first;
    }





    RowReferencePair GetCustomAttributesRange(FullReference const& parent)
    {
        Detail::Verify([&]{ return parent.IsRowReference(); });

        auto const range(Detail::EqualRange(
            parent.GetDatabase().Begin<TableId::CustomAttribute>(),
            parent.GetDatabase().End<TableId::CustomAttribute>(),
            parent.AsRowReference(),
            CompositeIndexPrimaryKeyStrictWeakOrdering(CompositeIndex::HasCustomAttribute)));

        return std::make_pair(range.first.GetReference(), range.second.GetReference());
    }
    
    RowIterator<TableId::CustomAttribute> BeginCustomAttributes(FullReference const& parent)
    {
        return RowIterator<TableId::CustomAttribute>(
            &parent.GetDatabase(),
            GetCustomAttributesRange(parent).first.GetIndex());
    }

    RowIterator<TableId::CustomAttribute> EndCustomAttributes(FullReference const& parent)
    {
        return RowIterator<TableId::CustomAttribute>(
            &parent.GetDatabase(),
            GetCustomAttributesRange(parent).second.GetIndex());
    }





    RowReferencePair GetMethodImplsRange(FullReference const& parent)
    {
        Detail::Verify([&]{ return parent.IsRowReference(); });

        auto const range(Detail::EqualRange(
            parent.GetDatabase().Begin<TableId::MethodImpl>(),
            parent.GetDatabase().End<TableId::MethodImpl>(),
            parent.AsRowReference(),
            TableIdPrimeryKeyStrictWeakOrdering(TableId::TypeDef)));

        return std::make_pair(range.first.GetReference(), range.second.GetReference());
    }
    
    RowIterator<TableId::MethodImpl> BeginMethodImpls(FullReference const& parent)
    {
        return RowIterator<TableId::MethodImpl>(
            &parent.GetDatabase(),
            GetMethodImplsRange(parent).first.GetIndex());
    }

    RowIterator<TableId::MethodImpl> EndMethodImpls(FullReference const& parent)
    {
        return RowIterator<TableId::MethodImpl>(
            &parent.GetDatabase(),
            GetMethodImplsRange(parent).second.GetIndex());
    }





    RowReferencePair GetMethodSemanticsRange(FullReference const& parent)
    {
        Detail::Verify([&]{ return parent.IsRowReference(); });

        auto const range(Detail::EqualRange(
            parent.GetDatabase().Begin<TableId::MethodSemantics>(),
            parent.GetDatabase().End<TableId::MethodSemantics>(),
            parent.AsRowReference(),
            CompositeIndexPrimaryKeyStrictWeakOrdering(CompositeIndex::HasSemantics)));

        return std::make_pair(range.first.GetReference(), range.second.GetReference());
    }

    RowIterator<TableId::MethodSemantics> BeginMethodSemantics(FullReference const& parent)
    {
        return RowIterator<TableId::MethodSemantics>(
            &parent.GetDatabase(),
            GetMethodSemanticsRange(parent).first.GetIndex());
    }

    RowIterator<TableId::MethodSemantics> EndMethodSemantics(FullReference const& parent)
    {
        return RowIterator<TableId::MethodSemantics>(
            &parent.GetDatabase(),
            GetMethodSemanticsRange(parent).second.GetIndex());
    }

} }
