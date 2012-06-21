
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_DATABASE_HPP_
#define CXXREFLECT_METADATA_DATABASE_HPP_

#include "cxxreflect/metadata/tokens.hpp"

namespace cxxreflect { namespace metadata {

    /// \defgroup cxxreflect_metadata_database Metadata -> Database
    ///
    /// Types for loading a metadata database from a PE file and interpreting its contents





    /// A four-component version number (major, minor, build, and revision)
    ///
    /// Note that the reflection library also has a `version` type that represents a four-component
    /// version number.  It's a bit more full-featured.
    class four_component_version
    {
    public:

        /// Each component of the version number is a 16-bit unsigned integer
        typedef std::uint16_t component;

        four_component_version() { }

        four_component_version(component major, component minor, component build, component revision)
            : _major(major), _minor(minor), _build(build), _revision(revision)
        {
        }

        auto major()    const -> component { return _major.get();    }
        auto minor()    const -> component { return _minor.get();    }
        auto build()    const -> component { return _build.get();    }
        auto revision() const -> component { return _revision.get(); }

    private:

        core::value_initialized<component> _major;
        core::value_initialized<component> _minor;
        core::value_initialized<component> _build;
        core::value_initialized<component> _revision;
    };





    /// A stream from a metadata database
    ///
    /// A metadata database is composed of a set of five streams that contain tables, strings,
    /// guids, user strings, and blobs.  A stream is just a byte array.  This wrapper class does not
    /// have much logic, it just encapsulates the byte array and provides reinterpretation functions
    /// for interpreting the contents.
    ///
    /// This is an infrastructure type; it is expected that this type will only be constructed by
    /// a `database` instance.
    class database_stream
    {
    public:

        database_stream();

        /// Constructs a new `database_stream` from the provided offset of a file
        ///
        /// `file` must be a cursor into a PE file.  The `offset` is the offset of the metadata
        /// stream in the PE file, and the `size` is the length in bytes of the metadata stream.  If
        /// `offset` or `offset + stream` is an index beyond the end of the file, a `metadata_error`
        /// is thrown.
        ///
        /// The newly created instance only wraps access to the stream in the file; it does not take
        /// ownership of the file, so it is the responsibility of the caller to ensure that the
        /// underlying byte array remains available for the lifetime of the `database_stream`.
        database_stream(core::const_byte_cursor file, core::size_type offset, core::size_type size);

        auto begin() const -> core::const_byte_iterator;
        auto end()   const -> core::const_byte_iterator;
        auto size()  const -> core::size_type;

        auto is_initialized() const -> bool;

        /// Obtains a pointer to the element at index `index`; equivalent to `begin() + index`
        ///
        /// This function is range-checked; if `index` is past the end of the stream, it will throw
        /// a `metadata_error`.  The range check is performed regardless of compilation options.
        auto operator[](core::size_type index) const -> core::const_byte_iterator;

        /// Reads a `T` object from index `index`, returning a copy of it
        ///
        /// This function is range-checked; if `index` is past the end of the stream or if the
        /// reinterpretation would yield an object that extends beyond the end of the stream (i.e.
        /// if `index + sizeof(T)` is past the end), it will throw a `metadata_error`.  .  The range
        /// check is performed regardless of compilation options.
        template <typename T>
        auto read_as(core::size_type const index) const -> T const&
        {
            return *reinterpret_as<T>(index);
        }

        /// Reinterprets the byte array starting at index `index` as a `T` object
        ///
        /// This function is range-checked; if `index` is past the end of the stream or if the
        /// reinterpretation would yield an object that extends beyond the end of the stream (i.e.
        /// if `index + sizeof(T)` is past the end), it will throw a `metadata_error`.  .  The range
        /// check is performed regardless of compilation options.
        template <typename T>
        auto reinterpret_as(core::size_type const index) const -> T const*
        {
            return reinterpret_cast<T const*>(range_checked_at(index, core::convert_integer(sizeof(T))));
        }

    private:

        auto range_checked_at(core::size_type index, core::size_type size) const -> core::const_byte_iterator;

