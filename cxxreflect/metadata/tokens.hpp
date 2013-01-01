
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_TOKENS_HPP_
#define CXXREFLECT_METADATA_TOKENS_HPP_

#include "cxxreflect/metadata/constants.hpp"

namespace cxxreflect { namespace metadata {

    class database;

    /// \defgroup cxxreflect_metadata_tokens Metadata -> Tokens and Signatures
    ///
    /// Types for representing a reference to a row in a metadata table or to a signature blob
    ///
    ///
    ///
    /// **Terminology**
    ///
    /// A metadata database is a set of tables (see the `table_id` enumeration for the list).  Each
    /// table has a sequence of rows.  The database is a partially-denormalized relational database,
    /// so there are relationships between the tables.  A reference to a row in a database table is
    /// called a "token."
    ///
    /// A metadata database also contains signature information, represented in blobs.  A blob is
    /// simply reference to a sequence of bytes.
    ///
    /// Each token and blob has a pointer to the metadata database from which it originated:  this
    /// pointer is called the "scope."
    ///
    ///
    ///
    /// **What is a token?**
    ///
    /// A token consists of three parts:
    ///
    ///  * Its scope
    ///  * A `table_id` identifier identifying the table in which the row is located
    ///  * The index of the referenced row in the table.
    ///
    /// The concept of a "token" comes from the CLI specification (ECMA-335).  In that specification
    /// a token is a 32-bit integer in which the upper eight bits encode the table identifier and
    /// the lower 24 bits encode the index of the row.  In this form, rows are indexed starting from
    /// one.  A token value of zero is a "null token."
    ///
    /// This metadata library represents a token similarly, with one small difference:  we adjust
    /// the row index so that it is zero-based instead of one-based.  A token with a row index of
    /// `0x00ffffff` is a null token (remember:  only 24 bits are used to represent the row index).
    /// We do this to avoid confusion:  zero-based indices are much easier to deal with.
    ///
    ///
    ///
    /// **What is a blob?**
    ///
    /// A blob is an arbitrary sequence of bytes, often read from the blob heap of a metadata
    /// database, but it may refer elsewhere too.  For example, we initially refer to GUIDs as blobs
    /// even though they are stored in the GUID heap.  More commonly, we may instantiate a signature
    /// into a buffer not directly associated with a database.  In this case, we still refer to that
    /// signature's bytes as a blob.
    ///
    /// A blob consists of three parts:
    ///
    ///  * Its scope
    ///  * A pointer to its initial byte
    ///  * A pointer to one past its last byte
    ///
    /// As described above, a blob may not be located in a metadata database.  However, it will
    /// always be derived from a blob that was obtained from a metadata database.  When a blob
    /// refers to an instantiated signature, the scope of that blob will be the metadata database
    /// from which we obtained the original blob.
    ///
    ///
    ///
    /// **What is the purpose of all of these types?**
    ///
    /// A token is just a pointer and a 32-bit integer and a blob is really just a set of three
    /// pointers, so we could easily represent these with very few lines of code.  Why all the 
    /// complexity?
    ///
    /// The types defined here provide type safety benefits and make it much harder to write
    /// incorrect code.  Most of the logic is designed to restrict the usage of tokens, so we'll
    /// describe an example involving tokens to demonstrate the purpose of these types.
    ///
    /// As we have already described, a token is a 32-bit integer that can refer to any row in any
    /// table.  This is a useful and compact representation.  However, most of the time, when we
    /// have a token, it can only refer to a row in one particular table or in one of a small set
    /// of tables.  For example, when we have a type definition (TypeDef) and we get the token for
    /// its first field, we know that it will refer to a row in the FieldDef table.  If it doesn't
    /// then the metadata database is ill-formed.
    ///
    /// Similarly, if we have a GenericParam row and we want to find its parent, we know that the
    /// parent will be a row in the TypeDef or MethodDef table.  It is not possible for the owner
    /// to be from any other table.  (In many cases, these restrictions are not only mandated by the
    /// specification, they are often imposed by the way tokens are represented in metadata:  it is
    /// often impossible for an invalid token to be represented).
    ///
    /// If we simply represent a token value using a 32-bit integer or using a thin facade around an
    /// integer, we lose this information:  a 32-bit integer can represent a row in any table with
    /// no restrictions.  This means that we must use many checks at runtime to verify that we have
    /// the right kind of token:  every argument must be checked and every return value must be
    /// checked.  This leads to a substantial amount of overhead and adds a lot of unnecessary
    /// error-checking boilerplate to the code.
    ///
    ///
    ///
    /// **Ok, so how have we solved this problem?**
    ///
    /// Templates are the solution to every problem in C++, so we have used templates to solve this
    /// problem and to help to encode table restrictions such that the C++ type system will check
    /// our token usage for us.
    ///
    /// There are currently 38 metadata tables.  Each metadata table has an identifier.  The
    /// identifiers are in the range [0,44].  We define a bitfield, named `table_mask`, which has
    /// one enumerator per table identifier and the value of each enumerator is `1 << n`, where `n`
    /// is the numeric value of the table identifier.  With a bitfield of this type we can
    /// represent any subset of tables.
    ///
    /// We define a class template named `restricted_token<Mask>`.  The `Mask` template argument is
    /// a value of type `table_mask`.  A specialization of this class template may contain a token
    /// that refers to a row in any table specified in the mask.  Consider a few examples:
    ///
    ///  * `restricted_token<module>` can refer only to a row in the Module table.
    ///
    ///  * `restricted_token<type_def | method_def>` can refer to a row in either the TypeDef or the
    ///    MethodDef table.
    ///
    /// Because we construct a restricted token from an integer value that we obtain from a metadata
    /// database, we verify in its constructor that it is being initialized with a token value that
    /// refers to an allowed table.  This check is done at runtime.  Because we only construct a
    /// token in a handful of places in the metadata database code, this check is only really done
    /// in a handful of places in the code, leaving a small surface area for error.
    ///
    /// Only safe conversions between `restricted_token<Mask>` specializations are only allowed.  In
    /// effect, only "widening" conversions are implicit:  a conversion is only allowed if all valid
    /// values for the source specialization are valid in the target specialization.  Let's consider
    /// a few examples:
    ///
    ///  * A `restricted_token<type_def>` is convertible to a `restricted_token<type_def>` because
    ///    they both have the same `Mask` value.  In short, copy construction is permitted.  This
    ///    should be obvious.
    ///
    ///  * A `restricted_token<type_def | type_ref>` is implicitly convertible to a 
    ///    `restricted_token<type_def | type_ref | type_spec>`.  This is because every value of the
    ///    source token type is also a valid value for the target token type:  the source can
    ///    represent any row in the TypeDef or TypeRef tables and the target can represent any row
    ///    in the TypeDef, TypeRef, or TypeSpec tables.
    ///
    ///  * The converse is not true:  a `restricted_token<type_def | type_ref | type_spec>` is not
    ///    implicitly convertible to a `restricted_token<type_def | type_ref>`.  This is because the
    ///    source token may refer to a row in the TypeSpec table, which is not valid for the target
    ///    token type.
    ///
    /// Using bitwise arithmetic, we can say that a conversion from mask `S` to mask `T` is allowed
    /// if and only if `S == (S & T)`.  That is, if every bit in `S` is also set in `T`.  We enforce
    /// this constraint via SFINAE-enabled converting constructors.
    ///
    /// This is all well and good, except that if we have a token that refers to a row in one of a
    /// set of tables, we'll eventually need to convert it somehow to a more restrictive type.  For
    /// example, given a `restricted_token<type_def | method_def>`, we'll eventually need to get
    /// either a `restricted_token<type_def>` or a `restricted_token<method_def>` from it.
    ///
    /// To do  this, the `restricted_token<Mask>` provides `is<Target>()` and `as<Target>()` member
    /// functions.  `as<Target>()` converts the token to a more restrictive type.  If the conversion
    /// is invalid (i.e., if the value of the source is not representable by the target type), it
    /// fails with an assertion.  The `is<Target>()` tests whether `as<Target>()` would succeed.  It
    /// is the responsibility of the caller to be sure to test `is<Target>()` before calling 
    /// `as<Target>()` because these checks are only done when debug assertions are enabled (the
    /// checks are simple, but enormously expensive due to their frequency).
    ///
    /// The `Target` template parameter provided to the `is` and `as` member function templates may
    /// be either another specialization of `restricted_token<Mask>` or may be a `token_mask` value.
    ///
    /// For simplicitly, we define a number of commonly-used specializations of `restricted_token`.
    /// These are all defined in this header file.  Most of these use masks that are combinations
    /// that occur in metadata databases, but some of them are hybrid token combinations that only
    /// originate from other parts of this library (we define them all here anyway so they can be
    /// used uniformly).
    ///
    ///
    ///
    /// **What do blobs have to do with all this?**
    ///
    /// Why, I'm glad you asked that!  Tokens and blobs are entirely unrelated:  they are separate
    /// and distinct things.  However, there are a few select places where we need to represent
    /// "a token or a blob."  Notably, there are places in the library where we may have either a
    /// TypeDef token _or_ a blob containing the signature of a type.
    ///
    /// To represent this, we define a `restricted_token_or_blob<Mask>` that can represent either
    /// a `restricted_token<Mask>` or a `blob`.  This class template allows conversion similar to
    /// those allowed by `restricted_token<Mask>` for different `Mask` values.  It also allows
    /// conversions from `blob` itself or from `restricted_token<Mask>`, for valid `Mask` values.
    ///
    /// A `restricted_token_or_blob<Mask>` allows direct access to its scope, since both blobs and
    /// tokens have a scope.  For access to any other data, it must be converted to a token or a
    /// blob type, via its `as_token()` and `as_blob()` member functions.
    ///
    ///
    ///
    /// **How can I get to a row from a token?**
    ///
    /// There are row types defined in the `rows.hpp` header file.  There is one row type per table.
    /// each row type provides access to all of the columns for the table whose rows it represents.
    ///
    /// If you have a "unique token" (a token that can refer to a row in exactly one table), you can
    /// get its row object via the following expression:
    ///
    ///     t.scope()[t]
    ///
    /// Because this is a common operation and because we'd like to avoid having to name `t` twice,
    /// there is a nonmember utility function that can be used for this (most of the time, we'll use
    /// this and make the call unqualified, letting it be found via ADL):
    ///
    ///     row_from(t)
    ///
    /// Because each table has its own row type, this only works for unique tokens.  If you try to
    /// get a row from a nonunique token, you may get an ugly compilation error.
    ///
    ///
    ///
    /// **What about arithmetic on tokens?**
    ///
    /// We actually lied above:  the `restricted_token` template has two template parameters:  the
    /// second is a `bool` nontype template parameter that specifies whether the token allows
    /// pointer-like arithmetic operations.  This allows a unique token to be used like an iterator
    /// into a database table.  Because we rarely use arithmetic with tokens, we only use token
    /// types with arithmetic operations occasionally.  When we use them, they are injected via a
    /// CRTP base class, `base_token_with_arithmetic`.
    ///
    ///
    ///
    /// **A reminder about error checking**
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

