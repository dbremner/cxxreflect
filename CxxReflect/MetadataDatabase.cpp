//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/Platform.hpp"
#include "CxxReflect/Utility.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <vector>

namespace cdx = CxxReflect::detail;

using namespace CxxReflect;
using namespace CxxReflect::Metadata;

namespace {

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

    std::size_t ComputeOffsetFromRva(PeSectionHeader section, PeRvaAndSize rvaAndSize)
    {
        return rvaAndSize._rva - section._virtualAddress + section._rawDataOffset;
    }

    struct FourComponentVersion
    {
        std::uint16_t _major;
        std::uint16_t _minor;
        std::uint16_t _build;
        std::uint16_t _revision;
    };

    static_assert(sizeof(FourComponentVersion) == 8, "Invalid FourComponentVersion Definition");

    struct PeSectionsAndCliHeader
    {
        PeSectionHeaderSequence _sections;
        PeCliHeader             _cliHeader;
    };

    // Predicate that tests whether a PE section contains a specified RVA
    class PeSectionContainsRva
    {
    public:

        PeSectionContainsRva(std::size_t rva)
            : _rva(rva)
        {
        }

        bool operator()(PeSectionHeader const& section) const
        {
            return _rva >= section._virtualAddress
                && _rva < section._virtualAddress + section._virtualSize;
        }

    private:

        std::size_t _rva;
    };

    PeSectionsAndCliHeader ReadPeSectionsAndCliHeader(cdx::file_handle& file)
    {
        // The index of the PE Header is located at index 0x3c of the DOS header
        file.seek(0x3c, cdx::file_handle::begin);
        
        std::uint32_t fileHeaderOffset(0);
        file.read(&fileHeaderOffset, sizeof fileHeaderOffset, 1);
        file.seek(fileHeaderOffset, cdx::file_handle::begin);

        PeFileHeader fileHeader = { 0 };
        file.read(&fileHeader, sizeof fileHeader, 1);
        if (fileHeader._sectionCount == 0 || fileHeader._sectionCount > 100)
            throw ReadException("PE section count is out of range");

        PeSectionHeaderSequence sections(fileHeader._sectionCount);
        file.read(sections.data(), sizeof *sections.begin(), sections.size());

        auto cliHeaderSectionIt(std::find_if(
            sections.begin(), sections.end(),
            PeSectionContainsRva(fileHeader._cliHeaderTable._rva)));

        if (cliHeaderSectionIt == sections.end())
            throw ReadException("Failed to locate PE file section containing CLI header");

        std::size_t cliHeaderTableOffset(ComputeOffsetFromRva(
            *cliHeaderSectionIt,
            fileHeader._cliHeaderTable));

        file.seek(cliHeaderTableOffset, cdx::file_handle::begin);

        PeCliHeader cliHeader = { 0 };
        file.read(&cliHeader, sizeof cliHeader, 1);

        PeSectionsAndCliHeader result;
        result._sections = std::move(sections);
        result._cliHeader = cliHeader;
        return result;
    }

    PeCliStreamHeaderSequence ReadPeCliStreamHeaders(cdx::file_handle& file,
                                                     PeSectionsAndCliHeader const& peHeader)
    {
        auto metadataSectionIt(std::find_if(
            peHeader._sections.begin(),
            peHeader._sections.end(),
            PeSectionContainsRva(peHeader._cliHeader._metadata._rva)));

        if (metadataSectionIt == peHeader._sections.end())
            throw ReadException("Failed to locate PE file section containing CLI metadata");

        std::size_t metadataOffset(ComputeOffsetFromRva(
            *metadataSectionIt,
            peHeader._cliHeader._metadata));

        file.seek(metadataOffset, cdx::file_handle::begin);

        std::uint32_t magicSignature(0);
        file.read(&magicSignature, sizeof magicSignature, 1);
        if (magicSignature != 0x424a5342)
            throw ReadException("Magic signature does not match required value 0x424a5342");

        file.seek(8, cdx::file_handle::current);

        std::uint32_t versionLength(0);
        file.read(&versionLength, sizeof versionLength, 1);
        file.seek(versionLength + 2, cdx::file_handle::current); // Add 2 to account for unused flags

        std::uint16_t streamCount(0);
        file.read(&streamCount, sizeof streamCount, 1);

        PeCliStreamHeaderSequence streamHeaders = { 0 };
        for (std::uint16_t i(0); i < streamCount; ++i)
        {
            PeCliStreamHeader header;
            header._metadataOffset = metadataOffset;
            file.read(&header._streamOffset, sizeof header._streamOffset, 1);
            file.read(&header._streamSize,   sizeof header._streamSize,   1);

            std::array<char, 12> currentName = { 0 };
            file.read(currentName.data(), sizeof *currentName.begin(), currentName.size());

            #define CXXREFLECT_GENERATE(name, id, reset)                                      \
                if (std::strcmp(currentName.data(), name) == 0 &&                             \
                    streamHeaders[cdx::as_integer(PeCliStreamKind::id)]._metadataOffset == 0) \
                {                                                                             \
                    streamHeaders[cdx::as_integer(PeCliStreamKind::id)] = header;             \
                    file.seek(reset, cdx::file_handle::current);                              \
                    used = true;                                                              \
                }

            bool used(false);
            CXXREFLECT_GENERATE("#Strings", StringStream,      0);
            CXXREFLECT_GENERATE("#US",      UserStringStream, -8);
            CXXREFLECT_GENERATE("#Blob",    BlobStream,       -4);
            CXXREFLECT_GENERATE("#GUID",    GuidStream,       -4);
            CXXREFLECT_GENERATE("#~",       TableStream,      -8);
            if (!used)
                throw ReadException("Unknown stream name encountered");

            #undef CXXREFLECT_GENERATE
        }

        return streamHeaders;
    }