        core::array_range<core::byte const> _data;
    };





    /// A table from a metadata database
    ///
    /// This represents a single table in the table stream of a metadata database.  It provides
    /// access to its rows only via byte iterators; the `database` class provides high-level access
    /// with `row_iterator` iterators.
    ///
    /// Note that the invariant `end() - begin() == row_count() * row_size()` will always be
    /// satisfied.  If the row count is zero, the row size may also be zero or may otherwise be
    /// incorrect because we will be unable to compute it (this shouldn't matter in practice).
    class database_table
    {
    public:

        database_table();

        /// Constructs a new `database_table`
        ///
        /// `data` must be a non-null pointer to the initial row of the database table.  `row_size`
        /// is the size of each row in bytes (all database tables have fixed size).  `row_count` is
        /// the total number of rows in the table.  The byte array pointed to by `data` must be
        /// large enough such that the pointer `data + row_count * row_size` is a valid pointer into
        /// the array (or points one-past-the-end of the array).
        ///
        /// The `is_sorted` tag should be `true` if the table is sorted by a primary key, otherwise
        /// `false`.  This tag is not used by the metadata database because it knows ahead-of-time
        /// which tables will be sorted (ECMA-335 mandates that some tables must be sorted).
        database_table(core::const_byte_iterator data,
                       core::size_type           row_size,
                       core::size_type           row_count,
                       bool                      is_sorted);

        /// Gets a pointer to the initial byte of the first row of this metadata table
        auto begin() const -> core::const_byte_iterator;

        /// Gets a pointer one-past-the-end of the last row of this metadata table
        auto end() const -> core::const_byte_iterator;

        /// Returns `true` if the rows in this table are sorted by a primary key; `false` otherwise
        auto is_sorted() const -> bool;

        /// Gets the number of rows in this table, in bytes
        auto row_count() const -> core::size_type;

        /// Gets the size of each row in this table, in bytes
        auto row_size() const -> core::size_type;

        /// Obtains a pointer to the row at index `index`
        auto operator[](core::size_type index) const -> core::const_byte_iterator;

        auto is_initialized() const -> bool;

    private:

        core::value_initialized<core::const_byte_iterator> _data;
        core::value_initialized<core::size_type>           _row_size;
        core::value_initialized<core::size_type>           _row_count;
        core::value_initialized<bool>                      _is_sorted;
    };





    /// The collection of tables in a metadata database
    ///
    /// This encapsulates the table stream from a metadata database; it constructs `database_table`
    /// objects for each table in the database, computes index sizes, and provides access to the
    /// tables.
    ///
    /// The index sizes are the number of bytes required to represent each index.  The value will
    /// always be either two or four; no other value is possible.
    ///
    /// A default-constructed `database_table_collection` is considered to be uninitialized.  No
    /// member function (other than `is_initialized()`) may be called on an uninitialized object.
    class database_table_collection
    {
    public:

        database_table_collection();
        explicit database_table_collection(database_stream const& stream);

        auto operator[](table_id table) const -> database_table const&;

        auto table_index_size(table_id table) const -> core::size_type;
        auto composite_index_size(composite_index index) const -> core::size_type;

        auto string_heap_index_size() const -> core::size_type;
        auto guid_heap_index_size()   const -> core::size_type;
        auto blob_heap_index_size()   const -> core::size_type;

        /// Gets the offset of column `column` in the requested `table`
        ///
        /// The caller must ensure that `column` identifies and actual column in the `table`.
        /// Whereas it might seem that the `database_table` would be the logical place to put this
        /// member function, we actually have to compute this information before we construct the
        /// `database_table` instances, so we store the information here.
        auto table_column_offset(table_id table, column_id column) const -> core::size_type;

        auto is_initialized() const -> bool;

    private:

        typedef std::array<database_table, table_id_count> table_sequence;

        enum { maximum_column_count = 8 };

        typedef std::array<core::size_type, maximum_column_count>  column_offset_sequence;
        typedef std::array<column_offset_sequence, table_id_count> table_column_offset_sequence;

