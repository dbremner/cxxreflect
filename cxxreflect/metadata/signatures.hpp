
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_SIGNATURES_HPP_
#define CXXREFLECT_METADATA_SIGNATURES_HPP_

#include "cxxreflect/metadata/type_resolver.hpp"

namespace cxxreflect { namespace metadata { namespace detail {

    // A generic iterator that reads elements from a sequence via FMaterialize until FSentinelCheck
    // returns false.  This is used for sequences of elements where the sequence is terminated by
    // failing to read another element (e.g. CustomMod sequences).
    template <typename Value,
              Value(*Realize)(database const&, core::const_byte_iterator&, core::const_byte_iterator),
              bool(*Sentinel)(database const&, core::const_byte_iterator,  core::const_byte_iterator)>
    class sentinel_iterator
    {
    public:

        typedef Value                     value_type;
        typedef Value const&              reference;
        typedef Value const*              pointer;
        typedef std::ptrdiff_t            difference_type;
        typedef std::forward_iterator_tag iterator_category;

        sentinel_iterator()
        {
        }

        sentinel_iterator(database const*           const scope,
                          core::const_byte_iterator const current,
                          core::const_byte_iterator const last)
            : _scope(scope), _current(current), _last(last)
        {
            core::assert_not_null(scope);

            if (current != last)
                realize();
        }

        auto operator*()  const -> reference { return _value.get();  }
        auto operator->() const -> pointer   { return &_value.get(); }

        auto operator++() -> sentinel_iterator&
        {
            realize();
            return *this;
        }

        auto operator++(int) -> sentinel_iterator
        {
            sentinel_iterator const it(*this);
            ++*this;
            return it;
        }

        friend auto operator==(sentinel_iterator const& lhs, sentinel_iterator const& rhs) -> bool
        {
            return lhs._current.get() == rhs._current.get();
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(sentinel_iterator)

    private:

        auto realize() -> void
        {
            if (Sentinel(*_scope.get(), _current.get(), _last.get()))
            {
                _current.get() = nullptr;
                _last.get()    = nullptr;
            }
            else
            {
                _value.get() = Realize(*_scope.get(), _current.get(), _last.get());
            }
        }

        core::value_initialized<database const*>           _scope;
        core::value_initialized<core::const_byte_iterator> _current;
        core::value_initialized<core::const_byte_iterator> _last;
        core::value_initialized<value_type>                _value;
    };





    // A default sentinel check that always returns false.  This is useful for iteration over a
    // sequence where the count is known and exact (e.g., there is no sentinel for which to look.
    template <typename T>
    auto always_false_sentinel_check(database const&, T, T) -> bool
    {
        return false;
    }





    // An iterator that yields elements up to a certain number (the count) or until a sentinel is
    // read from the sequence (verified via the FSentinelCheck).
    template <typename Value,
              Value(*Realize)(database const&, core::const_byte_iterator&, core::const_byte_iterator),
              bool(*Sentinel)(database const&, core::const_byte_iterator,  core::const_byte_iterator)
                  = &always_false_sentinel_check<core::const_byte_iterator>
    >
    class counting_iterator
    {
    public:

        typedef Value                     value_type;
        typedef Value const&              reference;
        typedef Value const*              pointer;
        typedef std::ptrdiff_t            difference_type;
        typedef std::forward_iterator_tag iterator_category;

        counting_iterator() { }

        counting_iterator(database const*           const scope,
                          core::const_byte_iterator const current,
                          core::const_byte_iterator const last,
                          core::size_type           const index,
                          core::size_type           const count)
            : _scope(scope), _current(current), _last(last), _index(index), _count(count)
        {
            core::assert_not_null(scope);

            if (current != last && index != count)
                realize();
        }

        auto operator*()  const -> reference
        {
            return _value.get();
        }
        auto operator->() const -> pointer
        {
            return &_value.get();
        }

        auto operator++() -> counting_iterator&
        {
            ++_index.get();
            if (_index.get() != _count.get())
                realize();

            return *this;
        }

