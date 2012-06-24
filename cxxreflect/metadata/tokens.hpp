
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_TOKENS_HPP_
#define CXXREFLECT_METADATA_TOKENS_HPP_

#include "cxxreflect/metadata/constants.hpp"

namespace cxxreflect { namespace metadata {

    class database;

    /// \defgroup cxxreflect_metadata_tokens Metadata -> Tokens
    ///
    /// Token types used for representing rows in metadata tables and signature blobs on disk
    ///
    /// The token type system constructed here provides a (relatively) type-safe way of referencing
    /// elements in a metadata database without significant performance overhead.  The core of the
    /// token type system is the `restricted_token<Mask>`.  The mask specifies the restricted set
    /// of tables in which the token can reference entities.  For example, the `type_def_token` is
    /// specialized such that it can only refer to rows in the TypeDef table, and the token type
    /// `type_def_ref_spec_token` type is specialized such that it can refer to rows in the TypeDef,
    /// TypeRef, or TypeSpec tables.
    ///
    /// A more restrictive token type may be implicitly converted to a less restrictive token type,
    /// similar to how a pointer to a derived class may be implicitly converted to a pointer to one
    /// of its base classes.  For example, there is an implicit conversion from `type_def_token` to
    /// `type_def_ref_spec_token` because all possible values representable by the former are
    /// representable by the latter.
    ///
    /// It is possible to convert from a less restrictive token type to a more restrictive token
    /// type (e.g. from `type_def_token` to `type_def_ref_spec_token`), but the conversion must be
    /// done using the `as` member function template.  The rule is that all implicit conversions are
    /// safe.  Any conversion which may fail is made explicit via the `as<>` member function
    /// template.
    ///
    /// Every token contains three pieces of information:  (1) the scope in which the element to
    /// which the token refers is located, (2) the table into which the token refers, and (3) the
    /// index of the row in the table to which the token refers.
    ///
    /// A token therefore has all of the information required to resolve the row to which it refers.
    /// If a token `t` is of a type that can refer to only a single table (e.g. a `type_def_token`),
    /// then the row to which the token refers may be resolved via:
    ///
    ///     t.scope()[t]
    ///
    /// There is a nonmember function to help with this, so that t needs only be evaluated once:
    ///
    ///     row_from(t)
    ///
    /// This function will be found via ADL, so long as `t` is a `restricted_token`.
    ///
    /// The other kind of entity in a metadata database is a blob, which is an array of bytes that
    /// usually represents a signature, but can also represent practically anything else, like a
    /// GUID or a constant value.  Blobs are represented by the `blob` type, which is basically a
    /// token type for blobs.  All blobs are of the same kinf, however, so there is only one blob 
    /// type.  To interpret a blob as a particular kind of signature, convert it to that signature
    /// type (e.g., to `type_signature`).
    ///
    /// There are many cases where one may have either a token or a blob.  For example, the `type`
    /// class type can refer either to a TypeDef directly or it can refer to a part of a type
    /// signature.  To handle this case, we provide the `restricted_token_or_blob` class template.
    /// This is effectively a discriminated union that can hold either a blob or a particular
    /// specialization of a restricted token.  To use this type, you must inspect the tag to find
    /// which kind of entity is represented, then convert the object to either a token or a blob.
    ///
    /// Note that most of the type checking is done only in debug builds.  Within the metadata
    /// library, any error that might originate from malformed metadata is caught before creation
    /// of a token or blob, so such errors should not require error checking within the token
    /// type system.  Similarly, user code that constructs tokens should ensure that they are
    /// constructing the tokens with valid data.  Errors will be caught in debug builds and will
    /// cause `core::assertion_error` exceptions to be thrown.
    ///
    /// @{





    /// Base class defining common functionality shared by all of the token types
    ///
    /// A token is a reference to a row in a table of a metadata database (a `database`).  It thus
    /// is composed of three parts:  a scope (the database), a table identifier, and a row index.
    ///
    /// The only types that derive from this base class are specializations of `restricted_token`.
    /// This base class defines all of the common features of the restricted tokens (scope, table,
    /// and index values, along with comparison operations).
    ///
    /// It not only reduces the number of entities that must be instantiated separately for each
    /// restricted token specialization, it also allows the `base_token_with_arithmetic` to be
    /// injected into the class hierarchy so that arithmetic operations may be added to any token
    /// type.
    ///
    /// A default-constructed token is considered to be uninitialized.  The only member function
    /// that may be called on an uninitialized token is `is_initialized()`.  Uninitialized tokens
    /// may also be compared with each other, but they may not be compared with initialized tokens.
    /// These constraints are checked only when debug assertions are enabled.
    class base_token
    {
    public:

        /// Type used for representing the table and index internally
        ///
        /// The token value is represented by a 32-bit integer, with the upper eight bits containing
        /// a table identifier and the lower 24 bits containing the zero-based index of a row in the
        /// table.
        ///
        /// Note that this internal representation is not the same as the value returned by the
        /// `value()` member function.  The `value()` member function returns the metadata token,
        /// which uses one-based indexing (an index of zero is a null token value), but our internal
        /// representation uses zero-based indexing (an index of `0x00ffffff` is a null token value)
        /// to make computations elsewhere in the library simpler more consistent.
        typedef core::size_type value_type;

        enum : value_type
        {
            invalid_value = core::max_size_type,

            table_bits = 8,
            index_bits = 24,

            table_mask = 0xff000000,
            index_mask = 0x00ffffff
        };

        /// The scope of this token; this is the `database` into which this token points
        auto scope() const -> database const&
        {
            core::assert_initialized(*this);
            return *_scope.get();
        }

        /// The table into which this token points
        auto table() const -> table_id
        {
            core::assert_initialized(*this);
            return static_cast<table_id>((_value.get() & table_mask) >> index_bits);
        }

        /// The index of the row to which this token points
        auto index() const -> value_type
        {
            core::assert_initialized(*this);
            return _value.get() & index_mask;
        }

        /// The token value; this is equivalent to a metadata token and contains the table and index
        auto value() const -> value_type
        {
            core::assert_initialized(*this);
            return _value.get() + 1;
        }

        auto is_initialized() const -> bool
        {
            return _scope.get() != nullptr && _value.get() != invalid_value;
        }

        friend auto operator==(base_token const& lhs, base_token const& rhs) -> bool
        {
            core::assert_true([&]{ return lhs.is_initialized() == rhs.is_initialized(); });

            return std::make_tuple(lhs._scope.get(), lhs._value.get())
                == std::make_tuple(rhs._scope.get(), rhs._value.get());
        }

        friend auto operator<(base_token const& lhs, base_token const& rhs) -> bool
        {
            core::assert_true([&]{ return lhs.is_initialized() == rhs.is_initialized(); });

            return std::make_tuple(lhs._scope.get(), lhs._value.get())
                <  std::make_tuple(rhs._scope.get(), rhs._value.get());
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(base_token)

    protected:

        base_token() { }

        base_token(database const* const scope, value_type const token)
            : _scope(scope), _value(token - 1)
        {
            core::assert_not_null(scope);
            core::assert_true([&]{ return is_valid_table_id(table()); });
        }
        
        base_token(database const* const scope, table_id const table, value_type const index)
            : _scope(scope), _value(compose_value(table, index))
        {
            core::assert_not_null(scope);
            // compose_value will verify the correctness of the table and index arguments
        }

        ~base_token() { }

    private:

        /// Combines a table id and an index into a single value, for compactness
        ///
        /// Note:  The value composed by this function is one less than the value returned by
        /// `value()`.  A metadata token (like that returned by `value()` uses one-based indexing
        /// with an index value of zero being a null token.  We use zero-based indexing everywhere,
        /// including in the `_value` data member, for consistency and to help to avoid off-by-one
        /// errors.
        static auto compose_value(table_id const table, value_type const index) -> value_type
        {
            core::assert_true([&]{ return is_valid_table_id(table);                    });
            core::assert_true([&]{ return core::as_integer(table) < (1 << table_bits); });
            core::assert_true([&]{ return index <  index_mask;                         });

            value_type const table_component((core::as_integer(table) << index_bits) & table_mask);
            value_type const index_component(index & index_mask);

            return table_component | index_component;
        }

        core::value_initialized<database const*> _scope;
        core::value_initialized<value_type     > _value;
    };





