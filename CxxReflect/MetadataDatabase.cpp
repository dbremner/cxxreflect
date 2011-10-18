//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/Utility.hpp"

#define _X86_ // TODO WHY IS THIS NEEDED? :'(
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
#include <winnls.h>

using CxxReflect::Utility::AsInteger;
using CxxReflect::Utility::DebugFail;
using CxxReflect::Utility::DebugVerify;
using CxxReflect::Utility::EnhancedCString;
using CxxReflect::Utility::FileHandle;

using CxxReflect::Metadata::Database;
using CxxReflect::Metadata::ReadException;
using CxxReflect::Metadata::TableId;
using CxxReflect::Metadata::TableIdSizeArray;

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

    struct PeSectionsAndCliHeader
    {
        PeSectionHeaderSequence _sections;
        PeCliHeader             _cliHeader;
    };

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

    PeSectionsAndCliHeader ReadPeSectionsAndCliHeader(FileHandle& file)
    {
        // The index of the PE Header is located at index 0x3c of the DOS header
        file.Seek(0x3c, FileHandle::Begin);
        
        std::uint32_t fileHeaderOffset(0);
        file.Read(&fileHeaderOffset, sizeof fileHeaderOffset, 1);
        file.Seek(fileHeaderOffset, FileHandle::Begin);

        PeFileHeader fileHeader = { 0 };
        file.Read(&fileHeader, sizeof fileHeader, 1);
        if (fileHeader._sectionCount == 0 || fileHeader._sectionCount > 100)
            throw ReadException("PE section count is out of range");

        PeSectionHeaderSequence sections(fileHeader._sectionCount);
        file.Read(sections.data(), sizeof *sections.begin(), sections.size());

        auto cliHeaderSectionIt(std::find_if(
            sections.begin(), sections.end(),
            PeSectionContainsRva(fileHeader._cliHeaderTable._rva)));

        if (cliHeaderSectionIt == sections.end())
            throw ReadException("Failed to locate PE file section containing CLI header");

        std::size_t cliHeaderTableOffset(ComputeOffsetFromRva(
            *cliHeaderSectionIt,
            fileHeader._cliHeaderTable));

        file.Seek(cliHeaderTableOffset, FileHandle::Begin);

        PeCliHeader cliHeader = { 0 };
        file.Read(&cliHeader, sizeof cliHeader, 1);

        PeSectionsAndCliHeader result;
        result._sections = std::move(sections);
        result._cliHeader = cliHeader;
        return result;
    }

    PeCliStreamHeaderSequence ReadPeCliStreamHeaders(FileHandle& file,
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

        file.Seek(metadataOffset, FileHandle::Begin);

        std::uint32_t magicSignature(0);
        file.Read(&magicSignature, sizeof magicSignature, 1);
        if (magicSignature != 0x424a5342)
            throw ReadException("Magic signature does not match required value 0x424a5342");

        file.Seek(8, FileHandle::Current);

        std::uint32_t versionLength(0);
        file.Read(&versionLength, sizeof versionLength, 1);
        file.Seek(versionLength + 2, FileHandle::Current); // Add 2 to account for unused flags

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

            #define CXXREFLECT_GENERATE(name, id, reset)                                \
                if (std::strcmp(currentName.data(), name) == 0 &&                       \
                    streamHeaders[AsInteger(PeCliStreamKind::id)]._metadataOffset == 0) \
                {                                                                       \
                    streamHeaders[AsInteger(PeCliStreamKind::id)] = header;             \
                    file.Seek(reset, FileHandle::Current);                              \
                    used = true;                                                        \
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

    #define CXXREFLECT_GENERATE(x) (tableSizes[AsInteger(TableId::x)] < (1 << (16 - tagBits)))

    std::size_t ComputeTypeDefOrRefIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(2);
        return CXXREFLECT_GENERATE(TypeDef)
            && CXXREFLECT_GENERATE(TypeRef)
            && CXXREFLECT_GENERATE(TypeSpec) ? 2 : 4;
    }

    std::size_t ComputeHasConstantIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(2);
        return CXXREFLECT_GENERATE(Field)
            && CXXREFLECT_GENERATE(Param)
            && CXXREFLECT_GENERATE(Property) ? 2 : 4;
    }

    std::size_t ComputeHasCustomAttributeIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(5);
        return CXXREFLECT_GENERATE(MethodDef)
            && CXXREFLECT_GENERATE(Field)
            && CXXREFLECT_GENERATE(TypeRef)
            && CXXREFLECT_GENERATE(TypeDef)
            && CXXREFLECT_GENERATE(Param)
            && CXXREFLECT_GENERATE(InterfaceImpl)
            && CXXREFLECT_GENERATE(MemberRef)
            && CXXREFLECT_GENERATE(Module)
            && CXXREFLECT_GENERATE(Property)
            && CXXREFLECT_GENERATE(Event)
            && CXXREFLECT_GENERATE(StandaloneSig)
            && CXXREFLECT_GENERATE(ModuleRef)
            && CXXREFLECT_GENERATE(TypeSpec)
            && CXXREFLECT_GENERATE(Assembly)
            && CXXREFLECT_GENERATE(AssemblyRef)
            && CXXREFLECT_GENERATE(File)
            && CXXREFLECT_GENERATE(ExportedType)
            && CXXREFLECT_GENERATE(ManifestResource)
            && CXXREFLECT_GENERATE(GenericParam)
            && CXXREFLECT_GENERATE(GenericParamConstraint)
            && CXXREFLECT_GENERATE(MethodSpec) ? 2 : 4;
    }

    std::size_t ComputeHasFieldMarshalIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(1);
        return CXXREFLECT_GENERATE(Field) && CXXREFLECT_GENERATE(Param) ? 2 : 4;
    }

    std::size_t ComputeHasDeclSecurityIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(2);
        return CXXREFLECT_GENERATE(TypeDef)
            && CXXREFLECT_GENERATE(MethodDef)
            && CXXREFLECT_GENERATE(Assembly) ? 2 : 4;
    }

    std::size_t ComputeMemberRefParentIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(3);
        return CXXREFLECT_GENERATE(TypeDef)
            && CXXREFLECT_GENERATE(TypeRef)
            && CXXREFLECT_GENERATE(ModuleRef)
            && CXXREFLECT_GENERATE(MethodDef)
            && CXXREFLECT_GENERATE(TypeSpec) ? 2 : 4;
    }

    std::size_t ComputeHasSemanticsIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(1);
        return CXXREFLECT_GENERATE(Event) && CXXREFLECT_GENERATE(Property) ? 2 : 4;
    }

    std::size_t ComputeMethodDefOrRefIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(1);
        return CXXREFLECT_GENERATE(MethodDef) && CXXREFLECT_GENERATE(MemberRef) ? 2 : 4;
    }

    std::size_t ComputeMemberForwardedIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(1);
        return CXXREFLECT_GENERATE(Field) && CXXREFLECT_GENERATE(MethodDef) ? 2 : 4;
    }

    std::size_t ComputeImplementationIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(2);
        return CXXREFLECT_GENERATE(File)
            && CXXREFLECT_GENERATE(AssemblyRef)
            && CXXREFLECT_GENERATE(ExportedType) ? 2 : 4;
    }

    std::size_t ComputeCustomAttributeTypeIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(3);
        return CXXREFLECT_GENERATE(MethodDef) && CXXREFLECT_GENERATE(MemberRef) ? 2 : 4;
    }

    std::size_t ComputeResolutionScopeIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(2);
        return CXXREFLECT_GENERATE(Module)
            && CXXREFLECT_GENERATE(ModuleRef)
            && CXXREFLECT_GENERATE(AssemblyRef)
            && CXXREFLECT_GENERATE(TypeRef) ? 2 : 4;
    }

    std::size_t ComputeTypeOrMethodDefIndexSize(TableIdSizeArray const& tableSizes)
    {
        std::uint32_t const tagBits(1);
        return CXXREFLECT_GENERATE(TypeDef) && CXXREFLECT_GENERATE(MethodDef) ? 2 : 4;
    }

    #undef CXXREFLECT_GENERATE

    template <typename T>
    T const& ReadAs(std::uint8_t const* const data, std::size_t const index)
    {
        return *reinterpret_cast<T const*>(data + index);
    }

    std::uint32_t ReadTableIndex(Database const*     const database,
                                 std::uint8_t const* const data,
                                 TableId             const table,
                                 std::size_t         const index = 0)
    {
        switch (database->GetTables().GetTableIndexSize(table))
        {
        case 2:  return ReadAs<std::uint16_t>(data, index);
        case 4:  return ReadAs<std::uint32_t>(data, index);
        default: DebugFail("Invalid table index size");
        }

        return 0;
    }

    std::uint32_t ReadBlobHeapIndex(Database const*     const database,
                                    std::uint8_t const* const data,
                                    std::size_t         const index = 0)
    {
        switch (database->GetTables().GetBlobHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, index);
        case 4:  return ReadAs<std::uint32_t>(data, index);
        default: DebugFail("Invalid blob heap index size");
        }

        return 0;
    }

    std::uint32_t ReadStringHeapIndex(Database const*     const database,
                                      std::uint8_t const* const data,
                                      std::size_t         const index = 0)
    {
        switch (database->GetTables().GetStringHeapIndexSize())
        {
        case 2:  return ReadAs<std::uint16_t>(data, index);
        case 4:  return ReadAs<std::uint32_t>(data, index);
        default: DebugFail("Invalid string heap index size");
        }

        return 0;
    }
}

