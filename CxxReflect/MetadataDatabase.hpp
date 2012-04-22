#ifndef CXXREFLECT_METADATADATABASE_HPP_
#define CXXREFLECT_METADATADATABASE_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// The MetadataDatabase and its supporting classes provide a physical layer implementation for
// reading metadata from an assembly.  This file contains the core MetadataDatabase functionality
// for accessing tables and streams and for reading strings, GUIDs, and table rows.  The blob
// parsing functionality is in MetadataSignature.

#include "CxxReflect/MetadataCommon.hpp"

namespace CxxReflect { namespace Metadata {

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
        static std::array<Byte, 0x40> const mask =
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





    // Represents a reference to a row in a table.  This is effectively a metadata token, except we
    // adjust the index so that it is zero-based instead of one-based.  The invalid token value uses
    // all bits one instead of all bits zero.
    class RowReference
    {
    public:

        enum : std::uint32_t
        {
            InvalidValue     = static_cast<std::uint32_t>(-1),
            InvalidIndex     = static_cast<std::uint32_t>(-1),

            ValueTableIdMask = 0xff000000,
            ValueIndexMask   = 0x00ffffff,

            ValueTableIdBits = 8,
            ValueIndexBits   = 24
        };

        typedef std::uint32_t ValueType;
        typedef std::uint32_t TokenType;

        RowReference();
        RowReference(TableId tableId, SizeType index);

        TableId   GetTable() const;
        SizeType  GetIndex() const;
        ValueType GetValue() const;

        // The metadata token is the same as the value we store, except that it uses a one-based
        // indexing scheme rather than a zero-based indexing scheme.  We check in ComposeValue
        // to ensure that adding one here will not cause the index to overflow.
        TokenType GetToken() const;

        bool IsValid()       const;
        bool IsInitialized() const;

        friend bool operator==(RowReference const& lhs, RowReference const& rhs);
        friend bool operator< (RowReference const& lhs, RowReference const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(RowReference)
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(RowReference, _value.Get(), std::int32_t)

        static RowReference FromToken(TokenType const token);

    private:

        static ValueType ComposeValue(TableId const tableId, SizeType const index);

        void AssertInitialized() const;

        // The value is the composition of the table id in the high eight bits and the zero-based
        // index in the remaining 24 bits.  This is similar to a metadata token, but metadata tokens
        // use one-based indices.
        Detail::ValueInitialized<ValueType> _value;
    };





    // Represents a reference to a blob.
    class BlobReference
    {
    public:

        BlobReference();
        BlobReference(ConstByteIterator first, ConstByteIterator last);

        template <typename TSignature>
        explicit BlobReference(TSignature const& signature,
                               typename std::enable_if<std::is_class<TSignature>::value>::type* = nullptr)
            : _first(signature.BeginBytes()), _last(signature.EndBytes())
        {
        }

        ConstByteIterator Begin()         const;
        ConstByteIterator End()           const;
        bool              IsInitialized() const;

        template <typename TSignature>
        TSignature As() const
        {
            AssertInitialized();
            return TSignature(_first.Get(), _last.Get());
        }

        static BlobReference ComputeFromStream(ConstByteIterator first, ConstByteIterator last);

        friend bool operator==(BlobReference const& lhs, BlobReference const& rhs);
        friend bool operator< (BlobReference const& lhs, BlobReference const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(BlobReference)

    private:

        void AssertInitialized() const;

        Detail::ValueInitialized<ConstByteIterator> _first;
        Detail::ValueInitialized<ConstByteIterator> _last;
    };





    // Represents a reference to either a blob or a row.
    class BaseElementReference
    {
    public:

        enum : SizeType
        {
            InvalidElementSentinel = static_cast<SizeType>(-1)
        };

        BaseElementReference();
        BaseElementReference(RowReference  const& reference);
        BaseElementReference(BlobReference const& reference);

        bool IsRowReference()  const;
        bool IsBlobReference() const;

        bool IsValid()         const;
        bool IsInitialized()   const;

        RowReference  AsRowReference()  const;
        BlobReference AsBlobReference() const;

        friend bool operator==(BaseElementReference const& lhs, BaseElementReference const& rhs);
        friend bool operator< (BaseElementReference const& lhs, BaseElementReference const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(BaseElementReference)
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(BaseElementReference, _index, std::int32_t)

    protected:

        void AssertInitialized() const;

        enum : SizeType
        {
            KindMask     = 0x80000000,
            TableKindBit = 0x00000000,
            BlobKindBit  = 0x80000000
        };

        union
        {
            SizeType          _index;
            ConstByteIterator _first;
        };

        Detail::ValueInitialized<SizeType> _size;
    };