    /// Intermediate base class defining arithmetic operations for a token type
    ///
    /// This class uses CRTP to provide arithmetic operators for a `restricted_token`.  Most of the
    /// time we do not want to allow arithmetic operations for tokens because usually they do not
    /// make sense.  However, there are a few select circumstances in which we use tokens for
    /// iteration over the rows of a table, so we need to be able to perform pointer-like arithmetic
    /// on tokens.
    ///
    /// This intermediate base class sits between the `base_token` and a `restricted_token` class
    /// template specialization to inject all of the pointer-like arithmetic operators into the
    /// token type, allowing tokens to be treated like non-dereferenceable pointers.
    ///
    /// This type relies on CRTP, so a derived class must provide its own type as the template
    /// argument.  Failure to do this will make things not work right.
    template <typename DerivedType>
    class base_token_with_arithmetic : public base_token
    {
    public:

        friend auto operator++(DerivedType& x) -> DerivedType&
        {
            core::assert_true([&]{ return x.index() != index_mask; });

            x = DerivedType(&x.scope(), x.value() + 1);
            return x;
        }

        friend auto operator++(DerivedType& x, int) -> DerivedType
        {
            DerivedType const copy(x);
            ++x;
            return copy;
        }

        friend auto operator--(DerivedType& x) -> DerivedType&
        {
            core::assert_true([&]{ return x.index() != 0; });

            x = DerivedType(&x.scope(), x.value() - 1);
            return x;
        }

        friend auto operator--(DerivedType& x, int) -> DerivedType
        {
            DerivedType const copy(x);
            --x;
            return copy;
        }

        friend auto operator+(DerivedType x, core::difference_type const n) -> DerivedType
        {
            x += n;
            return x;
        }

        friend auto operator+(core::difference_type const n, DerivedType const x) -> DerivedType
        {
            return x + n;
        }

        friend auto operator-(DerivedType x, core::difference_type n) -> DerivedType
        {
            x -= n;
            return x;
        }

        friend auto operator-(DerivedType const& lhs, DerivedType const& rhs) -> core::difference_type
        {
            // We primarily use arithmetic tokens for iteration.  When we compare iterators for
            // equality, we typically compute the difference between the iterators to check if the
            // range is empty.  Since we use uninitialized tokens to represent an empty range, we
            // return zero as the difference between two uninitialized tokens.
            if (!lhs.is_initialized() && !rhs.is_initialized())
                return 0;

            return static_cast<core::difference_type>(lhs.index())
                 - static_cast<core::difference_type>(rhs.index());
        }

        friend auto operator+=(DerivedType& x, core::difference_type const n) -> DerivedType&
        {
            core::assert_true([&]{ return is_in_range(x, false, n); });

            x = DerivedType(&x.scope(), x.value() + n);
            return x;
        }

        friend auto operator-=(DerivedType& x, core::difference_type const n) -> DerivedType&
        {
            core::assert_true([&]{ return is_in_range(x, true, n); });

            x = DerivedType(&x.scope(), x.value() - n);
            return x;
        }

    protected:

        base_token_with_arithmetic() { }

        base_token_with_arithmetic(database const* const scope, value_type const token)
            : base_token(scope, token)
        {
        }
        
        base_token_with_arithmetic(database const* const scope, table_id const table, value_type const index)
            : base_token(scope, table, index)
        {
        }

        base_token_with_arithmetic(base_token const& other)
            : base_token(other)
        {
        }

        ~base_token_with_arithmetic() { }

    private:

        static auto is_in_range(DerivedType           const& x,
                                bool                  const  is_subtraction,
                                core::difference_type const n) -> bool
        {
            static core::difference_type const difference_mask(static_cast<core::difference_type>(index_mask));

            // Check if the computation cannot possibly result in an in-range value:
            if ((n < 0) ? (n < -difference_mask) : (n > difference_mask))
                return false;
            
            // Otherwise, perform the arithmetic and see if we'll overflow.  Note that the above
            // check ensures that we are only working with 24 bits, so we cannot possibly end up
            // with actual overflow (the undefined kind).

            core::difference_type const multiplier(is_subtraction ? -1 : 1);
            core::difference_type const addition_n(n * multiplier);

            core::difference_type const difference_index(core::convert_integer(x.index()));

            core::difference_type const result(difference_index + addition_n);
            return result >= 0 && result < table_mask;
        }
    };