    CompositeIndexSizeArray const CompositeIndexTagSize =
    {
        2, 2, 5, 1, 2, 3, 1, 1, 1, 2, 3, 2, 1
    };

    #define CXXREFLECT_GENERATE(x, y)                                               \
        (tableSizes[cdx::as_integer(TableId::y)] <                                  \
        (1ull << (16 - CompositeIndexTagSize[cdx::as_integer(CompositeIndex::x)])))

    std::size_t ComputeTypeDefOrRefIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(TypeDefOrRef, TypeDef)
            && CXXREFLECT_GENERATE(TypeDefOrRef, TypeRef)
            && CXXREFLECT_GENERATE(TypeDefOrRef, TypeSpec) ? 2 : 4;
    }

    std::size_t ComputeHasConstantIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasConstant, Field)
            && CXXREFLECT_GENERATE(HasConstant, Param)
            && CXXREFLECT_GENERATE(HasConstant, Property) ? 2 : 4;
    }

    std::size_t ComputeHasCustomAttributeIndexSize(TableIdSizeArray const& tableSizes)
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

    std::size_t ComputeHasFieldMarshalIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasFieldMarshal, Field)
            && CXXREFLECT_GENERATE(HasFieldMarshal, Param) ? 2 : 4;
    }

    std::size_t ComputeHasDeclSecurityIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasDeclSecurity, TypeDef)
            && CXXREFLECT_GENERATE(HasDeclSecurity, MethodDef)
            && CXXREFLECT_GENERATE(HasDeclSecurity, Assembly) ? 2 : 4;
    }

    std::size_t ComputeMemberRefParentIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(MemberRefParent, TypeDef)
            && CXXREFLECT_GENERATE(MemberRefParent, TypeRef)
            && CXXREFLECT_GENERATE(MemberRefParent, ModuleRef)
            && CXXREFLECT_GENERATE(MemberRefParent, MethodDef)
            && CXXREFLECT_GENERATE(MemberRefParent, TypeSpec) ? 2 : 4;
    }

    std::size_t ComputeHasSemanticsIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(HasSemantics, Event)
            && CXXREFLECT_GENERATE(HasSemantics, Property) ? 2 : 4;
    }

    std::size_t ComputeMethodDefOrRefIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(MethodDefOrRef, MethodDef)
            && CXXREFLECT_GENERATE(MethodDefOrRef, MemberRef) ? 2 : 4;
    }

    std::size_t ComputeMemberForwardedIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(MemberForwarded, Field)
            && CXXREFLECT_GENERATE(MemberForwarded, MethodDef) ? 2 : 4;
    }

    std::size_t ComputeImplementationIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(Implementation, File)
            && CXXREFLECT_GENERATE(Implementation, AssemblyRef)
            && CXXREFLECT_GENERATE(Implementation, ExportedType) ? 2 : 4;
    }

    std::size_t ComputeCustomAttributeTypeIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(CustomAttributeType, MethodDef)
            && CXXREFLECT_GENERATE(CustomAttributeType, MemberRef) ? 2 : 4;
    }

    std::size_t ComputeResolutionScopeIndexSize(TableIdSizeArray const& tableSizes)
    {
        return CXXREFLECT_GENERATE(ResolutionScope, Module)
            && CXXREFLECT_GENERATE(ResolutionScope, ModuleRef)
            && CXXREFLECT_GENERATE(ResolutionScope, AssemblyRef)
            && CXXREFLECT_GENERATE(ResolutionScope, TypeRef) ? 2 : 4;
    }

    std::size_t ComputeTypeOrMethodDefIndexSize(TableIdSizeArray const& tableSizes)
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

    std::uint32_t ReadTableIndex(Database const&       database,
                                 ByteIterator    const data,
                                 TableId         const table,
                                 SizeType        const offset)
    {
        switch (database.GetTables().GetTableIndexSize(table))
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset);
        case 4:  return ReadAs<std::uint32_t>(data, offset);
        default: cdx::verify_fail("Invalid table index size");
        }

        return 0;
    }

    std::uint32_t ReadCompositeIndex(Database const&       database,
                                     ByteIterator    const data,
                                     CompositeIndex  const index,
                                     SizeType        const offset)
    {
        switch (database.GetTables().GetCompositeIndexSize(index))
        {
            case 2:  return ReadAs<std::uint16_t>(data, offset);
            case 4:  return ReadAs<std::uint32_t>(data, offset);
            default: cdx::verify_fail("Invalid composite index size");
        }
        
        return 0;
    }

    std::uint32_t ReadBlobHeapIndex(Database const& database, ByteIterator const data, SizeType const offset)
    {
        switch (database.GetTables().GetBlobHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset);
        case 4:  return ReadAs<std::uint32_t>(data, offset);
        default: cdx::verify_fail("Invalid blob heap index size");
        }

        return 0;
    }

    std::uint32_t ReadStringHeapIndex(Database const& database, ByteIterator const data, SizeType const offset)
    {
        switch (database.GetTables().GetStringHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, offset);
        case 4:  return ReadAs<std::uint32_t>(data, offset);
        default: cdx::verify_fail("Invalid string heap index size");
        }

        return 0;
    }

    String ReadString(Database const& database, ByteIterator const data, SizeType const offset)
    {
        return database.GetStrings().At(ReadStringHeapIndex(database, data, offset));
    }

    TableReference ReadTableReference(Database     const& database,
                                      ByteIterator const  data,
                                      TableId      const  table,
                                      SizeType     const  offset)
    {
        return TableReference(table, ReadTableIndex(database, data, table, offset));
    }

    typedef std::pair<std::uint32_t, std::uint32_t> TagIndexPair;

    TagIndexPair SplitCompositeIndex(CompositeIndex const index, std::uint32_t const value)
    {
        std::uint32_t const tagBits(CompositeIndexTagSize[cdx::as_integer(index)]);
        return std::make_pair(
            value & ((static_cast<std::uint32_t>(1u) << tagBits) - 1),
            value >> tagBits
            );
    }

    TableReference DecodeCustomAttributeTypeIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::CustomAttributeType, value));
        switch (split.first)
        {
        case 2:  return TableReference(TableId::MethodDef, split.second);
        case 3:  return TableReference(TableId::MemberRef, split.second);
        default: throw ReadException("Invalid CustomAttributeType composite index value encountered");
        }
    }

    TableReference DecodeHasConstantIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasConstant, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::Field,    split.second);
        case 1:  return TableReference(TableId::Param,    split.second);
        case 2:  return TableReference(TableId::Property, split.second);
        default: throw ReadException("Invalid HasConstant composite index value encountered");
        }
    }

    TableReference DecodeHasCustomAttributeIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasCustomAttribute, value));
        switch (split.first)
        {
        case  0: return TableReference(TableId::MethodDef,              split.second);
        case  1: return TableReference(TableId::Field,                  split.second);
        case  2: return TableReference(TableId::TypeRef,                split.second);
        case  3: return TableReference(TableId::TypeDef,                split.second);
        case  4: return TableReference(TableId::Param,                  split.second);
        case  5: return TableReference(TableId::InterfaceImpl,          split.second);
        case  6: return TableReference(TableId::MemberRef,              split.second);
        case  7: return TableReference(TableId::Module,                 split.second);
        // case  8: return TableReference(TableId::Permission,          split.second); // TODO WHAT IS THIS?
        case  9: return TableReference(TableId::Property,               split.second);
        case 10: return TableReference(TableId::Event,                  split.second);
        case 11: return TableReference(TableId::StandaloneSig,          split.second);
        case 12: return TableReference(TableId::ModuleRef,              split.second);
        case 13: return TableReference(TableId::TypeSpec,               split.second);
        case 14: return TableReference(TableId::Assembly,               split.second);
        case 15: return TableReference(TableId::AssemblyRef,            split.second);
        case 16: return TableReference(TableId::File,                   split.second);
        case 17: return TableReference(TableId::ExportedType,           split.second);
        case 18: return TableReference(TableId::ManifestResource,       split.second);
        case 19: return TableReference(TableId::GenericParam,           split.second);
        case 20: return TableReference(TableId::GenericParamConstraint, split.second);
        case 21: return TableReference(TableId::MethodSpec,             split.second);
        default: throw ReadException("Invalid HasCustomAttribute composite index value encountered");
        }
    }

    TableReference DecodeHasDeclSecurityIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasFieldMarshal, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::TypeDef,   split.second);
        case 1:  return TableReference(TableId::MethodDef, split.second);
        case 2:  return TableReference(TableId::Assembly,  split.second);
        default: throw ReadException("Invalid HasFieldMarshal composite index value encountered");
        }
    }

    TableReference DecodeHasFieldMarshalIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasFieldMarshal, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::Field, split.second);
        case 1:  return TableReference(TableId::Param, split.second);
        default: cdx::verify_fail("Too many bits!"); return TableReference();
        }
    }

    TableReference DecodeHasSemanticsIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::HasSemantics, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::Event,    split.second);
        case 1:  return TableReference(TableId::Property, split.second);
        default: cdx::verify_fail("Too many bits!"); return TableReference();
        }
    }

    TableReference DecodeImplementationIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::Implementation, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::File,         split.second);
        case 1:  return TableReference(TableId::AssemblyRef,  split.second);
        case 2:  return TableReference(TableId::ExportedType, split.second);
        default: throw ReadException("Invalid Implementation composite index value encountered");
        }
    }

    TableReference DecodeMemberForwardedIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::MemberForwarded, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::Field,     split.second);
        case 1:  return TableReference(TableId::MethodDef, split.second);
        default: cdx::verify_fail("Too many bits!"); return TableReference();
        }
    }

    TableReference DecodeMemberRefParentIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::MemberRefParent, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::TypeDef,   split.second);
        case 1:  return TableReference(TableId::TypeRef,   split.second);
        case 2:  return TableReference(TableId::ModuleRef, split.second);
        case 3:  return TableReference(TableId::MethodDef, split.second);
        case 4:  return TableReference(TableId::TypeSpec,  split.second);
        default: throw ReadException("Invalid MemberRefParent composite index value encountered");
        }
    }

    TableReference DecodeMethodDefOrRefIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::MethodDefOrRef, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::MethodDef, split.second);
        case 1:  return TableReference(TableId::MemberRef, split.second);
        default: cdx::verify_fail("Too many bits!"); return TableReference();
        }
    }

    TableReference DecodeResolutionScopeIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::ResolutionScope, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::Module,      split.second);
        case 1:  return TableReference(TableId::ModuleRef,   split.second);
        case 2:  return TableReference(TableId::AssemblyRef, split.second);
        case 3:  return TableReference(TableId::TypeRef,     split.second);
        default: cdx::verify_fail("Too many bits!"); return TableReference();
        }
    }

    TableReference DecodeTypeDefOrRefIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::TypeDefOrRef, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::TypeDef,  split.second);
        case 1:  return TableReference(TableId::TypeRef,  split.second);
        case 2:  return TableReference(TableId::TypeSpec, split.second);
        default: throw ReadException("Invalid TypeDefOrRef composite index value encountered");
        }
    }

    TableReference DecodeTypeOrMethodDefIndex(std::uint32_t const value)
    {
        TagIndexPair const split(SplitCompositeIndex(CompositeIndex::TypeOrMethodDef, value));
        switch (split.first)
        {
        case 0:  return TableReference(TableId::TypeDef,   split.second);
        case 1:  return TableReference(TableId::MethodDef, split.second);
        default: cdx::verify_fail("Too many bits!"); return TableReference();
        }
    }

    TableReference ReadTableReference(Database const&       database,
                                      ByteIterator    const data,
                                      CompositeIndex  const index,
                                      SizeType        const offset)
    {
        std::uint32_t const value(ReadCompositeIndex(database, data, index, offset));
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
        default:  cdx::verify_fail("Invalid index");     return TableReference();
        }
    }

    template <TableId TSourceId, TableId TTargetId, typename TFirstFunction>
    TableReference ComputeLastTableReference(Database       const& database,
                                             ByteIterator   const  data,
                                             TFirstFunction const  first)
    {
        SizeType const byteOffset(data - database.GetTables().GetTable(TSourceId).Begin());
        SizeType const rowSize(database.GetTables().GetTable(TSourceId).GetRowSize());
        SizeType const logicalIndex(byteOffset / rowSize);

        SizeType const targetTableRowCount(database.GetTables().GetTable(TSourceId).GetRowCount());
        if (logicalIndex == targetTableRowCount)
        {
            return TableReference(TTargetId, targetTableRowCount);
        }
        else
        {
            return (database.GetRow<TSourceId>(logicalIndex + 1).*first)();
        }
    }
}

