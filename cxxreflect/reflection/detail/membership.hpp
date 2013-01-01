
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_MEMBERSHIP_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_MEMBERSHIP_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    /// Member traits are used to specialize the type building algorithm for each kind of member
    ///
    /// There are certain operations that are specific to each kind of member:  different calls are
    /// required to get the range of elements for each kind of member because the members are all
    /// stored in different metadata tables.  Similarly, signatures are stored differently for each
    /// kind of member.
    ///
    /// This also allows for better type checking in the type contexts:  we can specifically name
    /// the expected token and signature types to avoid having to use unrestricted_token all over
    /// the place.
    ///
    /// We specialize this traits type for each of the previously-defined tag types.  Each of the
    /// specializations defines the same set of members, and each of the members follows the same
    /// pattern in each specialization.
    ///
    /// One thing that warrants further comment:  the `interim_type` is the type used during table
    /// computation:  we compute the table using this type, then once the table is computed it is
    /// reduced either to `member_table_entry` or `member_table_entry_with_instantiation`, as
    /// approprite.  For most types, we simply use `member_table_entry_with_instantiation` for the
    /// interim type.  The exception is methods:  we need to store additional information to
    /// correctly compute overrides.  More information is available in the implementation of the
    /// method traits's `insert_member` static member function.
    template <member_kind MemberTag>
    class member_traits;

    template <>
    class member_traits<member_kind::event>
    {
    public:

        static const metadata::table_id member_table_id = metadata::table_id::event;
        static const member_kind        member_kind_tag = member_kind::event;

        typedef metadata::event_token                   token_type;
        typedef metadata::event_row                     row_type;
        typedef metadata::row_iterator<member_table_id> row_iterator_type;
        typedef core::iterator_range<row_iterator_type> row_range_type;
        typedef metadata::type_signature                signature_type;

        typedef member_table_entry_with_instantiation   interim_type;
        typedef std::vector<interim_type>               interim_sequence_type;

        static auto get_members  (metadata::type_def_token const& type) -> row_range_type;

        static auto get_signature(token_type               const& member) -> metadata::blob;

        static auto insert_member(interim_sequence_type         & member_table,
                                  interim_type             const& new_member,
                                  core::size_type          const  inherited_member_count) -> core::size_type;
    };

    template <>
    class member_traits<member_kind::field>
    {
    public:

        static const metadata::table_id member_table_id = metadata::table_id::field;
        static const member_kind        member_kind_tag = member_kind::field;

        typedef metadata::field_token                   token_type;
        typedef metadata::field_row                     row_type;
        typedef metadata::row_iterator<member_table_id> row_iterator_type;
        typedef core::iterator_range<row_iterator_type> row_range_type;
        typedef metadata::field_signature               signature_type;

        typedef member_table_entry_with_instantiation   interim_type;
        typedef std::vector<interim_type>               interim_sequence_type;

        static auto get_members  (metadata::type_def_token const& type) -> row_range_type;

        static auto get_signature(token_type               const& member) -> metadata::blob;

        static auto insert_member(interim_sequence_type         & member_table,
                                  interim_type             const& new_member,
                                  core::size_type          const  inherited_member_count) -> core::size_type;
    };

    template <>
    class member_traits<member_kind::interface_>
    {
    public:

        // An interface may be represented by one of two possible parents:  an interface_impl, which
        // specifies that a type definition implements an interface, or a generic_param_constraint,
        // which specifies that a generic parameter is constrained such that it must implement the
        // interface.
        //
        // For most operations here, we simply handle the interface_impl case:  it is more common
        // and we need special handling in the table builder logic to correctly handle the case of
        // the generic_param_constraint anyway.  Because the element may actually be represented by
        // either token, though, the token_type allows either of them.  Callers must disambiguate
        // between the two possible token types.  Usually this should be trivial because it is well-
        // known what kind of token is expected given the context in which it is used.

        static const metadata::table_id member_table_id = metadata::table_id::interface_impl;
        static const member_kind        member_kind_tag = member_kind::interface_;

        typedef metadata::interface_impl_or_constraint_token token_type;
        typedef metadata::interface_impl_row                 row_type;
        typedef metadata::row_iterator<member_table_id>      row_iterator_type;
        typedef core::iterator_range<row_iterator_type>      row_range_type;
        typedef metadata::type_signature                     signature_type;

        typedef member_table_entry_with_instantiation        interim_type;
        typedef std::vector<interim_type>                    interim_sequence_type;

        static auto get_members  (metadata::type_def_token const& type) -> row_range_type;

        static auto get_signature(token_type               const& member) -> metadata::blob;

        static auto insert_member(interim_sequence_type         & member_table,
                                  interim_type             const& new_member,
                                  core::size_type          const  inherited_member_count) -> core::size_type;

        /// Gets the type of the intefrace referred to by the given parent token
        ///
        /// For an `interface_impl`, this returns the interface.  For a `generic_param_constraint`,
        /// this returns the constraint.  In either case, it returns the type of the interface to
        /// which the parent refers.
        ///
        /// This function does not resolve the type; it returns the actual token stored in the
        /// metadata database; it is incumbent upon the caller to resolve the type if required.
        static auto get_interface_type(token_type const& parent)
            -> metadata::type_def_ref_spec_token;
    };

    template <>
    class member_traits<member_kind::method>
    {
    public:

        static const metadata::table_id member_table_id = metadata::table_id::method_def;
        static const member_kind        member_kind_tag = member_kind::method;

        typedef metadata::method_def_token              token_type;
        typedef metadata::method_def_row                row_type;
        typedef metadata::row_iterator<member_table_id> row_iterator_type;
        typedef core::iterator_range<row_iterator_type> row_range_type;
        typedef metadata::method_signature              signature_type;

        /// Represents a method slot for the purpose of override computation during table building
        class override_slot
        {
        public:

            override_slot();
            override_slot(metadata::type_def_or_signature const& type,
                          metadata::method_def_token      const& method);

            auto declaring_type()  const -> metadata::type_def_or_signature const&;
            auto declared_method() const -> metadata::method_def_token      const&;

            auto is_initialized() const -> bool;

            friend auto operator==(override_slot const&, override_slot const&) -> bool;

            CXXREFLECT_GENERATE_EQUALITY_OPERATORS(override_slot)

        private:

            metadata::type_def_or_signature _declaring_type;
            metadata::method_def_token      _declared_method;
        };

        typedef member_table_entry_with_override_slot interim_type;
        typedef std::vector<interim_type>             interim_sequence_type;

        static auto get_members  (metadata::type_def_token const& type) -> row_range_type;

        static auto get_signature(token_type               const& member) -> metadata::blob;

        static auto insert_member(interim_sequence_type         & member_table,
                                  interim_type             const& new_member,
                                  core::size_type          const  inherited_member_count) -> core::size_type;

        static auto compute_override_slot(token_type const& member) -> override_slot;
    };

    template <>
    class member_traits<member_kind::property>
    {
    public:

        static const metadata::table_id member_table_id = metadata::table_id::property;
        static const member_kind        member_kind_tag = member_kind::property;

        typedef metadata::property_token                token_type;
        typedef metadata::property_row                  row_type;
        typedef metadata::row_iterator<member_table_id> row_iterator_type;
        typedef core::iterator_range<row_iterator_type> row_range_type;
        typedef metadata::property_signature            signature_type;

        typedef member_table_entry_with_instantiation   interim_type;
        typedef std::vector<interim_type>               interim_sequence_type;

        static auto get_members  (metadata::type_def_token const& type) -> row_range_type;

        static auto get_signature(token_type               const& member) -> metadata::blob;

        static auto insert_member(interim_sequence_type         & member_table,
                                  interim_type             const& new_member,
                                  core::size_type          const  inherited_member_count) -> core::size_type;
    };





    /// An entry in a member table that represents a single member
    ///
    /// Essentially, a member is represented by a token referring into one of the five member tables
    /// in a metadata database.  The parent of a member is a type.  The parent is computable via the
    /// metadata database from the member token.
    ///
    /// This type represents a member in a member table.  It is member kind neutral, so it can
    /// represent any kind of member.  Each table consists of entries either of this type or of the
    /// `member_table_entry_with_instantiation_type`.  Each table must consist of one or the other;
    /// mixing and matching is not permitted (this ensures that each element in a particular table
    /// has the same size, for simpler iteration and offset computation).  We could use the
    /// `member_table_entry_with_instantiation_type` for every table, but it is a waste of space,
    /// and for this code, every byte counts.
    class member_table_entry
    {
    public:

        member_table_entry();
        member_table_entry(metadata::unrestricted_token const& member_token_);

        auto member_token() const -> metadata::unrestricted_token const&;

        auto is_initialized() const -> bool;

    private:

        metadata::unrestricted_token _member_token;
    };

    /// An entry in a member table that represents a single member with an instantiated signature
    ///
    /// This class is related to `member_table_entry`; it stores the additional instantiation data
    /// that members of a generic type instantiation may require (if a member makes use of any type
    /// variable in its signature, then it requires instantiation in a generic type instance).
    ///
    /// This type is implicitly convertible from `member_table_entry`, to allow for easier use.  To
    /// convert from this type back to a `member_table_entry`, simply construct the target object
    /// with the `member_token()` from this type.
    class member_table_entry_with_instantiation
    {
    public:

        member_table_entry_with_instantiation();
        member_table_entry_with_instantiation(member_table_entry const& context);
        member_table_entry_with_instantiation(metadata::unrestricted_token    const& member_token_,
                                              metadata::type_def_or_signature const& instantiating_type_,
                                              metadata::blob                  const& instantiated_signature_);

        auto member_token()           const -> metadata::unrestricted_token    const&;
        auto instantiating_type()     const -> metadata::type_def_or_signature const&;
        auto instantiated_signature() const -> metadata::blob                  const&;

        auto is_initialized() const -> bool;

    private:

        metadata::unrestricted_token    _member_token;

        metadata::type_def_or_signature _instantiating_type;
        metadata::blob                  _instantiated_signature;
    };

    /// Represents a `member_table_entry_with_instantiation`, plus an `override_slot`
    class member_table_entry_with_override_slot
    {
    public:

        typedef method_traits::override_slot override_slot;

        member_table_entry_with_override_slot();

        member_table_entry_with_override_slot(member_table_entry const& context,
                                              override_slot      const& slot = override_slot());

        member_table_entry_with_override_slot(metadata::unrestricted_token    const& member_token,
                                              metadata::type_def_or_signature const& instantiating_type,
                                              metadata::blob                  const& instantiated_signature,
                                              override_slot                   const& slot = override_slot());

        /// This type is implicitly convertible to `member_table_entry_with_instantiation`
        ///
        /// This allows plug-in compatibility with the rest of the table building logic.
        operator member_table_entry_with_instantiation const&() const;

        auto member_token()           const -> metadata::unrestricted_token    const&;
        auto instantiating_type()     const -> metadata::type_def_or_signature const&;
        auto instantiated_signature() const -> metadata::blob                  const&;
        auto slot()                   const -> override_slot                   const&;

        auto is_initialized() const -> bool;

    private:

        member_table_entry_with_instantiation _entry;
        override_slot                         _override_slot;
    };

    // The size of the two types cannot be the same because the size is used to determine the entry
    // type when we dereference an iterator into a member table.
    CXXREFLECT_STATIC_ASSERT(
        sizeof(member_table_entry) !=
        sizeof(member_table_entry_with_instantiation));

    // We use a common allocator for the two types, so they must have the same alignment.  As long
    // as they have the same alignment, we don't have to worry about alignment.
    CXXREFLECT_STATIC_ASSERT(
        std::alignment_of<member_table_entry>::value ==
        std::alignment_of<member_table_entry_with_instantiation>::value);





    /// The internal storage type of a member table
    ///
    /// Because a member table may have entries either of type `member_table_entry` or of type
    /// `member_table_entry_with_instantiation`, we cannot use a particular type for this table.
    /// Instead, we will simply use an array of bytes and keep track of the kind of element that
    /// is stored in the table (both types should have the same alignment requirement).
    ///
    /// When we iterate over the table, we'll use the `stride_iterator` with the correctly
    /// computed stride value.
    typedef core::array_range<core::byte const> member_table;





    /// A generic, type-checked member context type
    ///
    /// This type encapsulates a `member_table_entry` behind an interface that actually uses the
    /// types with which we want to work.  The `member_table_entry` type is intentionally general:
    /// it is designed to be usable for all member types.  When we use a particular member table,
    /// though, we want to use it naturally, as if it refers to the particular kind of member that
    /// the table contains.  This wrapper provides that interface.
    template <member_kind MemberTag>
    class member_table_entry_facade
    {
    public:

        static const member_kind member_kind_tag = MemberTag;

        typedef member_traits<member_kind_tag>       traits_type;
        typedef typename traits_type::token_type     token_type;
        typedef typename traits_type::row_type       row_type;
        typedef typename traits_type::signature_type signature_type;

        auto member_token()     const -> token_type;
        auto member_signature() const -> signature_type;

        auto has_instantiating_type() const -> bool;
        auto instantiating_type()     const -> metadata::type_def_or_signature;

        auto has_instantiated_signature() const -> bool;
        auto instantiated_signature()     const -> metadata::blob;

        auto is_initialized()  const -> bool;
        auto is_instantiated() const -> bool;

        auto realize() const -> member_table_entry_with_instantiation;

        static auto from(member_table_entry                    const*) -> member_table_entry_facade const*;
        static auto from(member_table_entry_with_instantiation const*) -> member_table_entry_facade const*;
        static auto from(member_table_entry_with_override_slot const*) -> member_table_entry_facade const*;

    private:

        member_table_entry_facade();
        member_table_entry_facade(member_table_entry_facade const&);
        auto operator=(member_table_entry_facade const&) -> void;
        ~member_table_entry_facade();

        auto entry()                    const -> member_table_entry const&;
        auto entry_with_instantiation() const -> member_table_entry_with_instantiation const&;
    };





    template <member_kind MemberTag>
    class member_table_iterator_constructor
    {
    public:

        auto operator()(member_kind const& kind, core::stride_iterator const& current) const ->
            member_table_entry_facade<MemberTag> const*;
    };





    /// Generator for member context iterator types
    ///
    /// Do not use this type directly--it is defined here only to support the typedefs that follow
    /// it.  It's used to shorten typedefs.
    template <member_kind MemberTag>
    class member_table_iterator_generator
    {
    public:

        typedef core::instantiating_iterator<
            core::stride_iterator,
            member_table_entry_facade<MemberTag> const*,
            member_kind,
            member_table_iterator_constructor<MemberTag>
        > type;

        typedef core::iterator_range<type> range_type;
    };





    class membership_context;

    template <member_kind MemberTag>
    class membership_context_base
    {
    private:

        friend membership_context;

        core::atomic<core::const_byte_iterator> _first;
        core::atomic<core::const_byte_iterator> _last;
    };

    class membership_context
        : private membership_context_base<member_kind::event>,
          private membership_context_base<member_kind::field>,
          private membership_context_base<member_kind::interface_>,
          private membership_context_base<member_kind::method>,
          private membership_context_base<member_kind::property>
    {
    public:

        enum class state : core::size_type
        {
            events     = (core::size_type)1 << (core::size_type)member_kind::event,
            fields     = (core::size_type)1 << (core::size_type)member_kind::field,
            interfaces = (core::size_type)1 << (core::size_type)member_kind::interface_,
            methods    = (core::size_type)1 << (core::size_type)member_kind::method,
            properties = (core::size_type)1 << (core::size_type)member_kind::property,

            events_are_instantiated     = (core::size_type)events     << (core::size_type)8,
            fields_are_instantiated     = (core::size_type)fields     << (core::size_type)8,
            interfaces_are_instantiated = (core::size_type)interfaces << (core::size_type)8,
            methods_are_instantiated    = (core::size_type)methods    << (core::size_type)8,
            properties_are_instantiated = (core::size_type)properties << (core::size_type)8
        };

        typedef core::flags<state> state_flags;

        membership_context();

        auto get_state() const -> state_flags;

        template <member_kind MemberTag>
        auto get_table() const
            -> core::optional<typename member_table_iterator_generator<MemberTag>::range_type>;

        template <member_kind MemberTag>
        auto get_range() const
            -> core::optional<core::const_byte_range>;

        template <member_kind MemberTag>
        auto set_table(core::const_byte_range table_range, bool is_instantiated)
            -> typename member_table_iterator_generator<MemberTag>::range_type;

        static auto primary_state_flag_for     (member_kind kind) -> state;
        static auto instantiated_state_flag_for(member_kind kind) -> state;

    private:

        core::atomic<state> _state;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(membership_context::state)





    class membership_handle
    {
    public:

        membership_handle();
        membership_handle(membership_storage* storage, membership_context* context, core::internal_key);

        auto get_events()     const -> event_table_range;
        auto get_fields()     const -> field_table_range;
        auto get_interfaces() const -> interface_table_range;
        auto get_methods()    const -> method_table_range;
        auto get_properties() const -> property_table_range;

        auto context(core::internal_key) const -> membership_context&;

        auto is_initialized() const -> bool;

    private:

        template <member_kind MemberTag>
        auto get_table() const -> typename member_table_iterator_generator<MemberTag>::range_type;

        core::checked_pointer<membership_storage> mutable _storage;
        core::checked_pointer<membership_context> mutable _context;
    };





    class membership_storage
    {
    public:

        typedef core::linear_array_allocator<core::byte, (1 << 16)> allocator_type;
        typedef metadata::type_def_or_signature                     key_type;
        typedef std::map<key_type, membership_context>              index_type;

        membership_storage();

        auto get_membership(key_type const& key) -> membership_handle;

        auto allocate_signature(core::const_byte_range transient_range, core::internal_key) -> core::const_byte_range;
        auto allocate_table    (core::const_byte_range transient_range, core::internal_key) -> core::const_byte_range;

        template <member_kind MemberTag>
        auto create_table(membership_context& context, core::internal_key) -> void;

    private:

        membership_storage(membership_storage const&);
        auto operator=(membership_storage const&) -> membership_storage&;

        static auto allocate_range(allocator_type& allocator, core::const_byte_range transient_range) -> core::const_byte_range;

        static auto key_from_context(membership_context& key) -> key_type const&;

        core::recursive_mutex _sync;
        index_type            _index;

        // Note:  We use two allocators to ensure that table allocations are correctly aligned.
        allocator_type        _signature_allocator;
        allocator_type        _table_allocator;
    };

} } }

#endif