    class ElementReference
        : public BaseElementReference
    {
    public:

        ElementReference();
        ElementReference(RowReference  const& reference);
        ElementReference(BlobReference const& reference);

        // Note that the addition/subtraction operators are only usable for RowReferences.
        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(ElementReference)
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(ElementReference, _index, std::int32_t)
    };

    class FullReference
        : public BaseElementReference
    {
    public:

        FullReference();
        FullReference(Database const* database, RowReference     const& r);
        FullReference(Database const* database, BlobReference    const& r);
        FullReference(Database const* database, ElementReference const& r);

        Database const& GetDatabase() const;

        friend bool operator==(FullReference const& lhs, FullReference const& rhs);
        friend bool operator< (FullReference const& lhs, FullReference const& rhs);

        // Note that the addition/subtraction operators are only usable for RowReferences.
        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(FullReference)
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(FullReference, _index, std::int32_t)

    private:

        Detail::ValueInitialized<Database const*> _database;
    };

    





    // Represents a four-component assembly version number (major.minor.build.revision).
    class FourComponentVersion
    {
    public:

        typedef std::uint16_t Component;

        FourComponentVersion();
        FourComponentVersion(Component major, Component minor, Component build, Component revision);

        Component GetMajor()    const;
        Component GetMinor()    const;
        Component GetBuild()    const;
        Component GetRevision() const;

    private:

        Detail::ValueInitialized<Component> _major;
        Detail::ValueInitialized<Component> _minor;
        Detail::ValueInitialized<Component> _build;
        Detail::ValueInitialized<Component> _revision;
    };





    // Represents a metadata stream.  A metadata stream is a sequence of bytes in the assembly that
    // contains metadata.  When we are constructed, we bulk copy the entire sequence of bytes into
    // an array in memory, then provide access to that data via offsets into the stream.
    class Stream
    {
    public:

        Stream();
        Stream(Detail::FileHandle& file, SizeType metadataOffset, SizeType streamOffset, SizeType streamSize);
        Stream(Stream&& other);

        Stream& operator=(Stream&& other);

        ConstByteIterator Begin()            const;
        ConstByteIterator End()              const;
        SizeType          Size()             const;
        bool              IsInitialized()    const;

        ConstByteIterator At(SizeType index) const;

        template <typename T>
        T const& ReadAs(SizeType const index) const
        {
            AssertInitialized();
            Detail::Assert([&] { return (index + sizeof (T)) <= Size(); });
            return *reinterpret_cast<T const*>(_data.Begin() + index);
        }

        template <typename T>
        T const* ReinterpretAs(SizeType const index) const
        {
            AssertInitialized();
            Detail::Assert([&] { return index <= Size(); });
            return reinterpret_cast<T const*>(_data.Begin() + index);
        }

    private:

        // This class is movable and noncopyable.
        Stream(Stream const&);
        Stream& operator=(Stream const&);

        void AssertInitialized() const;

        Detail::FileRange                  _data;
    };





    // This provides a mapping between enumerators of the TableId enumeration and their corresponding
    // row types.  For each table, we forward-declare the row type, and specialize the two mapping
    // templates.
    template <typename TRow>
    struct RowTypeToTableId;

    template <TableId TId>
    struct TableIdToRowType;

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





    // This provides general-purpose access to a metadata table; it does not own the table data, it
    // is just a wrapper to provide convenient access for row offset computation and bounds checking.
    class Table
    {
    public:

        Table();
        Table(ConstByteIterator data, SizeType rowSize, SizeType rowCount, bool isSorted);

        ConstByteIterator Begin()       const;
        ConstByteIterator End()         const;
        bool              IsSorted()    const;
        SizeType          GetRowCount() const;
        SizeType          GetRowSize()  const;

        ConstByteIterator At(SizeType index) const;

        bool IsInitialized() const;

    private:

        void AssertInitialized() const;

        Detail::ValueInitialized<ConstByteIterator> _data;
        Detail::ValueInitialized<SizeType>          _rowSize;
        Detail::ValueInitialized<SizeType>          _rowCount;
        Detail::ValueInitialized<bool>              _isSorted;
    };





    // This encapsulates the tables of the metadata database; it owns the stream in which the tables
    // are stored and computes the offsets, row sizes, and other metadata about each table.
    class TableCollection
    {
    public:

        TableCollection();
        explicit TableCollection(Stream&& stream);
        TableCollection(TableCollection&& other);

        TableCollection& operator=(TableCollection&& other);

        void Swap(TableCollection& other);

        Table const& GetTable(TableId tableId) const;

        SizeType GetTableIndexSize(TableId tableId) const;
        SizeType GetCompositeIndexSize(CompositeIndex index) const;

        SizeType GetStringHeapIndexSize() const;
        SizeType GetGuidHeapIndexSize()   const;
        SizeType GetBlobHeapIndexSize()   const;

        SizeType GetTableColumnOffset(TableId tableId, SizeType column) const;

        bool IsInitialized() const;

    private:

        typedef std::array<Table, TableIdCount> TableSequence;

        enum { MaximumColumnCount = 8 };

        typedef std::array<SizeType, MaximumColumnCount>       ColumnOffsetSequence;
        typedef std::array<ColumnOffsetSequence, TableIdCount> TableColumnOffsetSequence;

        TableCollection(TableCollection const&);
        TableCollection& operator=(TableCollection const&);

        void AssertInitialized() const;

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

            TableColumnOffsetSequence _columnOffsets;

            CompositeIndexSizeArray _compositeIndexSizes;

            TableSequence _tables;
        };

        Stream                          _stream;
        Detail::ValueInitialized<State> _state;
    };





    // The StringCollection has a mutable cache, which requires us to use synchronization to access
    // it.  Because C++/CLI translation units do not support the C++11 synchronization primitives,
    // we use this implementation class with the pimpl idiom.
    class StringCollectionCache;





    // Encapsulates the strings stream, providing conversion from the raw UTF-8 strings to the
    // more convenient UTF-16 used by Windows.  It caches the transformed strings so that we can
    // just use references to the strings everywhere and not have to copy tons of data.
    class StringCollection
    {
    public:

        StringCollection();
        explicit StringCollection(Stream&& stream);
        StringCollection(StringCollection&& other);

        StringCollection& operator=(StringCollection&& other);

        ~StringCollection();

        StringReference At(SizeType index) const;

        bool IsInitialized() const;

    private:

        void AssertInitialized() const;

        std::unique_ptr<StringCollectionCache> _cache;
    };





    template <TableId TId>
    class RowIterator;





    // The core metadata database interface.  This loads the database from the assembly file and
    // initializes all of the data structures required for accessing the metadata.
    class Database
    {
    public:

        Database(String fileName);
        Database(Database&& other);

        Database& operator=(Database&& other);

        void Swap(Database& other);

        template <TableId TId> RowIterator<TId> Begin() const;
        template <TableId TId> RowIterator<TId> End()   const;

        template <TableId TId> typename TableIdToRowType<TId>::Type GetRow(SizeType                    index    ) const;
        template <TableId TId> typename TableIdToRowType<TId>::Type GetRow(RowReference         const& reference) const;
        template <TableId TId> typename TableIdToRowType<TId>::Type GetRow(BaseElementReference const& reference) const;

        StringReference GetString(SizeType index) const;

        TableCollection  const& GetTables()  const;
        StringCollection const& GetStrings() const;
        Stream           const& GetBlobs()   const;

        bool IsInitialized() const;

        friend bool operator==(Database const& lhs, Database const& rhs);
        friend bool operator< (Database const& lhs, Database const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Database)

    private:

        Database(Database const&);
        Database& operator=(Database const&);

        void AssertInitialized() const;

        String _fileName;

        Stream _blobStream;
        Stream _guidStream;

        StringCollection _strings;
        TableCollection  _tables;
    };





    TypeDefRow GetOwnerOfMethodDef(Database const& database, MethodDefRow const& methodDef);
    TypeDefRow GetOwnerOfField(Database const& database, FieldRow const& field);





    // This iterator type provides a random access container interface for the metadata database.
    template <TableId TId>
    class RowIterator
    {
    public:

        enum { TableId = TId };

        typedef std::random_access_iterator_tag          iterator_category;
        typedef DifferenceType                           difference_type;
        typedef typename TableIdToRowType<TId>::Type     value_type;
        typedef value_type                               reference;
        typedef Detail::Dereferenceable<value_type>      pointer;

        typedef value_type                               ValueType;
        typedef reference                                Reference;
        typedef pointer                                  Pointer;

        RowIterator()
        {
        }

