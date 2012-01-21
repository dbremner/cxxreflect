//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreInternals.hpp"

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

    PeSectionsAndCliHeader ReadPeSectionsAndCliHeader(Detail::FileHandle& file)
    {
        // The index of the PE Header is located at index 0x3c of the DOS header
        file.Seek(0x3c, Detail::FileHandle::Begin);
        
        std::uint32_t fileHeaderOffset(0);
        file.Read(&fileHeaderOffset, sizeof fileHeaderOffset, 1);
        file.Seek(fileHeaderOffset, Detail::FileHandle::Begin);

        PeFileHeader fileHeader = { 0 };
        file.Read(&fileHeader, sizeof fileHeader, 1);
        if (fileHeader._sectionCount == 0 || fileHeader._sectionCount > 100)
            throw ReadError("PE section count is out of range");

        PeSectionHeaderSequence sections(fileHeader._sectionCount);
        file.Read(sections.data(), sizeof *sections.begin(), sections.size());

        auto cliHeaderSectionIt(std::find_if(
            sections.begin(), sections.end(),
            PeSectionContainsRva(fileHeader._cliHeaderTable._rva)));

        if (cliHeaderSectionIt == sections.end())
            throw ReadError("Failed to locate PE file section containing CLI header");

        std::size_t cliHeaderTableOffset(ComputeOffsetFromRva(
            *cliHeaderSectionIt,
            fileHeader._cliHeaderTable));

        file.Seek(cliHeaderTableOffset, Detail::FileHandle::Begin);

        PeCliHeader cliHeader = { 0 };
        file.Read(&cliHeader, sizeof cliHeader, 1);

        PeSectionsAndCliHeader result;
        result._sections = std::move(sections);
        result._cliHeader = cliHeader;
        return result;
    }

    PeCliStreamHeaderSequence ReadPeCliStreamHeaders(Detail::FileHandle          & file,
                                                     PeSectionsAndCliHeader const& peHeader)
    {
        auto metadataSectionIt(std::find_if(
            peHeader._sections.begin(),
            peHeader._sections.end(),
            PeSectionContainsRva(peHeader._cliHeader._metadata._rva)));

        if (metadataSectionIt == peHeader._sections.end())
            throw ReadError("Failed to locate PE file section containing CLI metadata");

        SizeType metadataOffset(ComputeOffsetFromRva(
            *metadataSectionIt,
            peHeader._cliHeader._metadata));

        file.Seek(metadataOffset, Detail::FileHandle::Begin);

        std::uint32_t magicSignature(0);
        file.Read(&magicSignature, sizeof magicSignature, 1);
        if (magicSignature != 0x424a5342)
            throw ReadError("Magic signature does not match required value 0x424a5342");

        file.Seek(8, Detail::FileHandle::Current);

        std::uint32_t versionLength(0);
        file.Read(&versionLength, sizeof versionLength, 1);
        file.Seek(versionLength + 2, Detail::FileHandle::Current); // Add 2 to account for unused flags

        std::uint16_t streamCount(0);
        file.Read(&streamCount, sizeof streamCount, 1);

        PeCliStreamHeaderSequence streamHeaders = { 0 };
        for (std::uint16_t i(0); i < streamCount; ++i)
        {
            PeCliStreamHeader header;
            header._metadataOffset = metadataOffset;
            file.Read(&header._streamOffset, sizeof header._streamOffset, 1);
            file.Read(&header._streamSize,   sizeof header._streamSize,   1);

            std::array<char, 12> currentName = { 0 };
            file.Read(currentName.data(), sizeof *currentName.begin(), currentName.size());

            #define CXXREFLECT_GENERATE(name, id, reset)                                        \
                if (std::strcmp(currentName.data(), name) == 0 &&                               \
                    streamHeaders[Detail::AsInteger(PeCliStreamKind::id)]._metadataOffset == 0) \
                {                                                                               \
                    streamHeaders[Detail::AsInteger(PeCliStreamKind::id)] = header;             \
                    file.Seek(reset, Detail::FileHandle::Current);                              \
                    used = true;                                                                \
                }

            bool used(false);
            CXXREFLECT_GENERATE("#Strings", StringStream,      0);
            CXXREFLECT_GENERATE("#US",      UserStringStream, -8);
            CXXREFLECT_GENERATE("#Blob",    BlobStream,       -4);
            CXXREFLECT_GENERATE("#GUID",    GuidStream,       -4);
            CXXREFLECT_GENERATE("#~",       TableStream,      -8);
            if (!used)
                throw ReadError("Unknown stream name encountered");

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
    T const& ReadAs(ByteIterator const data, SizeType const index)
    {
        return *reinterpret_cast<T const*>(data + index);
    }

    std::uint32_t ReadTableIndex(Database     const& database,
                                 ByteIterator const  data,
                                 TableId      const  table,
                                 SizeType     const  offset)
    {
        // Table indexes are one-based, to allow zero to be used as a null reference value.  In
        // order to simplify row offset calculations, we subtract one from all table indices to
        // make them zero-based, with -1 (0xffffffff) representing a null reference.
        switch (database.GetTables().GetTableIndexSize(table))
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset) - 1;
        case 4:  return ReadAs<std::uint32_t>(data, offset) - 1;
        default: Detail::VerifyFail("Invalid table index size");
        }

        return 0;
    }

    std::uint32_t ReadCompositeIndex(Database       const& database,
                                     ByteIterator   const  data,
                                     CompositeIndex const  index,
                                     SizeType       const  offset)
    {
        switch (database.GetTables().GetCompositeIndexSize(index))
        {
            case 2:  return ReadAs<std::uint16_t>(data, offset);
            case 4:  return ReadAs<std::uint32_t>(data, offset);
            default: Detail::VerifyFail("Invalid composite index size");
        }
        
        return 0;
    }

    std::uint32_t ReadBlobHeapIndex(Database const& database, ByteIterator const data, SizeType const offset)
    {
        switch (database.GetTables().GetBlobHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset);
        case 4:  return ReadAs<std::uint32_t>(data, offset);
        default: Detail::VerifyFail("Invalid blob heap index size");
        }

        return 0;
    }

    BlobReference ReadBlobReference(Database const& database, ByteIterator const data, SizeType const offset)
    {
        return BlobReference(ReadBlobHeapIndex(database, data, offset));
    }

    std::uint32_t ReadStringHeapIndex(Database const& database, ByteIterator const data, SizeType const offset)
    {
        switch (database.GetTables().GetStringHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset);
        case 4:  return ReadAs<std::uint32_t>(data, offset);
        default: Detail::VerifyFail("Invalid string heap index size");
        }

        return 0;
    }

    StringReference ReadStringReference(Database const& database, ByteIterator const data, SizeType const offset)
    {
        return database.GetString(ReadStringHeapIndex(database, data, offset));
    }

    RowReference ReadRowReference(Database     const& database,
                                  ByteIterator const  data,
                                  TableId      const  table,
                                  SizeType     const  offset)
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

    RowReference DecodeCustomAttributeTypeIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::CustomAttributeType, value));
        switch (split.first)
        {
        case 2:  return RowReference(TableId::MethodDef, split.second);
        case 3:  return RowReference(TableId::MemberRef, split.second);
        default: throw ReadError("Invalid CustomAttributeType composite index value encountered");
        }
    }

    RowReference DecodeHasConstantIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasConstant, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::Field,    split.second);
        case 1:  return RowReference(TableId::Param,    split.second);
        case 2:  return RowReference(TableId::Property, split.second);
        default: throw ReadError("Invalid HasConstant composite index value encountered");
        }
    }

    RowReference DecodeHasCustomAttributeIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasCustomAttribute, value));
        switch (split.first)
        {
        case  0: return RowReference(TableId::MethodDef,              split.second);
        case  1: return RowReference(TableId::Field,                  split.second);
        case  2: return RowReference(TableId::TypeRef,                split.second);
        case  3: return RowReference(TableId::TypeDef,                split.second);
        case  4: return RowReference(TableId::Param,                  split.second);
        case  5: return RowReference(TableId::InterfaceImpl,          split.second);
        case  6: return RowReference(TableId::MemberRef,              split.second);
        case  7: return RowReference(TableId::Module,                 split.second);
        // case  8: return RowReference(TableId::Permission,          split.second); // TODO WHAT IS THIS?
        case  9: return RowReference(TableId::Property,               split.second);
        case 10: return RowReference(TableId::Event,                  split.second);
        case 11: return RowReference(TableId::StandaloneSig,          split.second);
        case 12: return RowReference(TableId::ModuleRef,              split.second);
        case 13: return RowReference(TableId::TypeSpec,               split.second);
        case 14: return RowReference(TableId::Assembly,               split.second);
        case 15: return RowReference(TableId::AssemblyRef,            split.second);
        case 16: return RowReference(TableId::File,                   split.second);
        case 17: return RowReference(TableId::ExportedType,           split.second);
        case 18: return RowReference(TableId::ManifestResource,       split.second);
        case 19: return RowReference(TableId::GenericParam,           split.second);
        case 20: return RowReference(TableId::GenericParamConstraint, split.second);
        case 21: return RowReference(TableId::MethodSpec,             split.second);
        default: throw ReadError("Invalid HasCustomAttribute composite index value encountered");
        }
    }

    RowReference DecodeHasDeclSecurityIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasFieldMarshal, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::TypeDef,   split.second);
        case 1:  return RowReference(TableId::MethodDef, split.second);
        case 2:  return RowReference(TableId::Assembly,  split.second);
        default: throw ReadError("Invalid HasFieldMarshal composite index value encountered");
        }
    }

    RowReference DecodeHasFieldMarshalIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasFieldMarshal, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::Field, split.second);
        case 1:  return RowReference(TableId::Param, split.second);
        default: Detail::VerifyFail("Too many bits!"); return RowReference();
        }
    }

    RowReference DecodeHasSemanticsIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasSemantics, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::Event,    split.second);
        case 1:  return RowReference(TableId::Property, split.second);
        default: Detail::VerifyFail("Too many bits!"); return RowReference();
        }
    }

    RowReference DecodeImplementationIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::Implementation, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::File,         split.second);
        case 1:  return RowReference(TableId::AssemblyRef,  split.second);
        case 2:  return RowReference(TableId::ExportedType, split.second);
        default: throw ReadError("Invalid Implementation composite index value encountered");
        }
    }

    RowReference DecodeMemberForwardedIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::MemberForwarded, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::Field,     split.second);
        case 1:  return RowReference(TableId::MethodDef, split.second);
        default: Detail::VerifyFail("Too many bits!"); return RowReference();
        }
    }

    RowReference DecodeMemberRefParentIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::MemberRefParent, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::TypeDef,   split.second);
        case 1:  return RowReference(TableId::TypeRef,   split.second);
        case 2:  return RowReference(TableId::ModuleRef, split.second);
        case 3:  return RowReference(TableId::MethodDef, split.second);
        case 4:  return RowReference(TableId::TypeSpec,  split.second);
        default: throw ReadError("Invalid MemberRefParent composite index value encountered");
        }
    }

    RowReference DecodeMethodDefOrRefIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::MethodDefOrRef, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::MethodDef, split.second);
        case 1:  return RowReference(TableId::MemberRef, split.second);
        default: Detail::VerifyFail("Too many bits!"); return RowReference();
        }
    }

    RowReference DecodeResolutionScopeIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::ResolutionScope, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::Module,      split.second);
        case 1:  return RowReference(TableId::ModuleRef,   split.second);
        case 2:  return RowReference(TableId::AssemblyRef, split.second);
        case 3:  return RowReference(TableId::TypeRef,     split.second);
        default: Detail::VerifyFail("Too many bits!"); return RowReference();
        }
    }

    RowReference DecodeTypeDefOrRefIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::TypeDefOrRef, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::TypeDef,  split.second);
        case 1:  return RowReference(TableId::TypeRef,  split.second);
        case 2:  return RowReference(TableId::TypeSpec, split.second);
        default: throw ReadError("Invalid TypeDefOrRef composite index value encountered");
        }
    }

    RowReference DecodeTypeOrMethodDefIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::TypeOrMethodDef, value));
        switch (split.first)
        {
        case 0:  return RowReference(TableId::TypeDef,   split.second);
        case 1:  return RowReference(TableId::MethodDef, split.second);
        default: Detail::VerifyFail("Too many bits!"); return RowReference();
        }
    }

    RowReference ReadRowReference(Database       const& database,
                                  ByteIterator   const  data,
                                  CompositeIndex const  index,
                                  SizeType       const  offset)
    {
        std::uint32_t const value(ReadCompositeIndex(database, data, index, offset));
        if (value == 0)
            return RowReference();
        switch (index)
        {
        case CompositeIndex::CustomAttributeType: return DecodeCustomAttributeTypeIndex(value);
        case CompositeIndex::HasConstant:         return DecodeHasConstantIndex(value);
        case CompositeIndex::HasCustomAttribute:  return DecodeHasCustomAttributeIndex(value);
        case CompositeIndex::HasDeclSecurity:     return DecodeHasDeclSecurityIndex(value);
        case CompositeIndex::HasFieldMarshal:     return DecodeHasFieldMarshalIndex(value);
        case CompositeIndex::HasSemantics:        return DecodeHasSemanticsIndex(value);
        case CompositeIndex::Implementation:      return DecodeImplementationIndex(value);
        case CompositeIndex::MemberForwarded:     return DecodeMemberForwardedIndex(value);
        case CompositeIndex::MemberRefParent:     return DecodeMemberRefParentIndex(value);
        case CompositeIndex::MethodDefOrRef:      return DecodeMethodDefOrRefIndex(value);
        case CompositeIndex::ResolutionScope:     return DecodeResolutionScopeIndex(value);
        case CompositeIndex::TypeDefOrRef:        return DecodeTypeDefOrRefIndex(value);
        case CompositeIndex::TypeOrMethodDef:     return DecodeTypeOrMethodDefIndex(value);
        default:  Detail::VerifyFail("Invalid index");     return RowReference();
        }
    }

    template <TableId TSourceId, TableId TTargetId, typename TFirstFunction>
    RowReference ComputeLastRowReference(Database const& database, ByteIterator const data, TFirstFunction const first)
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

} } } }