        auto operator++(int) -> counting_iterator
        {
            counting_iterator const it(*this);
            ++*this;
            return it;
        }

        friend auto operator==(counting_iterator const& lhs, counting_iterator const& rhs) -> bool
        {
            if (lhs._current.get() == rhs._current.get())
                return true;

            if (lhs.is_end() && rhs.is_end())
                return true;

            return false;
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(counting_iterator)

    private:

        auto is_end() const -> bool
        {
            if (_current.get() == nullptr)
                return true;

            // Note that we do not check whether _current == _last because _current always points
            // one past the current element (we materialize the current element on-the-fly).  Also,
            // the index check is sufficient to identify an end iterator.

            if (_index.get() == _count.get())
                return true;

            return false;
        }

        auto realize() -> void
        {
            if (Sentinel(*_scope.get(), _current.get(), _last.get()))
            {
                _current.get() = nullptr;
                _last.get()    = nullptr;
            }
            else
            {
                _value.get() = Realize(*_scope.get(), _current.get(), _last.get());
            }
        }

        core::value_initialized<database const*>           _scope;

        core::value_initialized<core::const_byte_iterator> _current;
        core::value_initialized<core::const_byte_iterator> _last;

        core::value_initialized<core::size_type>           _index;
        core::value_initialized<core::size_type>           _count;

        core::value_initialized<value_type>                _value;
    };

} } }

namespace cxxreflect { namespace metadata {

    class array_shape;
    class custom_modifier;
    class field_signature;
    class method_signature;
    class property_signature;
    class type_signature;

    /// \defgroup cxxreflect_metadata_signatures Metadata -> Signatures
    ///
    /// Signature types for parsing each kind of signature blob supported by the metadata library.
    /// Note that not all kinds are supported:  local variables, for example, remain unimplemented.
    ///
    /// TODO:  The signature parsers defined here have absurdly poor performance characteristics.
    /// Most common operations may require multiple scans of the signature data.  There are many
    /// possible improvements here, but for the time being, things work, which is good enough. :-)
    ///
    /// @{





    /// Base class that defines common functionality used by all signature types
    ///
    /// This class exists solely for code sharing among the signature types.  It is not polymorphic.
    /// Note that it does not define all common members, only common members that have common
    /// implementations as well (e.g., all signature types have `seek_to` and `compute_size` member
    /// functions, but these have different implementations for each derived signature type).
    class base_signature
    {
    public:

        auto scope()       const -> database const&;

        auto begin_bytes() const -> core::const_byte_iterator;
        auto end_bytes()   const -> core::const_byte_iterator;

        auto is_initialized() const -> bool;

    protected:

        base_signature();
        base_signature(database const* scope, core::const_byte_iterator first, core::const_byte_iterator last);
        
        base_signature(base_signature const& other);
        auto operator=(base_signature const& other) -> base_signature&;

        ~base_signature();

    private:

        core::value_initialized<database const*>           _scope;
        core::value_initialized<core::const_byte_iterator> _first;
        core::value_initialized<core::const_byte_iterator> _last;
    };





