#ifndef CXXREFLECT_METADATADATABASE_HPP_
#define CXXREFLECT_METADATADATABASE_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/MetadataCommon.hpp"

namespace CxxReflect { namespace Metadata {

    /// \defgroup cxxreflect_metadata_database Metadata :: Database
    ///
    /// The Metadata Database loads metadata from a CLI assembly and provides a friendly, low-level
    /// interface for reading data from the tables of the metadata database.  It reinterprets field
    /// values, transforms strings to UTF-16, and provides access to blobs as byte sequences.  There
    /// are utilities for finding data in the database via the primary and foreign keys of various
    /// tables.
    ///
    /// The functionality is roughly equivalent to that provided by the CLR's native interfaces,
    /// including `IMetaDataImport2`, `IMetaDataAssemblyImport`, and `IMetaDataTables`.  The
    /// interface is much more C++-like.
    ///
    /// @{





    /// Identifiers for each of the tables in a metadata database
    ///
    /// The enumerator values match those specified in ECMA 335-2010 II.22, which contains the
    /// specification for the metadata logical format.
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
        /// A value one larger than the largest `TableId` enumerator value
        ///
        /// This is not exactly the count of the `TableId` enumerators because there are unassigned
        /// values that are not used for any `TableId`.  However, this is a number that is large
        /// enough that it may be used to define an array `a` large enough so that the expression
        /// `a[TableId::{Enumerator}]` is a valid indexing expression for any `TableId` enumerator.
        TableIdCount = 0x2d
    };

    typedef std::array<SizeType, TableIdCount> TableIdSizeArray;





    /// Tests whether the integer `id` maps to a valid `TableId` enumerator
    ///
    /// \param    value The value to test
    /// \returns  `true` if `id` is a valid `TableId` enumerator value; `false` otherwise
    /// \nothrows
    bool IsValidTableId(SizeType value);