        RowIterator(Database const* const database, SizeType const index)
            : _database(database), _index(index)
        {
            Detail::AssertNotNull(database);
            Detail::Assert([&]{ return index != BaseElementReference::InvalidElementSentinel; });
        }

        RowReference GetReference()  const { AssertInitialized(); return RowReference(TId, _index.Get()); } 

        Reference    Get()           const { return GetValue(); }
        Reference    operator*()     const { return GetValue(); }
        Pointer      operator->()    const { return GetValue(); }

        RowIterator& operator++()    { AssertInitialized(); ++_index.Get(); return *this; }
        RowIterator  operator++(int) { RowIterator const it(*this); ++*this; return it;   }

        RowIterator& operator--()    { AssertInitialized(); --_index.Get(); return *this; }
        RowIterator  operator--(int) { RowIterator const it(*this); --*this; return it;   }

        RowIterator& operator+=(DifferenceType const n)
        {
            AssertInitialized();
            _index.Get() = static_cast<SizeType>(static_cast<DifferenceType>(_index.Get()) + n);
            return *this;
        }
        RowIterator& operator-=(DifferenceType const n)
        {
            AssertInitialized();
            _index.Get() = static_cast<SizeType>(static_cast<DifferenceType>(_index.Get()) - n);
            return *this;
        }

        Reference operator[](DifferenceType const n) const
        {
            AssertInitialized();
            return _database->GetRow<ValueType>(_index.Get() + n);
        }

        friend RowIterator operator+(RowIterator it, DifferenceType const n) { return it +=  n; }
        friend RowIterator operator+(DifferenceType const n, RowIterator it) { return it +=  n; }
        friend RowIterator operator-(RowIterator it, DifferenceType const n) { return it += -n; }

        friend DifferenceType operator-(RowIterator const& lhs, RowIterator const& rhs)
        {
            AssertComparable(lhs, rhs);
            return lhs._index.Get() - rhs._index.Get();
        }

        friend bool operator==(RowIterator const& lhs, RowIterator const& rhs)
        {
            AssertComparable(lhs, rhs);
            return lhs._index.Get() == rhs._index.Get();
        }

        friend bool operator<(RowIterator const& lhs, RowIterator const& rhs)
        {
            lhs.AssertInitialized();
            rhs.AssertInitialized();
            AssertComparable(lhs, rhs);
            return lhs._index.Get() < rhs._index.Get();
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(RowIterator)

    private:

        static void AssertComparable(RowIterator const& lhs, RowIterator const& rhs)
        {
            Detail::Assert([&]{ return lhs._database.Get() == rhs._database.Get(); });
        }

        void AssertInitialized() const
        {
            Detail::Assert([&]{ return _database.Get() != nullptr; });
        }

        Reference GetValue() const
        {
            AssertInitialized();
            return _database.Get()->GetRow<TId>(_index.Get());
        }

        Detail::ValueInitialized<Database const*> _database;
        Detail::ValueInitialized<SizeType>        _index;
    };





    // This function serves as a pseudo-constructor for the Row classes, as a workaround for the
    // lack of inheriting constructors.  The BaseRow defines an Initialize() member function that
    // constructs the object, and this function is the only function that is befriended and which
    // can call it.
    template <typename TRow>
    TRow CreateRow(Database const* const database, ConstByteIterator const data)
    {
        TRow row;
        row.Initialize(database, data);
        return row;
    }

    template <TableId TTableId>
    class BaseRow
    {
    public:

        bool IsInitialized() const
        {
            return _database.Get() != nullptr && _data.Get() != nullptr;
        }

        Database const& GetDatabase() const
        {
            AssertInitialized();
            
            return *_database.Get();
        }

        RowReference GetSelfReference() const
        {
            AssertInitialized();

            Table    const& table(GetDatabase().GetTables().GetTable(TTableId));
            SizeType const  index(static_cast<SizeType>((GetIterator() - table.Begin()) / table.GetRowSize()));
            return RowReference(TTableId, index);
        }

    protected:

        ~BaseRow()
        {
        }

        ConstByteIterator GetIterator() const
        {
            AssertInitialized();
            
            return _data.Get();
        }

        void AssertInitialized() const
        {
            Detail::Assert([&]{ return IsInitialized(); });
        }

        SizeType GetColumnOffset(SizeType const column) const
        {
            AssertInitialized();

            return GetDatabase().GetTables().GetTableColumnOffset(TTableId, column);
        }

    private:

        template <typename TRow>
        friend TRow CreateRow(Database const*, ConstByteIterator);

        void Initialize(Database const* const database, ConstByteIterator const data)
        {
            Detail::AssertNotNull(database);
            Detail::AssertNotNull(data);
            Detail::Assert([&]{ return !IsInitialized(); });

            _database.Get() = database;
            _data.Get()     = data;
        }

        Detail::ValueInitialized<Database const*>   _database;
        Detail::ValueInitialized<ConstByteIterator> _data;
    };

    class AssemblyRow : public BaseRow<TableId::Assembly>
    {
    public:

        AssemblyHashAlgorithm GetHashAlgorithm() const;
        FourComponentVersion  GetVersion()       const;
        AssemblyFlags         GetFlags()         const;
        BlobReference         GetPublicKey()     const;
        StringReference       GetName()          const;
        StringReference       GetCulture()       const;
    };

    class AssemblyOsRow : public BaseRow<TableId::AssemblyOs>
    {
    public:

        std::uint32_t GetOsPlatformId()   const;
        std::uint32_t GetOsMajorVersion() const;
        std::uint32_t GetOsMinorVersion() const;
    };

    class AssemblyProcessorRow : public BaseRow<TableId::AssemblyProcessor>
    {
    public:

        std::uint32_t GetProcessor() const;
    };

    class AssemblyRefRow : public BaseRow<TableId::AssemblyRef>
    {
    public:

        FourComponentVersion GetVersion()   const;
        AssemblyFlags        GetFlags()     const;
        BlobReference        GetPublicKey() const;
        StringReference      GetName()      const;
        StringReference      GetCulture()   const;
        BlobReference        GetHashValue() const;
    };

    class AssemblyRefOsRow : public BaseRow<TableId::AssemblyRefOs>
    {
    public:

        std::uint32_t GetOsPlatformId()   const;
        std::uint32_t GetOsMajorVersion() const;
        std::uint32_t GetOsMinorVersion() const;
        RowReference  GetAssemblyRef()    const;
    };

    class AssemblyRefProcessorRow : public BaseRow<TableId::AssemblyRefProcessor>
    {
    public:

        std::uint32_t GetProcessor()   const;
        RowReference  GetAssemblyRef() const;
    };

    class ClassLayoutRow : public BaseRow<TableId::ClassLayout>
    {
    public:

        std::uint16_t GetPackingSize()   const;
        std::uint32_t GetClassSize()     const;
        RowReference  GetParentTypeDef() const;
    };

    class ConstantRow : public BaseRow<TableId::Constant>
    {
    public:

        std::uint8_t  GetType()   const;
        RowReference  GetParent() const;
        BlobReference GetValue()  const;
    };

    class CustomAttributeRow : public BaseRow<TableId::CustomAttribute>
    {
    public:

        RowReference  GetParent() const;
        RowReference  GetType()   const;
        BlobReference GetValue()  const;
    };

    class DeclSecurityRow : public BaseRow<TableId::DeclSecurity>
    {
    public:

        std::uint16_t GetAction()        const;
        RowReference  GetParent()        const;
        BlobReference GetPermissionSet() const;
    };

    class EventMapRow : public BaseRow<TableId::EventMap>
    {
    public:

        RowReference GetParent()     const;
        RowReference GetFirstEvent() const;
        RowReference GetLastEvent()  const;
    };

    class EventRow : public BaseRow<TableId::Event>
    {
    public:

        EventFlags      GetFlags() const;
        StringReference GetName()  const;
        RowReference    GetType()  const;
    };

    class ExportedTypeRow : public BaseRow<TableId::ExportedType>
    {
    public:

        TypeFlags       GetFlags()          const;
        std::uint32_t   GetTypeDefId()      const;
        StringReference GetName()           const;
        StringReference GetNamespace()      const;
        RowReference    GetImplementation() const;
    };

    class FieldRow : public BaseRow<TableId::Field>
    {
    public:

        FieldFlags      GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    class FieldLayoutRow : public BaseRow<TableId::FieldLayout>
    {
    public:

        std::uint32_t GetOffset() const;
        RowReference  GetField()  const;
    };

    class FieldMarshalRow : public BaseRow<TableId::FieldMarshal>
    {
    public:

        RowReference  GetParent()     const;
        BlobReference GetNativeType() const;
    };

    class FieldRvaRow : public BaseRow<TableId::FieldRva>
    {
    public:

        std::uint32_t GetRva()   const;
        RowReference  GetField() const;
    };