    /// Represents an **ArrayShape** signature item (ECMA 335-2010 II.23.2.13)
    class array_shape
        : public base_signature
    {
    private:

        static auto read_size     (database const& scope, core::const_byte_iterator& current, core::const_byte_iterator last) -> core::size_type;
        static auto read_low_bound(database const& scope, core::const_byte_iterator& current, core::const_byte_iterator last) -> core::size_type;

    public:

        enum class part
        {
            begin,
            rank,
            num_sizes,
            first_size,
            num_low_bounds,
            first_low_bound,
            end
        };

        typedef detail::counting_iterator<core::size_type, &array_shape::read_size>      size_iterator;
        typedef detail::counting_iterator<core::size_type, &array_shape::read_low_bound> low_bound_iterator;

        array_shape();
        array_shape(database const* scope, core::const_byte_iterator first, core::const_byte_iterator last);

        auto rank()             const -> core::size_type;

        auto size_count()       const -> core::size_type;
        auto begin_sizes()      const -> size_iterator;
        auto end_sizes()        const -> size_iterator;

        auto low_bound_count()  const -> core::size_type;
        auto begin_low_bounds() const -> low_bound_iterator;
        auto end_low_bounds()   const -> low_bound_iterator;

        auto compute_size()     const -> core::size_type;
        auto seek_to(part p)    const -> core::const_byte_iterator;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(array_shape::part)





    /// Represents a **CustomMod** signature item (ECMA 335-2010 II.23.2.7)
    class custom_modifier
        : public base_signature
    {
    public:

        enum class part
        {
            begin,
            req_opt_flag,
            type,
            end
        };

        custom_modifier();
        custom_modifier(database const* scope, core::const_byte_iterator first, core::const_byte_iterator last);

        auto is_optional()   const -> bool;
        auto is_required()   const -> bool;
        auto type()          const -> type_def_ref_spec_token;

        auto compute_size()  const -> core::size_type;
        auto seek_to(part p) const -> core::const_byte_iterator;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(custom_modifier::part)





    /// Represents a **FieldSig** signature item (ECMA 335-2010 II.23.2.4)
    ///
    /// Note that a FieldSig includes an optional CustomMod sequence.  This signature type does not
    /// include this sequence; rather, that sequence is included in the `type_signature` that is
    /// returned by the `type()` member function.
    class field_signature
        : public base_signature
    {
    public:

        enum class part
        {
            begin,
            field_tag,
            type,
            end
        };

        field_signature();
        field_signature(database const* scope, core::const_byte_iterator first, core::const_byte_iterator last);

        auto type()          const -> type_signature;

        auto compute_size()  const -> core::size_type;
        auto seek_to(part p) const -> core::const_byte_iterator;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(field_signature::part);





    /// Represents a **PropertySig** signature item (ECMA 335-2010 II.23.2.5)
    ///
    /// Note that a FieldSig includes an optional CustomMod sequence.  This signature type does not
    /// include this sequence; rather, that sequence is included in the `type_signature` that is
    /// returned by the `type()` member function.
    class property_signature
        : public base_signature
    {
    private:

        static auto read_parameter(database const& scope, core::const_byte_iterator& current, core::const_byte_iterator last) -> type_signature;

    public:

        typedef detail::counting_iterator<type_signature, &property_signature::read_parameter> parameter_iterator;

        enum class part
        {
            begin,
            property_tag,
            parameter_count,
            type,
            first_parameter,
            end
        };

        property_signature();
        property_signature(database const* scope, core::const_byte_iterator first, core::const_byte_iterator last);

        auto has_this()         const -> bool;
        auto parameter_count()  const -> core::size_type;
        auto begin_parameters() const -> parameter_iterator;
        auto end_parameters()   const -> parameter_iterator;
        auto type()             const -> type_signature;

        auto compute_size()     const -> core::size_type;
        auto seek_to(part p)    const -> core::const_byte_iterator;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(property_signature::part);





    /// Represents a method signature (there are several kinds of method signatures)
    ///
    /// The represented signature may be a **MethodDefSig** (ECMA 335-2010 II.23.2.1), a
    /// **MethodRefSig** (II.23.2.2), or a **StandAloneMethodSig** (II.23.2.3).
    class method_signature
        : public base_signature
    {
    private:

        static auto parameter_end_check(database const& scope, core::const_byte_iterator  current, core::const_byte_iterator last) -> bool;
        static auto read_parameter     (database const& scope, core::const_byte_iterator& current, core::const_byte_iterator last) -> type_signature;

    public:

        typedef detail::counting_iterator<
            type_signature,
            &method_signature::read_parameter,
            &method_signature::parameter_end_check
        > parameter_iterator;

        enum class part
        {
            begin,
            type_tag,
            gen_param_count,
            param_count,
            ret_type,
            first_param,
            sentinel,
            first_vararg_param,
            end
        };

        method_signature();
        method_signature(database const* scope, core::const_byte_iterator first, core::const_byte_iterator last);

        auto has_this()          const -> bool;
        auto has_explicit_this() const -> bool;

        // Calling conventions; exactly one of these will be true.
        auto calling_convention()      const -> signature_attribute;
        auto has_default_convention()  const -> bool;
        auto has_vararg_convention()   const -> bool;
        auto has_c_convention()        const -> bool;
        auto has_stdcall_convention()  const -> bool;
        auto has_thiscall_convention() const -> bool;
        auto has_fastcall_convention() const -> bool;

        auto is_generic()              const -> bool;
        auto generic_parameter_count() const -> core::size_type;

        auto return_type()             const -> type_signature;
        auto parameter_count()         const -> core::size_type;
        auto begin_parameters()        const -> parameter_iterator;
        auto end_parameters()          const -> parameter_iterator;
        auto begin_vararg_parameters() const -> parameter_iterator;
        auto end_vararg_parameters()   const -> parameter_iterator;

        auto compute_size()  const -> core::size_type;
        auto seek_to(part p) const -> core::const_byte_iterator;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(method_signature::part)





    /// Represents a type signature (there are several kinds of type signatures)
    ///
    /// The represented signature may be a **Param** (ECMA 335-2010 II.23.2.10), a **RetType**
    /// (II.23.2.11), a **Type** (II.23.2.12), a **TypeSpec** (II.23.2.14), or the core parts of a
    /// **FieldSig** (II.23.2.4) or **PropertySig** (II.23.2.5).
    class type_signature
        : public base_signature
    {
    private:

        static auto custom_modifier_end_check(database const& scope, core::const_byte_iterator  current, core::const_byte_iterator last) -> bool;
        static auto read_custom_modifier     (database const& scope, core::const_byte_iterator& current, core::const_byte_iterator last) -> custom_modifier;
        static auto read_type                (database const& scope, core::const_byte_iterator& current, core::const_byte_iterator last) -> type_signature;

    public:

        enum class kind
        {
            mask             = 0xff00,

            unknown          = 0x0000,

            primitive        = 0x0100, // BOOLEAN, CHAR, In, Un, Rn, OBJECT, STRING, VOID, TYPEDBYREF
            general_array    = 0x0200, // ARRAY
            simple_array     = 0x0300, // SZ_ARRAY
            class_type       = 0x0400, // CLASS, VALUETYPE
            function_pointer = 0x0500, // FNPTR
            generic_instance = 0x0600, // GENERICINST
            pointer          = 0x0700, // PTR
            variable         = 0x0800  // VAR, MVAR
        };

        enum class part
        {
            begin            = 0x00,
            first_custom_mod = 0x01,
            by_ref_tag       = 0x02,

            // The TypeCode marks the start of the actual 'Type' signature element
            cross_module_type_reference = 0x03,
            type_code                   = 0x04,
            
            general_array_type              = static_cast<core::size_type>(kind::general_array)    + 0x05,
            general_array_shape             = static_cast<core::size_type>(kind::general_array)    + 0x06,
            
            simple_array_type               = static_cast<core::size_type>(kind::simple_array)     + 0x05,

            class_type_type                 = static_cast<core::size_type>(kind::class_type)       + 0x05,
            class_type_scope                = static_cast<core::size_type>(kind::class_type)       + 0x06,

            function_pointer_type           = static_cast<core::size_type>(kind::function_pointer) + 0x05,

            generic_instance_type_code      = static_cast<core::size_type>(kind::generic_instance) + 0x05,
            generic_instance_type           = static_cast<core::size_type>(kind::generic_instance) + 0x06,
            generic_instance_argument_count = static_cast<core::size_type>(kind::generic_instance) + 0x07,
            first_generic_instance_argument = static_cast<core::size_type>(kind::generic_instance) + 0x08,

            pointer_type                    = static_cast<core::size_type>(kind::pointer)          + 0x05,

            variable_number                 = static_cast<core::size_type>(kind::variable)         + 0x05,
            variable_context                = static_cast<core::size_type>(kind::variable)         + 0x06,

            end              = 0x09
        };

        typedef detail::sentinel_iterator<
            custom_modifier,
            &type_signature::read_custom_modifier,
            &type_signature::custom_modifier_end_check
        > custom_modifier_iterator;

        typedef detail::counting_iterator<
            type_signature,
            &type_signature::read_type
        > generic_argument_iterator;

        type_signature();
        type_signature(database const* scope, core::const_byte_iterator first, core::const_byte_iterator last);

        auto compute_size()                   const -> core::size_type;
        auto seek_to(part p)                  const -> core::const_byte_iterator;
        auto get_kind()                       const -> kind;
        auto is_kind(kind k)                  const -> bool;
        auto get_element_type()               const -> element_type;
        auto is_cross_module_type_reference() const -> bool;

        // FieldSig, PropertySig, Param, RetType signatures, and PTR and SZARRAY Type signatures:
        auto begin_custom_modifiers() const -> custom_modifier_iterator;
        auto end_custom_modifiers()   const -> custom_modifier_iterator;

        // Param and RetType signatures:
        auto is_by_ref() const -> bool;

        // BOOLEAN, CHAR, I1, U1, I2, U2, I4, U4, I8, U8, R4, R8, I, U, OBJECT, and STRING (also,
        // VOID for RetType signatures and TYPEDBYREF for Param and RetType signatures).
        auto is_primitive()   const -> bool;
        auto primitive_type() const -> element_type;

        // ARRAY, SZARRAY:
        auto is_general_array() const -> bool;
        auto is_simple_array()  const -> bool;
        auto array_type()       const -> type_signature;
        auto array_shape()      const -> metadata::array_shape; // ARRAY only

        // CLASS and VALUETYPE:
        auto is_class_type() const -> bool;
        auto is_value_type() const -> bool;
        auto class_type()    const -> type_def_ref_spec_token;

        // FNPTR:
        auto is_function_pointer() const -> bool;
        auto function_type()       const -> method_signature;

        // GENERICINST:
        auto is_generic_instance()            const -> bool;
        auto is_generic_class_type_instance() const -> bool;
        auto is_generic_value_type_instance() const -> bool;
        auto generic_type()                   const -> type_def_ref_spec_token;
        auto generic_argument_count()         const -> core::size_type;
        auto begin_generic_arguments()        const -> generic_argument_iterator;
        auto end_generic_arguments()          const -> generic_argument_iterator;

        // PTR:
        auto is_pointer()             const -> bool;
        auto pointer_type() const -> type_signature;

        // MVAR and VAR:
        auto is_class_variable()  const -> bool;
        auto is_method_variable() const -> bool;
        auto variable_number()    const -> core::size_type;
        auto variable_context()   const -> type_or_method_def_token;

    private:

        auto assert_kind(kind k) const -> void;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(type_signature::kind)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(type_signature::part)





    /// An equality comparer for metadata signatures
    ///
    /// This function object type compares metadata signatures using the compatibility and
    /// equivalence rules as specified by ECMA 335-2010 section 8.6.1.6, "Signature Matching."
    class signature_comparer
    {
    public:

        signature_comparer(type_resolver const* resolver);

        auto operator()(array_shape        const& lhs, array_shape        const& rhs) const -> bool;
        auto operator()(custom_modifier    const& lhs, custom_modifier    const& rhs) const -> bool;
        auto operator()(field_signature    const& lhs, field_signature    const& rhs) const -> bool;
        auto operator()(method_signature   const& lhs, method_signature   const& rhs) const -> bool;
        auto operator()(property_signature const& lhs, property_signature const& rhs) const -> bool;
        auto operator()(type_signature     const& lhs, type_signature     const& rhs) const -> bool;

    private:

        auto operator()(type_def_ref_spec_token const& lhs, type_def_ref_spec_token const& rhs) const -> bool;

        core::value_initialized<type_resolver const*> _resolver;
    };





    class signature_instantiation_arguments
    {
    public:

        typedef std::vector<type_signature>     argument_sequence;
        typedef std::vector<core::byte>         argument_signature;
        typedef std::vector<argument_signature> argument_signature_sequence;

        signature_instantiation_arguments();

        explicit signature_instantiation_arguments(database const* scope);

        signature_instantiation_arguments(database const*               scope,
                                          argument_sequence&&           arguments,
                                          argument_signature_sequence&& signatures);

        signature_instantiation_arguments(signature_instantiation_arguments&& other);

        auto operator=(signature_instantiation_arguments&& other) -> signature_instantiation_arguments&;

        auto scope() const -> database const&;

        auto operator[](core::size_type n) const -> type_signature;

        auto size() const -> core::size_type;

        auto is_initialized() const -> bool;

    private:

        signature_instantiation_arguments(signature_instantiation_arguments const&);
        auto operator=(signature_instantiation_arguments const&) -> void;

        core::checked_pointer<database const> _scope;
        argument_sequence                     _arguments;
        argument_signature_sequence           _signatures;
    };





    class signature_instantiator
    {
    public:

        typedef signature_instantiation_arguments arguments_type;

        signature_instantiator(arguments_type const* arguments);
        signature_instantiator(arguments_type const* arguments, type_def_token);
        signature_instantiator(arguments_type const* arguments, method_def_token);
        signature_instantiator(arguments_type const* arguments, type_def_token, method_def_token);

        template <typename Signature>
        auto would_instantiate(Signature const& signature) const -> bool;

        // Instantiates 'signature' by replacing each generic class variables in it with the
        // corresponding generic argument provided in the constructor of this functor.  The returned
        // signature is a range in an internal buffer and the caller is respondible for copying the
        // returned signature into a more permanent buffer.
        template <typename Signature>
        auto instantiate(Signature const& signature) const -> Signature;

        auto is_initialized() const -> bool;

        static auto create_arguments(type_signature const& type) -> arguments_type;

        template <typename Signature>
        static auto requires_instantiation(Signature const& signature) -> bool;

    private:

        typedef core::checked_pointer<arguments_type const> arguments_pointer;
        typedef std::vector<core::byte>                     internal_buffer;

        class context
        {
        public:

            context();

            context(arguments_type const* arguments, type_def_token type_source, method_def_token method_source);

            auto arguments()     const -> arguments_type const&;
            auto type_source()   const -> type_def_token const&;
            auto method_source() const -> method_def_token const&;

            auto is_initialized() const -> bool;

        private:

            arguments_pointer _arguments;
            type_def_token    _type_source;
            method_def_token  _method_source;
        };

        signature_instantiator(signature_instantiator const&);
        auto operator=(signature_instantiator const&) -> void;

        static auto instantiate_into(internal_buffer& buffer, array_shape        const& s, context const& c) -> void;
        static auto instantiate_into(internal_buffer& buffer, field_signature    const& s, context const& c) -> void;
        static auto instantiate_into(internal_buffer& buffer, method_signature   const& s, context const& c) -> void;
        static auto instantiate_into(internal_buffer& buffer, property_signature const& s, context const& c) -> void;
        static auto instantiate_into(internal_buffer& buffer, type_signature     const& s, context const& c) -> void;

        template <typename Signature, typename Part>
        static auto copy_bytes_into(internal_buffer& buffer, Signature const& s, Part first, Part last) -> void;

        template <typename ForwardIterator>
        static auto instantiate_range_into(internal_buffer& buffer,
                                           ForwardIterator  first,
                                           ForwardIterator  last,
                                           context const&   arguments) -> void;

        static auto requires_instantiation_internal(array_shape        const& s) -> bool;
        static auto requires_instantiation_internal(field_signature    const& s) -> bool;
        static auto requires_instantiation_internal(method_signature   const& s) -> bool;
        static auto requires_instantiation_internal(property_signature const& s) -> bool;
        static auto requires_instantiation_internal(type_signature     const& s) -> bool;

        template <typename ForwardIterator>
        static auto any_requires_instantiation_internal(ForwardIterator first, ForwardIterator last) -> bool;

        context                 _context;
        internal_buffer mutable _buffer;
    };





    /// @}

} }

#endif

// AMDG //