    /// A restricted token class template that may refer to a row in a restricted set of tables
    ///
    /// This is the most-derived token type.  All of the token types used throughout the library are
    /// specializations of this class template.  This class template has two template parameters:
    /// the first is the `Mask`, which specifies the restricted set of tables that this token may
    /// represent.  The second is `WithArithmetic`.  If this is `true`, then the specialization is
    /// derived from `base_token_with_arithmetic`; if this is `false`, then the specialization is
    /// derived directly from `base_token`.
    ///
    /// The restrictedness of this token type allows substantial static verification that we are
    /// correctly handling all possible cases, especially in scenarios where we have a token that
    /// might refer to a row in one of a number of tables.
    ///
    /// All of the constraints specified in the `base_token` documentation hold, notably those
    /// concerning the uninitialized state.  Note that implicit conversions are checked statically,
    /// at compile time, and are thus always enabled.  Dynamic conversions (using the `as()` member
    /// function template) are checked at runtime and are only checked when debug assertions are
    /// enabled.
    template <table_mask Mask, bool WithArithmetic>
    class restricted_token
        : public std::conditional<
              WithArithmetic,
              base_token_with_arithmetic<restricted_token<Mask, WithArithmetic>>,
              base_token>::type
    {
    public:

        typedef typename std::conditional<
            WithArithmetic,
            base_token_with_arithmetic<restricted_token<Mask, WithArithmetic>>,
            base_token
        >::type base_type;

        typedef metadata::table_mask         mask_type;
        typedef metadata::integer_table_mask integer_mask_type;

        static mask_type         const mask           = Mask;
        static integer_mask_type const integer_mask   = static_cast<integer_mask_type>(mask);
        static bool              const has_arithmetic = WithArithmetic;

        restricted_token() { }

        restricted_token(database const* const scope, core::size_type const token)
            : base_type(scope, token)
        {
            core::assert_true([&]{ return (mask & table_mask_for(this->table())) != 0; });   
        }

        restricted_token(database const* const scope, table_id const table, core::size_type const index)
            : base_type(scope, table, index)
        {
            core::assert_true([&]{ return (mask & table_mask_for(table)) != 0; });   
        }

        /// Converting constructor that allows "widening" token conversions
        ///
        /// This converting constructor allows only safe conversions.  For example, it will allow a
        /// TypeDef token to be converted to a TypeDefOrRefOrSpec token.  It will disallow any
        /// conversions where the target token type cannot represent all of the tables representable
        /// by the source token type.
        ///
        /// To perform unsafe conversions, use the `is()` and `as()` member functions.
        template <mask_type SourceMask, bool BaseFlag>
        restricted_token(restricted_token<SourceMask, BaseFlag> const& other,
                         typename std::enable_if<
                            integer_value_of_mask<SourceMask>::value == 
                            (integer_value_of_mask<SourceMask>::value & integer_mask)
                         >::type* = nullptr)
            : base_type(other)
        {
        }

        /// Tests whether the table into which this token refers is one of those in `TargetMask`
        template <mask_type TargetMask>
        auto is() const -> bool
        {
            return (table_mask_for(this->table()) & TargetMask) != 0;
        }

        /// Tests whether this token can be successfully converted to `Target` using `as<T>()`
        template <typename Target>
        auto is() const -> bool
        {
            return is<Target::mask>();
        }

        /// Converts this token to a `restricted_token<TargetMask, WithArithmetic>` token
        ///
        /// This token must refer to a table that is represented in the target mask.  This is only
        /// checked if debug assertions are enabled.  Be sure to call `is<Mask>()` to see whether
        /// this call would succeed.
        template <mask_type TargetMask>
        auto as() const -> restricted_token<TargetMask, WithArithmetic>
        {
            core::assert_true([&]{ return is<TargetMask>(); });
            return restricted_token<TargetMask, WithArithmetic>(&this->scope(), this->value());
        }

        /// Converts this token to a `Target` token
        ///
        /// `Target` must be a specialization of `restricted_token`.  It performs the same checks
        /// and conversions as the other `as` member function.
        template <typename Target>
        auto as() const -> Target
        {
            return as<Target::mask>();
        }
    };





    /// Resolves a `restricted_token` in its scope and returns the row object for the pointed-to row
    ///
    /// The `restricted_token` type must have a unique mask (that is, the token type must only be
    /// able to refer to a row in a single table).  So, a `type_def_token` is a valid argument type,
    /// but a `type_def_or_ref_or_spec` token is not, because it can refer to a row in one of three
    /// tables.  This restriction is required because each row has a different type.
    template <table_mask Mask, bool WithArithmetic>
    auto row_from(restricted_token<Mask, WithArithmetic> const& t) -> typename row_type_for_mask<Mask>::type
    {
        core::assert_initialized(t);

        return t.scope()[restricted_token<Mask, false>(t)];
    }