namespace CxxReflect { namespace Metadata {

    RowReference::ValueType RowReference::ComposeValue(TableId const tableId, IndexType const index)
    {
        Detail::Verify([&]{ return IsValidTableId(Detail::AsInteger(tableId)); });
        Detail::Verify([&]{ return Detail::AsInteger(tableId) < (1 << ValueTableIdBits); });
        Detail::Verify([&]{ return index < ValueIndexMask; });

        ValueType const tableIdValue(Detail::AsInteger(tableId) & (ValueTableIdMask >> ValueIndexBits));

        ValueType const tableIdComponent(tableIdValue << ValueIndexBits);
        ValueType const indexComponent(index & ValueIndexMask);

        return tableIdComponent | indexComponent;
    }

    RowReference RowReference::FromToken(TokenType const token)
    {
        Detail::Verify([&]{ return (token & ValueIndexMask) != 0; });
        RowReference result;
        result._value.Get() = token - 1;
        return result;
    }




    Blob::Range Blob::ComputeBounds(ByteIterator const first, ByteIterator const last, SizeType const size)
    {
        if (first == last)
            throw ReadError("Invalid blob");

        if (size > 0)
            return std::make_pair(first, first + size);

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
            throw ReadError("Invalid blob");
        }

        if (static_cast<SizeType>(last - first) < blobSizeBytes)
            throw ReadError("Invalid blob");

