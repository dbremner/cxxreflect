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

    /// Identifiers for each of the tables in a metadata database.
    enum class TableId : Byte
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

    enum
    {
        /// One larger than the largest `TableId` enumerator value.
        ///
        /// This is not exactly the count of the `TableId` enumerators because there are unassigned
        /// values that are not used for any `TableId`.  However, this is a number that is large
        /// enough that it may be used to define an array such that a[TableId::{Enumerator}] is
        /// always a valid indexing expression.
        TableIdCount = 0x2d
    };

    typedef std::array<SizeType, TableIdCount> TableIdSizeArray;


    /// Tests whether the integer `id` maps to a valid `TableId` enumerator.
    ///
    /// \param    id The identifier to test.
    /// \returns  `true` if `id` is a valid enumerator value; `false` otherwise.
    /// \nothrows
    inline bool IsValidTableId(SizeType const id)
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

    /// Identifiers for each of the composite indices used in a metadata database.
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

    enum
    {
        /// The number of composite indices.
        CompositeIndexCount = 0x0d
    };

    typedef std::array<SizeType, CompositeIndexCount> CompositeIndexSizeArray;





    /// Represents a reference to a row in a metadata table.
    ///
    /// This is effectively a metadata token, except that we adjust the index so that it is zero-
    /// based instead of one-based.  The invalid token value uses all bits set to one instead of all
    /// bits set to zero (i.e., -1 converted to unsigned instead of 0).
    class RowReference
    {
    public:

        enum : SizeType
        {
            InvalidValue     = static_cast<SizeType>(-1),
            InvalidIndex     = static_cast<SizeType>(-1),

            ValueTableIdMask = 0xff000000,
            ValueIndexMask   = 0x00ffffff,

            ValueTableIdBits = 8,
            ValueIndexBits   = 24
        };

        typedef SizeType ValueType;
        typedef SizeType TokenType;

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





    /// Represents a reference to a blob.
    ///
    /// A blob may be contained in a metadata database, or it may be instantiated outside of a 
    /// metadata database, e.g. during generic type instantiation.
    class BlobReference
    {
    public:

        /// Constructs an uninitialized blob reference.
        BlobReference();

        /// Constructs a reference to a blob deliniated by the iterators `first` and `last`.
        BlobReference(ConstByteIterator first, ConstByteIterator last);

        /// Constructs a reference to a blob from a metadata signature.
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

    





    /// Represents a four-component version number (major, minor, build, and revision).
    class FourComponentVersion
    {
    public:

        /// Each component of the version number is a 16-bit unsigned integer.
        typedef std::uint16_t Component;

        /// Default constructs a `FourComponentVersion` with each component having a value of zero.
        ///
        /// \nothrows
        FourComponentVersion();

        /// Constructs a `FourComponentVersion` with the provided version number components.
        ///
        /// \param    major    The major component value.
        /// \param    minor    The minor component value.
        /// \param    build    The build component value.
        /// \param    revision The revision component value.
        /// \nothrows
        FourComponentVersion(Component major, Component minor, Component build, Component revision);

        /// \returns  The major component value.
        /// \nothrows
        Component GetMajor()    const;

        /// \returns  The minor component value.
        /// \nothrows
        Component GetMinor()    const;

        /// \returns  The build component value.
        /// \nothrows
        Component GetBuild()    const;

        /// \returns  The revision component value.
        /// \nothrows
        Component GetRevision() const;

    private:

        Detail::ValueInitialized<Component> _major;
        Detail::ValueInitialized<Component> _minor;
        Detail::ValueInitialized<Component> _build;
        Detail::ValueInitialized<Component> _revision;
    };





    /// Represents a stream in a metadata database.
    ///
    /// A metadata stream is a sequence of bytes in the assembly that contains metadata.  When a 
    /// `Stream` object is constructed, it copies the entire sequence of bytes into an array in
    /// memory (alternatively, memory-mapped I/O may be used).  The `Stream` class then provides
    /// access to the stream data via offsets into the stream.
    ///
    /// This type is moveable and noncopyable.
    ///
    /// \todo Ownership and lifetimes?
    class Stream
    {
    public:

        /// Constructs an uninitialized stream.  Calling any of the member functions on an
        /// uninitialized stream will cause a `LogicError` to be thrown.
        Stream();

        /// Constructs a stream from the provided file information.
        ///
        /// This constructor seeks in the `file` to the beginning of the metadata stream, as
        /// specified by the sum of `metadataOffset` and `streamOffset`, then reads `streamSize`
        /// bytes from the file and uses those to initialize the `Stream`.
        ///
        /// It is unspecified where the read cursor of `file` is located when this constructor ends.
        ///
        /// \param file           The file in which the metadata stream is located.
        /// \param metadataOffset The offset in the `file` at which the metadata database begins.
        /// \param streamOffset   The offset in the metadata at which the stream begins.
        /// \param streamSize     The size of the stream, in bytes.
        ///
        /// \throws LogicError  If `file` is not initialized.
        /// \throws FileIOError If the stream is not successfully read from the file, for any reason.
        Stream(Detail::ConstByteCursor file, SizeType metadataOffset, SizeType streamOffset, SizeType streamSize);

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

        void AssertInitialized() const;

        ConstByteRange _data;
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





    /// Represents a CLI metadata database.
    ///
    /// A `Database` represents a CLI assembly, which is a PE file that contains CLI metadata.  This
    /// class loads the database from the PE file and initializes all of the data structures required
    /// for accessing the metadata.
    class Database
    {
    public:

        /// Loads the file at the specified path into memory and constructs a `Database` from it.
        ///
        /// \param   path The full path to the manifest-containing assembly.
        /// \returns A `Database` loaded from the file.
        ///
        /// \todo    Figure out what this throws.
        static Database CreateFromFile(StringReference path);

        Database(Detail::FileRange&& file);
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

        Stream _blobStream;
        Stream _guidStream;

        StringCollection _strings;
        TableCollection  _tables;

        Detail::FileRange _file;
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





    /// A pseudo-constructor for the table row classes.
    ///
    /// This function serves as a pseudo-constructor for the row classes, as a workaround for the
    /// lack of inheriting constructors in Visual C++.  The `BaseRow<Id>` defines an `Initialize()`
    /// member function that constructs the object, and this function is the only function that is
    /// befriended and which can call it.
    ///
    /// \tparam  T        The type of row to be created.
    /// \param   database A pointer to the database that owns the row
    /// \param   data     A pointer to the initial byte of the row in the database
    /// \returns The constructed row of type `T`.
    /// \throws  LogicError If `database` or `data` is `nullptr`.
    template <typename TRow>
    TRow CreateRow(Database const* const database, ConstByteIterator const data)
    {
        TRow row;
        row.Initialize(database, data);
        return row;
    }

    /// Defines common functionality used by all of the row types.
    ///
    /// There is a 1:1 mapping between the concrete, derived row types and `BaseRow<T>` template
    /// instantiations.  Each derived class instantiates `BaseRow<T>` with its corresponding
    /// `TableId` enumerator.  This is used whenever the row is re-resolved in its owning database.
    template <TableId TTableId>
    class BaseRow
    {
    public:

        /// Tests whether the row is initialized.
        ///
        /// \returns  `true` if the row is initialized; `false` otherwise.
        /// \nothrows
        bool IsInitialized() const
        {
            return _database.Get() != nullptr && _data.Get() != nullptr;
        }

        /// Gets the `Database` that owns the row.
        ///
        /// \returns The `Database` that owns the row.
        /// \throws  LogicError if the row is not initialized.
        Database const& GetDatabase() const
        {
            AssertInitialized();
            
            return *_database.Get();
        }

        /// Gets a `RowReference` that refers to this row.
        ///
        /// The `RowReference` returned by this function can be round-tripped through the database
        /// returned by `GetDatabase()`.
        ///
        /// \returns A `RowReference` that refers to this row.
        /// \throws  LogicError if the row is not initialized.
        RowReference GetSelfReference() const
        {
            AssertInitialized();

            Table    const& table(GetDatabase().GetTables().GetTable(TTableId));
            SizeType const  index(static_cast<SizeType>((GetIterator() - table.Begin()) / table.GetRowSize()));
            return RowReference(TTableId, index);
        }

    protected:

        /// Protected destructor.
        ///
        /// This class template is not intended to be used directly, so we prevent polymorphic
        /// destruction by making the destructor protected.
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

        /// Gets the offset of a particular row, in bytes, from the initial byte of this row.
        ///
        /// \param   column The column for which to get the offset.
        /// \returns The offset of the column, in bytes.
        /// \throws  LogicError If this object is not initialized, if the metadata database is not
        ///          fully initialized, or if the column number is greater than the number of
        ///          columns in the table.
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

    /// Represents a row in the Assembly table.
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

    /// Represents a row in the AssemblyOS table.
    class AssemblyOsRow : public BaseRow<TableId::AssemblyOs>
    {
    public:

        std::uint32_t GetOsPlatformId()   const;
        std::uint32_t GetOsMajorVersion() const;
        std::uint32_t GetOsMinorVersion() const;
    };

    /// Represents a row in the Assembly Processor table.
    class AssemblyProcessorRow : public BaseRow<TableId::AssemblyProcessor>
    {
    public:

        std::uint32_t GetProcessor() const;
    };

    /// Represents a row in the AssemblyRef table.
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

    /// Represents a row in the AssemblyRefOS table.
    class AssemblyRefOsRow : public BaseRow<TableId::AssemblyRefOs>
    {
    public:

        std::uint32_t GetOsPlatformId()   const;
        std::uint32_t GetOsMajorVersion() const;
        std::uint32_t GetOsMinorVersion() const;
        RowReference  GetAssemblyRef()    const;
    };

    /// Represents a row in the AssemblyRef Processor table.
    class AssemblyRefProcessorRow : public BaseRow<TableId::AssemblyRefProcessor>
    {
    public:

        std::uint32_t GetProcessor()   const;
        RowReference  GetAssemblyRef() const;
    };

    /// Represents a row in the ClassLayout table.
    class ClassLayoutRow : public BaseRow<TableId::ClassLayout>
    {
    public:

        std::uint16_t GetPackingSize()   const;
        std::uint32_t GetClassSize()     const;
        RowReference  GetParentTypeDef() const;
    };

    /// Represents a row in the Constant table.
    class ConstantRow : public BaseRow<TableId::Constant>
    {
    public:

        std::uint8_t  GetType()   const;
        RowReference  GetParent() const;
        BlobReference GetValue()  const;
    };

    /// Represents a row in the CustomAttribute table.
    class CustomAttributeRow : public BaseRow<TableId::CustomAttribute>
    {
    public:

        RowReference  GetParent() const;
        RowReference  GetType()   const;
        BlobReference GetValue()  const;
    };

    /// Represents a row in the DeclSecurity table.
    class DeclSecurityRow : public BaseRow<TableId::DeclSecurity>
    {
    public:

        std::uint16_t GetAction()        const;
        RowReference  GetParent()        const;
        BlobReference GetPermissionSet() const;
    };

    /// Represents a row in the EventMap table.
    class EventMapRow : public BaseRow<TableId::EventMap>
    {
    public:

        RowReference GetParent()     const;
        RowReference GetFirstEvent() const;
        RowReference GetLastEvent()  const;
    };

    /// Represents a row in the Event table.
    class EventRow : public BaseRow<TableId::Event>
    {
    public:

        EventFlags      GetFlags() const;
        StringReference GetName()  const;
        RowReference    GetType()  const;
    };

    /// Represents a row in the ExportedType table.
    class ExportedTypeRow : public BaseRow<TableId::ExportedType>
    {
    public:

        TypeFlags       GetFlags()          const;
        std::uint32_t   GetTypeDefId()      const;
        StringReference GetName()           const;
        StringReference GetNamespace()      const;
        RowReference    GetImplementation() const;
    };

    /// Represents a row in the Field table.
    class FieldRow : public BaseRow<TableId::Field>
    {
    public:

        FieldFlags      GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    /// Represents a row in the FieldLayout table.
    class FieldLayoutRow : public BaseRow<TableId::FieldLayout>
    {
    public:

        std::uint32_t GetOffset() const;
        RowReference  GetField()  const;
    };

    /// Represents a row in the FieldMarshal table.
    class FieldMarshalRow : public BaseRow<TableId::FieldMarshal>
    {
    public:

        RowReference  GetParent()     const;
        BlobReference GetNativeType() const;
    };

    /// Represents a row in the Field RVA table.
    class FieldRvaRow : public BaseRow<TableId::FieldRva>
    {
    public:

        std::uint32_t GetRva()   const;
        RowReference  GetField() const;
    };

    /// Represents a row in the File table.
    class FileRow : public BaseRow<TableId::File>
    {
    public:

        FileFlags       GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetHashValue() const;
    };

    /// Represents a row in the GenericParam table.
    class GenericParamRow : public BaseRow<TableId::GenericParam>
    {
    public:

        std::uint16_t         GetNumber() const;
        GenericParameterFlags GetFlags()  const;
        RowReference          GetOwner()  const;
        StringReference       GetName()   const;
    };

    /// Represents a row in the GenericParamConstraint table.
    class GenericParamConstraintRow : public BaseRow<TableId::GenericParamConstraint>
    {
    public:

        RowReference GetOwner()      const;
        RowReference GetConstraint() const;
    };

    /// Represents a row in the ImplMap table.
    class ImplMapRow : public BaseRow<TableId::ImplMap>
    {
    public:

        PInvokeFlags    GetMappingFlags()    const;
        RowReference    GetMemberForwarded() const;
        StringReference GetImportName()      const;
        RowReference    GetImportScope()     const;
    };

    /// Represents a row in the InterfaceImpl table.
    class InterfaceImplRow : public BaseRow<TableId::InterfaceImpl>
    {
    public:

        RowReference GetClass()     const;
        RowReference GetInterface() const;
    };

    /// Represents a row in the ManifestResource table.
    class ManifestResourceRow : public BaseRow<TableId::ManifestResource>
    {
    public:

        std::uint32_t         GetOffset()         const;
        ManifestResourceFlags GetFlags()          const;
        StringReference       GetName()           const;
        RowReference          GetImplementation() const;
    };

    /// Represents a row in the MemberRef table.
    class MemberRefRow : public BaseRow<TableId::MemberRef>
    {
    public:

        RowReference    GetClass()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    /// Represents a row in the MethodDef table.
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

    /// Represents a row in the MethodImpl table.
    class MethodImplRow : public BaseRow<TableId::MethodImpl>
    {
    public:

        RowReference GetClass()             const;
        RowReference GetMethodBody()        const;
        RowReference GetMethodDeclaration() const;
    };

    /// Represents a row in the MethodSemantics table.
    class MethodSemanticsRow : public BaseRow<TableId::MethodSemantics>
    {
    public:

        MethodSemanticsFlags GetSemantics()   const;
        RowReference         GetMethod()      const;
        RowReference         GetAssociation() const;
    };

    /// Represents a row in the MethodSpec table.
    class MethodSpecRow : public BaseRow<TableId::MethodSpec>
    {
    public:

        RowReference  GetMethod()        const;
        BlobReference GetInstantiation() const;
    };

    /// Represents a row in the Module table.
    class ModuleRow : public BaseRow<TableId::Module>
    {
    public:

        StringReference GetName() const;
    };

    /// Represents a row in the ModuleRef table.
    class ModuleRefRow : public BaseRow<TableId::ModuleRef>
    {
    public:

        StringReference GetName() const;
    };

    /// Represents a row in the NestedClass table.
    class NestedClassRow : public BaseRow<TableId::NestedClass>
    {
    public:

        RowReference GetNestedClass()    const;
        RowReference GetEnclosingClass() const;
    };

    /// Represents a row in the Param table.
    class ParamRow : public BaseRow<TableId::Param>
    {
    public:

        ParameterFlags  GetFlags()    const;
        std::uint16_t   GetSequence() const;
        StringReference GetName()     const;
    };

    /// Represents a row in the Property table.
    class PropertyRow : public BaseRow<TableId::Property>
    {
    public:

        PropertyFlags   GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    /// Represents a row in the PropertyMap table.
    class PropertyMapRow : public BaseRow<TableId::PropertyMap>
    {
    public:

        RowReference GetParent()        const;
        RowReference GetFirstProperty() const;
        RowReference GetLastProperty()  const;
    };

    /// Represents a row in the StandaloneSig table.
    class StandaloneSigRow : public BaseRow<TableId::StandaloneSig>
    {
    public:

        BlobReference GetSignature() const;
    };

    /// Represents a row in the TypeDef table.
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

    /// Represents a row in the TypeRef table.
    class TypeRefRow : public BaseRow<TableId::TypeRef>
    {
    public:

        RowReference    GetResolutionScope() const;
        StringReference GetName()            const;
        StringReference GetNamespace()       const;
    };

    /// Represents a row in the TypeSpec table.
    class TypeSpecRow : public BaseRow<TableId::TypeSpec>
    {
    public:

        BlobReference GetSignature() const;
    };

} }

#endif