    class FileRow : public BaseRow<TableId::File>
    {
    public:

        FileFlags       GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetHashValue() const;
    };

    class GenericParamRow : public BaseRow<TableId::GenericParam>
    {
    public:

        std::uint16_t         GetNumber() const;
        GenericParameterFlags GetFlags()  const;
        RowReference          GetOwner()  const;
        StringReference       GetName()   const;
    };

    class GenericParamConstraintRow : public BaseRow<TableId::GenericParamConstraint>
    {
    public:

        RowReference GetOwner()      const;
        RowReference GetConstraint() const;
    };

    class ImplMapRow : public BaseRow<TableId::ImplMap>
    {
    public:

        PInvokeFlags    GetMappingFlags()    const;
        RowReference    GetMemberForwarded() const;
        StringReference GetImportName()      const;
        RowReference    GetImportScope()     const;
    };

    class InterfaceImplRow : public BaseRow<TableId::InterfaceImpl>
    {
    public:

        RowReference GetClass()     const;
        RowReference GetInterface() const;
    };

    class ManifestResourceRow : public BaseRow<TableId::ManifestResource>
    {
    public:

        std::uint32_t         GetOffset()         const;
        ManifestResourceFlags GetFlags()          const;
        StringReference       GetName()           const;
        RowReference          GetImplementation() const;
    };

    class MemberRefRow : public BaseRow<TableId::MemberRef>
    {
    public:

        RowReference    GetClass()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    class MethodDefRow : public BaseRow<TableId::MethodDef>
    {
    public:

        std::uint32_t             GetRva()                 const;
        MethodImplementationFlags GetImplementationFlags() const;
        MethodFlags               GetFlags()               const;
        StringReference           GetName()                const;
        BlobReference             GetSignature()           const;

        RowReference              GetFirstParameter()      const;
        RowReference              GetLastParameter()       const;
    };

    class MethodImplRow : public BaseRow<TableId::MethodImpl>
    {
    public:

        RowReference GetClass()             const;
        RowReference GetMethodBody()        const;
        RowReference GetMethodDeclaration() const;
    };

    class MethodSemanticsRow : public BaseRow<TableId::MethodSemantics>
    {
    public:

        MethodSemanticsFlags GetSemantics()   const;
        RowReference         GetMethod()      const;
        RowReference         GetAssociation() const;
    };

    class MethodSpecRow : public BaseRow<TableId::MethodSpec>
    {
    public:

        RowReference  GetMethod()        const;
        BlobReference GetInstantiation() const;
    };

    class ModuleRow : public BaseRow<TableId::Module>
    {
    public:

        StringReference GetName() const;
    };

    class ModuleRefRow : public BaseRow<TableId::ModuleRef>
    {
    public:

        StringReference GetName() const;
    };

    class NestedClassRow : public BaseRow<TableId::NestedClass>
    {
    public:

        RowReference GetNestedClass()    const;
        RowReference GetEnclosingClass() const;
    };

    class ParamRow : public BaseRow<TableId::Param>
    {
    public:

        ParameterFlags  GetFlags()    const;
        std::uint16_t   GetSequence() const;
        StringReference GetName()     const;
    };

    class PropertyRow : public BaseRow<TableId::Property>
    {
    public:

        PropertyFlags   GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    class PropertyMapRow : public BaseRow<TableId::PropertyMap>
    {
    public:

        RowReference GetParent()        const;
        RowReference GetFirstProperty() const;
        RowReference GetLastProperty()  const;
    };

    class StandaloneSigRow : public BaseRow<TableId::StandaloneSig>
    {
    public:

        BlobReference GetSignature() const;
    };

    class TypeDefRow : public BaseRow<TableId::TypeDef>
    {
    public:

        TypeFlags       GetFlags()       const;
        StringReference GetName()        const;
        StringReference GetNamespace()   const;
        RowReference    GetExtends()     const;

        RowReference    GetFirstField()  const;
        RowReference    GetLastField()   const;

        RowReference    GetFirstMethod() const;
        RowReference    GetLastMethod()  const;
    };

    class TypeRefRow : public BaseRow<TableId::TypeRef>
    {
    public:

        RowReference    GetResolutionScope() const;
        StringReference GetName()            const;
        StringReference GetNamespace()       const;
    };

    class TypeSpecRow : public BaseRow<TableId::TypeSpec>
    {
    public:

        BlobReference GetSignature() const;
    };

} }

#endif