namespace CxxReflect { namespace Metadata {

    String StringCollection::At(SizeType const index) const
    {
        auto const existingIt(_index.find(index));
        if (existingIt != _index.end())
            return existingIt->second;

        char const* pointer(_stream.ReinterpretAs<char>(index));
        int const required(Platform::ComputeUtf16LengthOfUtf8String(pointer));

        auto const range(_buffer.allocate(required));
        if (!Platform::ConvertUtf8ToUtf16(pointer, range.begin(), required))
            throw std::logic_error("wtf");

        return _index.insert(std::make_pair(index, String(range.begin(), range.end()))).first->second;
    }

    Stream::Stream(cdx::file_handle& file,
                   SizeType const metadataOffset,
                   SizeType const streamOffset,
                   SizeType const streamSize)
        : _size(streamSize)
    {
        _data.reset(new Byte[streamSize]);
        file.seek(metadataOffset + streamOffset, cdx::file_handle::begin);
        file.read(_data.get(), streamSize, 1);
    }

    TableCollection::TableCollection(Stream&& stream)
        : _stream(std::move(stream)),
          _initialized(true),
          _state()
    {
        std::bitset<8> heapSizes(_stream.ReadAs<std::uint8_t>(6));
        _state._stringHeapIndexSize = heapSizes.test(0) ? 4 : 2;
        _state._guidHeapIndexSize   = heapSizes.test(1) ? 4 : 2;
        _state._blobHeapIndexSize   = heapSizes.test(2) ? 4 : 2;

        _state._validBits  = _stream.ReadAs<std::uint64_t>(8);
        _state._sortedBits = _stream.ReadAs<std::uint64_t>(16);

        SizeType index(24);
        for (unsigned x(0); x < 64; ++x)
        {
            if (!_state._validBits.test(x))
                continue;

            if (!IsValidTableId(x))
                throw ReadException("Metadata table presence vector has invalid bits set");

            _state._rowCounts[x] = _stream.ReadAs<std::uint32_t>(index);
            index += 4;
        }

        ComputeCompositeIndexSizes();
        ComputeTableRowSizes();

        for (unsigned x(0); x < 64; ++x)
        {
            if (!_state._validBits.test(x) || _state._rowCounts[x] == 0)
                continue;

            _state._tables[x] = Table(_stream.At(index),
                                      _state._rowSizes[x],
                                      _state._rowCounts[x],
                                      _state._sortedBits.test(x));
            index += _state._rowSizes[x] * _state._rowCounts[x];
        }
    }