        SizeType blobSize(initialByte);
        for (unsigned i(1); i < blobSizeBytes; ++ i)
            blobSize = (blobSize << 8) + *(first + i);

        if (static_cast<SizeType>(last - first) < blobSizeBytes + blobSize)
            throw ReadError("Invalid blob");

        return std::make_pair(first + blobSizeBytes, first + blobSizeBytes + blobSize);
    }




    StringCollection::StringCollection()
    {
    }

    StringCollection::StringCollection(Stream&& stream)
        : _stream(std::move(stream))
    {
    }

    StringCollection::StringCollection(StringCollection&& other)
        : _stream(std::move(other._stream)),
          _buffer(std::move(other._buffer)),
          _index(std::move(other._index))
    {
    }

    StringCollection& StringCollection::operator=(StringCollection&& other)
    {
        Swap(other);
        return *this;
    }

    void StringCollection::Swap(StringCollection& other)
    {
        std::swap(other._stream, _stream);
        std::swap(other._buffer, _buffer);
        std::swap(other._index,  _index );
    }

    StringReference StringCollection::At(SizeType const index) const
    {
        auto const existingIt(_index.find(index));
        if (existingIt != _index.end())
            return existingIt->second;

        char const* pointer(_stream.ReinterpretAs<char>(index));
        int const required(Detail::ComputeUtf16LengthOfUtf8String(pointer));

        auto const range(_buffer.Allocate(required));
        if (!Detail::ConvertUtf8ToUtf16(pointer, range.Begin(), required))
            throw std::logic_error("wtf");

        return _index.insert(std::make_pair(index, StringReference(range.Begin(), range.End()))).first->second;
    }