namespace CxxReflect { namespace Metadata {

    String StringCollection::At(SizeType const index) const
    {
        auto const existingIt(_index.find(index));
        if (existingIt != _index.end())
            return existingIt->second;

        char const* pointer(_stream.ReinterpretAs<char>(index));
        int const required(MultiByteToWideChar(CP_UTF8, 0, pointer, -1, nullptr, 0));

        auto const range(_buffer.Allocate(required));
        int const actual(MultiByteToWideChar(CP_UTF8, 0, pointer, -1, range.Begin(), required));
        if (actual != required)
            throw std::logic_error("wtf");

        return _index.insert(std::make_pair(index, String(range.Begin(), range.End()))).first->second;
    }

    Stream::Stream(FileHandle& file, SizeType metadataOffset, SizeType streamOffset, SizeType streamSize)
        : _size(streamSize)
    {
        _data.reset(new Byte[streamSize]);
        file.Seek(metadataOffset + streamOffset, FileHandle::Begin);
        file.Read(_data.get(), streamSize, 1);
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
        #define CXXREFLECT_GENERATE(x)                                     \
            _state._compositeIndexSizes[AsInteger(CompositeIndex::x)] =    \
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
        // These macros make the computations much easier to read and verify.  TI, CI, and HI get the
        // size of a table index, a composite index, or a heap, respectively.  If you pick the wrong
        // one, a compiler error will result.  When reading the list, it's easiest just to think of
        // them all as "an index".
        #define RS(x) _state._rowSizes[AsInteger(TableId::x)]
        #define TI(x) GetTableIndexSize(TableId::x)
        #define CI(x) GetCompositeIndexSize(CompositeIndex::x)
        #define HI(x) _state._ ## x ## HeapIndexSize

        RS(Assembly)               = 16 + HI(blob) + (2 * HI(string));
        RS(AssemblyOs)             = 12;
        RS(AssemblyProcessor)      = 4;
        RS(AssemblyRef)            = 12 + (2 * HI(blob)) + (2 * HI(string));
        RS(AssemblyRefOs)          = 12 + TI(AssemblyRef);
        RS(AssemblyRefProcessor)   = 4 + TI(AssemblyRef);
        RS(ClassLayout)            = 6 + TI(TypeDef);
        RS(Constant)               = 2 + HI(blob) + CI(HasConstant);
        RS(CustomAttribute)        = CI(HasCustomAttribute) + CI(CustomAttributeType);
        RS(DeclSecurity)           = 2 + HI(blob) + CI(HasDeclSecurity);
        RS(EventMap)               = TI(TypeDef) + TI(TypeRef);
        RS(Event)                  = 2 + HI(string) + CI(TypeDefOrRef);
        RS(ExportedType)           = 8 + (2 * HI(string)) + CI(Implementation);
        RS(Field)                  = 2 + HI(string) + HI(blob);
        RS(FieldLayout)            = 4 + TI(Field);
        RS(FieldMarshal)           = HI(blob) + CI(HasFieldMarshal);
        RS(FieldRva)               = 4 + TI(Field);
        RS(File)                   = 4 + HI(string) + HI(blob);
        RS(GenericParam)           = 4 + HI(string) + CI(TypeOrMethodDef);
        RS(GenericParamConstraint) = TI(GenericParam) + CI(TypeDefOrRef);
        RS(ImplMap)                = 2 + HI(string) + CI(MemberForwarded) + TI(ModuleRef);
        RS(InterfaceImpl)          = TI(TypeDef) + CI(TypeDefOrRef);
        RS(ManifestResource)       = 8 + HI(string) + CI(Implementation);
        RS(MemberRef)              = HI(string) + HI(blob) + CI(MemberRefParent);
        RS(MethodDef)              = 8 + HI(string) + HI(blob) + TI(Param);
        RS(MethodImpl)             = TI(TypeDef) + (2 * CI(MethodDefOrRef));
        RS(MethodSemantics)        = 2 + TI(MethodDef) + CI(HasSemantics);
        RS(MethodSpec)             = HI(blob) + CI(MethodDefOrRef);
        RS(Module)                 = 2 + HI(string) + (3 * HI(guid));
        RS(ModuleRef)              = HI(string);
        RS(NestedClass)            = 2 * TI(TypeDef);
        RS(Param)                  = 4 + HI(string);
        RS(Property)               = 2 + HI(string) + HI(blob);
        RS(PropertyMap)            = TI(TypeDef) + TI(Property);
        RS(StandaloneSig)          = HI(blob);
        RS(TypeDef)                = 4 + (2 * HI(string)) + CI(TypeDefOrRef) + TI(Field) + TI(MethodDef);
        RS(TypeRef)                = (2 * HI(string)) + CI(ResolutionScope);
        RS(TypeSpec)               = HI(blob);

        #undef HI
        #undef CI
        #undef TI
        #undef RS
    }

