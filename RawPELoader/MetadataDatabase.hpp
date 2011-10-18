//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is an internal header; it is not intended to be included by clients.  It defines all of the
// classes used for reading and interpreting the raw metadata from an assembly.
#ifndef PELOADER_METADATADATABASE_HPP_
#define PELOADER_METADATADATABASE_HPP_

#include "RawPELoader/Utility.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace PeLoader { namespace Metadata {

    typedef wchar_t                                Character;
    typedef Utility::EnhancedCString<Character>    String;
    typedef std::size_t                            SizeType;
    typedef std::uint8_t                           Byte;
    typedef Byte const*                            ByteIterator;
    typedef std::uint32_t                          BlobIndex;

    struct ReadException : std::runtime_error
    {
        ReadException(char const* const message) : std::runtime_error(message) { }
    };

    class Stream
    {
    public:

        Stream(Utility::FileHandle& file, SizeType metadataOffset, SizeType streamOffset, SizeType streamSize);

        Stream()
            : _size()
        {
        }

        Stream(Stream&& other)
            : _data(std::move(other._data)),
              _size(std::move(other._size))
        {
            other._size = 0;
        }

        Stream& operator=(Stream&& other)
        {
            Swap(other);
            return *this;
        }

        void Swap(Stream& other)
        {
            std::swap(other._data, _data);
            std::swap(other._size, _size);
        }

        ByteIterator  Begin()         const { VerifyInitialized(); return _data.get();            }
        ByteIterator  End()           const { VerifyInitialized(); return _data.get() + _size;    }
        SizeType      Size()          const { VerifyInitialized(); return _size;                  }
        bool          IsInitialized() const {                      return _data.get() != nullptr; }

        ByteIterator At(SizeType const index) const
        {
            VerifyInitialized();
            Utility::DebugVerify([&] { return index <= _size; }, "Index out of range");
            return _data.get() + index;
        }

        template <typename T>
        T const& ReadAs(SizeType const index) const
        {
            VerifyInitialized();
            Utility::DebugVerify([&] { return (index + sizeof (T)) <= _size; }, "Index out of range");
            return *reinterpret_cast<T const*>(_data.get() + index);
        }

        template <typename T>
        T const* ReinterpretAs(SizeType const index) const
        {
            VerifyInitialized();
            Utility::DebugVerify([&] { return index <= _size; }, "Index out of range");
            return reinterpret_cast<T const*>(_data.get() + index);
        }

    private:

        typedef std::unique_ptr<Byte[]> OwnedPointer;

        Stream(Stream const&);
        Stream& operator=(Stream const&);

        void VerifyInitialized() const
        {
            Utility::DebugVerify([&] { return IsInitialized(); }, "Stream is not initialized");
        }

        OwnedPointer    _data;
        SizeType        _size;
    };

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

    enum { TableIdCount = 0x2d };

    typedef std::array<SizeType, TableIdCount> TableIdSizeArray;

    inline bool IsValidTableId(std::uint32_t const id)
    {
        static std::array<Byte, 64> const mask =
        {
            1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };
            
        return id < mask.size() && mask[id] == 1;
    }

    enum class CompositeIndex
    {
        TypeDefOrRef        = 0x00,
        HasConstant         = 0x01,
        HasCustomAttribute  = 0x02,
        HasFieldMarshal     = 0x03,
        HasDeclSecurity     = 0x04,
        MemberRefParent     = 0x05,
        HasSemantics        = 0x06,
        MethodDefOrRef      = 0x07,
        MemberForwarded     = 0x08,
        Implementation      = 0x09,
        CustomAttributeType = 0x0a,
        ResolutionScope     = 0x0b,
        TypeOrMethodDef     = 0x0c
    };

    enum { CompositeIndexCount = 0x0d };

    typedef std::array<SizeType, CompositeIndexCount> CompositeIndexSizeArray;

    class TableReference
    {
    public:

        TableReference()
            : _table(), _index(0)
        {
        }

        TableReference(TableId const table, std::uint32_t const index)
            : _table(table), _index(index)
        {
        }

        TableId       GetTable() const { return _table; }
        std::uint32_t GetIndex() const { return _index; }

    private:

        TableId          _table;
        std::uint32_t    _index;
    };

    template <typename TRow>
    struct RowTypeToTableId;

    template <TableId TId>
    struct TableIdToRowType;

    // We specialize the RowTypeToTableId and TableIdToRowType maps for each table; we declare the
    // specializations before we define the classes so that functions defined inline in the classes
    // can reference the transformations... we generate some of these inline members using macros.
    #define CXXREFLECT_GENERATE(t)        \
    class t ## Row;                       \
                                          \
    template <>                           \
    struct RowTypeToTableId<t ## Row>     \
    {                                     \
        enum { Value = TableId::t };      \
    };                                    \
                                          \
    template <>                           \
    struct TableIdToRowType<TableId::t>   \
    {                                     \
        typedef t ## Row Type;            \
    }

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

    class Table
    {
    public:

        Table()
            : _data(nullptr), _rowSize(0), _rowCount(0), _isSorted(false)
        {
        }

        Table(ByteIterator const data, SizeType const rowSize, SizeType const rowCount, bool const isSorted)
            : _data(data), _rowSize(rowSize), _rowCount(rowCount), _isSorted(isSorted)
        {
        }

        ByteIterator  Begin()         const { VerifyInitialized(); return _data;                        }
        ByteIterator  End()           const { VerifyInitialized(); return _data + _rowCount * _rowSize; }
        bool          IsSorted()      const { VerifyInitialized(); return _isSorted;                    }
        bool          IsInitialized() const {                      return _data != nullptr;             }
        SizeType      GetRowSize()    const { VerifyInitialized(); return _rowSize;                     }

        ByteIterator At(SizeType const index) const
        {
            VerifyInitialized();
            Utility::DebugVerify([&] { return index < _rowCount; }, "Index out of range");
            return _data + _rowSize * index;
        }

    private:

        void VerifyInitialized() const
        {
            Utility::DebugVerify([&] { return IsInitialized(); }, "Table is not initialized");
        }

        ByteIterator  _data;
        SizeType      _rowSize;
        SizeType      _rowCount;
        bool          _isSorted;
    };

    class TableCollection
    {
    public:

        TableCollection()
            : _initialized(false), _state()
        {
        }

        explicit TableCollection(Stream&& stream);

        TableCollection(TableCollection&& other)
            : _stream(std::move(other._stream)),
              _initialized(std::move(other._initialized)),
              _state(std::move(other._state))
        {
            other._initialized = false;
            other._state = State();
        }

        TableCollection& operator=(TableCollection&& other)
        {
            Swap(other);
            return *this;
        }

        void Swap(TableCollection& other)
        {
            std::swap(other._stream,      _stream);
            std::swap(other._initialized, _initialized);
            std::swap(other._state,       _state);
        }

        Table const& GetTable(TableId id) const;

        SizeType GetRowCount(TableId id) const;

        SizeType GetTableIndexSize(TableId id) const;
        SizeType GetCompositeIndexSize(CompositeIndex index) const;

        SizeType GetStringHeapIndexSize() const { return _state._stringHeapIndexSize; }
        SizeType GetGuidHeapIndexSize()   const { return _state._guidHeapIndexSize;   }
        SizeType GetBlobHeapIndexSize()   const { return _state._blobHeapIndexSize;   }

    private:

        TableCollection(TableCollection const&);
        TableCollection& operator=(TableCollection const&);

        typedef std::array<Table, TableIdCount> TableSequence;

        void ComputeCompositeIndexSizes();
        void ComputeTableRowSizes();

        // Most of our state is copyable; we encapsulate it for easier move-op definition
        struct State
        {
            SizeType         _stringHeapIndexSize;
            SizeType         _guidHeapIndexSize;
            SizeType         _blobHeapIndexSize;

            std::bitset<64>  _validBits;
            std::bitset<64>  _sortedBits;

            TableIdSizeArray _rowCounts;
            TableIdSizeArray _rowSizes;

            CompositeIndexSizeArray _compositeIndexSizes;

            TableSequence _tables;
        };

        Stream _stream;
        bool   _initialized;
        State  _state;
    };

    class StringCollection
    {
    public:

        StringCollection()
        {
        }

        explicit StringCollection(Stream&& stream)
            : _stream(std::move(stream))
        {
        }

        StringCollection(StringCollection&& other)
            : _stream(std::move(other._stream)),
              _buffer(std::move(other._buffer)),
              _index(std::move(other._index))
        {
        }

        StringCollection& operator=(StringCollection&& other)
        {
            Swap(other);
            return *this;
        }

        void Swap(StringCollection& other)
        {
            std::swap(other._stream, _stream);
            std::swap(other._buffer, _buffer);
            std::swap(other._index,  _index);
        }

        String At(SizeType index) const;

    private:

        typedef Utility::LinearArrayAllocator<Character, (1 << 16)> Allocator;

        StringCollection(StringCollection const&);
        StringCollection& operator=(StringCollection const&);

        Stream                             _stream;
        mutable Allocator                  _buffer;
        mutable std::map<SizeType, String> _index;
    };

    template <TableId TId>
    class RowIterator;

    class Database
    {
    public:

        Database(wchar_t const* fileName);

        template <TableId TId>
        RowIterator<TId> Begin() const
        {
            return RowIterator<TId>(this, 0);
        }

        template <TableId TId>
        RowIterator<TId> End() const
        {
            return RowIterator<TId>(this, _tables.GetRowCount(TId));
        }

        template <TableId TId>
        typename TableIdToRowType<TId>::Type GetRow(SizeType index) const
        {
            typedef typename TableIdToRowType<TId>::Type ReturnType;
            return ReturnType(this, _tables.GetTable(TId).At(index));
        }

        TableCollection  const& GetTables()  const { return _tables;  }
        StringCollection const& GetStrings() const { return _strings; }

    private:

        std::wstring  _fileName;

        Stream _blobStream;
        Stream _guidStream;

        StringCollection _strings;
        TableCollection  _tables;
    };

    template <TableId TId>
    class RowIterator
    {
    public:

        enum { TableId = TId };

        typedef std::random_access_iterator_tag          iterator_category;
        typedef std::ptrdiff_t                           difference_type;
        typedef typename TableIdToRowType<TId>::Type     value_type;
        typedef value_type                               reference;
        typedef Utility::Dereferenceable<value_type>     pointer;

        typedef std::size_t                              SizeType;
        typedef difference_type                          DifferenceType;
        typedef value_type                               ValueType;
        typedef reference                                Reference;
        typedef pointer                                  Pointer;

        RowIterator()
            : _database(nullptr), _index(0)
        {
        }

        explicit RowIterator(Database const* const database, SizeType const index)
            : _database(database), _index(index)
        {
        }

        Reference    Get()           const { return _database->GetRow<TId>(_index); }
        Reference    operator*()     const { return _database->GetRow<TId>(_index); }
        Pointer      operator->()    const { return _database->GetRow<TId>(_index); }

        RowIterator& operator++()    { ++_index; return *this;                      }
        RowIterator  operator++(int) { RowIterator it(*this); ++*this; return it;   }

        RowIterator& operator--()    { --_index; return *this;                      }
        RowIterator  operator--(int) { RowIterator it(*this); --*this; return it;   }

        RowIterator& operator+=(DifferenceType const n) const { _index += n; return *this; }
        RowIterator& operator-=(DifferenceType const n) const { _index -= n; return *this; }

        Reference operator[](DifferenceType const n) const
        {
            return _database->GetRow<ValueType>(_index + n);
        }

        friend RowIterator operator+(RowIterator it, DifferenceType const n) { return it +=  n; }
        friend RowIterator operator+(DifferenceType const n, RowIterator it) { return it +=  n; }
        friend RowIterator operator-(RowIterator it, DifferenceType const n) { return it += -n; }

        friend DifferenceType operator-(RowIterator const& lhs, RowIterator const& rhs)
        {
            return lhs._index - rhs._index;
        }

        friend bool operator==(RowIterator const& lhs, RowIterator const& rhs)
        {
            return lhs._index == rhs._index;
        }

        friend bool operator< (RowIterator const& lhs, RowIterator const& rhs)
        {
            return lhs._index < rhs._index;
        }

        friend bool operator!=(RowIterator const& lhs, RowIterator const& rhs) { return !(lhs == rhs); }
        friend bool operator> (RowIterator const& lhs, RowIterator const& rhs) { return   rhs <  lhs ; }
        friend bool operator<=(RowIterator const& lhs, RowIterator const& rhs) { return !(rhs <  lhs); }
        friend bool operator>=(RowIterator const& lhs, RowIterator const& rhs) { return !(lhs <  rhs); }

    private:

        Database const* _database;
        SizeType        _index;
    };

    enum class AssemblyAttribute : std::uint32_t
    {
        PublicKey                  = 0x0001,
        Retargetable               = 0x0100,
        DisableJitCompileOptimizer = 0x4000,
        EnableJitCompileTracking   = 0x8000
    };

    enum class AssemblyHashAlgorithm : std::uint16_t
    {
        None     = 0x0000,
        MD5      = 0x8003,
        SHA1     = 0x8004
    };

    enum class EventAttribute
    {
        SpecialName        = 0x0200,
        RuntimeSpecialName = 0x0400
    };

    enum class FieldAttribute
    {
        FieldAccessMask    = 0x0007,

        CompilerControlled = 0x0000,
        Private            = 0x0001,
        FamilyAndAssembly  = 0x0002,
        Assembly           = 0x0003,
        Family             = 0x0004,
        FamilyOrAssembly   = 0x0005,
        Public             = 0x0006,

        Static             = 0x0010,
        InitOnly           = 0x0020,
        Literal            = 0x0040,
        NotSerialized      = 0x0080,
        SpecialName        = 0x0200,

        PInvokeImpl        = 0x2000,

        RuntimeSpecialName = 0x0400,
        HasFieldMarshal    = 0x1000,
        HasDefault         = 0x8000,
        HasFieldRva        = 0x0100
    };

    enum class FileAttribute
    {
        ContainsMetadata   = 0x0000,
        ContainsNoMetadata = 0x0001
    };

    enum class GenericParameterAttribute
    {
        VarianceMask                   = 0x0003,
        None                           = 0x0000,
        Covariant                      = 0x0001,
        Contravariant                  = 0x0002,

        SpecialConstraintMask          = 0x001c,
        ReferenceTypeConstraint        = 0x0004,
        NotNullableValueTypeConstraint = 0x0008,
        DefaultConstructorConstraint   = 0x0010
    };

    enum class ManifestResourceAttribute
    {
        VisibilityMask = 0x0007,
        Public         = 0x0001,
        Private        = 0x0002
    };

    enum class MethodAttribute
    {
        MemberAccessMask      = 0x0007,
        CompilerControlled    = 0x0000,
        Private               = 0x0001,
        FamilyAndAssembly     = 0x0002,
        Assembly              = 0x0003,
        Family                = 0x0004,
        FamilyOrAssembly      = 0x0005,
        Public                = 0x0006,

        Static                = 0x0010,
        Final                 = 0x0020,
        Virtual               = 0x0040,
        HideBySig             = 0x0080,

        VTableLayoutMask      = 0x0100,
        ReuseSlot             = 0x0000,
        NewSlot               = 0x0100,

        Strict                = 0x0200,
        Abstract              = 0x0400,
        SpecialName           = 0x0800,

        PInvokeImpl           = 0x2000,
        RuntimeSpecialName    = 0x1000,
        HasSecurity           = 0x4000,
        RequireSecurityObject = 0x8000
    };

    enum class MethodImplementationAttribute
    {
        CodeTypeMask   = 0x0003,
        IL             = 0x0000,
        Native         = 0x0001,
        Runtime        = 0x0003,

        ManagedMask    = 0x0004,
        Unmanaged      = 0x0004,
        Managed        = 0x0000,

        ForwardRef     = 0x0010,
        PreserveSig    = 0x0080,
        InternalCall   = 0x1000,
        Synchronized   = 0x0020,
        NoInlining     = 0x0008,
        NoOptimization = 0x0040
    };

    enum class MethodSemanticsAttribute
    {
        Setter   = 0x0001,
        Getter   = 0x0002,
        Other    = 0x0004,
        AddOn    = 0x0008,
        RemoveOn = 0x0010,
        Fire     = 0x0020
    };

    enum class ParameterAttribute
    {
        In              = 0x0001,
        Out             = 0x0002,
        Optional        = 0x0010,
        HasDefault      = 0x1000,
        HasFieldMarshal = 0x2000
    };

    enum class PInvokeAttribute
    {
        NoMangle                     = 0x0001,

        CharacterSetMask             = 0x0006,
        CharacterSetNotSpecified     = 0x0000,
        CharacterSetAnsi             = 0x0002,
        CharacterSetUnicode          = 0x0004,
        CharacterSetAuto             = 0x0006,

        SupportsLastError            = 0x0040,

        CallingConventionMask        = 0x0700,
        CallingConventionPlatformApi = 0x0100,
        CallingConventionCDecl       = 0x0200,
        CallingConventionStdCall     = 0x0300,
        CallingConventionThisCall    = 0x0400,
        CallingConventionFastCall    = 0x0500
    };

    enum class PropertyAttribute
    {
        SpecialName        = 0x0200,
        RuntimeSpecialName = 0x0400,
        HasDefault         = 0x1000
    };

    enum class TypeAttribute
    {
        VisibilityMask          = 0x00000007,
        NotPublic               = 0x00000000,
        Public                  = 0x00000001,
        NestedPublic            = 0x00000002,
        NestedPrivate           = 0x00000003,
        NestedFamily            = 0x00000004,
        NestedAssembly          = 0x00000005,
        NestedFamilyAndAssembly = 0x00000006,
        NestedFamilyOrAssembly  = 0x00000007,

        LayoutMask              = 0x00000018,
        AutoLayout              = 0x00000000,
        SequentialLayout        = 0x00000008,
        ExplicitLayout          = 0x00000010,

        ClassSemanticsMask      = 0x00000020,
        Class                   = 0x00000000,
        Interface               = 0x00000020,

        Abstract                = 0x00000080,
        Sealed                  = 0x00000100,
        SpecialName             = 0x00000400,

        Import                  = 0x00001000,
        Serializable            = 0x00002000,

        StringFormatMask        = 0x00030000,
        AnsiClass               = 0x00000000,
        UnicodeClass            = 0x00010000,
        AutoClass               = 0x00020000,
        CustomFormatClass       = 0x00030000,
        CustomStringFormatMask  = 0x00c00000,

        BeforeFieldInit         = 0x00100000,

        RuntimeSpecialName      = 0x00000800,
        HasSecurity             = 0x00040000,
        IsTypeForwarder         = 0x00200000
    };

    typedef Utility::FlagSet<AssemblyAttribute>             AssemblyFlags;
    typedef Utility::FlagSet<EventAttribute>                EventFlags;
    typedef Utility::FlagSet<FieldAttribute>                FieldFlags;
    typedef Utility::FlagSet<FileAttribute>                 FileFlags;
    typedef Utility::FlagSet<GenericParameterAttribute>     GenericParameterFlags;
    typedef Utility::FlagSet<ManifestResourceAttribute>     ManifestResourceFlags;
    typedef Utility::FlagSet<MethodAttribute>               MethodFlags;
    typedef Utility::FlagSet<MethodImplementationAttribute> MethodImplementationFlags;
    typedef Utility::FlagSet<MethodSemanticsAttribute>      MethodSemanticsFlags;
    typedef Utility::FlagSet<ParameterAttribute>            ParameterFlags;
    typedef Utility::FlagSet<PInvokeAttribute>              PInvokeFlags;
    typedef Utility::FlagSet<PropertyAttribute>             PropertyFlags;
    typedef Utility::FlagSet<TypeAttribute>                 TypeFlags;

    enum class ElementType
    {
        End                         = 0x00,
        Void                        = 0x01,
        Boolean                     = 0x02,
        Char                        = 0x03,
        I1                          = 0x04,
        U1                          = 0x05,
        I2                          = 0x06,
        U2                          = 0x07,
        I4                          = 0x08,
        U4                          = 0x09,
        I8                          = 0x0a,
        U8                          = 0x0b,
        R4                          = 0x0c,
        R8                          = 0x0d,
        String                      = 0x0e,
        Ptr                         = 0x0f,
        ByRef                       = 0x10,
        ValueType                   = 0x11,
        Class                       = 0x12,
        Var                         = 0x13,
        Array                       = 0x14,
        GenericInst                 = 0x15,
        TypedByRef                  = 0x16,

        I                           = 0x18,
        U                           = 0x19,
        FnPtr                       = 0x1b,
        Object                      = 0x1c,
        SzArray                     = 0x1d,
        MVar                        = 0x1e,

        CustomModifierRequired      = 0x1f,
        CustomModifierOptional      = 0x20,

        Internal                    = 0x21,
        Modifier                    = 0x40,
        Sentinel                    = 0x41,
        Pinned                      = 0x45,

        Type                        = 0x50,
        CustomAttributeBoxedObject  = 0x51,
        CustomAttributeField        = 0x53,
        CustomAttributeProperty     = 0x54,
        CustomAttributeEnum         = 0x55
    };

    // IsInitialized, VerifyInitialized, and the data members could be hoisted into a base
    // class, but the constructors cannot, so it's easier to just declare and define them
    // all here, via a macro.
    #define CXXREFLECT_GENERATE(name)                                                                   \
        private:                                                                                        \
                                                                                                        \
            Database const*     _database;                                                              \
            std::uint8_t const* _data;                                                                  \
                                                                                                        \
        public:                                                                                         \
                                                                                                        \
            name()                                                                                      \
                : _database(nullptr), _data(nullptr)                                                    \
            {                                                                                           \
            }                                                                                           \
                                                                                                        \
            name(Database const* const database, std::uint8_t const* const data)                        \
                : _database(database), _data(data)                                                      \
            {                                                                                           \
                VerifyInitialized();                                                                    \
            }                                                                                           \
                                                                                                        \
            bool IsInitialized() const                                                                  \
            {                                                                                           \
                return _database != nullptr && _data != nullptr;                                        \
            }                                                                                           \
                                                                                                        \
            void VerifyInitialized() const                                                              \
            {                                                                                           \
                Utility::DebugVerify([&] { return IsInitialized(); }, # name " is not initialized");    \
            }                                                                                           \
                                                                                                        \
            TableReference GetSelfReference() const                                                     \
            {                                                                                           \
                VerifyInitialized();                                                                    \
                                                                                                        \
                TableId const tableId(static_cast<TableId>(RowTypeToTableId<name>::Value));             \
                Table const& table(_database->GetTables().GetTable(tableId));                           \
                                                                                                        \
                SizeType const index((_data - table.Begin()) / table.GetRowSize());                     \
                return TableReference(tableId, index);                                                  \
            }                                                                                           \
                                                                                                        \
        private:

    class AssemblyRow
    {
        CXXREFLECT_GENERATE(AssemblyRow)

    public:

        AssemblyHashAlgorithm GetHashAlgorithm() const;
        //Version               GetVersion()       const; // TODO
        AssemblyFlags         GetFlags()         const;
        BlobIndex             GetPublicKey()     const;
        String                GetName()          const;
        String                GetCulture()       const;
    };

    class AssemblyOsRow
    {
        CXXREFLECT_GENERATE(AssemblyOsRow)

    public:

        std::uint32_t GetOsPlatformId()   const;
        std::uint32_t GetOsMajorVersion() const;
        std::uint32_t GetOsMinorVersion() const;
    };

    class AssemblyProcessorRow
    {
        CXXREFLECT_GENERATE(AssemblyProcessorRow)

    public:

        std::uint32_t GetProcessor() const;
    };

    class AssemblyRefRow
    {
        CXXREFLECT_GENERATE(AssemblyRefRow)

    public:

        // Version GetVersion() const; // TODO
        AssemblyFlags GetFlags()            const;
        BlobIndex     GetPublicKeyOrToken() const;
        String        GetName()             const;
        String        GetCulture()          const;
        BlobIndex     GetHashValue()        const;
    };

    class AssemblyRefOsRow
    {
        CXXREFLECT_GENERATE(AssemblyRefOsRow)

    public:

        std::uint32_t  GetOsPlatformId()   const;
        std::uint32_t  GetOsMajorVersion() const;
        std::uint32_t  GetOsMinorVersion() const;
        TableReference GetAssemblyRef()    const;
    };

    class AssemblyRefProcessorRow
    {
        CXXREFLECT_GENERATE(AssemblyRefProcessorRow)

    public:

        std::uint32_t  GetProcessor()   const;
        TableReference GetAssemblyRef() const;
    };

    class ClassLayoutRow
    {
        CXXREFLECT_GENERATE(ClassLayoutRow)

    public:

        std::uint16_t  GetPackingSize()   const;
        std::uint32_t  GetClassSize()     const;
        TableReference GetParentTypeDef() const;
    };

    class ConstantRow
    {
        CXXREFLECT_GENERATE(ConstantRow)

    public:

        std::uint8_t   GetType()   const;
        TableReference GetParent() const;
        BlobIndex      GetValue()  const;
    };

    class CustomAttributeRow
    {
        CXXREFLECT_GENERATE(CustomAttributeRow)

    public:

        TableReference GetParent() const;
        TableReference GetType()   const;
        BlobIndex      GetValue()  const;
    };

    class DeclSecurityRow
    {
        CXXREFLECT_GENERATE(DeclSecurityRow)

    public:

        std::uint16_t  GetAction()        const;
        TableReference GetParent()        const;
        BlobIndex      GetPermissionSet() const;
    };

    class EventMapRow
    {
        CXXREFLECT_GENERATE(EventMapRow)

    public:

        TableReference GetParent()   const;
        TableReference BeginEvents() const; // TODO??
        TableReference EndEvents()   const; // TODO??
    };

    class EventRow
    {
        CXXREFLECT_GENERATE(EventRow)

    public:

        EventFlags     GetFlags() const;
        String         GetName()  const;
        TableReference GetType()  const;
    };

    class ExportedTypeRow
    {
        CXXREFLECT_GENERATE(ExportedTypeRow)

    public:

        TypeFlags      GetFlags()          const;
        std::uint32_t  GetTypeDefId()      const;
        String         GetName()           const;
        String         GetNamespace()      const;
        TableReference GetImplementation() const;
    };

    class FieldRow
    {
        CXXREFLECT_GENERATE(FieldRow)

    public:

        FieldFlags GetFlags()     const;
        String     GetName()      const;
        BlobIndex  GetSignature() const;
    };

    class FieldLayoutRow
    {
        CXXREFLECT_GENERATE(FieldLayoutRow)

    public:

        std::uint32_t  GetOffset() const;
        TableReference GetField()  const;
    };

    class FieldMarshalRow
    {
        CXXREFLECT_GENERATE(FieldMarshalRow)

    public:

        TableReference GetParent()     const;
        BlobIndex      GetNativeType() const;
    };

    class FieldRvaRow
    {
        CXXREFLECT_GENERATE(FieldRvaRow)

    public:

        std::uint32_t  GetRva()   const;
        TableReference GetField() const;
    };

    class FileRow
    {
        CXXREFLECT_GENERATE(FileRow)

    public:

        FileFlags GetFlags()     const;
        String    GetName()      const;
        BlobIndex GetHashValue() const;
    };

    class GenericParamRow
    {
        CXXREFLECT_GENERATE(GenericParamRow)

    public:

        std::uint16_t         GetNumber() const;
        GenericParameterFlags GetFlags()  const;
        TableReference        GetOwner()  const;
        String                GetName()   const;
    };

    class GenericParamConstraintRow
    {
        CXXREFLECT_GENERATE(GenericParamConstraintRow)

    public:

        TableReference GetOwner()      const;
        TableReference GetConstraint() const;
    };

    class ImplMapRow
    {
        CXXREFLECT_GENERATE(ImplMapRow)

    public:

        PInvokeFlags   GetMappingFlags()    const;
        TableReference GetMemberForwarded() const;
        String         GetImportName()      const;
        TableReference GetImportScope()     const;
    };

    class InterfaceImplRow
    {
        CXXREFLECT_GENERATE(InterfaceImplRow)

    public:

        TableReference GetClass()     const;
        TableReference GetInterface() const;
    };

    class ManifestResourceRow
    {
        CXXREFLECT_GENERATE(ManifestResourceRow)

    public:

        std::uint32_t         GetOffset()         const;
        ManifestResourceFlags GetFlags()          const;
        String                GetName()           const;
        TableReference        GetImplementation() const;
    };

    class MemberRefRow
    {
        CXXREFLECT_GENERATE(MemberRefRow)

    public:

        TableReference GetClass()     const;
        String         GetName()      const;
        BlobIndex      GetSignature() const;
    };

    class MethodDefRow
    {
        CXXREFLECT_GENERATE(MethodDefRow)

    public:

        std::uint32_t             GetRva()                 const;
        MethodImplementationFlags GetImplementationFlags() const;
        MethodFlags               GetFlags()               const;
        String                    GetName()                const;
        BlobIndex                 GetSignature()           const;

        TableReference            GetFirstParameter()      const;
        TableReference            GetLastParameter()       const;
    };

    class MethodImplRow
    {
        CXXREFLECT_GENERATE(MethodImplRow)

    public:

        TableReference GetClass()             const;
        TableReference GetMethodBody()        const;
        TableReference GetMethodDeclaration() const;
    };

    class MethodSemanticsRow
    {
        CXXREFLECT_GENERATE(MethodSemanticsRow)

    public:

        MethodSemanticsFlags GetSemantics()   const;
        TableReference       GetMethod()      const;
        TableReference       GetAssociation() const;
    };

    class MethodSpecRow
    {
        CXXREFLECT_GENERATE(MethodSpecRow)

    public:

        TableReference GetMethod()        const;
        BlobIndex      GetInstantiation() const;
    };

    class ModuleRow
    {
        CXXREFLECT_GENERATE(ModuleRow)

    public:

        String GetName() const;
    };

    class ModuleRefRow
    {
        CXXREFLECT_GENERATE(ModuleRefRow)

    public:

        String GetName() const;
    };

    class NestedClassRow
    {
        CXXREFLECT_GENERATE(NestedClassRow)

    public:

        TableReference GetNestedClass()    const;
        TableReference GetEnclosingClass() const;
    };

    class ParamRow
    {
        CXXREFLECT_GENERATE(ParamRow)

    public:

        ParameterFlags GetFlags()    const;
        std::uint16_t  GetSequence() const;
        String         GetName()     const;
    };

    class PropertyRow
    {
        CXXREFLECT_GENERATE(PropertyRow)

    public:

        PropertyFlags GetFlags()     const;
        String        GetName()      const;
        BlobIndex     GetSignature() const;
    };

    class PropertyMapRow
    {
        CXXREFLECT_GENERATE(PropertyMapRow)

    public:

        TableReference GetParent()        const;
        TableReference GetFirstProperty() const;
        TableReference GetLastProperty()  const;
    };

    class StandaloneSigRow
    {
        CXXREFLECT_GENERATE(StandaloneSigRow)

    public:

        BlobIndex GetSignature() const;
    };

    class TypeDefRow
    {
        CXXREFLECT_GENERATE(TypeDefRow)

    public:

        TypeFlags      GetFlags()       const;
        String         GetName()        const;
        String         GetNamespace()   const;
        TableReference GetExtends()     const;

        TableReference GetFirstField()  const;
        TableReference GetLastField()   const;

        TableReference GetFirstMethod() const;
        TableReference GetLastMethod()  const;
    };

    class TypeRefRow
    {
        CXXREFLECT_GENERATE(TypeRefRow)

    public:

        TableReference GetResolutionScope() const;
        String         GetName()            const;
        String         GetNamespace()       const;
    };

    class TypeSpecRow
    {
        CXXREFLECT_GENERATE(TypeSpecRow)

    public:

        BlobIndex GetSignature() const;
    };

    #undef CXXREFLECT_GENERATE

} }

#endif