    /// Identifiers for each of the composite indices used in a metadata database
    ///
    /// A composite index is used when a field may refer to a row in one of several possible tables.
    /// The enumerator values match those specified in ECMA 335-2010 II.24.2.6, which contains the
    /// specification for each of the composite indices.
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
        /// A value one larger than the largest `CompositeIndex` enumerator value
        ///
        /// This is also the number of `CompositeIndex` enumerators because there are no unused
        /// values in the enumeration.
        CompositeIndexCount = 0x0d,
    };

    typedef std::array<SizeType, CompositeIndexCount> CompositeIndexSizeArray;

    /// Converts a `CompositeIndex` key to the `TableId` it represents
    ///
    /// There is a 1:1 mapping from `CompositeIndex` key value to a `TableId`, but not all `TableId`
    /// values are represented in each `CompositeIndex`. If the specified `key` is not a valid value
    /// for `index`, `-1` is returned.
    ///
    /// \param    key   The composite index key to be converted
    /// \param    index The composite index that maps keys to `TableId` values
    /// \returns  The `TableId` represented by `key`, or `-1` if `key` is not valid in `index`
    /// \nothrows
    TableId  GetTableIdFromCompositeIndexKey(SizeType key, CompositeIndex index);

    /// Converts a `TableId` to its representation in a `CompositeIndex`
    ///
    /// There is a 1:1 mapping from a `TableId` to a `CompositeIndex` key, but not all `TableId`
    /// values are represented in each `CompositeIndex`.  If the specified `TableId` is not
    /// representable in the specified `CompositeIndex`, `-1` is returned.
    ///
    /// \param    tableId The table identifier to be converted
    /// \param    index   The composite index that maps keys to `TableId` values
    /// \returns  The key for `tableId` in the `index`, or `-1` if there is no index value
    /// \nothrows
    SizeType GetCompositeIndexKeyFromTableId(TableId tableId, CompositeIndex index);





    /// A reference to a row in a metadata table
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

        /// Constructs a new, invalid `RowReference`
        ///
        /// The newly constructed `RowReference` has a value of `InvalidValue`.
        ///
        /// \nothrows
        RowReference();

        /// Constructs a new `RowReference` from the provided `tableId` and `index`.
        ///
        /// The `tableId` must be a valid `TableId` enumerator value.  The `index` must be no larger
        /// than `ValueIndexMask`.  Arguments are checked only in debug builds.
        ///
        /// \nothrows
        RowReference(TableId tableId, SizeType index);

        /// Gets the upper 8 bits of the value, which contains the `TableId` of the referenced table
        ///
        /// \nothrows
        TableId   GetTable() const;

        /// Gets the lower 24 bits of the value, which is the index in the referenced table
        //
        /// \nothrows
        SizeType  GetIndex() const;

        /// Gets the full 32-bit value of the row index
        ///
        /// \nothrows
        ValueType GetValue() const;

        /// Gets the full 32-bit metadata token value
        ///
        /// The returned value is always `GetValue() + 1`, because a metadata token always has a
        /// value one larger than the value we store.  Metadata tokens start counting at one; we
        /// start counting at zero.
        ///
        /// \nothrows
        TokenType GetToken() const;

        /// Tests whether this `RowReference` is valid
        ///
        /// If this `RowReference` is not valid, it is a "null reference" that refers to now row.
        /// 
        /// \nothrows
        bool IsInitialized() const;

        friend bool operator==(RowReference const& lhs, RowReference const& rhs);
        friend bool operator< (RowReference const& lhs, RowReference const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(RowReference)
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(RowReference, _value.Get(), std::int32_t)

        /// Constructs a new `RowReference` from a metadata token value
        ///
        /// If the token is a null metadata token (i.e., if its lower 24 bits are all zero), the
        /// returned `RowReference` is invalid:  `IsValid()` will return `false` and `GetTable()`
        /// will not return a valid table value.
        ///
        /// Otherwise, calling `GetToken()` on the returned `RowReference` will yield the provided
        /// `token` value.
        ///
        /// \nothrows
        static RowReference FromToken(TokenType token);

    private:

        static ValueType ComposeValue(TableId tableId, SizeType index);

        Detail::ValueInitialized<ValueType> _value;
    };





    /// A pair of `RowReference` objects
    ///
    /// This is used by some of the Metadata API when a range of rows is returned.  Typically both
    /// references will refer to the same table, and `.first` will refer to a row before `.second`
    /// (or, in the case of an empty range, to the same row as `.second`).
    typedef std::pair<RowReference, RowReference> RowReferencePair;





    /// Represents a reference to a blob
    ///
    /// A blob is a sequence of bytes.  The blob may be contained in a metadata database, or it may
    /// be located in some other buffer (e.g., during generic type instantiation, type and method
    /// signatures are instantiated, yielding new instantiated signature blobs).
    class BlobReference
    {
    public:

        /// Constructs a new, uninitialized `BlobReference`
        ///
        /// After construction, `Begin() == End()`, and both are `nullptr`.
        ///
        /// \nothrows
        BlobReference();

        /// Constructs a new `BlobReference` that refers to the range `[first, last)`
        ///
        /// Neither argument may be `nullptr` (if you want to construct an invalid, uninitialized
        /// reference, use the default constructor).  Arguments are checked only in debug builds.
        ///
        /// \warning Do not use this constructor with iterators into a metadata database's blob
        ///          heap.  Instead, use `BlobReference::ComputeFromStream()` for this case.
        ///
        /// \nothrows
        BlobReference(ConstByteIterator first, ConstByteIterator last);

        /// Constructs a reference to a blob from a metadata signature
        ///
        /// The provided `signature` must be valid and initialized.  The argument is checked only in
        /// debug builds.
        ///
        /// \nothrows
        explicit BlobReference(BaseSignature const& signature);

        /// Gets an iterator to the initial byte of the signature
        ///
        /// \nothrows
        ConstByteIterator Begin() const;

        /// Gets an iterator one-past-the-end of the signature
        ///
        /// \nothrows
        ConstByteIterator End() const;

        /// Tests if this `BlobReference` is initialized and valid
        ///
        /// Any default-constructed `BlobReference` is invalid.  All other `BlobReference` objects
        /// are valid and initialized, so long as the constraints on the constructor arguments are
        /// adhered to.
        ///
        /// \nothrows
        bool IsInitialized() const;

        /// Constructs a metadata signature object from this blob reference
        ///
        /// `TSignature` should be one of the Metadata Signature types from this library, or another
        /// type that matches the semantics of those types.  It should have a constructor that takes
        /// an iterator range, into which will be passed the `Begin()` and `End()` iterators of this
        /// `BlobReference`.
        ///
        /// This function is equivalent to the following expression:
        ///
        ///     TSignature(blob.Begin(), blob.End())
        ///
        /// \nothrows
        template <typename TSignature>
        TSignature As() const
        {
            AssertInitialized();
            return TSignature(_first.Get(), _last.Get());
        }

        /// Constructs a new, initialized `BlobReference` from a metadata blob
        ///
        /// Blobs in a metadata database are stored with the length encoded in the first few bytes
        /// of the blob.  This function will decode that length, advance `first` to point to the
        /// first byte of the actual blob data, and use `first + [computed length]` for the `last`
        /// iterator.  It then uses these new `first` and `last` iterators to construct a new
        /// `BlobReference`.
        ///
        /// \param first A pointer to the beginning of the blob's encoded length value in the heap
        /// \param last  A pointer to the end of the blob heap (not to the end of the blob)
        /// \returns     A `BlobReference` that refers to the decoded blob sequence
        ///
        /// \throws MetadataError If an error occurs when reading the blob length.  Notably, if
        ///         `first == last`, or if the encoded length is malformed, or if the encoded length
        ///         yields a blob that is larger than the stream, this exception will be thrown.
        static BlobReference ComputeFromStream(ConstByteIterator first, ConstByteIterator last);

        friend bool operator==(BlobReference const& lhs, BlobReference const& rhs);
        friend bool operator< (BlobReference const& lhs, BlobReference const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(BlobReference)

    private:

        void AssertInitialized() const;

        Detail::ValueInitialized<ConstByteIterator> _first;
        Detail::ValueInitialized<ConstByteIterator> _last;
    };





    /// A reference to either a row or a blob (we refer to the two as "elements")
    ///
    /// In general, this type should not be used directly.  Instead, use the `ElementReference` or
    /// `FullReference` types.
    ///
    /// \note This class provides addition and subtraction operators so that it can be used during
    ///       iteration of a table.  These operators only have valid semantics for references to
    ///       rows.  They are meaningless for references to blobs and using them on a reference to
    ///       a blob yields undefined behavior.
    ///
    /// \todo It would be worth reconsidering the reference classes design to see if there is a
    ///       better way to organize the classes to avoid the issue in the above note.
    class BaseElementReference
    {
    public:

        enum : SizeType
        {
            InvalidElementSentinel = static_cast<SizeType>(-1)
        };

        /// Constructs a new, uninitialized `BaseElementReference`
        ///
        /// After construction, `IsInitialized()`, `IsRowReference()`, and `IsBlobReference()` all
        /// return `false`.  Attempting to call `AsRowReference()` or `AsBlobReference()` will yield
        /// undefined behavior.
        ///
        /// \nothrows
        BaseElementReference();

        /// Converting constructor that creates a new object that refers to the provided row
        ///
        /// `reference` must be initialized.  The argument is only checked in debug builds.
        ///
        /// \nothrows
        BaseElementReference(RowReference const& reference);
        
        /// Converting constructor that creates a new object that refers to the provided blob
        ///
        /// `reference` must be initialized.  The argument is only checked in debug builds.
        ///
        /// \nothrows
        BaseElementReference(BlobReference const& reference);

        /// Tests whether this element reference refers to a row
        ///
        /// \nothrows
        bool IsRowReference() const;

        /// Tests whether this element reference refers to a blob
        ///
        /// \nothrows
        bool IsBlobReference() const;

        /// Tests whether this element reference is valid and initialized
        ///
        /// A default-constructed `BaseElementReference` is invalid and not initialized.  All other
        /// `BaseElementReference` objects are valid and initialized, assuming the constructor
        /// constraints are adhered to.
        ///
        /// \nothrows
        bool IsInitialized() const;

        /// Converts the stored reference into a `RowReference`
        ///
        /// \warning This is only valid if `IsRowReference()` returns `true`. The type of the stored
        ///          reference is only checked in debug builds.  Calling this function on an element
        ///          reference that refers to a row yields undefined behavior.
        ///
        /// \nothrows
        RowReference AsRowReference() const;

        /// Converts the stored reference into a `BlobReference`
        ///
        /// \warning This is only valid if `IsBlobReference()` returns `true`.  The type of the
        ///          stored reference is only checked in debug builds.  Calling this function on an
        ///          element reference that refers to a row yields undefined behavior.
        ///
        /// \nothrows
        BlobReference AsBlobReference() const;

        friend bool operator==(BaseElementReference const& lhs, BaseElementReference const& rhs);
        friend bool operator< (BaseElementReference const& lhs, BaseElementReference const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(BaseElementReference)

        // Note that the addition/subtraction operators are only usable for RowReferences.
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





    /// A reference to either a row or a blob
    ///
    /// This type is derived from `BaseElementReference` and provides no additional functionality.
    /// (The base type is provided solely so that we can derive both this type and the other 
    /// `FullReference` type from it and share most of the functionality.
    ///
    /// See the documentation for `BaseElementReference` for information about this type.
    class ElementReference
        : public BaseElementReference
    {
    public:

        ElementReference();
        ElementReference(RowReference  const& reference);
        ElementReference(BlobReference const& reference);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(ElementReference)

        // Note that the addition/subtraction operators are only usable for RowReferences.
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(ElementReference, _index, std::int32_t)
    };





    /// A reference to either a row or a blob, with the database in which the row or blob is located
    ///
    /// This type provides a sort of scoped-reference.  It represents either kind of element from
    /// the database (row or blob reference) and the database (the scope) from which the row or blob
    /// was obtained.
    ///
    /// For references to rows, the database can be used to resolve the row into one of the row
    /// types.  For references to blobs, the database can be used to resolve parts of the blob's
    /// signature.
    ///
    /// \note This class provides addition and subtraction operators so that it can be used during
    ///       iteration of a table.  These operators only have valid semantics for references to
    ///       rows.  They are meaningless for references to blobs and using them on a reference to
    ///       a blob yields undefined behavior.
    ///
    /// \todo It would be worth reconsidering the reference classes design to see if there is a
    ///       better way to organize the classes to avoid the issue in the above note.
    class FullReference
        : public BaseElementReference
    {
    public:

        /// Constructs a new, uninitialized `FullReference`
        ///
        /// After construction, all of the postconditions of the base class type hold true.  A call
        /// to `GetDatabase()` will yield undefined behavior.
        ///
        /// \nothrows
        FullReference();

        /// Constructs a new `FullReference` that refers to the provided row and its scope
        ///
        /// `reference` must be initialized and `database` must point to a valid database.  The
        /// arguments are only checked in debug builds.
        ///
        /// \nothrows
        FullReference(Database const* database, RowReference const& reference);

        /// Constructs a new `FullReference` that refers to the provided blob and its scope
        ///
        /// `reference` must be initialized and `database` must point to a valid database.  The
        /// arguments are only checked in debug builds.
        ///
        /// \nothrows
        FullReference(Database const* database, BlobReference const& reference);

        /// Constructs a new `FullReference` that refers to the provided element and its scope
        ///
        /// `reference` must be initialized and `database` must point to a valid database.  The
        /// arguments are only checked in debug builds.
        ///
        /// \nothrows
        FullReference(Database const* database, ElementReference const& reference);

        /// Gets a reference to the database in which the element is scoped
        ///
        /// If this object is not initialized, or if the constraints of the constructor arguments
        /// were not met and the database is a `nullptr`, a call to this function yields undefined
        /// behavior.  Validity of the database is checked in debug builds only.
        ///
        /// \nothrows
        Database const& GetDatabase() const;

        friend bool operator==(FullReference const& lhs, FullReference const& rhs);
        friend bool operator< (FullReference const& lhs, FullReference const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(FullReference)

        // Note that the addition/subtraction operators are only usable for RowReferences.
        CXXREFLECT_GENERATE_ADDITION_SUBTRACTION_OPERATORS(FullReference, _index, std::int32_t)

    private:

        Detail::ValueInitialized<Database const*> _database;
    };

    



    /// Represents a four-component version number (major, minor, build, and revision)
    class FourComponentVersion
    {
    public:

        /// Each component of the version number is a 16-bit unsigned integer
        typedef std::uint16_t Component;

        /// Default constructs a `FourComponentVersion` with each component having a value of zero
        ///
        /// \nothrows
        FourComponentVersion();

        /// Constructs a `FourComponentVersion` with the provided version number components
        ///
        /// \param    major    The major component value
        /// \param    minor    The minor component value
        /// \param    build    The build component value
        /// \param    revision The revision component value
        /// \nothrows
        FourComponentVersion(Component major, Component minor, Component build, Component revision);

        /// Gets the major component value
        ///
        /// \nothrows
        Component GetMajor()    const;

        /// Gets the minor component value
        ///
        /// \nothrows
        Component GetMinor()    const;

        /// Gets the build component value
        ///
        /// \nothrows
        Component GetBuild()    const;

        /// Gets the revision component value
        ///
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
    /// This type is merely a facade over an array of bytes.  It provides helpful functions like
    /// ReadAs() and ReinterpretAs() to aid in reading binary data from a metadata database.
    ///
    /// \todo Consider removing this class and moving its functionality elsewhere.  This class was
    ///       useful when it owned the array of bytes, but it no longer does now that we've moved
    ///       to use memory mapped I/O.
    class Stream
    {
    public:

        /// Constructs an uninitialized stream
        ///
        /// If a stream is not initialized, 
        Stream();

        /// Constructs a stream from the provided file information
        ///
        /// This constructor seeks in the `file` to the beginning of the metadata stream, as
        /// specified by the sum of `metadataOffset` and `streamOffset`, then reads `streamSize`
        /// bytes from the file and uses those to initialize the `Stream`.
        ///
        /// `file` must be initialized.  This argument is only checked in debug builds.
        ///
        /// \param file           The file in which the metadata stream is located
        /// \param metadataOffset The offset in the `file` at which the metadata database begins
        /// \param streamOffset   The offset in the metadata at which the stream begins
        /// \param streamSize     The size of the stream, in bytes
        ///
        /// \throws MetadataError If the stream cannot be read from the file (notably, out-of-
        ///         range errors, where the stream offsets are past the end of the file or the 
        ///         stream size extends the stream beyond the end of the file will throw).
        Stream(Detail::ConstByteCursor file, SizeType metadataOffset, SizeType streamOffset, SizeType streamSize);

        /// Gets an iterator to the initial byte in the metadata stream
        ///
        /// If this stream is not initialized, this will return `nullptr`.
        ///
        /// \nothrows
        ConstByteIterator Begin() const;

        /// Gets an iterator to the end of the metadata stream
        ///
        /// If this stream is not initialized, this will return `nullptr`.
        ///
        /// \nothrows
        ConstByteIterator End() const;

        /// Gets the size of this metadata stream, in bytes
        ///
        /// This computes `Begin() - End()`.  If this stream is not initialized, this returns `0`.
        ///
        /// \nothrows
        SizeType Size() const;

        /// Tests whether this stream is initialized
        ///
        /// \nothrows
        bool IsInitialized() const;

        /// Gets an iterator to the byte at index `index` in the stream
        ///
        /// \throws MetadataError if `index > Size()`
        ConstByteIterator At(SizeType index) const;

        /// Reads an element of type `T` from `index` and returns a reference to it
        ///
        /// \throws MetadataError If the operation would attempt to read past the stream's end
        template <typename T>
        T const& ReadAs(SizeType const index) const
        {
            return *ReinterpretAs<T>(index);
        }

        /// Reinterprets an element of type `T` from `index` and returns a pointer to it
        ///
        /// \throws MetadataError If the operation would attempt to read past the stream's end
        template <typename T>
        T const* ReinterpretAs(SizeType const index) const
        {
            return reinterpret_cast<T const*>(RangeCheckedAt(index, static_cast<SizeType>(sizeof(T))));
        }

    private:

        void AssertInitialized() const;

        ConstByteIterator RangeCheckedAt(SizeType index, SizeType size) const;

        ConstByteRange _data;
    };





    /// Maps each row type to its corresponding TableId enumerator.
    ///
    /// This class template is specialized for each table.  It is for infrastructure use only.
    template <typename TRow>
    struct RowTypeToTableId;

    /// Maps each TableId enumerator to its corresponding row type.
    ///
    /// This class template is specialized for each table.  It is for infrastructure use only.
    template <TableId TId>
    struct TableIdToRowType;

    /// \cond CXXREFLECT_DOXYGEN_FALSE

    template <TableId TId>
    class RowIterator;

    #define CXXREFLECT_GENERATE(t)                                              \
    class t ## Row;                                                             \
                                                                                \
    template <>                                                                 \
    struct RowTypeToTableId<t ## Row>                                           \
    {                                                                           \
        enum { Value = TableId::t };                                            \
    };                                                                          \
                                                                                \
    template <>                                                                 \
    struct TableIdToRowType<TableId::t>                                         \
    {                                                                           \
        typedef t ## Row Type;                                                  \
    };                                                                          \
                                                                                \
    typedef RowIterator<TableId::t> t ## RowIterator;                           \
    typedef std::pair<t ## RowIterator, t ## RowIterator> t ## RowIteratorPair

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

    /// \endcond





    /// A Table in a metadata database
    ///
    /// This provides general-purpose access to a metadata table; it does not own the table data, it
    /// is just a wrapper to provide convenient access for row offset compution and bounds checking.
    class Table
    {
    public:

        /// Default-constructs an uninitialized `Table`
        ///
        /// \nothrows
        Table();

        /// Constructs a new `Table` with the provided parameters
        ///
        /// The arguments are checked only in debug builds.
        ///
        /// \param data     A pointer to the initial byte of the table; it must be a valid pointer
        /// \param rowSize  The size of each row in the table, in bytes
        /// \param rowCount The number of rows in the table
        /// \param isSorted `true` if the data in the table is sorted; false otherwise
        /// \nothrows
        Table(ConstByteIterator data, SizeType rowSize, SizeType rowCount, bool isSorted);

        /// Gets an iterator to the initial byte of the table
        ///
        /// \nothrows
        ConstByteIterator Begin() const;

        /// Gets an iterator to the byte one-past-the-end of the table
        ///
        /// \nothrows
        ConstByteIterator End() const;

        /// Gets whether the table is sorted
        ///
        /// \nothrows
        bool IsSorted() const;

        /// Gets the number of rows in the table
        ///
        /// \nothrows
        SizeType GetRowCount() const;

        /// Gets the size of each row in the table, in bytes
        ///
        /// \nothrows
        SizeType GetRowSize() const;

        /// Gets an iterator to the initial byte of the row at the specified index in the table
        ///
        /// \throws MetadataError If the `index` is out of range
        ConstByteIterator At(SizeType index) const;

        /// Tests whether the `Table` is initialized
        ///
        /// A default-constructed `Table` is not initialized.  All other `Table` objects should be
        /// initialized, unless the constraints on the constructor arguments were not adhered to.
        ///
        /// \nothrows
        bool IsInitialized() const;

    private:

        void AssertInitialized() const;

        Detail::ValueInitialized<ConstByteIterator> _data;
        Detail::ValueInitialized<SizeType>          _rowSize;
        Detail::ValueInitialized<SizeType>          _rowCount;
        Detail::ValueInitialized<bool>              _isSorted;
    };





    /// Encapsulates the tables in the metadata database
    ///
    /// This class owns the stream in which the tables are stored and computes the offsets, row
    /// sizes, and other metadata about each table.
    class TableCollection
    {
    public:

        /// Default-constructs an uninitialized `TableCollection`
        TableCollection();
        explicit TableCollection(Stream const& stream);

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





    /// A cache used internally by the StringCollection.
    ///
    /// The StringColection has a mutable cache that requires internal synchronization.  Because
    /// C++/CLI translation units do not support the C++11 threading headers, we use this pimpl'ed
    /// class to own both the mutex and the cache.
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





    class StrideIterator;





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

        StrideIterator LightweightBegin(TableId tableId) const;
        StrideIterator LightweightEnd  (TableId tableId) const;


        /// Gets the row at the specified index in a table.
        ///
        /// \tparam  TId   The identifier of the table from which to get the row.
        /// \param   index The index (or a reference containing the index) of the row to be returned.
        /// \returns The row read from the metadata data.
        /// \throws  LogicError if this object is not initialized.
        /// \throws  LogicError if a reference is provided and it is not initialized.
        /// \throws  LogicError if a reference is provided and it is not a `RowReference`.
        /// \throws  LogicError if a reference is provided and it refers to a table other than `TId`.
        /// \throws  LogicError if the index is greater than the number of rows in the table.
        template <TableId TId> typename TableIdToRowType<TId>::Type GetRow(SizeType                    index    ) const;
        template <TableId TId> typename TableIdToRowType<TId>::Type GetRow(RowReference         const& reference) const;
        template <TableId TId> typename TableIdToRowType<TId>::Type GetRow(BaseElementReference const& reference) const;

        StringReference GetString(SizeType index) const;

        TableCollection  const& GetTables()  const;
        StringCollection const& GetStrings() const;
        Stream           const& GetBlobs()   const;
        Stream           const& GetGuids()   const;

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





    /// An iterator that iterates a range of bytes in strides
    ///
    /// This is used as a low-level row iterator.  It never instantiates any objects; it is merely a
    /// ConstByteIterator where advancing or retreating the iterator moves the pointer by a 'stride'
    /// instead of by a single byte.  This is used for iterating over a metadata table, wherein the
    /// stride is the size of each row.
    ///
    /// We've introduced this class as an internal replacement for RowIterator, which while very
    /// friendly and easy to use, is far too slow for internal operations that should be quick,
    /// especially binary searches of a sorted column in a metadata table.
    class StrideIterator
    {
    public:

        typedef std::random_access_iterator_tag     iterator_category;
        typedef DifferenceType                      difference_type;
        typedef ConstByteIterator                   value_type;
        typedef value_type                          reference;
        typedef Detail::Dereferenceable<value_type> pointer;

        typedef value_type                          ValueType;
        typedef reference                           Reference;
        typedef pointer                             Pointer;

        StrideIterator()
        {
        }

        StrideIterator(ConstByteIterator const current, SizeType const stride)
            : _current(current), _stride(stride)
        {
            Detail::AssertNotNull(current);
            Detail::Assert([&]{ return stride > 0; });
        }

        Reference    Get()           const { return GetValue(); }
        Reference    operator*()     const { return GetValue(); }
        Pointer      operator->()    const { return GetValue(); }

        StrideIterator& operator++()    { AssertInitialized(); _current.Get() += _stride.Get(); return *this;  }
        StrideIterator  operator++(int) { StrideIterator const it(*this); ++*this; return it;                  }

        StrideIterator& operator--()    { AssertInitialized(); _current.Get() -= _stride.Get(); return *this;  }
        StrideIterator  operator--(int) { StrideIterator const it(*this); --*this; return it;                  }

        StrideIterator& operator+=(DifferenceType const n)
        {
            AssertInitialized();
            _current.Get() += n * _stride.Get();
            return *this;
        }
        StrideIterator& operator-=(DifferenceType const n)
        {
            AssertInitialized();
            _current.Get() -= n * _stride.Get();
            return *this;
        }

        Reference operator[](DifferenceType const n) const
        {
            AssertInitialized();
            return _current.Get() + n * _stride.Get();
        }

        friend StrideIterator operator+(StrideIterator it, DifferenceType const n) { return it +=  n; }
        friend StrideIterator operator+(DifferenceType const n, StrideIterator it) { return it +=  n; }
        friend StrideIterator operator-(StrideIterator it, DifferenceType const n) { return it += -n; }

        friend DifferenceType operator-(StrideIterator const& lhs, StrideIterator const& rhs)
        {
            AssertComparable(lhs, rhs);
            return (lhs._current.Get() - rhs._current.Get()) / lhs._stride.Get();
        }

        friend bool operator==(StrideIterator const& lhs, StrideIterator const& rhs)
        {
            AssertComparable(lhs, rhs);
            return lhs._current.Get() == rhs._current.Get();
        }

        friend bool operator<(StrideIterator const& lhs, StrideIterator const& rhs)
        {
            lhs.AssertInitialized();
            rhs.AssertInitialized();
            AssertComparable(lhs, rhs);
            return lhs._current.Get() < rhs._current.Get();
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(StrideIterator)

    private:

        static void AssertComparable(StrideIterator const& lhs, StrideIterator const& rhs)
        {
            Detail::Assert([&]{ return lhs._stride.Get() == rhs._stride.Get(); });
        }

        void AssertInitialized() const
        {
            Detail::Assert([&]{ return _current.Get() != nullptr; });
        }

        Reference GetValue() const
        {
            AssertInitialized();
            return _current.Get();
        }

        Detail::ValueInitialized<ConstByteIterator> _current;
        Detail::ValueInitialized<SizeType>          _stride;
    };





    /// An iterator that facilitates random access iteration over a metadata table.
    ///
    /// This iterator provides the complete random access iterator interface.  Because the metadata
    /// database is read-only, it is always a const iterator.  It materializes Row objects when it
    /// is dereferenced, so the result of `*it` is not an lvalue (the iterator is dereferenceable
    /// via `operator->`, however, through the use of a proxy type).
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

        static RowIterator FromRowPointer(Database const* const database, ConstByteIterator const iterator)
        {
            Detail::AssertNotNull(database);

            auto const& table(database->GetTables().GetTable(TId));
            return RowIterator(database, (iterator - table.Begin()) / table.GetRowSize());
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

        /// Gets a `FullReference` that refers to this row.
        ///
        /// \returns A `FullReference` that refers to this row.
        /// \throws  LogicError if the row is not initialized.
        FullReference GetSelfFullReference() const
        {
            AssertInitialized();

            return FullReference(_database.Get(), GetSelfReference());
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





    /// Represents a row in the **Assembly** table (ECMA 335-2010 II.22.2)
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

    /// Represents a row in the **AssemblyOS** table (ECMA 335-2010 II.22.3)
    class AssemblyOsRow : public BaseRow<TableId::AssemblyOs>
    {
    public:

        std::uint32_t GetOsPlatformId()   const;
        std::uint32_t GetOsMajorVersion() const;
        std::uint32_t GetOsMinorVersion() const;
    };

    /// Represents a row in the **AssemblyProcessor** table (ECMA 335-2010 II.22.4)
    class AssemblyProcessorRow : public BaseRow<TableId::AssemblyProcessor>
    {
    public:

        std::uint32_t GetProcessor() const;
    };

    /// Represents a row in the **AssemblyRef** table (ECMA 335-2010 II.22.5)
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

    /// Represents a row in the **AssemblyRefOS** table (ECMA 335-2010 II.22.6)
    class AssemblyRefOsRow : public BaseRow<TableId::AssemblyRefOs>
    {
    public:

        std::uint32_t GetOsPlatformId()   const;
        std::uint32_t GetOsMajorVersion() const;
        std::uint32_t GetOsMinorVersion() const;
        RowReference  GetAssemblyRef()    const;
    };

    /// Represents a row in the **AssemblyRef** Processor table (ECMA 335-2010 II.22.7)
    class AssemblyRefProcessorRow : public BaseRow<TableId::AssemblyRefProcessor>
    {
    public:

        std::uint32_t GetProcessor()   const;
        RowReference  GetAssemblyRef() const;
    };

    /// Represents a row in the **ClassLayout** table (ECMA 335-2010 II.22.8)
    class ClassLayoutRow : public BaseRow<TableId::ClassLayout>
    {
    public:

        std::uint16_t GetPackingSize() const;
        std::uint32_t GetClassSize()   const;
        RowReference  GetParent()      const;
    };

    /// Represents a row in the **Constant** table (ECMA 335-2010 II.22.9)
    class ConstantRow : public BaseRow<TableId::Constant>
    {
    public:

        /// Gets the `ElementType` of the value pointed to by `GetValue()`
        ///
        /// In ECMA 335-2010, this is called the "Type" field.
        ElementType   GetElementType() const;
        RowReference  GetParent()      const;
        SizeType      GetParentRaw()   const;
        BlobReference GetValue()       const;
    };

    /// Represents a row in the **CustomAttribute** table (ECMA 335-2010 II.22.10)
    class CustomAttributeRow : public BaseRow<TableId::CustomAttribute>
    {
    public:

        RowReference  GetParent()    const;
        SizeType      GetParentRaw() const;
        RowReference  GetType()      const;
        SizeType      GetTypeRaw()   const;
        BlobReference GetValue()     const;
    };

    /// Represents a row in the **DeclSecurity** table (ECMA 335-2010 II.22.11)
    ///
    /// \note This column is referred to elsewhere as *PermissionSet* (e.g., see its value in the
    ///       **HasCustomAttribute** composite index.
    class DeclSecurityRow : public BaseRow<TableId::DeclSecurity>
    {
    public:

        std::uint16_t GetAction()        const;
        RowReference  GetParent()        const;
        SizeType      GetParentRaw()     const;
        BlobReference GetPermissionSet() const;
    };

    /// Represents a row in the **EventMap** table (ECMA 335-2010 II.22.12)
    class EventMapRow : public BaseRow<TableId::EventMap>
    {
    public:

        RowReference GetParent()     const;
        RowReference GetFirstEvent() const;
        RowReference GetLastEvent()  const;
    };

    /// Represents a row in the **Event** table (ECMA 335-2010 II.22.13)
    class EventRow : public BaseRow<TableId::Event>
    {
    public:

        EventFlags      GetFlags()   const;
        StringReference GetName()    const;
        RowReference    GetType()    const;
        SizeType        GetTypeRaw() const;
    };

    /// Represents a row in the **ExportedType** table (ECMA 335-2010 II.22.14)
    class ExportedTypeRow : public BaseRow<TableId::ExportedType>
    {
    public:

        TypeFlags       GetFlags()             const;
        std::uint32_t   GetTypeDefId()         const;
        StringReference GetName()              const;
        StringReference GetNamespace()         const;
        RowReference    GetImplementation()    const;
        SizeType        GetImplementationRaw() const;
    };

    /// Represents a row in the **Field** table (ECMA 335-2010 II.22.15)
    class FieldRow : public BaseRow<TableId::Field>
    {
    public:

        FieldFlags      GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    /// Represents a row in the **FieldLayout** table (ECMA 335-2010 II.22.16)
    class FieldLayoutRow : public BaseRow<TableId::FieldLayout>
    {
    public:

        SizeType      GetOffset() const;
        RowReference  GetParent() const;
    };

    /// Represents a row in the **FieldMarshal** table (ECMA 335-2010 II.22.17)
    class FieldMarshalRow : public BaseRow<TableId::FieldMarshal>
    {
    public:

        RowReference  GetParent()     const;
        SizeType      GetParentRaw()  const;
        BlobReference GetNativeType() const;
    };

    /// Represents a row in the **FieldRVA** table (ECMA 335-2010 II.22.18)
    class FieldRvaRow : public BaseRow<TableId::FieldRva>
    {
    public:

        SizeType      GetRva()   const;
        RowReference  GetParent() const;
    };

    /// Represents a row in the **File** table (ECMA 335-2010 II.22.19)
    class FileRow : public BaseRow<TableId::File>
    {
    public:

        FileFlags       GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetHashValue() const;
    };

    /// Represents a row in the **GenericParam** table (ECMA 335-2010 II.22.20)
    class GenericParamRow : public BaseRow<TableId::GenericParam>
    {
    public:

        std::uint16_t         GetSequence()  const;
        GenericParameterFlags GetFlags()     const;
        RowReference          GetParent()    const;
        SizeType              GetParentRaw() const;
        StringReference       GetName()      const;
    };

    /// Represents a row in the **GenericParamConstraint** table (ECMA 335-2010 II.22.21)
    class GenericParamConstraintRow : public BaseRow<TableId::GenericParamConstraint>
    {
    public:

        RowReference GetParent()        const;
        RowReference GetConstraint()    const;
        SizeType     GetConstraintRaw() const;
    };

    /// Represents a row in the **ImplMap** table (ECMA 335-2010 II.22.22)
    class ImplMapRow : public BaseRow<TableId::ImplMap>
    {
    public:

        PInvokeFlags    GetMappingFlags()       const;
        RowReference    GetMemberForwarded()    const;
        SizeType        GetMemberForwardedRaw() const;
        StringReference GetImportName()         const;
        RowReference    GetImportScope()        const;
    };

    /// Represents a row in the **InterfaceImpl** table (ECMA 335-2010 II.22.23)
    class InterfaceImplRow : public BaseRow<TableId::InterfaceImpl>
    {
    public:

        RowReference GetClass()        const;
        RowReference GetInterface()    const;
        SizeType     GetInterfaceRaw() const;
    };

    /// Represents a row in the **ManifestResource** table (ECMA 335-2010 II.22.24)
    class ManifestResourceRow : public BaseRow<TableId::ManifestResource>
    {
    public:

        SizeType              GetOffset()            const;
        ManifestResourceFlags GetFlags()             const;
        StringReference       GetName()              const;
        RowReference          GetImplementation()    const;
        SizeType              GetImplementationRaw() const;
    };

    /// Represents a row in the **MemberRef** table (ECMA 335-2010 II.22.25)
    class MemberRefRow : public BaseRow<TableId::MemberRef>
    {
    public:

        RowReference    GetClass()     const;
        SizeType        GetClassRaw()  const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    /// Represents a row in the **MethodDef** table (ECMA 335-2010 II.22.26)
    class MethodDefRow : public BaseRow<TableId::MethodDef>
    {
    public:

        SizeType                  GetRva()                 const;
        MethodImplementationFlags GetImplementationFlags() const;
        MethodFlags               GetFlags()               const;
        StringReference           GetName()                const;
        BlobReference             GetSignature()           const;

        RowReference              GetFirstParameter()      const;
        RowReference              GetLastParameter()       const;
    };

    /// Represents a row in the **MethodImpl** table (ECMA 335-2010 II.22.27)
    class MethodImplRow : public BaseRow<TableId::MethodImpl>
    {
    public:

        /// Gets a reference to the **TypeDef** that owns this **MethodImpl** row
        ///
        /// This column is the primary key.  The table is sorted by this column's value.  Note that
        /// in ECMA 335-2010, this is called the "Class" field.
        RowReference GetParent() const;

        RowReference GetMethodBody()           const;
        SizeType     GetMethodBodyRaw()        const;
        RowReference GetMethodDeclaration()    const;
        SizeType     GetMethodDeclarationRaw() const;
    };

    /// Represents a row in the **MethodSemantics** table (ECMA 335-2010 II.22.28)
    class MethodSemanticsRow : public BaseRow<TableId::MethodSemantics>
    {
    public:

        MethodSemanticsFlags GetSemantics()   const;
        RowReference         GetMethod()      const;

        /// Gets a reference to the **Event** or **Property** that owns this **MethodSemantics** row
        ///
        /// Note that in ECMA 335-2010, this is called the "Association" field.  We have named it
        /// "Parent" for consistency with other tables in the database.
        RowReference         GetParent()      const;
        SizeType             GetParentRaw()   const;
    };

    /// Represents a row in the **MethodSpec** table (ECMA 335-2010 II.22.29)
    class MethodSpecRow : public BaseRow<TableId::MethodSpec>
    {
    public:

        RowReference  GetMethod()    const;
        SizeType      GetMethodRaw() const;
        BlobReference GetSignature() const;
    };

    /// Represents a row in the **Module** table (ECMA 335-2010 II.22.30)
    class ModuleRow : public BaseRow<TableId::Module>
    {
    public:

        StringReference GetName() const;
        BlobReference   GetMvid() const;
    };

    /// Represents a row in the **ModuleRef** table (ECMA 335-2010 II.22.31)
    class ModuleRefRow : public BaseRow<TableId::ModuleRef>
    {
    public:

        StringReference GetName() const;
    };

    /// Represents a row in the **NestedClass** table (ECMA 335-2010 II.22.32)
    class NestedClassRow : public BaseRow<TableId::NestedClass>
    {
    public:

        RowReference GetNestedClass()    const;
        RowReference GetEnclosingClass() const;
    };

    /// Represents a row in the **Param** table (ECMA 335-2010 II.22.33)
    class ParamRow : public BaseRow<TableId::Param>
    {
    public:

        ParameterFlags  GetFlags()    const;
        std::uint16_t   GetSequence() const;
        StringReference GetName()     const;
    };

    /// Represents a row in the **Property** table (ECMA 335-2010 II.22.34)
    class PropertyRow : public BaseRow<TableId::Property>
    {
    public:

        PropertyFlags   GetFlags()     const;
        StringReference GetName()      const;
        BlobReference   GetSignature() const;
    };

    /// Represents a row in the **PropertyMap** table (ECMA 335-2010 II.22.35)
    class PropertyMapRow : public BaseRow<TableId::PropertyMap>
    {
    public:

        /// Gets a reference to the **TypeDef** that owns this **PropertyMap** row
        RowReference GetParent()        const;

        RowReference GetFirstProperty() const;
        RowReference GetLastProperty()  const;
    };

    /// Represents a row in the **StandaloneSig** table (ECMA 335-2010 II.22.36)
    class StandaloneSigRow : public BaseRow<TableId::StandaloneSig>
    {
    public:

        BlobReference GetSignature() const;
    };

    /// Represents a row in the **TypeDef** table (ECMA 335-2010 II.22.37)
    class TypeDefRow : public BaseRow<TableId::TypeDef>
    {
    public:

        TypeFlags       GetFlags()       const;
        StringReference GetName()        const;
        StringReference GetNamespace()   const;
        RowReference    GetExtends()     const;
        SizeType        GetExtendsRaw()  const;

        RowReference    GetFirstField()  const;
        RowReference    GetLastField()   const;

        RowReference    GetFirstMethod() const;
        RowReference    GetLastMethod()  const;
    };

    /// Represents a row in the **TypeRef** table (ECMA 335-2010 II.22.38)
    class TypeRefRow : public BaseRow<TableId::TypeRef>
    {
    public:

        RowReference    GetResolutionScope()    const;
        SizeType        GetResolutionScopeRaw() const;
        StringReference GetName()               const;
        StringReference GetNamespace()          const;
    };

    /// Represents a row in the **TypeSpec** table (ECMA 335-2010 II.22.39)
    class TypeSpecRow : public BaseRow<TableId::TypeSpec>
    {
    public:

        BlobReference GetSignature() const;
    };





    /// Gets the **TypeDef** that owns an **Event**
    ///
    /// \param   eventRow The **Event** row for whom we wish to get the owning **TypeDef**.  This
    ///          must be a valid row object; it is only checked for validity in debug builds.
    /// \returns The **TypeDef** row that owns the **Event**
    /// \throws  MetadataError If the metadata is invalid and the owner cannot be computed
    TypeDefRow GetOwnerOfEvent(EventRow const& eventRow);
    
    /// Gets the **TypeDef** that owns a **Field**
    ///
    /// \param   field The **Field** row for whom we wish to get the owning **TypeDef**.  This must
    ///          be a valid row object; it is only checked for validity in debug builds.
    /// \returns The **TypeDef** row that owns the **Field**
    /// \throws  MetadataError If the metadata is invalid and the owner cannot be computed
    TypeDefRow GetOwnerOfField(FieldRow const& field);
    
    /// Gets the **TypeDef** that owns a **MethodDef**
    ///
    /// \param   methodDef The **MethodDef** row for whom we wish to get the owning **TypeDef**.
    ///          This must be a valid row object; it is only checked for validity in debug builds.
    /// \returns The **TypeDef** row that owns the **MethodDef**
    /// \throws  MetadataError If the metadata is invalid and the owner cannot be computed
    TypeDefRow GetOwnerOfMethodDef(MethodDefRow const& methodDef);

    /// Gets the **TypeDef** that owns a **Property**
    ///
    /// \param   propertyRow The **Property** row for whom we wish to get the owning **TypeDef**.
    ///          This must be a valid row object; it is only checked for validity in debug builds.
    /// \returns The **TypeDef** row that owns the **Property**
    /// \throws  MetadataError If the metadata is invalid and the owner cannot be computed
    TypeDefRow GetOwnerOfProperty(PropertyRow const& propertyRow);

    /// Gets the **MethodDef** that owns a **Param**
    ///
    /// \param   param The **Param** row for whom we wish to get the owning **MethodDef**.  This
    ///          must be a valid row object; it is only checked for validity in debug builds.
    /// \returns The **MethodDef** row that owns the **Param**
    /// \throws  MetadataError If the metadata is invalid and the owner cannot be computed
    MethodDefRow GetOwnerOfParam(ParamRow const& param);





    /// Gets the **Constant** for a **Field**, **Property**, or **Param**
    ///
    /// If the specified `parent` has no associated constant row, an empty (uninitialized)
    /// `ConstantRow` is returned.  The caller is responsible for verifying that the result is valid
    /// before using it.
    ///
    /// \param   parent A reference to the row for which to get the constant value.  This must be a
    ///          valid row in one of the aforementioned tables.  The argument is only checked for
    ///          validity in debug builds.
    /// \returns The constant for the parent row, if there is one; otherwise an empty `ConstantRow`
    /// \throws  MetadataError If the metadata is invalid and the constant cannot be computed
    ConstantRow GetConstant(FullReference const& parent);

    /// Gets the **FieldLayout** for a given **Field**
    ///
    /// If the specified `parent` has no associated field layout row, an empty (uninitialized)
    /// `FieldLayoutRow` is returned.  The caller is responsible for verifying that the result is
    /// valid before using it.
    ///
    /// \param   parent A reference to the **Field** row for which to get its field layout.  This
    ///          must be a valid row in the **Field** table.  The argument is only checked for 
    ///          validity in debug builds, however, an invalid argument in a release build will
    ///          yield an uninitialized `FieldLayoutRow`.
    /// \returns The field layout of the parent row, if there is one, otherwise an empty row
    /// \throws  MetadataError If the metadata is invalid and the result cannot be computed
    FieldLayoutRow GetFieldLayout(FullReference const& parent);





    /// Gets the range of **CustomAttribute** rows that are owned by the given parent row
    ///
    /// This is an infrastructure method.  Prefer to use the `BeginCustomAttributes()` and 
    /// `EndCustomAttributes()` methods, if possible, which return a more easily iterable range.
    ///
    /// \param   parent A reference to the parent row; this must be one of the types of rows that
    ///          can have an associated CustomAttribute.  This constraint is only checked in debug
    ///          builds.
    /// \returns The range of **CustomAttribute** rows owned by the parent row as `[first, second)`
    /// \throws  MetadataError If the metadata is invalid and the range cannot be computed
    CustomAttributeRowIteratorPair GetCustomAttributesRange(FullReference const& parent);

    /// Gets an iterator to the initial **CustomAttribute** owned by `parent`
    ///
    /// \throws MetadataError If the metadata is invalid and the iterator cannot be computed
    CustomAttributeRowIterator BeginCustomAttributes(FullReference const& parent);

    /// Gets an iterator to one-past-the-end of the **CustomAttributes** owned by `parent`
    ///
    /// \throws MetadataError If the metadata is invalid and the iterator cannot be computed
    CustomAttributeRowIterator EndCustomAttributes(FullReference const& parent);





    /// Gets the range of **MethodImpl** rows that are owned by the given parent row
    ///
    /// This is an infrastructure method.  Prefer to use the `BeginMethodImpls()` and 
    /// `EndMethodImpls()` methods, if possible, which return a more easily iterable range.
    ///
    /// \param   parent A reference to the parent row; this must be a row in the **TypeDef** table.
    ///          This constraint is only checked in debug builds.
    /// \returns The range of **MethodImpl** owned by the parent row as `[first, second)`
    /// \throws  MetadataError If the metadata is invalid and the range cannot be computed
    MethodImplRowIteratorPair GetMethodImplsRange(FullReference const& parent);

    /// Gets an iterator to the initial **MethodImpl** owned by `parent`
    ///
    /// \throws MetadataError If the metadata is invalid and the iterator cannot be computed
    MethodImplRowIterator BeginMethodImpls(FullReference const& parent);

    /// Gets an iterator to one-past-the-end of the **MethodImpls** owned by `parent`
    ///
    /// \throws MetadataError If the metadata is invalid and the iterator cannot be computed
    MethodImplRowIterator EndMethodImpls(FullReference const& parent);





    /// Gets the range of **MethodSemantics** rows that are owned by the given `parent` row
    ///
    /// This is an infrastructure method.  Prefer to use the `BeginMethodSemantics()` and 
    /// `EndMethodSemantics()` methods, if possible, which return a more easily iterable range.
    ///
    /// \param   parent A reference to the parent row; this msut refer either to the **Event** table
    ///          or to the **Property** table.  This constraint is only checked in debug builds.
    /// \returns The range of **MethodSemantics** owned by the parent row as `[first, second)`
    /// \throws  MetadataError If the metadata is invalid and the range cannot be computed
    MethodSemanticsRowIteratorPair GetMethodSemanticsRange(FullReference const& parent);

    /// Gets an iterator to the initial **MethodSemantics** owned by `parent`
    ///
    /// \throws MetadataError If the metadata is invalid and the iterator cannot be computed
    MethodSemanticsRowIterator BeginMethodSemantics(FullReference const& parent);

    /// Gets an iterator to one-past-the-end of the **MethodSemantics** owned by `parent`
    ///
    /// \throws MetadataError If the metadata is invalid and the iterator cannot be computed
    MethodSemanticsRowIterator EndMethodSemantics(FullReference const& parent);





    /// @}

} }

#endif