    SizeType TableCollection::GetCompositeIndexSize(CompositeIndex index) const
    {
        return _state._compositeIndexSizes[AsInteger(index)];
    }

    SizeType TableCollection::GetRowCount(TableId id) const
    {
        return _state._rowCounts[AsInteger(id)];
    }

    Table const& TableCollection::GetTable(TableId id) const
    {
        return _state._tables[AsInteger(id)];
    }

    SizeType TableCollection::GetTableIndexSize(TableId id) const
    {
        return _state._rowCounts[AsInteger(id)] < (1 << 16) ? 2 : 4;
    }

    Database::Database(wchar_t const* fileName)
        : _fileName(fileName)
    {
        FileHandle file(fileName);

        PeSectionsAndCliHeader peSectionsAndCliHeader(ReadPeSectionsAndCliHeader(file));

        PeCliStreamHeaderSequence streamHeaders(ReadPeCliStreamHeaders(file, peSectionsAndCliHeader));
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
        return static_cast<AssemblyHashAlgorithm>(ReadAs<std::uint32_t>(_data, 0));
    }
    /* TODO
    Version AssemblyRow::GetVersion() const
    {
        std::uint16_t const major   (ReadAs<std::uint16_t>(_data, 4 ));
        std::uint16_t const minor   (ReadAs<std::uint16_t>(_data, 6 ));
        std::uint16_t const build   (Readas<std::uint16_t>(_data, 8 ));
        std::uint16_t const revision(ReadAs<std::uint16_t>(_data, 10));
        return Version(major, minor, build, revision);
    }
    */
    AssemblyFlags AssemblyRow::GetFlags()     const { return AssemblyFlags(ReadAs<std::uint32_t>(_data, 12)); }
    BlobIndex     AssemblyRow::GetPublicKey() const { return ReadBlobHeapIndex(_database, _data, 16);         }