        auto compute_composite_index_sizes() -> void;
        auto compute_table_row_sizes()       -> void;

        core::value_initialized<core::size_type>              _string_heap_index_size;
        core::value_initialized<core::size_type>              _guid_heap_index_size;
        core::value_initialized<core::size_type>              _blob_heap_index_size;

        core::value_initialized<std::bitset<64>>              _valid_bits;
        core::value_initialized<std::bitset<64>>              _sorted_bits;

        core::value_initialized<table_id_size_array>          _row_counts;
        core::value_initialized<table_id_size_array>          _row_sizes;

        core::value_initialized<table_column_offset_sequence> _column_offsets;
        core::value_initialized<composite_index_size_array>   _composite_index_sizes;
        core::value_initialized<table_sequence>               _tables;

        database_stream _stream;
    };





    /// The collection of strings in a metadata database
    ///
    /// This encapsulates the strings heap for a metadata database.  Strings in metadata are stored
    /// in UTF-8.  Windows clients will expect strings in UTF-16, so we convert all strings to that
    /// form here and cache them.  
    ///
    /// TODO Consider adding support for other external-facing string types (like UTF-8 and UTF-32).
    /// TODO For performance, consider using UTF-8 everywhere internally and converting to UTF-16
    /// TODO only when the user requests direct access to the string.
    class database_string_collection
    {
    public:

        database_string_collection();
        explicit database_string_collection(database_stream&& stream);

        database_string_collection(database_string_collection&&);
        auto operator=(database_string_collection&&) -> database_string_collection&;

        /// Gets the string whose initial UTF-8 character is located at index `index` in the stream
        ///
        /// If `index` is past the end of the stream, this will throw a `metadata_error`.  All
        /// indices that originate in a metadata database should be valid, assuming the metadata
        /// database is well-formed.
        ///
        /// Note that access to the internal cache is synchronized, so this function must take a
        /// lock.  In practice this lock should not be a point of great contention.
        auto operator[](core::size_type index) const -> core::string_reference;

        auto is_initialized() const -> bool;

    private:

        database_string_collection(database_string_collection const&);
        auto operator=(database_string_collection const&) -> void;

        typedef core::linear_array_allocator<core::character, (1 << 16)> allocator;
        typedef std::map<core::size_type, core::string_reference>        string_map;

        database_stream               _stream;
        allocator             mutable _buffer;  // Stores the transformed UTF-16 strings
        string_map            mutable _index;   // Maps string heap indices into indices in the buffer
        core::recursive_mutex mutable _sync;
    };





    /// A metadata database
    ///
    /// This is the root from which all of the other metadata database types are created and is the
    /// only type that should be directly created by a user of this library.  The `database`
    /// represents a metadata database contained in a CLI module, which is itself contained in a PE
    /// file.
    ///
    /// Most of the objects that are created by a `database` or returned from member functions or
    /// helper functions related to the `database` class contain pointers or references back into
    /// the `database`, so users must ensure that the lifetime of the `database` is at least as long
    /// as the lifetimes of any objects created through it.
    ///
    /// The `database` provides direct access to the four relevant metadata streams via the
    /// `tables()`, `blobs()`, `strings()`, and `guids()` accessors.  These may be used as
    /// described in the documentation for the other database classes.
    ///
    /// There are utility member functions that provide two forms of iteration over the tables in
    /// the metadata database.  The two forms are as follows:
    ///
    /// * `begin<Id>() -- end<Id>()`:  These member function templates take as a template parameter
    ///   the `table_id` of the table for which iterators are to be obtained.  They return
    ///   `row_iterator<Id>` objects that point to the initial row of the table and one-past-the-end
    ///   of the last row of the table.  If a table has no rows, these functions return an empty
    ///   range.
    ///
    ///   This form of iteration is suitable for most access to the database tables.  Note,
    ///   however, that dereferencing one of these iterators instantiates a new row object (which
    ///   will be of the row type for the specified table; see the row types in rows.h for details).
    ///   This can be very expensive, especially when done in a tight loop or within an algorithm
    ///   (e.g., during a binary search of a table).  If you need performance, read on.
    ///
    /// * `stride_begin() -- stride_end()`:  These member functions provide lightweight iteration
    ///   over a metadata table.  They never materialize any row objects directly; rather, one of
    ///   these stride iterators simply contains a pointer to the initial byte of a row and when
    ///   dereferenced it returns this pointer.  Incrementing this iterator will move the pointer
    ///   to point to the initial byte of the next row (this is the "stride").  The other arithmetic
    ///   operations are implemented consistently.
    ///
    ///   These iterators are more hazardous than the higher-level iterators that realize rows
    ///   directly, because it is far easier to screw up.  However, if you find that you really
    ///   need performance, you should use them.
    class database
    {
    public:

        typedef core::unique_byte_array file_range;

        /// Constructs a new `database` from the CLI module located at `path`
        ///
        /// `path` must refer to an accessible, readable file that contains a valid metadata
        /// database.  If the file cannot be read or if there are errors reading the metadata
        /// database, some form of `runtime_error` will be thrown.
        static auto create_from_file(core::string_reference path) -> database;

        /// Constructs a new `database` from the bytes contained in the array `file`
        ///
        /// Note that `file` needs not be an actual file.  It is possible to pass an array from
        /// memory into this function.  The `database` instance takes ownership of the `file` and
        /// will destroy it when it itself is destroyed.
        ///
        /// If the bytes in `file` do not represent a valid metadata database, a `runtime_error`
        /// will be thrown.  Note that validity of the database is only checked on-demand, so any
        /// operation on the database might throw an exception.  During construction, only the
        /// validity of the database header and stream headers is typically checked.
        database(file_range&& file);

        database(database&& other);
        auto operator=(database&& other) -> database&;

        auto swap(database& other) -> void;

        auto stride_begin(table_id table) const -> core::stride_iterator;
        auto stride_end  (table_id table) const -> core::stride_iterator;

        template <table_id Id>
        auto begin() const -> row_iterator<Id>
        {
            core::assert_initialized(*this);
            return row_iterator<Id>(this, 0);
        }

        template <table_id Id>
        auto end() const -> row_iterator<Id>
        {
            core::assert_initialized(*this);
            return row_iterator<Id>(this, tables()[Id].row_count());
        }

        /// Gets the row to which `token` refers
        ///
        /// The `Mask` must have exactly one bit set, and it must be a bit for a valid table.  If
        /// multiple bits are set, this function template should fail to be instantiated.  Note also
        /// that the `row_from(token)` nonmember function, defined in tokens.hpp, may be used for
        /// more succinct realization of row objects.
        template <table_mask Mask>
        auto operator[](restricted_token<Mask> const token) const -> typename row_type_for_mask<Mask>::type
        {
            core::assert_initialized(*this);
            core::assert_initialized(token);
            return create_row<typename row_type_for_mask<Mask>::type>(
                this,
                tables()[table_id_for_mask<Mask>::value][token.index()]);
        }

        auto tables()  const -> database_table_collection const&;
        auto strings() const -> database_string_collection const&;
        auto blobs()   const -> database_stream const&;
        auto guids()   const -> database_stream const&;

        auto is_initialized() const -> bool;

        friend auto operator==(database const&, database const&) -> bool;
        friend auto operator< (database const&, database const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(database)

    private:

        database(database const&);
        auto operator=(database const&) -> void;

        database_stream _blobs;
        database_stream _guids;

        database_string_collection _strings;
        database_table_collection  _tables;

        file_range _file;
    };





    /// Iterator that facilitates random access iteration over the rows of a metadata table
    ///
    /// This iterator provides random access over the rows of a table in a metadata database.  It
    /// materializes the pointed-to row object when it is indirected, so the result of `*it` is not
    /// an lvalue.
    template <table_id Id>
    class row_iterator
    {
    public:

        typedef typename token_type_for_table_id<Id>::type token_type;

