//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// The MetadataDatabase and its supporting classes provide a physical layer implementation for
// reading metadata from an assembly.  This file contains the core MetadataDatabase functionality
// for accessing tables and streams and for reading strings, GUIDs, and table rows.  The blob
// parsing functionality is in MetadataBlob.
#ifndef CXXREFLECT_METADATADATABASE_HPP_
#define CXXREFLECT_METADATADATABASE_HPP_

#include "CxxReflect/Core.hpp"

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




    // Represents a reference into a table.  This is effectively a metadata token, except we adjust
    // the index so that it is zero-based instead of one-based.  We represent the invalid token
    // using all bits one instead of all bits zero.
    class TableReference
        : Detail::Comparable<TableReference>
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

        TableReference()
            : _value(InvalidValue)
        {
        }

        TableReference(TableId const tableId, IndexType const index)
            : _value(ComposeValue(tableId, index))
        {
        }

        TableId GetTable() const
        {
            return static_cast<TableId>((_value & ValueTableIdMask) >> ValueIndexBits);
        }

        IndexType GetIndex() const { return _value & ValueIndexMask; }
        ValueType GetValue() const { return _value;                  }

        // The metadata token is the same as the value we store, except that it uses a one-based
        // indexing scheme rather than a zero-based indexing scheme.  We check in ComposeValue
        // to ensure that adding one here will not cause the index to overflow.
        TokenType GetToken() const { return _value + 1;              }

        bool IsValid()       const { return _value != InvalidValue;  }
        bool IsInitialized() const { return _value != InvalidValue;  }

        friend bool operator==(TableReference const& lhs, TableReference const& rhs)
        {
            return lhs._value == rhs._value;
        }

        friend bool operator<(TableReference const& lhs, TableReference const& rhs)
        {
            return lhs._value < rhs._value;
        }

        static TableReference FromToken(TokenType const token);

    private:

        static ValueType ComposeValue(TableId const tableId, IndexType const index);

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

        // The value is the composition of the table id in the high eight bits and the zero-based
        // index in the remaining 24 bits.  This is similar to a metadata token, but metadata tokens
        // use one-based indices.
        ValueType _value;
    };




    // Represents a reference into the blob stream.  TODO What functionality are we going to put
    // in here, and how are we going to expose it?
    class BlobReference
        : Detail::Comparable<BlobReference>
    {
    public:

        enum : SizeType
        {
            InvalidIndex = static_cast<std::uint32_t>(-1)
        };

        BlobReference()
            : _index(InvalidIndex)
        {
        }

        explicit BlobReference(SizeType const index, SizeType const size = 0)
            : _index(index), _size(size)
        {
            VerifyInitialized();
        }

        SizeType GetIndex() const { VerifyInitialized(); return _index.Get(); }
        SizeType GetSize()  const { VerifyInitialized(); return _size.Get();  }

        bool IsValid() const
        {
            return _index.Get() != InvalidIndex;
        }

        bool IsInitialized() const
        {
            return _index.Get() != InvalidIndex;
        }

        friend bool operator==(BlobReference const& lhs, BlobReference const& rhs)
        {
            return lhs._index.Get() == rhs._index.Get();
        }

        friend bool operator<(BlobReference const& lhs, BlobReference const& rhs)
        {
            return lhs._index.Get() < rhs._index.Get();
        }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); });
        }

        Detail::ValueInitialized<SizeType> _index;
        Detail::ValueInitialized<SizeType> _size;
    };




    // A space-efficient discriminated union that can hold a TableReference or a BlobReference.  It
    // relies on the fact that the high bit in a table index or a blob index is never set, so we can
    // store the "kind tag" in the high bit.
    class TableOrBlobReference
        : Detail::Comparable<TableOrBlobReference>
    {
    public:

        enum : std::uint32_t
        {
            InvalidIndex = static_cast<IndexType>(-1)
        };

        typedef std::uint32_t ValueType;

        TableOrBlobReference()
            : _index(InvalidIndex)
        {
        }

        TableOrBlobReference(TableReference const reference)
            : _index((reference.GetToken() & ~KindMask) | TableKindBit)
        {
            Detail::Verify([&]{ return (reference.GetToken() & ~KindMask) == reference.GetToken(); });
        }

        TableOrBlobReference(BlobReference const reference)
            : _index((reference.GetIndex() & ~KindMask) | BlobKindBit), _size(reference.GetSize())
        {
            Detail::Verify([&]{ return (reference.GetIndex() & ~KindMask) == reference.GetIndex(); });
        }

        TableReference AsTableReference() const
        {
            Detail::Verify([&]{ return IsTableReference(); });
            return TableReference::FromToken(_index.Get() & ~KindMask);
        }

        BlobReference AsBlobReference() const
        {
            Detail::Verify([&]{ return IsBlobReference(); });
            return BlobReference(_index.Get() & ~KindMask, _size.Get());
        }

        bool IsTableReference() const { return IsValid() && (_index.Get() & KindMask) == TableKindBit; }
        bool IsBlobReference()  const { return IsValid() && (_index.Get() & KindMask) == BlobKindBit;  }

        bool IsValid()       const { return _index.Get() != InvalidIndex; }
        bool IsInitialized() const { return _index.Get() != InvalidIndex; }

        friend bool operator==(TableOrBlobReference const& lhs, TableOrBlobReference const& rhs)
        {
            return lhs._index.Get() == rhs._index.Get();
        }

        friend bool operator<(TableOrBlobReference const& lhs, TableOrBlobReference const& rhs)
        {
            return lhs._index.Get() < rhs._index.Get();
        }

    private:

        enum : SizeType
        {
            KindMask     = 0x80000000,
            TableKindBit = 0x00000000,
            BlobKindBit  = 0x80000000
        };

        Detail::ValueInitialized<SizeType> _index;
        Detail::ValueInitialized<SizeType> _size;
    };




    // Represents a TableReference along with a reference to the database in which the row to which
    // the TableReference refers is located.
    class DatabaseReference
    {
    public:

        DatabaseReference() { }

        DatabaseReference(Database const* database, TableReference const& tableReference)
            : _database(database), _tableReference(tableReference)
        {
            VerifyInitialized();
        }

        Database       const& GetDatabase()       const { VerifyInitialized(); return *_database.Get(); }
        TableReference const& GetTableReference() const { VerifyInitialized(); return _tableReference;  }

        bool IsInitialized() const
        {
            return _database.Get() != nullptr && _tableReference.IsInitialized();
        }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

        Detail::ValueInitialized<Database const*> _database;
        TableReference                            _tableReference;
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

        void Swap(Stream& other);

        ByteIterator  Begin()         const { VerifyInitialized(); return _data.get();               }
        ByteIterator  End()           const { VerifyInitialized(); return _data.get() + _size.Get(); }
        SizeType      Size()          const { VerifyInitialized(); return _size.Get();               }
        bool          IsInitialized() const {                      return _data.get() != nullptr;    }

        ByteIterator At(SizeType const index) const
        {
            VerifyInitialized();
            Detail::Verify([&] { return index <= _size.Get(); });
            return _data.get() + index;
        }

        template <typename T>
        T const& ReadAs(SizeType const index) const
        {
            VerifyInitialized();
            Detail::Verify([&] { return (index + sizeof (T)) <= _size.Get(); });
            return *reinterpret_cast<T const*>(_data.get() + index);
        }

        template <typename T>
        T const* ReinterpretAs(SizeType const index) const
        {
            VerifyInitialized();
            Detail::Verify([&] { return index <= _size.Get(); });
            return reinterpret_cast<T const*>(_data.get() + index);
        }

    private:

        Stream(Stream const&);
        Stream& operator=(Stream const&);

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Stream is not initialized");
        }

        std::unique_ptr<Byte[]>            _data;
        Detail::ValueInitialized<SizeType> _size;
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




    // Represents a Blob in the metadata database
    class Blob
    {
    public:

        Blob()
        {
        }

        // Note that this 'last' is not the end of the blob, it is the end of the whole blob stream.
        Blob(ByteIterator const first, ByteIterator const last, SizeType const size = 0)
        {
            Detail::VerifyNotNull(first);
            Detail::VerifyNotNull(last);

            std::tie(_first.Get(), _last.Get()) = ComputeBounds(first, last, size);
        }

        ByteIterator Begin()   const { VerifyInitialized(); return _first.Get(); }
        ByteIterator End()     const { VerifyInitialized(); return _last.Get();  }

        SizeType GetSize() const
        {
            VerifyInitialized();
            return static_cast<SizeType>(_last.Get() - _first.Get());
        }

        bool IsInitialized() const
        {
            return _first.Get() != nullptr && _last.Get() != nullptr;
        }

        template <typename TSignature>
        TSignature As() const
        {
            VerifyInitialized();
            return TSignature(_first.Get(), _last.Get());
        }

    private:

        typedef std::pair<ByteIterator, ByteIterator> Range;

        static Range ComputeBounds(ByteIterator const first, ByteIterator const last, SizeType const size);

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };




    // This provides general-purpose access to a metadata table; it does not own the table data, it
    // is just a wrapper to provide convenient access for row offset computation and bounds checking.
    class Table
    {
    public:

        Table()
        {
        }

        Table(ByteIterator const data, SizeType const rowSize, SizeType const rowCount, bool const isSorted)
            : _data(data), _rowSize(rowSize), _rowCount(rowCount), _isSorted(isSorted)
        {
            Detail::VerifyNotNull(data);
            Detail::Verify([&]{ return rowSize != 0 && rowCount != 0; });
        }

        ByteIterator Begin()       const { return _data.Get();                                    }
        ByteIterator End()         const { return _data.Get() + _rowCount.Get() * _rowSize.Get(); }
        bool         IsSorted()    const { return _isSorted.Get();                                }
        SizeType     GetRowCount() const { return _rowCount.Get();                                }
        SizeType     GetRowSize()  const { return _rowSize.Get();                                 }

        ByteIterator At(SizeType const index) const
        {
            VerifyInitialized();

            // Row identifiers are one-based, not zero-based, so <= is correct here.
            Detail::Verify([&] { return index <= _rowCount.Get(); }, "Index out of range");
            return _data.Get() + _rowSize.Get() * index;
        }

        bool IsInitialized() const
        {
            return _data.Get() != nullptr;
        }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

        Detail::ValueInitialized<ByteIterator> _data;
        Detail::ValueInitialized<SizeType>     _rowSize;
        Detail::ValueInitialized<SizeType>     _rowCount;
        Detail::ValueInitialized<bool>         _isSorted;
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

        SizeType GetStringHeapIndexSize() const { return _state.Get()._stringHeapIndexSize; }
        SizeType GetGuidHeapIndexSize()   const { return _state.Get()._guidHeapIndexSize;   }
        SizeType GetBlobHeapIndexSize()   const { return _state.Get()._blobHeapIndexSize;   }

        SizeType GetTableColumnOffset(TableId tableId, SizeType column) const;

        bool IsInitialized() const
        {
            return _stream.IsInitialized();
        }

    private:

        typedef std::array<Table, TableIdCount> TableSequence;

        enum { MaximumColumnCount = 8 };

        typedef std::array<SizeType, MaximumColumnCount>       ColumnOffsetSequence;
        typedef std::array<ColumnOffsetSequence, TableIdCount> TableColumnOffsetSequence;

        TableCollection(TableCollection const&);
        TableCollection& operator=(TableCollection const&);

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

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

        Stream                           _stream;
        Detail::ValueInitialized<bool>   _initialized;
        Detail::ValueInitialized<State>  _state;
    };




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

        void Swap(StringCollection& other);

        StringReference At(SizeType index) const;

        bool IsInitialized() const;

    private:

        typedef Detail::LinearArrayAllocator<Character, (1 << 16)> Allocator;
        typedef std::map<SizeType, StringReference>                StringMap;

        StringCollection(StringCollection const&);
        StringCollection& operator=(StringCollection const&);

        void VerifyInitialized() const;

        Stream            _stream;
        mutable Allocator _buffer; // Stores the transformed UTF-16 strings
        mutable StringMap _index;  // Maps string heap indices into indices in the buffer
    };




    template <TableId TId>
    class RowIterator;




    // The core metadata database interface.  This loads the database from the assembly file and
    // initializes all of the data structures required for accessing the metadata.
    class Database
        : public Detail::Comparable<Database>
    {
    public:

        Database(String fileName);
        Database(Database&& other);

        Database& operator=(Database&& other);

        void Swap(Database& other);

        template <TableId TId>
        RowIterator<TId> Begin() const
        {
            return RowIterator<TId>(this, 0);
        }

        template <TableId TId>
        RowIterator<TId> End() const
        {
            return RowIterator<TId>(this, _tables.GetTable(TId).GetRowCount());
        }

        template <TableId TId>
        typename TableIdToRowType<TId>::Type GetRow(SizeType const index) const
        {
            typedef typename TableIdToRowType<TId>::Type ReturnType;
            return CreateRow<ReturnType>(this, _tables.GetTable(TId).At(index));
        }

        Blob GetBlob(BlobReference const blobReference) const
        {
            return Blob(_blobStream.At(blobReference.GetIndex()), _blobStream.End(), blobReference.GetSize());
        }

        StringReference GetString(SizeType const index) const
        {
            return StringReference(_strings.At(index));
        }

        TableCollection  const& GetTables()  const { return _tables;     }
        StringCollection const& GetStrings() const { return _strings;    }
        Stream           const& GetBlobs()   const { return _blobStream; }

        bool IsInitialized() const
        {
            return _blobStream.IsInitialized()
                && _guidStream.IsInitialized()
                && _strings.IsInitialized()
                && _tables.IsInitialized();
        }

        // Every Database object is unique, so we can compare via address:
        friend bool operator==(Database const& lhs, Database const& rhs)
        {
            return &lhs == &rhs;
        }

        friend bool operator<(Database const& lhs, Database const& rhs)
        {
            return std::less<Database const*>()(&lhs, &rhs);
        }

    private:

        Database(Database const&);
        Database& operator=(Database const&);

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

        String _fileName;

        Stream _blobStream;
        Stream _guidStream;

        StringCollection _strings;
        TableCollection  _tables;
    };




    // This iterator type provides a random access container interface for the metadata database.
    template <TableId TId>
    class RowIterator
        : Detail::Comparable<RowIterator<TId>>
    {
    public:

        enum { TableId = TId };

        typedef std::random_access_iterator_tag          iterator_category;
        typedef std::ptrdiff_t                           difference_type;
        typedef typename TableIdToRowType<TId>::Type     value_type;
        typedef value_type                               reference;
        typedef Detail::Dereferenceable<value_type>      pointer;

        typedef std::size_t                              SizeType;
        typedef difference_type                          DifferenceType;
        typedef value_type                               ValueType;
        typedef reference                                Reference;
        typedef pointer                                  Pointer;

        RowIterator()
        {
        }

        RowIterator(Database const* const database, IndexType const index)
            : _database(database), _index(index)
        {
            Detail::VerifyNotNull(database);
            Detail::Verify([&]{ return index != static_cast<IndexType>(-1); });
        }

        Reference    Get()           const { return _database.Get()->GetRow<TId>(_index.Get()); }
        Reference    operator*()     const { return _database.Get()->GetRow<TId>(_index.Get()); }
        Pointer      operator->()    const { return _database.Get()->GetRow<TId>(_index.Get()); }

        RowIterator& operator++()    { ++_index.Get(); return *this;                }
        RowIterator  operator++(int) { RowIterator it(*this); ++*this; return it;   }

        RowIterator& operator--()    { --_index.Get(); return *this;                }
        RowIterator  operator--(int) { RowIterator it(*this); --*this; return it;   }

        RowIterator& operator+=(DifferenceType const n) { _index.Get() += n; return *this; }
        RowIterator& operator-=(DifferenceType const n) { _index.Get() -= n; return *this; }

        Reference operator[](DifferenceType const n) const
        {
            return _database->GetRow<ValueType>(_index.Get() + n);
        }

        friend RowIterator operator+(RowIterator it, DifferenceType const n) { return it +=  n; }
        friend RowIterator operator+(DifferenceType const n, RowIterator it) { return it +=  n; }
        friend RowIterator operator-(RowIterator it, DifferenceType const n) { return it += -n; }

        friend DifferenceType operator-(RowIterator const& lhs, RowIterator const& rhs)
        {
            VerifyComparable(lhs, rhs);
            return lhs._index.Get() - rhs._index.Get();
        }

        friend bool operator==(RowIterator const& lhs, RowIterator const& rhs)
        {
            VerifyComparable(lhs, rhs);
            return lhs._index.Get() == rhs._index.Get();
        }

        friend bool operator<(RowIterator const& lhs, RowIterator const& rhs)
        {
            VerifyComparable(lhs, rhs);
            return lhs._index.Get() < rhs._index.Get();
        }

    private:

        static void VerifyComparable(RowIterator const& lhs, RowIterator const& rhs)
        {
            Detail::Verify([&]{ return lhs._database.Get() == rhs._database.Get(); });
        }

        Detail::ValueInitialized<Database const*> _database;
        Detail::ValueInitialized<IndexType>       _index;
    };




    template <typename TRow>
    TRow CreateRow(Database const* const database, ByteIterator const data)
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

        TableReference GetSelfReference() const
        {
            VerifyInitialized();

            Table    const& table(GetDatabase().GetTables().GetTable(TTableId));
            SizeType const  index((GetIterator() - table.Begin()) / table.GetRowSize());
            return TableReference(TTableId, index);
        }

    protected:

        ~BaseRow()
        {
        }

        Database const& GetDatabase() const { VerifyInitialized(); return *_database.Get(); }
        ByteIterator    GetIterator() const { VerifyInitialized(); return _data.Get();      }

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

        SizeType GetColumnOffset(SizeType const column) const
        {
            VerifyInitialized();

            return GetDatabase().GetTables().GetTableColumnOffset(TTableId, column);
        }

    private:

        template <typename TRow>
        friend TRow CreateRow(Database const*, ByteIterator);

        void Initialize(Database const* database, std::uint8_t const* data)
        {
            Detail::VerifyNotNull(database);
            Detail::VerifyNotNull(data);
            Detail::Verify([&]{ return !IsInitialized(); });

            _database.Get() = database;
            _data.Get()     = data;
        }

        Detail::ValueInitialized<Database const*> _database;
        Detail::ValueInitialized<ByteIterator>    _data;
    };

    class AssemblyRow : public BaseRow<TableId::Assembly>
    {
    public:

        AssemblyHashAlgorithm GetHashAlgorithm() const;
        Version               GetVersion()       const;
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

        Version         GetVersion()   const;
        AssemblyFlags   GetFlags()     const;
        BlobReference   GetPublicKey() const;
        StringReference GetName()      const;
        StringReference GetCulture()   const;
        BlobReference   GetHashValue() const;
    };

    class AssemblyRefOsRow : public BaseRow<TableId::AssemblyRefOs>
    {
    public:

        std::uint32_t  GetOsPlatformId()   const;
        std::uint32_t  GetOsMajorVersion() const;
        std::uint32_t  GetOsMinorVersion() const;
        TableReference GetAssemblyRef()    const;
    };

    class AssemblyRefProcessorRow : public BaseRow<TableId::AssemblyRefProcessor>
    {
    public:

        std::uint32_t  GetProcessor()   const;
        TableReference GetAssemblyRef() const;
    };

    class ClassLayoutRow : public BaseRow<TableId::ClassLayout>
    {
    public:

        std::uint16_t  GetPackingSize()   const;
        std::uint32_t  GetClassSize()     const;
        TableReference GetParentTypeDef() const;
    };

    class ConstantRow : public BaseRow<TableId::Constant>
    {
    public:

        std::uint8_t   GetType()   const;
        TableReference GetParent() const;
        BlobReference  GetValue()  const;
    };

    class CustomAttributeRow : public BaseRow<TableId::CustomAttribute>
    {
    public:

        TableReference GetParent() const;
        TableReference GetType()   const;
        BlobReference  GetValue()  const;
    };

    class DeclSecurityRow : public BaseRow<TableId::DeclSecurity>
    {
    public:

        std::uint16_t  GetAction()        const;
        TableReference GetParent()        const;
        BlobReference  GetPermissionSet() const;
    };

    class EventMapRow : public BaseRow<TableId::EventMap>
    {
    public:

        TableReference GetParent()     const;
        TableReference GetFirstEvent() const;
        TableReference GetLastEvent()  const;
    };

    class EventRow : public BaseRow<TableId::Event>
    {
    public:

        EventFlags      GetFlags() const;
        StringReference GetName()  const;
        TableReference  GetType()  const;
    };

    class ExportedTypeRow : public BaseRow<TableId::ExportedType>
    {
    public:

        TypeFlags       GetFlags()          const;
        std::uint32_t   GetTypeDefId()      const;
        StringReference GetName()           const;
        StringReference GetNamespace()      const;
        TableReference  GetImplementation() const;
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

        std::uint32_t  GetOffset() const;
        TableReference GetField()  const;
    };

    class FieldMarshalRow : public BaseRow<TableId::FieldMarshal>
    {
    public:

        TableReference GetParent()     const;
        BlobReference  GetNativeType() const;
    };

    class FieldRvaRow : public BaseRow<TableId::FieldRva>
    {
    public:

        std::uint32_t  GetRva()   const;
        TableReference GetField() const;
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
        TableReference        GetOwner()  const;
        StringReference       GetName()   const;
    };

    class GenericParamConstraintRow : public BaseRow<TableId::GenericParamConstraint>
    {
    public:

        TableReference GetOwner()      const;
        TableReference GetConstraint() const;
    };

    class ImplMapRow : public BaseRow<TableId::ImplMap>
    {
    public:

        PInvokeFlags    GetMappingFlags()    const;
        TableReference  GetMemberForwarded() const;
        StringReference GetImportName()      const;
        TableReference  GetImportScope()     const;
    };

    class InterfaceImplRow : public BaseRow<TableId::InterfaceImpl>
    {
    public:

        TableReference GetClass()     const;
        TableReference GetInterface() const;
    };

    class ManifestResourceRow : public BaseRow<TableId::ManifestResource>
    {
    public:

        std::uint32_t         GetOffset()         const;
        ManifestResourceFlags GetFlags()          const;
        StringReference       GetName()           const;
        TableReference        GetImplementation() const;
    };

    class MemberRefRow : public BaseRow<TableId::MemberRef>
    {
    public:

        TableReference  GetClass()     const;
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

        TableReference            GetFirstParameter()      const;
        TableReference            GetLastParameter()       const;
    };

    class MethodImplRow : public BaseRow<TableId::MethodImpl>
    {
    public:

        TableReference GetClass()             const;
        TableReference GetMethodBody()        const;
        TableReference GetMethodDeclaration() const;
    };

    class MethodSemanticsRow : public BaseRow<TableId::MethodSemantics>
    {
    public:

        MethodSemanticsFlags GetSemantics()   const;
        TableReference       GetMethod()      const;
        TableReference       GetAssociation() const;
    };

    class MethodSpecRow : public BaseRow<TableId::MethodSpec>
    {
    public:

        TableReference GetMethod()        const;
        BlobReference  GetInstantiation() const;
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

        TableReference GetNestedClass()    const;
        TableReference GetEnclosingClass() const;
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

        TableReference GetParent()        const;
        TableReference GetFirstProperty() const;
        TableReference GetLastProperty()  const;
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
        TableReference  GetExtends()     const;

        TableReference  GetFirstField()  const;
        TableReference  GetLastField()   const;

        TableReference  GetFirstMethod() const;
        TableReference  GetLastMethod()  const;
    };

    class TypeRefRow : public BaseRow<TableId::TypeRef>
    {
    public:

        TableReference  GetResolutionScope() const;
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
