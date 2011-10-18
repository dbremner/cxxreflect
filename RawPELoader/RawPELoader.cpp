#include "RawPELoader/Utility.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using namespace PeLoader;
    using namespace PeLoader::Utility;

    template <typename T>
    class FixedSizeArray
    {
    public:

        FixedSizeArray(FixedSizeArray&& other)
            : _data(std::move(other._data)),
              _size(other._size)
        {
            other._size = 0;
        }

        FixedSizeArray& operator=(FixedSizeArray&& other)
        {
            Swap(other);
            return *this;
        }

        void Swap(FixedSizeArray& other)
        {
            std::swap(_data, other._data);
            std::swap(_size, other._size);
        }

    private:

        std::unique_ptr<T> _data;
        std::size_t        _size;
    };

    // The PE headers and related structures are naturally aligned, so we shouldn't need any custom
    // pragmas, attributes, or directives to pack the structures.  We use static assertions to ensure
    // that there is no padding, just in case.

    struct PeVersion
    {
        std::uint16_t Major;
        std::uint16_t Minor;
    };

    static_assert(sizeof(PeVersion) == 4, "Invalid PeVersion Definition");

    struct PeRvaAndSize
    {
        std::uint32_t Rva;
        std::uint32_t Size;
    };

    static_assert(sizeof(PeRvaAndSize) == 8, "Invalid PeRvaAndSize Definition");

    struct PeFileHeader
    {
        // PE Signature;
        std::uint32_t Signature;

        // PE Header
        std::uint16_t Machine;
        std::uint16_t SectionCount;
        std::uint32_t CreationTimestamp;
        std::uint32_t SymbolTablePointer;
        std::uint32_t SymbolCount;
        std::uint16_t OptionalHeaderSize;
        std::uint16_t Characteristics;

        // PE Optional Header Standard Fields
        std::uint16_t Magic;
        std::uint16_t MajorMinor;
        std::uint32_t CodeSize;
        std::uint32_t InitializedDataSize;
        std::uint32_t UninitializedDataSize;
        std::uint32_t EntryPointRva;
        std::uint32_t CodeRva;
        std::uint32_t DataRva;

        // PE Optional Header Windows NT-Specific Fields
        std::uint32_t ImageBase;
        std::uint32_t SectionAlignment;
        std::uint32_t FileAlignment;
        PeVersion     OsVersion;
        PeVersion     UserVersion;
        PeVersion     SubsystemVersion;
        std::uint16_t Reserved;
        std::uint32_t ImageSize;
        std::uint32_t HeaderSize;
        std::uint32_t FileChecksum;
        std::uint16_t Subsystem;
        std::uint16_t DllFlags;
        std::uint32_t StackReserveSize;
        std::uint32_t StackCommitSize;
        std::uint32_t HeapReserveSize;
        std::uint32_t HeapCommitSize;
        std::uint32_t LoaderFlags;
        std::uint32_t DataDirectoryCount;

        // Data Directories
        PeRvaAndSize  ExportTable;
        PeRvaAndSize  ImportTable;
        PeRvaAndSize  ResourceTable;
        PeRvaAndSize  ExceptionTable;
        PeRvaAndSize  CertificateTable;
        PeRvaAndSize  BaseRelocationTable;
        PeRvaAndSize  DebugTable;
        PeRvaAndSize  CopyrightTable;
        PeRvaAndSize  GlobalPointerTable;
        PeRvaAndSize  ThreadLocalStorageTable;
        PeRvaAndSize  LoadConfigTable;
        PeRvaAndSize  BoundImportTable;
        PeRvaAndSize  ImportAddressTable;
        PeRvaAndSize  DelayImportDescriptorTable;
        PeRvaAndSize  CliHeaderTable;
        PeRvaAndSize  ReservedTableHeader;
    };

    static_assert(sizeof(PeFileHeader) == 248, "Invalid PeFileHeader Definition");

    struct PeSectionHeader
    {
        char          Name[8];
        std::uint32_t VirtualSize;
        std::uint32_t VirtualAddress;

        std::uint32_t RawDataSize;
        std::uint32_t RawDataOffset;

        std::uint32_t RelocationsOffset;
        std::uint32_t LineNumbersOffset;
        std::uint16_t RelocationsCount;
        std::uint16_t LineNumbersCount;

        std::uint32_t Characteristics;
    };

    static_assert(sizeof(PeSectionHeader) == 40, "Invalid PeSectionHeader Definition");

    struct PeCliHeader
    {
        std::uint32_t SizeInBytes;
        PeVersion     RuntimeVersion;
        PeRvaAndSize  Metadata;
        std::uint32_t Flags;
        std::uint32_t EntryPointToken;
        PeRvaAndSize  Resources;
        PeRvaAndSize  StrongNameSignature;
        PeRvaAndSize  CodeManagerTable;
        PeRvaAndSize  VtableFixups;
        PeRvaAndSize  ExportAddressTableJumps;
        PeRvaAndSize  ManagedNativeHeader;
    };

    static_assert(sizeof(PeCliHeader) == 72, "Invalid PeCliHeader Definition");

    unsigned int ComputeOffsetFromRva(PeSectionHeader section, PeRvaAndSize rvaAndSize)
    {
        return rvaAndSize.Rva - section.VirtualAddress + section.RawDataOffset;
    }

    std::unique_ptr<std::uint8_t[]> LoadRawMetadataFromFile(std::wstring fileName, std::size_t& size)
    {
        FileHandle file(fileName.c_str());
        file.Seek(0x3c, FileHandle::Begin);

        std::uint32_t signatureOffset(0);
        file.Read(&signatureOffset, sizeof signatureOffset, 1);
        file.Seek(signatureOffset, FileHandle::Begin);

        PeFileHeader fileHeader = { 0 };
        file.Read(&fileHeader, sizeof fileHeader, 1);

        if (fileHeader.SectionCount > 100 || fileHeader.SectionCount == 0)
            throw std::logic_error("wtf");

        std::vector<PeSectionHeader> sections(fileHeader.SectionCount);
        file.Read(sections.data(), sizeof(*sections.begin()), sections.size());

        // Find the section with the CLI header:
        auto cliHeaderSectionIt(std::find_if(sections.begin(), sections.end(), [&](PeSectionHeader const& h)
        {
            return fileHeader.CliHeaderTable.Rva >= h.VirtualAddress
                && fileHeader.CliHeaderTable.Rva <  h.VirtualAddress + h.VirtualSize;
        }));

        if (cliHeaderSectionIt == sections.end())
            throw std::logic_error("wtf");

        unsigned int const cliHeaderTableOffset(ComputeOffsetFromRva(*cliHeaderSectionIt, fileHeader.CliHeaderTable));
        file.Seek(cliHeaderTableOffset, FileHandle::Begin);

        PeCliHeader cliHeader = { 0 };
        file.Read(&cliHeader, sizeof cliHeader, 1);

        auto metadataSectionIt(std::find_if(sections.begin(), sections.end(), [&](PeSectionHeader const& h)
        {
            return cliHeader.Metadata.Rva >= h.VirtualAddress
                && cliHeader.Metadata.Rva <  h.VirtualAddress + h.VirtualSize;
        }));

        if (metadataSectionIt == sections.end())
            throw std::logic_error("wtf");

        unsigned int const metadataOffset(ComputeOffsetFromRva(*metadataSectionIt, cliHeader.Metadata));
        file.Seek(metadataOffset, FileHandle::Begin);

        std::unique_ptr<std::uint8_t[]> result(new std::uint8_t[cliHeader.Metadata.Size]);
        file.Read(result.get(), cliHeader.Metadata.Size, 1);

        size = cliHeader.Metadata.Size;

        return result;
    }

    // Encapsulates the raw metadata database obtained from an assembly
    class MetadataDatabase
    {
    public:

        enum class TableId : std::uint8_t
        {
            Module                 = 0x00,
            TypeRef                = 0x01,
            TypeDef                = 0x02,
            Field                  = 0x04,
            MethodDef              = 0x06,
            Param                  = 0x08,
            InterfaceImpl          = 0x09,
            MemberRef              = 0x0a,
            Constant               = 0x0b,
            CustomAttribute        = 0x0c,
            FieldMarshal           = 0x0d,
            DeclSecurity           = 0x0e,
            ClassLayout            = 0x0f,
            FieldLayout            = 0x10,
            StandaloneSig          = 0x11,
            EventMap               = 0x12,
            Event                  = 0x14,
            PropertyMap            = 0x15,
            Property               = 0x17,
            MethodSemantics        = 0x18,
            MethodImpl             = 0x19,
            ModuleRef              = 0x1a,
            TypeSpec               = 0x1b,
            ImplMap                = 0x1c,
            FieldRva               = 0x1d,
            Assembly               = 0x20,
            AssemblyProcessor      = 0x21,
            AssemblyOs             = 0x22,
            AssemblyRef            = 0x23,
            AssemblyRefProcessor   = 0x24,
            AssemblyRefOs          = 0x25,
            File                   = 0x26,
            ExportedType           = 0x27,
            ManifestResource       = 0x28,
            NestedClass            = 0x29,
            GenericParam           = 0x2a,
            MethodSpec             = 0x2b,
            GenericParamConstraint = 0x2c
        };

        bool IsValidTableId(std::uint32_t id)
        {
            std::array<std::uint8_t, 64> mask =
            {
                1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
                1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
                1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
            };
            
            return id < mask.size() && mask[id] == 1;
        }

        template <TableId TId>
        struct TableBit
        {
            enum { Value = static_cast<typename 1 << std::underlying_type<TId>::type>(TId) };
        };

        typedef std::size_t                  SizeType;
        typedef std::uint8_t                 ValueType;
        typedef std::uint8_t const*          Pointer;
        typedef std::unique_ptr<ValueType[]> OwnedPointer;

        MetadataDatabase(wchar_t const* fileName)
        {
        }

        MetadataDatabase(OwnedPointer rawData, SizeType rawSize)
            : _rawData(std::move(rawData)),
              _rawSize(rawSize),
              _stringHeapIndexSize(0),
              _blobHeapIndexSize(0),
              _guidHeapIndexSize(0),
              _validBits(0),
              _sortedBits(0),
              _rowCounts(),
              _rowSizes()
        {
            // Check for the magic signature:
            if (ReadAs<std::uint32_t>(0) != 0x424a5342)
                throw std::logic_error("wtf");

            std::uint32_t const versionLength(ReadAs<std::uint32_t>(12));
            std::uint32_t const streamCount(ReadAs<std::uint16_t>(18 + versionLength));
            InitializeBlobs(20 + versionLength, streamCount);
            InitializeTables();
        }

        void InitializeBlobs(SizeType index, SizeType count)
        {
            SizeType streamIndex(index);
            for (unsigned i(0); i < count; ++i)
            {
                std::uint32_t const offset(ReadAs<std::uint32_t>(streamIndex));
                std::uint32_t const size(ReadAs<std::uint32_t>(streamIndex + 4));

                StreamRange const currentRange(_rawData.get() + offset, _rawData.get() + offset + size);
                char const* currentName(ReinterpretAs<char>(streamIndex + 8));

                if (std::strcmp(currentName, "#Strings") == 0 && !_stringHeap.IsInitialized())
                {
                    _stringHeap = currentRange;
                    streamIndex += 20; // 8 + strlen(#Strings____)
                }
                else if (std::strcmp(currentName, "#US") == 0 && !_userstringHeap.IsInitialized())
                {
                    _userstringHeap = currentRange;
                    streamIndex += 12; // 8 + strlen(#US_)
                }
                else if (std::strcmp(currentName, "#Blob") == 0 && !_blobHeap.IsInitialized())
                {
                    _blobHeap = currentRange;
                    streamIndex += 16; // 8 + strlen(#Blob___)
                }
                else if (std::strcmp(currentName, "#GUID") == 0 && !_guidHeap.IsInitialized())
                {
                    _guidHeap = currentRange;
                    streamIndex += 16; // 8 + strlen(#GUID___)
                }
                else if (std::strcmp(currentName, "#~") == 0 && !_tableHeap.IsInitialized())
                {
                    _tableHeap = currentRange;
                    streamIndex += 12; // 8 + strlen(#~__)
                }
                else
                {
                    streamIndex += 12;
                    //throw std::logic_error("wtf");
                }
            }
        }

        void InitializeTables()
        {
            if (!_tableHeap.IsInitialized())
                throw std::logic_error("wtf");

            std::bitset<8> heapSizes(_tableHeap.ReadAs<std::uint8_t>(6));

            _stringHeapIndexSize = heapSizes.test(1) ? 4 : 2;
            _guidHeapIndexSize   = heapSizes.test(2) ? 4 : 2;
            _blobHeapIndexSize   = heapSizes.test(3) ? 4 : 2;

            _validBits  = _tableHeap.ReadAs<std::uint64_t>(8);
            _sortedBits = _tableHeap.ReadAs<std::uint64_t>(16);

            SizeType index(24);
            for (unsigned x(0); x < 64; ++x)
            {
                if (!_validBits.test(x))
                    continue;

                if (!IsValidTableId(x))
                    throw std::logic_error("wtf");

                _rowCounts[x] = _tableHeap.ReadAs<std::uint32_t>(index);
                index += 4;
            }

            // Now that we know how many rows are in each table, we need to compute the size of each
            // table's rows; this is somewhat complex, since row sizes vary depending on the sizes
            // of the tables:
            _rowSizes[AsInteger(TableId::Assembly)]               = 16 + _blobHeapIndexSize + (2 * _stringHeapIndexSize);
            _rowSizes[AsInteger(TableId::AssemblyOs)]             = 12;
            _rowSizes[AsInteger(TableId::AssemblyProcessor)]      = 4;
            _rowSizes[AsInteger(TableId::AssemblyRef)]            = 12 + (2 * _blobHeapIndexSize) + (2 * _stringHeapIndexSize);
            _rowSizes[AsInteger(TableId::AssemblyRefOs)]          = 12 + GetTableIndexSize(TableId::AssemblyRef);
            _rowSizes[AsInteger(TableId::AssemblyRefProcessor)]   = 4 + GetTableIndexSize(TableId::AssemblyRef);
            _rowSizes[AsInteger(TableId::ClassLayout)]            = 6 + GetTableIndexSize(TableId::TypeDef);
            _rowSizes[AsInteger(TableId::Constant)]               = 2 + _blobHeapIndexSize + GetHasConstantIndexSize();
            _rowSizes[AsInteger(TableId::CustomAttribute)]        = GetHasCustomAttributeIndexSize() + GetCustomAttributeTypeIndexSize();
            _rowSizes[AsInteger(TableId::DeclSecurity)]           = 2 + _blobHeapIndexSize + GetHasDeclSecurityIndexSize();
            _rowSizes[AsInteger(TableId::EventMap)]               = GetTableIndexSize(TableId::TypeDef) + GetTableIndexSize(TableId::TypeRef);
            _rowSizes[AsInteger(TableId::Event)]                  = 2 + _stringHeapIndexSize + GetTypeDefOrRefIndexSize();
            _rowSizes[AsInteger(TableId::ExportedType)]           = 8 + (2 * _stringHeapIndexSize) + GetImplementationIndexSize();
            _rowSizes[AsInteger(TableId::Field)]                  = 2 + _stringHeapIndexSize + _blobHeapIndexSize;
            _rowSizes[AsInteger(TableId::FieldLayout)]            = 4 + GetTableIndexSize(TableId::Field);
            _rowSizes[AsInteger(TableId::FieldMarshal)]           = _blobHeapIndexSize + GetHasFieldMarshalIndexSize();
            _rowSizes[AsInteger(TableId::FieldRva)]               = 4 + GetTableIndexSize(TableId::Field);
            _rowSizes[AsInteger(TableId::File)]                   = 4 + _stringHeapIndexSize + _blobHeapIndexSize;
            _rowSizes[AsInteger(TableId::GenericParam)]           = 4 + _stringHeapIndexSize + GetTypeOrMethodDefIndexSize();
            _rowSizes[AsInteger(TableId::GenericParamConstraint)] = GetTableIndexSize(TableId::GenericParam) + GetTypeDefOrRefIndexSize();
            _rowSizes[AsInteger(TableId::ImplMap)]                = 2 + _stringHeapIndexSize + GetMemberForwardedIndexSize() + GetTableIndexSize(TableId::ModuleRef);
            _rowSizes[AsInteger(TableId::InterfaceImpl)]          = GetTableIndexSize(TableId::TypeDef) + GetTypeDefOrRefIndexSize();
            _rowSizes[AsInteger(TableId::ManifestResource)]       = 8 + _stringHeapIndexSize + GetImplementationIndexSize();
            _rowSizes[AsInteger(TableId::MemberRef)]              = _stringHeapIndexSize + _blobHeapIndexSize + GetMemberRefParentIndexSize();
            _rowSizes[AsInteger(TableId::MethodDef)]              = 8 + _stringHeapIndexSize + _blobHeapIndexSize + GetTableIndexSize(TableId::Param);
            _rowSizes[AsInteger(TableId::MethodImpl)]             = GetTableIndexSize(TableId::TypeDef) + (2 * GetMethodDefOrRefIndexSize());
            _rowSizes[AsInteger(TableId::MethodSemantics)]        = 2 + GetTableIndexSize(TableId::MethodDef) + GetHasSemanticsIndexSize();
            _rowSizes[AsInteger(TableId::MethodSpec)]             = _blobHeapIndexSize + GetMethodDefOrRefIndexSize();
            _rowSizes[AsInteger(TableId::Module)]                 = 2 + _stringHeapIndexSize + (3 * _guidHeapIndexSize);
            _rowSizes[AsInteger(TableId::ModuleRef)]              = _stringHeapIndexSize;
            _rowSizes[AsInteger(TableId::NestedClass)]            = 2 * GetTableIndexSize(TableId::TypeDef);
            _rowSizes[AsInteger(TableId::Param)]                  = 4 + _stringHeapIndexSize;
            _rowSizes[AsInteger(TableId::Property)]               = 2 + _stringHeapIndexSize + _blobHeapIndexSize;
            _rowSizes[AsInteger(TableId::PropertyMap)]            = GetTableIndexSize(TableId::TypeDef) + GetTableIndexSize(TableId::Property);
            _rowSizes[AsInteger(TableId::StandaloneSig)]          = _blobHeapIndexSize;
            _rowSizes[AsInteger(TableId::TypeDef)]                = 4 + (2 * _stringHeapIndexSize) + GetTypeDefOrRefIndexSize() + GetTableIndexSize(TableId::Field) + GetTableIndexSize(TableId::MethodDef);
            _rowSizes[AsInteger(TableId::TypeRef)]                = (2 * _stringHeapIndexSize) + GetResolutionScopeIndexSize();
            _rowSizes[AsInteger(TableId::TypeSpec)]               = _blobHeapIndexSize;
        
            // Now that we can compute the size of each table (row size * row count), we can build
            // the actual tables so it's easy to search for information:
        }

    private:

        class StreamRange
        {
        public:

            StreamRange()
                : _begin(nullptr), _end(nullptr)
            {
            }

            StreamRange(Pointer begin, Pointer end)
                : _begin(begin), _end(end)
            {
            }

            Pointer  Begin()         const { return _begin;                               }
            Pointer  End()           const { return _end;                                 }
            SizeType GetSize()       const { return _end - _begin;                        }
            Pointer  GetData()       const { return _begin;                               }
            bool     IsInitialized() const { return _begin != nullptr && _end != nullptr; }

            Pointer At(SizeType index) const
            {
                return _begin + index; // TODO RANGE CHECK
            }

            template <typename T>
            T const& ReadAs(SizeType index)
            {
                // TODO CHECK RANGE AND VALIDITY (AND ALIGNMENT?)
                return *reinterpret_cast<T const*>(_begin + index);
            }

            template <typename T>
            T const* ReinterpretAs(SizeType index)
            {
                // TODO CHECK RANGE AND VALIDITY (AND ALIGNMENT?)
                return reinterpret_cast<T const*>(_begin + index);
            }

        private:

            Pointer _begin;
            Pointer _end;
        };

        class Table
        {
        public:

            Table(Pointer data, SizeType rowCount, SizeType rowSize, bool isSorted)
                : _data(data), _rowCount(rowCount), _rowSize(rowSize), _isSorted(isSorted)
            {
            }

        private:

            Pointer  _data;
            SizeType _rowCount;
            SizeType _rowSize;
            bool     _isSorted;
        };

        template <typename T>
        T const& ReadAs(SizeType index)
        {
            // TODO CHECK RANGE AND VALIDITY (AND ALIGNMENT?)
            return *reinterpret_cast<T const*>(_rawData.get() + index);
        }

        template <typename T>
        T const* ReinterpretAs(SizeType index)
        {
            // TODO CHECK RANGE AND VALIDITY (AND ALIGNMENT?)
            return reinterpret_cast<T const*>(_rawData.get() + index);
        }

        enum { TableIdCount = 0x2d };

        typedef std::array<SizeType, TableIdCount> TableIdSizeArray;

        SizeType GetTableIndexSize(TableId id) const
        {
            return _rowCounts[AsInteger(id)] < (1 << 16) ? 2 : 4;
        }

        #ifdef CXXREFLECT_GENERATE
        #   error Illicit macro redefinition
        #endif
        #define CXXREFLECT_GENERATE(x) (_rowCounts[AsInteger(TableId::x)] < (1 << (16 - tagBits)))

        SizeType GetTypeDefOrRefIndexSize() const
        {
            std::uint32_t const tagBits(2);
            return CXXREFLECT_GENERATE(TypeDef)
                && CXXREFLECT_GENERATE(TypeRef)
                && CXXREFLECT_GENERATE(TypeSpec) ? 2 : 4;
        }

        SizeType GetHasConstantIndexSize() const
        {
            std::uint32_t const tagBits(2);
            return CXXREFLECT_GENERATE(Field)
                && CXXREFLECT_GENERATE(Param)
                && CXXREFLECT_GENERATE(Property) ? 2 : 4;
        }

        SizeType GetHasCustomAttributeIndexSize() const
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

        SizeType GetHasFieldMarshalIndexSize() const
        {
            std::uint32_t const tagBits(1);
            return CXXREFLECT_GENERATE(Field) && CXXREFLECT_GENERATE(Param) ? 2 : 4;
        }

        SizeType GetHasDeclSecurityIndexSize() const
        {
            std::uint32_t const tagBits(2);
            return CXXREFLECT_GENERATE(TypeDef)
                && CXXREFLECT_GENERATE(MethodDef)
                && CXXREFLECT_GENERATE(Assembly) ? 2 : 4;
        }

        SizeType GetMemberRefParentIndexSize() const
        {
            std::uint32_t const tagBits(3);
            return CXXREFLECT_GENERATE(TypeDef)
                && CXXREFLECT_GENERATE(TypeRef)
                && CXXREFLECT_GENERATE(ModuleRef)
                && CXXREFLECT_GENERATE(MethodDef)
                && CXXREFLECT_GENERATE(TypeSpec) ? 2 : 4;
        }

        SizeType GetHasSemanticsIndexSize() const
        {
            std::uint32_t const tagBits(1);
            return CXXREFLECT_GENERATE(Event) && CXXREFLECT_GENERATE(Property) ? 2 : 4;
        }

        SizeType GetMethodDefOrRefIndexSize() const
        {
            std::uint32_t const tagBits(1);
            return CXXREFLECT_GENERATE(MethodDef) && CXXREFLECT_GENERATE(MemberRef) ? 2 : 4;
        }

        SizeType GetMemberForwardedIndexSize() const
        {
            std::uint32_t const tagBits(1);
            return CXXREFLECT_GENERATE(Field) && CXXREFLECT_GENERATE(MethodDef) ? 2 : 4;
        }

        SizeType GetImplementationIndexSize() const
        {
            std::uint32_t const tagBits(2);
            return CXXREFLECT_GENERATE(File)
                && CXXREFLECT_GENERATE(AssemblyRef)
                && CXXREFLECT_GENERATE(ExportedType) ? 2 : 4;
        }

        SizeType GetCustomAttributeTypeIndexSize() const
        {
            std::uint32_t const tagBits(3);
            return CXXREFLECT_GENERATE(MethodDef) && CXXREFLECT_GENERATE(MemberRef) ? 2 : 4;
        }

        SizeType GetResolutionScopeIndexSize() const
        {
            std::uint32_t const tagBits(2);
            return CXXREFLECT_GENERATE(Module)
                && CXXREFLECT_GENERATE(ModuleRef)
                && CXXREFLECT_GENERATE(AssemblyRef)
                && CXXREFLECT_GENERATE(TypeRef) ? 2 : 4;
        }

        SizeType GetTypeOrMethodDefIndexSize() const
        {
            std::uint32_t const tagBits(1);
            return CXXREFLECT_GENERATE(TypeDef) && CXXREFLECT_GENERATE(MethodDef) ? 2 : 4;
        }

        #undef CXXREFLECT_GENERATE

        OwnedPointer _rawData;
        SizeType     _rawSize;

        StreamRange  _stringHeap;
        StreamRange  _userstringHeap;
        StreamRange  _blobHeap;
        StreamRange  _guidHeap;
        StreamRange  _tableHeap; // Not really a heap

        ValueType    _stringHeapIndexSize;
        ValueType    _guidHeapIndexSize;
        ValueType    _blobHeapIndexSize;

        std::bitset<64> _validBits;
        std::bitset<64> _sortedBits;

        TableIdSizeArray _rowCounts;
        TableIdSizeArray _rowSizes;
    };
}
/*
int main()
{
    std::wstring fileName(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");
    //std::wstring fileName(L"C:\\Users\\James\\Desktop\\Cust10Css_tools4.dll");
    std::size_t size;
    std::unique_ptr<std::uint8_t[]> md(LoadRawMetadataFromFile(fileName, size));
    //MetadataDatabase db(std::move(md), size);

    MetadataDatabase db(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");
}*/