    String AssemblyRow::GetName() const
    {
        SizeType const offset(16 + _database->GetTables().GetBlobHeapIndexSize());
        return _database->GetStrings().At(ReadStringHeapIndex(_database, _data, offset));
    }

    String AssemblyRow::GetCulture() const
    {
        SizeType const offset(16
                            + _database->GetTables().GetBlobHeapIndexSize()
                            + _database->GetTables().GetStringHeapIndexSize());
        return _database->GetStrings().At(ReadStringHeapIndex(_database, _data, offset));
    }

    std::uint32_t AssemblyOsRow::GetOsPlatformId()   const { return ReadAs<std::uint32_t>(_data, 0); }
    std::uint32_t AssemblyOsRow::GetOsMajorVersion() const { return ReadAs<std::uint32_t>(_data, 4); }
    std::uint32_t AssemblyOsRow::GetOsMinorVersion() const { return ReadAs<std::uint32_t>(_data, 8); }

    std::uint32_t AssemblyProcessorRow::GetProcessor() const
    {
        return ReadAs<std::uint32_t>(_data, 0);
    }
    /*TODO
    Version AssemblyRefRow::GetVersion() const
    {
        std::uint16_t const major   (ReadAs<std::uint16_t>(_data, 0));
        std::uint16_t const minor   (ReadAs<std::uint16_t>(_data, 2));
        std::uint16_t const build   (Readas<std::uint16_t>(_data, 4));
        std::uint16_t const revision(ReadAs<std::uint16_t>(_data, 6));
        return Version(major, minor, build, revision);
    }
    */
    AssemblyFlags AssemblyRefRow::GetFlags() const
    {
        return AssemblyFlags(ReadAs<std::uint32_t>(_data, 8));
    }

