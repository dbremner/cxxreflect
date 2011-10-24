//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// The MetadataDatabase and its supporting classes provide a physical layer implementation for
// reading metadata.  It maps heaps and tables into a more usable, somewhat queriable format; it
// provides UTF-8 to UTF-16 string transformation and caching of transformed strings; and it
// includes various utilities for working with the metadata.
#ifndef CXXREFLECT_METADATADATABASE_HPP_
#define CXXREFLECT_METADATADATABASE_HPP_

#include "CxxReflect/Core.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <map>
#include <memory>
#include <string>

namespace CxxReflect { namespace Metadata {

    struct ReadException : std::runtime_error
    {
        ReadException(char const* const message) : std::runtime_error(message) { }
    };

    class Stream
    {
    public:

        Stream(Detail::FileHandle& file, SizeType metadataOffset, SizeType streamOffset, SizeType streamSize);

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
            Detail::Verify([&] { return index <= _size; }, "Index out of range");
            return _data.get() + index;
        }

        template <typename T>
        T const& ReadAs(SizeType const index) const
        {
            VerifyInitialized();
            Detail::Verify([&] { return (index + sizeof (T)) <= _size; }, "Index out of range");
            return *reinterpret_cast<T const*>(_data.get() + index);
        }

        template <typename T>
        T const* ReinterpretAs(SizeType const index) const
        {
            VerifyInitialized();
            Detail::Verify([&] { return index <= _size; }, "Index out of range");
            return reinterpret_cast<T const*>(_data.get() + index);
        }

    private:

        typedef std::unique_ptr<Byte[]> OwnedPointer;

        Stream(Stream const&);
        Stream& operator=(Stream const&);

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Stream is not initialized");
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
            : _table(), _index(static_cast<std::uint32_t>(-1))
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
        SizeType      GetRowCount()   const { VerifyInitialized(); return _rowCount;                    }
        SizeType      GetRowSize()    const { VerifyInitialized(); return _rowSize;                     }

        ByteIterator At(SizeType const index) const
        {
            VerifyInitialized();
            Detail::Verify([&] { return index < _rowCount; }, "Index out of range");
            return _data + _rowSize * index;
        }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Table is not initialized");
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
            std::swap(other._stream,      _stream     );
            std::swap(other._initialized, _initialized);
            std::swap(other._state,       _state      );
        }

        Table const& GetTable(TableId id) const;

        SizeType GetTableIndexSize(TableId id) const;
        SizeType GetCompositeIndexSize(CompositeIndex index) const;

        SizeType GetStringHeapIndexSize() const { return _state._stringHeapIndexSize; }
        SizeType GetGuidHeapIndexSize()   const { return _state._guidHeapIndexSize;   }
        SizeType GetBlobHeapIndexSize()   const { return _state._blobHeapIndexSize;   }

        SizeType GetTableColumnOffset(TableId table, SizeType column) const;

    private:

        TableCollection(TableCollection const&);
        TableCollection& operator=(TableCollection const&);

        typedef std::array<Table, TableIdCount> TableSequence;

        enum { MaximumColumnCount = 8 };