    void TableCollection::ComputeCompositeIndexSizes()
    {
        #define CXXREFLECT_GENERATE(x)                                        \
            _state._compositeIndexSizes[cdx::as_integer(CompositeIndex::x)] = \
                Compute ## x ## IndexSize(_state._rowCounts)
        
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
        #define CXXREFLECT_GENERATE(x, n, o)                              \
            _state._columnOffsets[cdx::as_integer(TableId::x)][n] =       \
            _state._columnOffsets[cdx::as_integer(TableId::x)][n - 1] + o

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
        std::transform(_state._columnOffsets.begin(),
                       _state._columnOffsets.end(),
                       _state._rowSizes.begin(),
                       [](ColumnOffsetSequence const& x) -> SizeType
        {
            auto const it(std::find_if(x.rbegin(), x.rend(), [](SizeType n) { return n != 0; }));
            return it != x.rend() ? *it : 0;
        });
    }

    SizeType TableCollection::GetCompositeIndexSize(CompositeIndex const index) const
    {
        return _state._compositeIndexSizes[cdx::as_integer(index)];
    }

    SizeType TableCollection::GetTableColumnOffset(TableId const table, SizeType const column) const
    {
        cdx::verify([&] { return column < MaximumColumnCount; }, "Invalid column identifier");
        // TODO Check table-specific offset
        return _state._columnOffsets[cdx::as_integer(table)][column];
    }