    BlobIndex AssemblyRefRow::GetPublicKeyOrToken() const
    {
        return ReadBlobHeapIndex(_database, _data, 12);
    }

    String AssemblyRefRow::GetName() const
    {
        SizeType const offset(12 + _database->GetTables().GetBlobHeapIndexSize());
        return _database->GetStrings().At(ReadStringHeapIndex(_database, _data, offset));
    }

    String AssemblyRefRow::GetCulture() const
    {
        SizeType const offset(12
                            + _database->GetTables().GetBlobHeapIndexSize()
                            + _database->GetTables().GetStringHeapIndexSize());
        return _database->GetStrings().At(ReadStringHeapIndex(_database, _data, offset));
    }

    BlobIndex AssemblyRefRow::GetHashValue() const
    {
        SizeType const offset(12
                            + _database->GetTables().GetBlobHeapIndexSize()
                            + (2 * _database->GetTables().GetStringHeapIndexSize()));
        return ReadBlobHeapIndex(_database, _data, offset);
    }

    std::uint32_t AssemblyRefOsRow::GetOsPlatformId()   const { return ReadAs<std::uint32_t>(_data, 0); }
    std::uint32_t AssemblyRefOsRow::GetOsMajorVersion() const { return ReadAs<std::uint32_t>(_data, 4); }
    std::uint32_t AssemblyRefOsRow::GetOsMinorVersion() const { return ReadAs<std::uint32_t>(_data, 8); }

    TableReference AssemblyRefOsRow::GetAssemblyRef() const
    {
        return TableReference(TableId::AssemblyRef, ReadTableIndex(_database, _data, TableId::AssemblyRef, 12));
    }

    std::uint32_t AssemblyRefProcessorRow::GetProcessor() const
    {
        return ReadAs<std::uint32_t>(_data, 0);
    }

    TableReference AssemblyRefProcessorRow::GetAssemblyRef() const
    {
        return TableReference(TableId::AssemblyRef, ReadTableIndex(_database, _data, TableId::AssemblyRef, 4));
    }

    String ModuleRow::GetName() const
    {
        return _database->GetStrings().At(ReadStringHeapIndex(_database, _data, 2));
    }

    String TypeDefRow::GetName() const
    {
        return _database->GetStrings().At(ReadStringHeapIndex(_database, _data, 4));
    }

    std::uint32_t TypeSpecRow::GetSignature() const
    {
        return ReadBlobHeapIndex(_database, _data, 0);
    }

} }

int main()
{
    using namespace CxxReflect::Metadata;

    Database db(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");

    String moduleName(db.GetRow<TableId::Module>(0).GetName());

    std::vector<String> names;
    std::transform(db.Begin<TableId::TypeDef>(),
                   db.End<TableId::TypeDef>(),
                   std::back_inserter(names),
                   [&](TypeDefRow const& r)
    {
        return r.GetName();
    });
}