        typedef std::array<SizeType, MaximumColumnCount>       ColumnOffsetSequence;
        typedef std::array<ColumnOffsetSequence, TableIdCount> TableColumnOffsetSequence;

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
            std::swap(other._index,  _index );
        }

        StringReference At(SizeType index) const;

    private:

        typedef Detail::LinearArrayAllocator<Character, (1 << 16)> Allocator;
        typedef std::map<SizeType, StringReference>                StringMap;

        StringCollection(StringCollection const&);
        StringCollection& operator=(StringCollection const&);

        Stream            _stream;
        mutable Allocator _buffer;
        mutable StringMap _index;
    };

    // TODO We've hacked this up for one-byte-size
    class BlobReference
    {
    public:

        BlobReference()
            : _pointer(nullptr)
        {
        }

        explicit BlobReference(ByteIterator const pointer)
            : _size(*pointer), _pointer(pointer + 1)
        {
            Detail::VerifyNotNull(pointer);
        }

        ByteIterator Begin()         const { VerifyInitialized(); return _pointer;         }
        ByteIterator End()           const { VerifyInitialized(); return _pointer + _size; }
        SizeType     GetSize()       const { VerifyInitialized(); return _size;            }
        bool         IsInitialized() const { return _pointer != nullptr;                   }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Blob is not initialized");
        }

        SizeType     _size;
        ByteIterator _pointer;
    };

    template <TableId TId>
    class RowIterator;

    class Database
    {
    public:

        Database(Detail::NonNull<wchar_t const*> fileName);

        Database(Database&& other)
            : _fileName(std::move(other._fileName)),
              _blobStream(std::move(other._blobStream)),
              _guidStream(std::move(other._guidStream)),
              _strings(std::move(other._strings)),
              _tables(std::move(other._tables))
        {
        }

        Database& operator=(Database&& other)
        {
            Swap(other);
            return *this;
        }

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
            return ReturnType(this, _tables.GetTable(TId).At(index));
        }

        BlobReference GetBlob(SizeType const index) const
        {
            return BlobReference(_blobStream.At(index));
        }

        StringReference GetString(SizeType const index) const
        {
            return StringReference(_strings.At(index));
        }

        TableCollection  const& GetTables()  const { return _tables;  }
        StringCollection const& GetStrings() const { return _strings; }

        void Swap(Database& other)
        {
            std::swap(_fileName,   other._fileName  );
            std::swap(_blobStream, other._blobStream);
            std::swap(_guidStream, other._guidStream);
            std::swap(_strings,    other._strings   );
            std::swap(_tables,     other._tables    );
        }

    private:

        String _fileName;

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
        typedef Detail::Dereferenceable<value_type>      pointer;

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

        RowIterator& operator+=(DifferenceType const n) { _index += n; return *this; }
        RowIterator& operator-=(DifferenceType const n) { _index -= n; return *this; }

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

    enum class ElementType : std::uint8_t
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
                Detail::Verify([&] { return IsInitialized(); }, # name " is not initialized");          \
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
        private:                                                                                        \
                                                                                                        \
            SizeType GetColumnOffset(SizeType const column) const                                       \
            {                                                                                           \
                return _database->GetTables().GetTableColumnOffset(                                     \
                    static_cast<TableId>(RowTypeToTableId<name>::Value),                                \
                    column);                                                                            \
            }

    class AssemblyRow
    {
        CXXREFLECT_GENERATE(AssemblyRow)

    public:

        AssemblyHashAlgorithm GetHashAlgorithm() const;
        Version               GetVersion()       const;
        AssemblyFlags         GetFlags()         const;
        BlobReference         GetPublicKey()     const;
        StringReference       GetName()          const;
        StringReference       GetCulture()       const;
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

        Version         GetVersion()          const;
        AssemblyFlags   GetFlags()            const;
        BlobReference   GetPublicKeyOrToken() const;
        StringReference GetName()             const;
        StringReference GetCulture()          const;
        BlobReference   GetHashValue()        const;
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
        BlobReference  GetValue()  const;
    };

    class CustomAttributeRow
    {
        CXXREFLECT_GENERATE(CustomAttributeRow)

    public:

        TableReference GetParent() const;
        TableReference GetType()   const;
        BlobReference  GetValue()  const;
    };

    class DeclSecurityRow
    {
        CXXREFLECT_GENERATE(DeclSecurityRow)

    public:

        std::uint16_t  GetAction()        const;
        TableReference GetParent()        const;
        BlobReference  GetPermissionSet() const;
    };

    class EventMapRow
    {
        CXXREFLECT_GENERATE(EventMapRow)

    public:

        TableReference GetParent()     const;
        TableReference GetFirstEvent() const;
        TableReference GetLastEvent()  const;
    };

    class EventRow
    {
        CXXREFLECT_GENERATE(EventRow)

    public:

        EventFlags      GetFlags() const;
        StringReference GetName()  const;
        TableReference  GetType()  const;
    };

    class ExportedTypeRow
    {
        CXXREFLECT_GENERATE(ExportedTypeRow)

    public:

        TypeFlags       GetFlags()          const;
        std::uint32_t   GetTypeDefId()      const;
        StringReference GetName()           const;
        StringReference GetNamespace()      const;
        TableReference  GetImplementation() const;
    };

    class FieldRow
    {
        CXXREFLECT_GENERATE(FieldRow)

    public:

        FieldFlags      GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
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
        BlobReference  GetNativeType() const;
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

        FileFlags       GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetHashValue() const;
    };

    class GenericParamRow
    {
        CXXREFLECT_GENERATE(GenericParamRow)

    public:

        std::uint16_t         GetNumber() const;
        GenericParameterFlags GetFlags()  const;
        TableReference        GetOwner()  const;
        StringReference       GetName()   const;
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

        PInvokeFlags    GetMappingFlags()    const;
        TableReference  GetMemberForwarded() const;
        StringReference GetImportName()      const;
        TableReference  GetImportScope()     const;
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
        StringReference       GetName()           const;
        TableReference        GetImplementation() const;
    };

    class MemberRefRow
    {
        CXXREFLECT_GENERATE(MemberRefRow)

    public:

        TableReference  GetClass()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    class MethodDefRow
    {
        CXXREFLECT_GENERATE(MethodDefRow)

    public:

        std::uint32_t             GetRva()                 const;
        MethodImplementationFlags GetImplementationFlags() const;
        MethodFlags               GetFlags()               const;
        StringReference           GetName()                const;
        BlobReference             GetSignature()           const;

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
        BlobReference  GetInstantiation() const;
    };

    class ModuleRow
    {
        CXXREFLECT_GENERATE(ModuleRow)

    public:

        StringReference GetName() const;
    };

    class ModuleRefRow
    {
        CXXREFLECT_GENERATE(ModuleRefRow)

    public:

        StringReference GetName() const;
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

        ParameterFlags  GetFlags()    const;
        std::uint16_t   GetSequence() const;
        StringReference GetName()     const;
    };

    class PropertyRow
    {
        CXXREFLECT_GENERATE(PropertyRow)

    public:

        PropertyFlags   GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
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

        BlobReference GetSignature() const;
    };

    class TypeDefRow
    {
        CXXREFLECT_GENERATE(TypeDefRow)

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

    class TypeRefRow
    {
        CXXREFLECT_GENERATE(TypeRefRow)

    public:

        TableReference  GetResolutionScope() const;
        StringReference GetName()            const;
        StringReference GetNamespace()       const;
    };

    class TypeSpecRow
    {
        CXXREFLECT_GENERATE(TypeSpecRow)

    public:

        BlobReference GetSignature() const;
    };

    #undef CXXREFLECT_GENERATE

} }

#endif