    // Note:  The unique token types (those that can refer only to a single table) are typedef'ed in
    // the constants header, using the same macros that are used for the metafunctions that convert
    // between constants and token types.  The tokens defined here are the non-unique token types,
    // each of which can represent a row in one of several tables.  We only define types for non-
    // unique tokens that either are found natively in metadata or which are created elsewhere in
    // the library.
    
    /// Token that can refer to a row in any table of the metadata database
    typedef restricted_token<(table_mask)-1> unrestricted_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::assembly     |
        (integer_table_mask)table_mask::assembly_ref
    )> assembly_or_assembly_ref_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::method_def |
        (integer_table_mask)table_mask::member_ref
    )> custom_attribute_type_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::field    |
        (integer_table_mask)table_mask::param    |
        (integer_table_mask)table_mask::property
    )> has_constant_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::method_def               |
        (integer_table_mask)table_mask::field                    |
        (integer_table_mask)table_mask::type_ref                 |
        (integer_table_mask)table_mask::type_def                 |
        (integer_table_mask)table_mask::param                    |
        (integer_table_mask)table_mask::interface_impl           |
        (integer_table_mask)table_mask::member_ref               |
        (integer_table_mask)table_mask::module                   |
        (integer_table_mask)table_mask::decl_security            |
        (integer_table_mask)table_mask::property                 |
        (integer_table_mask)table_mask::event                    |
        (integer_table_mask)table_mask::standalone_sig           |
        (integer_table_mask)table_mask::module_ref               |
        (integer_table_mask)table_mask::type_spec                |
        (integer_table_mask)table_mask::assembly                 |
        (integer_table_mask)table_mask::assembly_ref             |
        (integer_table_mask)table_mask::file                     |
        (integer_table_mask)table_mask::exported_type            |
        (integer_table_mask)table_mask::manifest_resource        |
        (integer_table_mask)table_mask::generic_param            |
        (integer_table_mask)table_mask::generic_param_constraint |
        (integer_table_mask)table_mask::method_spec             
    )> has_custom_attribute_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_def   |
        (integer_table_mask)table_mask::method_def |
        (integer_table_mask)table_mask::assembly  
    )> has_decl_security_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::field |
        (integer_table_mask)table_mask::param
    )> has_field_marshal_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::event    |
        (integer_table_mask)table_mask::property
    )> has_semantics_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::file          |
        (integer_table_mask)table_mask::assembly_ref  |
        (integer_table_mask)table_mask::exported_type
    )> implementation_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::field      |
        (integer_table_mask)table_mask::method_def
    )> member_forwarded_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_def   |
        (integer_table_mask)table_mask::type_ref   |
        (integer_table_mask)table_mask::module_ref |
        (integer_table_mask)table_mask::method_def |
        (integer_table_mask)table_mask::type_spec 
    )> member_ref_parent_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::method_def |
        (integer_table_mask)table_mask::member_ref
    )> method_def_or_ref_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::module       |
        (integer_table_mask)table_mask::module_ref   |
        (integer_table_mask)table_mask::assembly_ref |
        (integer_table_mask)table_mask::type_ref    
    )> resolution_scope_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_def  |
        (integer_table_mask)table_mask::type_spec
    )> type_def_spec_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_def  |
        (integer_table_mask)table_mask::type_ref  |
        (integer_table_mask)table_mask::type_spec
    )> type_def_ref_spec_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_def   |
        (integer_table_mask)table_mask::method_def
    )> type_or_method_def_token;





    /// Metafunction that adds arithmetic operators to a `restricted_token` specialization
    ///
    /// This is used to convert a `restricted_token<Mask, x>` where `x` is either `true` or `false`
    /// to a `restricted_token<Mask, true>`, that is, it leaves unchanged any token type that
    /// already has operators, and adds operators to any token type that does not have them.
    ///
    /// We could also define a metafunction to remove the arithmetic operators, but so far we have
    /// not seen a need for such a metafunction.
    template <typename RestrictedToken>
    struct token_with_arithmetic
    {
        typedef restricted_token<RestrictedToken::mask, true> type;
    };





    /// A metadata blob, representing a signature, GUID, or other array of bytes from metadata
    ///
    /// A blob is really just an array of bytes.  This simply provides an encapsulation to allow us
    /// to work with them.  Blobs and tokens are the two primary forms of references to metadata
    /// objects.  A `blob` may refer to an element defined in a metadata database or, in the case of
    /// signatures especially, to an array of bytes defining an instantiation; this array of bytes
    /// may be located anywhere in memory, but in practice these blobs have storage duration that
    /// is at least as long as the lifetime of the database from which the blob was instantiated.
    ///
    /// A default-constructed `blob` is considered to be uninitialized, similar to an uninitialized
    /// token.
    class blob
    {
    public:

        blob() { }

        blob(database const*           const scope,
             core::const_byte_iterator const first,
             core::const_byte_iterator const last)
            : _scope(scope), _first(first), _last(last)
        {
            core::assert_not_null(scope);
            core::assert_not_null(first);
            core::assert_not_null(last );
        }

        /// Constructs a blob from a signature
        ///
        /// It is expected that the `Signature` type is one of the signatures provided by the
        /// metadata library.  If it is not, it must match the interface of those signature types.
        /// The provided signature must be initialized.
        template <typename Signature>
        blob(Signature const& signature)
            : _scope(&signature.scope()), _first(signature.begin_bytes()), _last(signature.end_bytes())
        {
            core::assert_initialized(signature);
        }

        /// Gets the scope (`database`) from which the pointed-to blob was obtained
        ///
        /// Note that the pointed-to blob may not be defined _in_ this `database`.  It may also have
        /// been instantiated from this `database`.
        auto scope() const -> database const&
        {
            core::assert_initialized(*this);
            return *_scope.get();
        }

        /// Gets an iterator to the initial byte of the pointed-to blob
        auto begin() const -> core::const_byte_iterator
        {
            core::assert_initialized(*this);
            return _first.get();
        }

        /// Gets a one-past-the-end iterator into the pointed-to blob
        auto end() const -> core::const_byte_iterator
        {
            core::assert_initialized(*this);
            return _last.get();
        }

        auto is_initialized() const -> bool
        {
            return _scope.get() != nullptr && _first.get() != nullptr && _last.get() != nullptr;
        }

        /// Converts this blob to the `Signature` signature type
        ///
        /// It is expected that the `Signature` type is one of the signatures provided by the
        /// metadata library.  If it is not, it must match the interface of those signature types.
        template <typename Signature>
        auto as() const -> Signature
        {
            core::assert_initialized(*this);
            return Signature(&scope(), begin(), end());
        }

        friend auto operator==(blob const& lhs, blob const& rhs) -> bool
        {
            core::assert_true([&]{ return lhs.is_initialized() == rhs.is_initialized(); });

            // Note:  We only compare pointers, not scopes; the scope doesn't matter because the
            // identity of the byte range uniquely identifies the blob.
            return lhs._first.get() == rhs._first.get();
        }

        friend auto operator<(blob const& lhs, blob const& rhs) -> bool
        {
            core::assert_true([&]{ return lhs.is_initialized() == rhs.is_initialized(); });

            // Note:  We only compare pointers, not scopes; the scope doesn't matter because the
            // identity of the byte range uniquely identifies the blob.
            return std::less<core::const_byte_iterator>()(lhs._first.get(), rhs._first.get());
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(blob)

        /// Constructs a new, initialized blob object from metadata
        ///
        /// Blobs in metadata are stored with the length encoded in the first few bytes of the blob.
        /// This function will decode that length, advance the `first` iterator to point to the
        /// first byte of the actual blob data, and use the range `first + [computed length]` for
        /// the `last` iterator.  It then uses these new `first` and `last` iterators to construct a
        /// new blob.
        static auto compute_from_stream(database const*           scope,
                                        core::const_byte_iterator first,
                                        core::const_byte_iterator last) -> blob
        {
            if (first == last)
                throw core::metadata_error(L"invalid blob encoding");

            core::byte initial_byte(*first);
            core::size_type blob_size_bytes(0);
            switch (initial_byte >> 5)
            {
            case 0:
            case 1:
            case 2:
            case 3:
                blob_size_bytes = 1;
                initial_byte   &= 0x7f;
                break;

            case 4:
            case 5:
                blob_size_bytes = 2;
                initial_byte   &= 0x3f;
                break;

            case 6:
                blob_size_bytes = 4;
                initial_byte   &= 0x1f;
                break;

            case 7:
            default:
                throw core::metadata_error(L"invalid blob encoding");
            }

            if (core::distance(first, last) < blob_size_bytes)
                throw core::metadata_error(L"invalid blob encoding");

            core::size_type blob_size(initial_byte);
            for (unsigned i(1); i < blob_size_bytes; ++ i)
                blob_size = (blob_size << 8) + *(first + i);

            if (core::distance(first, last) < blob_size_bytes + blob_size)
                throw core::metadata_error(L"invalid blob encoding");

            return blob(scope, first + blob_size_bytes, first + blob_size_bytes + blob_size);
        }

    private:

        core::value_initialized<database const*>           _scope;
        core::value_initialized<core::const_byte_iterator> _first;
        core::value_initialized<core::const_byte_iterator> _last;
    };





    /// A hybrid type that may represent either a token or a blob
    ///
    /// There are several scenarios in the library where we need to refer either to a row in a table
    /// or to a blob.  Notably, a type may be a type definition, which is represented by a row, or
    /// it may be a type represented by a signature (e.g., a by-ref type or a parameter or a generic
    /// instantiation).
    template <table_mask Mask>
    class restricted_token_or_blob
    {
    public:

        typedef metadata::table_mask         mask_type;
        typedef metadata::integer_table_mask integer_mask_type;

        static mask_type         const mask         = Mask;
        static integer_mask_type const integer_mask = static_cast<integer_mask_type>(mask);

        typedef restricted_token<mask> token_type;
        typedef blob                   blob_type;

        restricted_token_or_blob()
            : _first(nullptr), _size(invalid_size)
        {
        }

        /// Converting constructor to allow implicit conversions from `token_type`
        ///
        /// It is allowed for `value` to be uninitialized; if the `value` is uninitialized, this
        /// will construct an uninitialized `restricted_token_or_blob`.
        restricted_token_or_blob(token_type const& value)
            : _first(nullptr), _size(invalid_size)
        {
            if (!value.is_initialized())
                return;

            _scope.get() = &value.scope();
            _index       = value.value();
            _size.get()  = token_kind;
        }

        /// Converting constructor to allow implicit conversions from `blob_type`
        ///
        /// It is allowed for `value` to be uninitialized; if the `value` is uninitialized, this
        /// will construct an uninitialized `restricted_token_or_blob`.
        restricted_token_or_blob(blob_type const& value)
            : _first(nullptr), _size(invalid_size)
        {
            if (!value.is_initialized())
                return;

            _scope.get() = &value.scope();
            _first       = begin(value);
            _size.get()  = end(value) != nullptr
                ? core::convert_integer(core::distance(begin(value), end(value)))
                : 0;
        }

        /// A converting copy constructor that allows "widening" conversions
        ///
        /// This is similar to the conversion that allows widening of tokens. See `restricted_token`
        /// for details on what conversions are allowed.
        template <mask_type SourceMask>
        restricted_token_or_blob(restricted_token_or_blob<SourceMask> const& value,
                         typename std::enable_if<
                            integer_value_of_mask<SourceMask>::value == 
                            (integer_value_of_mask<SourceMask>::value & integer_mask)
                         >::type* = nullptr)
            : _first(nullptr), _size(invalid_size)
        {
            if (!value.is_initialized())
                return;

            _scope.get() = &value.scope();

            if (value.is_blob())
            {
                _first = value.as_blob().begin();
                _size.get() = core::distance(value.as_blob().begin(), value.as_blob().end());
            }
            else
            {
                _index = value.as_token().value();
                _size.get() = token_kind;
            }
        }

        /// A converting constructor that allows "widening" conversions from a `restricted_token`
        ///
        /// This is similar to the conversion that allows widening of tokens. See `restricted_token`
        /// for details on what conversions are allowed.
        template <mask_type SourceMask, bool BaseFlag>
        restricted_token_or_blob(restricted_token<SourceMask, BaseFlag> const& value,
                         typename std::enable_if<
                            integer_value_of_mask<SourceMask>::value == 
                            (integer_value_of_mask<SourceMask>::value & integer_mask)
                         >::type* = nullptr)
            : _first(nullptr), _size(invalid_size)
        {
            if (!value.is_initialized())
                return;

            _scope.get() = &value.scope();
            _index       = value.value();
            _size.get()  = token_kind;
        }

        /// Tests whether this object contains a `token`
        auto is_token() const -> bool
        {
            return is_initialized() && (_size.get() & kind_mask) == token_kind;
        }

        /// Tests whether this object contains a `blob`
        auto is_blob() const -> bool
        {
            return is_initialized() && (_size.get() & kind_mask) == blob_kind;
        }

        auto is_initialized() const -> bool
        {
            if (_scope.get() == nullptr)
                return false;

            if (_size.get() == invalid_size)
                return false;

            if ((_size.get() & kind_mask) == token_kind)
                return _index != base_token::invalid_value;
            else
                return _first != nullptr;
        }

        /// Converts this object to the `token` it represents
        ///
        /// This object must contain a token, otherwise the results are undefined (this is checked
        /// at runtime only when debug assertions are enabled).
        auto as_token() const -> token_type
        {
            core::assert_true([&]{ return is_token(); });
            return token_type(_scope.get(), _index);
        }

        /// Converts this object to the `blob` it represents
        ///
        /// This object must contain a blob, otherwise the results are undefined (this is checked
        /// at runtime only when debug assertions are enabled).
        auto as_blob() const -> blob_type
        {
            core::assert_true([&]{ return is_blob(); });

            core::const_byte_iterator const last(_first != nullptr ? _first + _size.get() : nullptr);

            return blob_type(_scope.get(), _first, last);
        }

        /// The scope into which the blob or token points
        ///
        /// This is the only part that is shared between both blobs and tokens, so we provide direct
        /// access to it if this object is initialized.
        auto scope() const -> database const&
        {
            core::assert_initialized(*this);

            return *_scope.get();
        }

        friend auto operator==(restricted_token_or_blob const& lhs, restricted_token_or_blob const& rhs) -> bool
        {
            core::assert_true([&]{ return lhs.is_initialized() == rhs.is_initialized(); });

            // For consistency with base_token, we allow comparisons between uninitialized tokens;
            // all such tokens compare equal
            if (!lhs.is_initialized() && !rhs.is_initialized())
                return true;

            return std::make_tuple(lhs._scope.get(), lhs.is_blob(), lhs.comparable_value())
                == std::make_tuple(rhs._scope.get(), rhs.is_blob(), rhs.comparable_value());
        }

        friend auto operator<(restricted_token_or_blob const& lhs, restricted_token_or_blob const& rhs) -> bool
        {
            core::assert_true([&]{ return lhs.is_initialized() == rhs.is_initialized(); });

            // For consistency with base_token, we allow comparisons between uninitialized tokens;
            // all such tokens compare equal
            if (!lhs.is_initialized() && !rhs.is_initialized())
                return false;

            return std::make_tuple(lhs._scope.get(), lhs.is_blob(), lhs.comparable_value())
                <  std::make_tuple(rhs._scope.get(), rhs.is_blob(), rhs.comparable_value());
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(restricted_token_or_blob)

    private:

        // We represent the kind of object (blob or token) using the high bit of the `_size` data
        // member.  The `_size` is only used for blobs, and it is known that no blob will exceed
        // two gigabytes in size (if one did, we'd be in a lot of trouble :-D).
        enum : core::size_type
        {
            kind_mask     = 0x80000000,
            token_kind    = 0x80000000,
            blob_kind     = 0x00000000,

            invalid_size  = 0xffffffff
        };

        /// Gets an integer value that contains `_index` or `_first`, whichever is set
        auto comparable_value() const -> std::uintptr_t
        {
            return is_blob() ? reinterpret_cast<std::uintptr_t>(_first) : static_cast<std::uintptr_t>(_index);
        }

        core::value_initialized<database const*> _scope;

        union
        {
            core::size_type           _index;
            core::const_byte_iterator _first;
        };

        core::value_initialized<core::size_type> _size;
    };
    




    typedef restricted_token_or_blob<table_mask::type_def> type_def_or_signature;

    typedef restricted_token_or_blob<(table_mask)(
        (integer_table_mask)table_mask::type_def  |
        (integer_table_mask)table_mask::type_spec
    )> type_def_spec_or_signature;

    typedef restricted_token_or_blob<(table_mask)(
        (integer_table_mask)table_mask::type_def  |
        (integer_table_mask)table_mask::type_ref  |
        (integer_table_mask)table_mask::type_spec
    )> type_def_ref_spec_or_signature;





    /// @}

} }

#endif

// AMDG //