    Table const& TableCollection::GetTable(TableId const id) const
    {
        return _state._tables[cdx::as_integer(id)];
    }

    SizeType TableCollection::GetTableIndexSize(TableId const id) const
    {
        return _state._rowCounts[cdx::as_integer(id)] < (1 << 16) ? 2 : 4;
    }

    Database::Database(wchar_t const* const fileName)
        : _fileName(fileName)
    {
        cdx::file_handle file(fileName);

        PeSectionsAndCliHeader const peSectionsAndCliHeader(ReadPeSectionsAndCliHeader(file));
        PeCliStreamHeaderSequence const streamHeaders(ReadPeCliStreamHeaders(file, peSectionsAndCliHeader));
        for (std::size_t i(0); i < streamHeaders.size(); ++i)
        {
            if (streamHeaders[i]._metadataOffset == 0)
                continue;

            Stream newStream(
                file,
                streamHeaders[i]._metadataOffset,
                streamHeaders[i]._streamOffset,
                streamHeaders[i]._streamSize);

            switch (static_cast<PeCliStreamKind>(i))
            {
            case PeCliStreamKind::StringStream:
                _strings = StringCollection(std::move(newStream));
                break;

            case PeCliStreamKind::UserStringStream:
                // We do not use the userstrings stream for metadata
                break;

            case PeCliStreamKind::BlobStream:
                _blobStream = std::move(newStream);
                break;

            case PeCliStreamKind::GuidStream:
                _guidStream = std::move(newStream);
                break;

            case PeCliStreamKind::TableStream:
                _tables = TableCollection(std::move(newStream));
                break;

            default:
                throw std::logic_error("wtf");
            }
        }
    }