    bool StringCollection::IsInitialized() const
    {
        return _stream.IsInitialized();
    }

    void StringCollection::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }




    Stream::Stream()
    {
    }

    Stream::Stream(Detail::FileHandle& file,
                   SizeType const metadataOffset,
                   SizeType const streamOffset,
                   SizeType const streamSize)
        : _size(streamSize)
    {
        _data.reset(new Byte[streamSize]);
        file.Seek(metadataOffset + streamOffset, Detail::FileHandle::Begin);
        file.Read(_data.get(), streamSize, 1);
    }

    Stream::Stream(Stream&& other)
        : _data(std::move(other._data)),
          _size(std::move(other._size))
    {
        other._size.Reset();
    }

    Stream& Stream::operator=(Stream&& other)
    {
        Swap(other);
        return *this;
    }

    void Stream::Swap(Stream& other)
    {
        std::swap(other._data, _data);
        std::swap(other._size, _size);
    }




    TableCollection::TableCollection()
    {
    }

    TableCollection::TableCollection(Stream&& stream)
        : _stream(std::move(stream))
    {
        std::bitset<8> heapSizes(_stream.ReadAs<std::uint8_t>(6));
        _state.Get()._stringHeapIndexSize = heapSizes.test(0) ? 4 : 2;
        _state.Get()._guidHeapIndexSize   = heapSizes.test(1) ? 4 : 2;
        _state.Get()._blobHeapIndexSize   = heapSizes.test(2) ? 4 : 2;

        _state.Get()._validBits  = _stream.ReadAs<std::uint64_t>(8);
        _state.Get()._sortedBits = _stream.ReadAs<std::uint64_t>(16);

        SizeType index(24);
        for (unsigned x(0); x < 64; ++x)
        {
            if (!_state.Get()._validBits.test(x))
                continue;

            if (!IsValidTableId(x))
                throw ReadError("Metadata table presence vector has invalid bits set");

            _state.Get()._rowCounts[x] = _stream.ReadAs<std::uint32_t>(index);
            index += 4;
        }

        ComputeCompositeIndexSizes();
        ComputeTableRowSizes();

        for (unsigned x(0); x < 64; ++x)
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

    TableCollection::TableCollection(TableCollection&& other)
        : _stream(std::move(other._stream)),
          _state (std::move(other._state ))
    {
        other._state.Reset();
    }

    TableCollection& TableCollection::operator=(TableCollection&& other)
    {
        Swap(other);
        return *this;
    }

    void TableCollection::Swap(TableCollection& other)
    {
        std::swap(other._stream,      _stream     );
        std::swap(other._state,       _state      );
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
        return _state.Get()._compositeIndexSizes[Detail::AsInteger(index)];
    }

    SizeType TableCollection::GetTableColumnOffset(TableId const tableId, SizeType const column) const
    {
        Detail::Verify([&] { return column < MaximumColumnCount; }, "Invalid column identifier");
        // TODO Check table-specific offset
        return _state.Get()._columnOffsets[Detail::AsInteger(tableId)][column];
    }

    Table const& TableCollection::GetTable(TableId const tableId) const
    {
        return _state.Get()._tables[Detail::AsInteger(tableId)];
    }

    SizeType TableCollection::GetTableIndexSize(TableId const tableId) const
    {
        return _state.Get()._rowCounts[Detail::AsInteger(tableId)] < (1 << 16) ? 2 : 4;
    }




    Database::Database(String fileName)
        : _fileName(std::move(fileName))
    {
        Detail::FileHandle file(_fileName.c_str(), Detail::FileMode::Read | Detail::FileMode::Binary);

        Private::PeSectionsAndCliHeader const peSectionsAndCliHeader(Private::ReadPeSectionsAndCliHeader(file));
        Private::PeCliStreamHeaderSequence const streamHeaders(Private::ReadPeCliStreamHeaders(file, peSectionsAndCliHeader));
        for (std::size_t i(0); i < streamHeaders.size(); ++i)
        {
            if (streamHeaders[i]._metadataOffset == 0)
                continue;

            Stream newStream(
                file,
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
                throw std::logic_error("wtf");
            }
        }
    }

    Database::Database(Database&& other)
        : _fileName(std::move(other._fileName)),
          _blobStream(std::move(other._blobStream)),
          _guidStream(std::move(other._guidStream)),
          _strings(std::move(other._strings)),
          _tables(std::move(other._tables))
    {
    }

    Database& Database::operator=(Database&& other)
    {
        Swap(other);
        return *this;
    }

    void Database::Swap(Database& other)
    {
        std::swap(_fileName,   other._fileName  );
        std::swap(_blobStream, other._blobStream);
        std::swap(_guidStream, other._guidStream);
        std::swap(_strings,    other._strings   );
        std::swap(_tables,     other._tables    );
    }




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

    RowReference ClassLayoutRow::GetParentTypeDef() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), TableId::TypeDef, GetColumnOffset(2));
    }

    std::uint8_t ConstantRow::GetType() const
    {
        return Private::ReadAs<std::uint8_t>(GetIterator(), GetColumnOffset(0));
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
        >(GetDatabase(), GetIterator(), &EventMapRow::GetLastEvent);
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

    std::uint32_t FieldLayoutRow::GetOffset() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    RowReference FieldLayoutRow::GetField() const
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

    std::uint32_t FieldRvaRow::GetRva() const
    {
        return Private::ReadAs<std::uint32_t>(GetIterator(), GetColumnOffset(0));
    }

    RowReference FieldRvaRow::GetField() const
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

    std::uint16_t GenericParamRow::GetNumber() const
    {
        return Private::ReadAs<std::uint16_t>(GetIterator(), GetColumnOffset(0));
    }

    GenericParameterFlags GenericParamRow::GetFlags() const
    {
        return Private::ReadAs<GenericParameterAttribute>(GetIterator(), GetColumnOffset(1));
    }

    RowReference GenericParamRow::GetOwner() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::TypeOrMethodDef, GetColumnOffset(2));
    }

    StringReference GenericParamRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(3));
    }

    RowReference GenericParamConstraintRow::GetOwner() const
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

    std::uint32_t ManifestResourceRow::GetOffset() const
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

    std::uint32_t MethodDefRow::GetRva() const
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
        >(GetDatabase(), GetIterator(), &MethodDefRow::GetLastParameter);
    }

    RowReference MethodImplRow::GetClass() const
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

    RowReference MethodSemanticsRow::GetAssociation() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::HasSemantics, GetColumnOffset(2));
    }

    RowReference MethodSpecRow::GetMethod() const
    {
        return Private::ReadRowReference(GetDatabase(), GetIterator(), CompositeIndex::MethodDefOrRef, GetColumnOffset(0));
    }

    BlobReference MethodSpecRow::GetInstantiation() const
    {
        return Private::ReadBlobReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
    }

    StringReference ModuleRow::GetName() const
    {
        return Private::ReadStringReference(GetDatabase(), GetIterator(), GetColumnOffset(1));
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

} }