            return std::tie(lhs._scope, lhs._value) == std::tie(rhs._scope, rhs._value);
        }

        friend auto operator<(base_token const& lhs, base_token const& rhs) -> bool
        {
            core::assert_true([&]{ return lhs.is_initialized() == rhs.is_initialized(); });

            return std::tie(lhs._scope, lhs._value) < std::tie(rhs._scope, rhs._value);
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(base_token)

    protected:

        base_token()
        {
        }

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

        ~base_token()
        {
        }

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

        core::checked_pointer<database const> _scope;
        core::value_initialized<value_type>   _value;
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

            return static_cast<core::difference_type>(lhs.index()) - static_cast<core::difference_type>(rhs.index());
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

        base_token_with_arithmetic()
        {
        }

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

        ~base_token_with_arithmetic()
        {
        }

    private:

        /// Tests whether `x {+,-} n` can be evaluated without overflow
        ///
        /// If `is_subtraction` is `true`, `x - n` is tested; otherwise `x + n` is tested.
        static auto is_in_range(DerivedType           const& x,
                                bool                  const  is_subtraction,
                                core::difference_type const  n) -> bool
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

        // Note:  These are not defined; they are for use in constant expressions only
        static mask_type         const mask           = Mask;
        static integer_mask_type const integer_mask   = static_cast<integer_mask_type>(mask);
        static bool              const has_arithmetic = WithArithmetic;

        restricted_token()
        {
        }

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
        ///
        /// Note that we have intentionally chosen to use SFINAE instead of static_cast, even though
        /// no other overload will ever match.  We do this so that the `std::is_convertible` type
        /// trait can be used to determine whether conversion will succeed at compile-time.  This
        /// allows us to write unit tests that verify nonconvertibility of incompatible token types.
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
    //
    // This is one of the few places in the library we use C-style casts.  The casts here are safe
    // because they are only evaluated at compile-time to compute the value of the integral constant
    // expression.
    
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
        (integer_table_mask)table_mask::field      |
        (integer_table_mask)table_mask::method_def
    )> field_or_method_def_token;

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
        (integer_table_mask)table_mask::generic_param_constraint |
        (integer_table_mask)table_mask::interface_impl
    )> interface_impl_or_constraint_token;

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
        (integer_table_mask)table_mask::type_def |
        (integer_table_mask)table_mask::type_ref
    )> type_def_ref_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_def  |
        (integer_table_mask)table_mask::type_ref  |
        (integer_table_mask)table_mask::type_spec
    )> type_def_ref_spec_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_def  |
        (integer_table_mask)table_mask::type_spec
    )> type_def_spec_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_def   |
        (integer_table_mask)table_mask::method_def
    )> type_or_method_def_token;

    typedef restricted_token<(table_mask)(
        (integer_table_mask)table_mask::type_ref  |
        (integer_table_mask)table_mask::type_spec
    )> type_ref_spec_token;





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

        blob()
        {
        }

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
        blob(Signature const& signature, typename std::enable_if<std::is_base_of<class base_signature, Signature>::value>::type* = nullptr)
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

        core::checked_pointer<database const>              _scope;
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

            return *_scope;
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

        core::checked_pointer<database const> _scope;

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
        (integer_table_mask)table_mask::type_ref
    )> type_def_ref_or_signature;

    typedef restricted_token_or_blob<(table_mask)(
        (integer_table_mask)table_mask::type_def  |
        (integer_table_mask)table_mask::type_ref  |
        (integer_table_mask)table_mask::type_spec
    )> type_def_ref_spec_or_signature;





    /// @}

} }

#endif