    AssemblyHashAlgorithm AssemblyRow::GetHashAlgorithm() const
    {
        return ReadAs<AssemblyHashAlgorithm>(_data, GetColumnOffset(0));
    }

    Version AssemblyRow::GetVersion() const
    {
        FourComponentVersion const version(ReadAs<FourComponentVersion>(_data, GetColumnOffset(1)));
        return Version(version._major, version._minor, version._build, version._revision);
    }

    AssemblyFlags AssemblyRow::GetFlags() const
    {
        return ReadAs<AssemblyAttribute>(_data, GetColumnOffset(2));
    }

    BlobIndex AssemblyRow::GetPublicKey() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(3));
    }

    String AssemblyRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(4));
    }

    String AssemblyRow::GetCulture() const
    {
        return ReadString(*_database, _data, GetColumnOffset(5));
    }

    std::uint32_t AssemblyOsRow::GetOsPlatformId()   const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(0));
    }

    std::uint32_t AssemblyOsRow::GetOsMajorVersion() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(1));
    }

    std::uint32_t AssemblyOsRow::GetOsMinorVersion() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(2));
    }

    std::uint32_t AssemblyProcessorRow::GetProcessor() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(0));
    }

    Version AssemblyRefRow::GetVersion() const
    {
        FourComponentVersion const version(ReadAs<FourComponentVersion>(_data, GetColumnOffset(0)));
        return Version(version._major, version._minor, version._build, version._revision);
    }

    AssemblyFlags AssemblyRefRow::GetFlags() const
    {
        return ReadAs<AssemblyAttribute>(_data, GetColumnOffset(1));
    }

    BlobIndex AssemblyRefRow::GetPublicKeyOrToken() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(2));
    }

    String AssemblyRefRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(3));
    }

    String AssemblyRefRow::GetCulture() const
    {
        return ReadString(*_database, _data, GetColumnOffset(4));
    }

    BlobIndex AssemblyRefRow::GetHashValue() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(5));
    }

    std::uint32_t AssemblyRefOsRow::GetOsPlatformId()   const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(0));
    }

    std::uint32_t AssemblyRefOsRow::GetOsMajorVersion() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(1));
    }

    std::uint32_t AssemblyRefOsRow::GetOsMinorVersion() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(2));
    }

    TableReference AssemblyRefOsRow::GetAssemblyRef() const
    {
        return ReadTableReference(*_database, _data, TableId::AssemblyRef, GetColumnOffset(3));
    }

    std::uint32_t AssemblyRefProcessorRow::GetProcessor() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(0));
    }

    TableReference AssemblyRefProcessorRow::GetAssemblyRef() const
    {
        return ReadTableReference(*_database, _data, TableId::AssemblyRef, GetColumnOffset(1));
    }

    std::uint16_t ClassLayoutRow::GetPackingSize() const
    {
        return ReadAs<std::uint16_t>(_data, GetColumnOffset(0));
    }

    std::uint32_t ClassLayoutRow::GetClassSize() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(1));
    }

    TableReference ClassLayoutRow::GetParentTypeDef() const
    {
        return ReadTableReference(*_database, _data, TableId::TypeDef, GetColumnOffset(2));
    }

    std::uint8_t ConstantRow::GetType() const
    {
        return ReadAs<std::uint8_t>(_data, GetColumnOffset(0));
    }

    TableReference ConstantRow::GetParent() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::HasConstant, GetColumnOffset(1));
    }

    BlobIndex ConstantRow::GetValue() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(2));
    }

    TableReference CustomAttributeRow::GetParent() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::HasCustomAttribute, GetColumnOffset(0));
    }

    TableReference CustomAttributeRow::GetType() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::CustomAttributeType, GetColumnOffset(1));
    }

    BlobIndex CustomAttributeRow::GetValue() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(2));
    }

    std::uint16_t DeclSecurityRow::GetAction() const
    {
        return ReadAs<std::uint16_t>(_data, GetColumnOffset(0));
    }

    TableReference DeclSecurityRow::GetParent() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::HasDeclSecurity, GetColumnOffset(1));
    }

    BlobIndex DeclSecurityRow::GetPermissionSet() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(2));
    }

    TableReference EventMapRow::GetParent() const
    {
        return ReadTableReference(*_database, _data, TableId::TypeDef, GetColumnOffset(0));
    }

    TableReference EventMapRow::GetFirstEvent() const
    {
        return ReadTableReference(*_database, _data, TableId::Event, GetColumnOffset(1));
    }

    TableReference EventMapRow::GetLastEvent() const
    {
        return ComputeLastTableReference<
            TableId::EventMap,
            TableId::Event
        >(*_database, _data, &EventMapRow::GetLastEvent);
    }

    EventFlags EventRow::GetFlags() const
    {
        return ReadAs<EventAttribute>(_data, GetColumnOffset(0));
    }

    String EventRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(1));
    }

    TableReference EventRow::GetType() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::TypeDefOrRef, GetColumnOffset(2));
    }

    TypeFlags ExportedTypeRow::GetFlags() const
    {
        return ReadAs<TypeAttribute>(_data, GetColumnOffset(0));
    }

    std::uint32_t ExportedTypeRow::GetTypeDefId() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(1));
    }

    String ExportedTypeRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(2));
    }

    String ExportedTypeRow::GetNamespace() const
    {
        return ReadString(*_database, _data, GetColumnOffset(3));
    }

    TableReference ExportedTypeRow::GetImplementation() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::Implementation, GetColumnOffset(4));
    }

    FieldFlags FieldRow::GetFlags() const
    {
        return ReadAs<FieldAttribute>(_data, GetColumnOffset(0));
    }

    String FieldRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(1));
    }

    BlobIndex FieldRow::GetSignature() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(2));
    }

    std::uint32_t FieldLayoutRow::GetOffset() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(0));
    }

    TableReference FieldLayoutRow::GetField() const
    {
        return ReadTableReference(*_database, _data, TableId::Field, GetColumnOffset(1));
    }

    TableReference FieldMarshalRow::GetParent() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::HasFieldMarshal, GetColumnOffset(0));
    }

    BlobIndex FieldMarshalRow::GetNativeType() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(1));
    }

    std::uint32_t FieldRvaRow::GetRva() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(0));
    }

    TableReference FieldRvaRow::GetField() const
    {
        return ReadTableReference(*_database, _data, TableId::Field, GetColumnOffset(1));
    }

    FileFlags FileRow::GetFlags() const
    {
        return ReadAs<FileAttribute>(_data, GetColumnOffset(0));
    }

    String FileRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(1));
    }

    BlobIndex FileRow::GetHashValue() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(2));
    }

    std::uint16_t GenericParamRow::GetNumber() const
    {
        return ReadAs<std::uint16_t>(_data, GetColumnOffset(0));
    }

    GenericParameterFlags GenericParamRow::GetFlags() const
    {
        return ReadAs<GenericParameterAttribute>(_data, GetColumnOffset(1));
    }

    TableReference GenericParamRow::GetOwner() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::TypeOrMethodDef, GetColumnOffset(2));
    }

    String GenericParamRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(3));
    }

    TableReference GenericParamConstraintRow::GetOwner() const
    {
        return ReadTableReference(*_database, _data, TableId::GenericParam, GetColumnOffset(0));
    }

    TableReference GenericParamConstraintRow::GetConstraint() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::TypeDefOrRef, GetColumnOffset(1));
    }

    PInvokeFlags ImplMapRow::GetMappingFlags() const
    {
        return ReadAs<PInvokeAttribute>(_data, GetColumnOffset(0));
    }

    TableReference ImplMapRow::GetMemberForwarded() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::MemberForwarded, GetColumnOffset(1));
    }

    String ImplMapRow::GetImportName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(2));
    }

    TableReference ImplMapRow::GetImportScope() const
    {
        return ReadTableReference(*_database, _data, TableId::ModuleRef, GetColumnOffset(3));
    }

    TableReference InterfaceImplRow::GetClass() const
    {
        return ReadTableReference(*_database, _data, TableId::TypeDef, GetColumnOffset(0));
    }

    TableReference InterfaceImplRow::GetInterface() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::TypeDefOrRef, GetColumnOffset(1));
    }

    std::uint32_t ManifestResourceRow::GetOffset() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(0));
    }

    ManifestResourceFlags ManifestResourceRow::GetFlags() const
    {
        return ReadAs<ManifestResourceAttribute>(_data, GetColumnOffset(1));
    }

    String ManifestResourceRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(2));
    }

    TableReference ManifestResourceRow::GetImplementation() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::Implementation, GetColumnOffset(3));
    }

    TableReference MemberRefRow::GetClass() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::MemberRefParent, GetColumnOffset(0));
    }

    String MemberRefRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(1));
    }

    BlobIndex MemberRefRow::GetSignature() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(2));
    }

    std::uint32_t MethodDefRow::GetRva() const
    {
        return ReadAs<std::uint32_t>(_data, GetColumnOffset(0));
    }

    MethodImplementationFlags MethodDefRow::GetImplementationFlags() const
    {
        return ReadAs<MethodImplementationAttribute>(_data, GetColumnOffset(1));
    }

    MethodFlags MethodDefRow::GetFlags() const
    {
        return ReadAs<MethodAttribute>(_data, GetColumnOffset(2));
    }

    String MethodDefRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(3));
    }

    BlobIndex MethodDefRow::GetSignature() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(4));
    }

    TableReference MethodDefRow::GetFirstParameter() const
    {
        return ReadTableReference(*_database, _data, TableId::Param, GetColumnOffset(5));
    }

    TableReference MethodDefRow::GetLastParameter() const
    {
        return ComputeLastTableReference<
            TableId::MethodDef,
            TableId::Param
        >(*_database, _data, &MethodDefRow::GetLastParameter);
    }

    TableReference MethodImplRow::GetClass() const
    {
        return ReadTableReference(*_database, _data, TableId::TypeDef, GetColumnOffset(0));
    }

    TableReference MethodImplRow::GetMethodBody() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::MethodDefOrRef, GetColumnOffset(1));
    }

    TableReference MethodImplRow::GetMethodDeclaration() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::MethodDefOrRef, GetColumnOffset(2));
    }

    MethodSemanticsFlags MethodSemanticsRow::GetSemantics() const
    {
        return ReadAs<MethodSemanticsAttribute>(_data, GetColumnOffset(0));
    }

    TableReference MethodSemanticsRow::GetMethod() const
    {
        return ReadTableReference(*_database, _data, TableId::MethodDef, GetColumnOffset(1));
    }

    TableReference MethodSemanticsRow::GetAssociation() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::HasSemantics, GetColumnOffset(2));
    }

    TableReference MethodSpecRow::GetMethod() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::MethodDefOrRef, GetColumnOffset(0));
    }

    BlobIndex MethodSpecRow::GetInstantiation() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(1));
    }

    String ModuleRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(1));
    }

    String ModuleRefRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(0));
    }

    TableReference NestedClassRow::GetNestedClass() const
    {
        return ReadTableReference(*_database, _data, TableId::TypeDef, GetColumnOffset(0));
    }

    TableReference NestedClassRow::GetEnclosingClass() const
    {
        return ReadTableReference(*_database, _data, TableId::TypeDef, GetColumnOffset(1));
    }

    ParameterFlags ParamRow::GetFlags() const
    {
        return ReadAs<ParameterAttribute>(_data, GetColumnOffset(0));
    }

    std::uint16_t ParamRow::GetSequence() const
    {
        return ReadAs<std::uint16_t>(_data, GetColumnOffset(1));
    }

    String ParamRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(2));
    }

    PropertyFlags PropertyRow::GetFlags() const
    {
        return ReadAs<PropertyAttribute>(_data, GetColumnOffset(0));
    }

    String PropertyRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(1));
    }

    BlobIndex PropertyRow::GetSignature() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(2));
    }

    TableReference PropertyMapRow::GetParent() const
    {
        return ReadTableReference(*_database, _data, TableId::TypeDef, GetColumnOffset(0));
    }

    TableReference PropertyMapRow::GetFirstProperty() const
    {
        return ReadTableReference(*_database, _data, TableId::Property, GetColumnOffset(1));
    }

    TableReference PropertyMapRow::GetLastProperty() const
    {
        return ComputeLastTableReference<
            TableId::PropertyMap,
            TableId::Property
        >(*_database, _data, &PropertyMapRow::GetFirstProperty);
    }

    BlobIndex StandaloneSigRow::GetSignature() const
    {
        return ReadBlobHeapIndex(*_database, _data, GetColumnOffset(0));
    }

    TypeFlags TypeDefRow::GetFlags() const
    {
        return ReadAs<TypeAttribute>(_data, GetColumnOffset(0));
    }

    String TypeDefRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(1));
    }

    String TypeDefRow::GetNamespace() const
    {
        return ReadString(*_database, _data, GetColumnOffset(2));
    }

    TableReference TypeDefRow::GetExtends() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::TypeDefOrRef, GetColumnOffset(3));
    }

    TableReference TypeDefRow::GetFirstField() const
    {
        return ReadTableReference(*_database, _data, TableId::Field, GetColumnOffset(4));
    }

    TableReference TypeDefRow::GetLastField() const
    {
        return ComputeLastTableReference<
            TableId::TypeDef,
            TableId::Field
        >(*_database, _data, &TypeDefRow::GetFirstField);
    }

    TableReference TypeDefRow::GetFirstMethod() const
    {
        return ReadTableReference(*_database, _data, TableId::MethodDef, GetColumnOffset(5));
    }

    TableReference TypeDefRow::GetLastMethod() const
    {
        return ComputeLastTableReference<
            TableId::TypeDef,
            TableId::MethodDef
        >(*_database, _data, &TypeDefRow::GetFirstMethod);
    }

    TableReference TypeRefRow::GetResolutionScope() const
    {
        return ReadTableReference(*_database, _data, CompositeIndex::ResolutionScope, GetColumnOffset(0));
    }

    String TypeRefRow::GetName() const
    {
        return ReadString(*_database, _data, GetColumnOffset(1));
    }

    String TypeRefRow::GetNamespace() const
    {
        return ReadString(*_database, _data, GetColumnOffset(2));
    }

    std::uint32_t TypeSpecRow::GetSignature() const
    {
        return ReadBlobHeapIndex(*_database, _data, 0);
    }

} }