        typedef std::random_access_iterator_tag            iterator_category;
        typedef core::difference_type                      difference_type;
        typedef typename row_type_for_table_id<Id>::type   value_type;
        typedef value_type                                 reference;
        typedef core::indirectable<value_type>             pointer;

        /// Creates a `row_iterator` from a pointer to the initial byte of the row in its table
        static auto from_row_pointer(database const*           const scope,
                                     core::const_byte_iterator const iterator) -> row_iterator
        {
            core::assert_not_null(scope);

            database_table const& table(scope->tables()[Id]);

            // If the table has no rows, all iterators into the table are equal, and all iterators
            // into the table are end iterators:
            if (table.row_size() == 0 || iterator == nullptr)
                return row_iterator();

            return row_iterator(scope, core::convert_integer((iterator - begin(table)) / table.row_size()));
        }

        row_iterator() { }

        row_iterator(database const* const scope, core::size_type const index)
            : _scope(scope), _index(index)
        {
            core::assert_not_null(scope);
            core::assert_true([&]{ return index != base_token::invalid_value;       });
            core::assert_true([&]{ return index <= scope->tables()[Id].row_count(); });
        }      

        /// Gets the metadata token that represents the pointed-to row
        auto token() const -> token_type
        {
            core::assert_initialized(*this);
            return token_type(_scope.get(), Id, _index.get());
        } 

        auto get()        const -> reference { return value(); }
        auto operator*()  const -> reference { return value(); }
        auto operator->() const -> pointer   { return value(); }

        auto operator++()    -> row_iterator& { core::assert_initialized(*this); ++_index.get(); return *this; }
        auto operator++(int) -> row_iterator  { row_iterator const it(*this); ++*this; return it;              }

        auto operator--()    -> row_iterator& { core::assert_initialized(*this); --_index.get(); return *this; }
        auto operator--(int) -> row_iterator  { row_iterator const it(*this); --*this; return it;              }

        auto operator+=(difference_type const n) -> row_iterator&
        {
            core::assert_initialized(*this);
            _index.get() = core::convert_integer(static_cast<difference_type>(_index.get()) + n);
            return *this;
        }

        auto operator-=(difference_type const n) -> row_iterator&
        {
            core::assert_initialized(*this);
            _index.get() = core::convert_integer(static_cast<difference_type>(_index.get()) - n);
            return *this;
        }

        auto operator[](difference_type const n) const -> reference
        {
            core::assert_initialized(*this);
            return _scope.get()->template get_row<value_type>(_index.get() + n);
        }

        auto is_initialized() const -> bool
        {
            return _scope.get() != nullptr && _index.get() != base_token::invalid_value;
        }

        friend auto operator+(row_iterator it, difference_type const n) -> row_iterator { return it +=  n; }
        friend auto operator+(difference_type const n, row_iterator it) -> row_iterator { return it +=  n; }
        friend auto operator-(row_iterator it, difference_type const n) -> row_iterator { return it += -n; }

        friend auto operator-(row_iterator const& lhs, row_iterator const& rhs) -> difference_type
        {
            assert_comparable(lhs, rhs);
            return lhs._index.get() - rhs._index.get();
        }

        friend auto operator==(row_iterator const& lhs, row_iterator const& rhs) -> bool
        {
            assert_comparable(lhs, rhs);
            return lhs._index.get() == rhs._index.get();
        }

        friend auto operator<(row_iterator const& lhs, row_iterator const& rhs) -> bool
        {
            core::assert_initialized(lhs);
            core::assert_initialized(rhs);
            assert_comparable(lhs, rhs);
            return lhs._index.get() < rhs._index.get();
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(row_iterator)

    private:

        static auto assert_comparable(row_iterator const& lhs, row_iterator const& rhs) -> void
        {
            core::assert_true([&]{ return lhs._scope.get() == rhs._scope.get(); });
        }

        auto value() const -> reference
        {
            core::assert_initialized(*this);
            return (*_scope.get())[token()];
        }

        core::value_initialized<database const*> _scope;
        core::value_initialized<core::size_type> _index;
    };





    /// @}

} }

#endif

// AMDG //
