
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_MODULE_CONTEXT_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_MODULE_CONTEXT_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/module_locator.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    class module_type_def_index_iterator_constructor
    {
    public:

        auto operator()(metadata::database const* const scope, std::vector<core::size_type>::const_iterator const it) const
            -> metadata::type_def_token;
    };





    /// An index that provides for fast (log N) lookup of a type definition by qualified name
    ///
    /// We do a huge number of type lookups by name, but type definitions are unordered in the
    /// metadata database.  This index type sorts type definitions by name and allows O(lg N) lookup
    /// of types by name.  The index is built when the cache is constructed (typically when a module
    /// is first loaded).  It therefore adds to the time required to load the module, but the cost
    /// should be acceptable--certainly more acceptable than having to perform O(N) searches.
    class module_type_def_index
    {
    public:

        typedef std::vector<core::size_type>                              index_type;
        typedef std::pair<core::string_reference, core::string_reference> type_name_pair;

        typedef module_type_def_index_iterator                  type_def_iterator;
        typedef core::iterator_range<type_def_iterator>         type_def_iterator_range;
        typedef std::pair<type_def_iterator, type_def_iterator> type_def_iterator_pair;

        /// The `scope` must be non-null and must point to a valid, initialized `database`.  The
        /// caller is responsible for the lifetime of the scope.  This builds the index and has
        /// N log N average time complexity, where N is the number of type definitions in the
        /// database.
        module_type_def_index(metadata::database const* scope);

        /// Finds a type by name; returns the token identifying the type on success and a null token
        /// on failure.  The index is built during construction, so this uses a binary search and
        /// has log N time complexity, where N is the number of type definitions in the database.
        auto find(core::string_reference const& namespace_name,
                  core::string_reference const& name) const -> metadata::type_def_token;

        /// Finds the range of types defined in a given namespace.  The types are ordered by
        /// qualified name so all of the types will be together in the index.
        auto find(core::string_reference const& namespace_name) const -> type_def_iterator_pair;

        auto begin() const -> type_def_iterator;
        auto end()   const -> type_def_iterator;

    private:

        class comparer
        {
        public:

            comparer(metadata::database const* scope);

            auto operator()(type_name_pair  const& lhs, type_name_pair  const& rhs) const -> bool;
            auto operator()(type_name_pair  const& lhs, core::size_type        rhs) const -> bool;
            auto operator()(core::size_type        lhs, type_name_pair  const& rhs) const -> bool;
            auto operator()(core::size_type        lhs, core::size_type        rhs) const -> bool;

            auto operator()(core::string_reference const& lhs, core::string_reference const& rhs) const -> bool;
            auto operator()(core::string_reference const& lhs, core::size_type               rhs) const -> bool;
            auto operator()(core::size_type               lhs, core::string_reference const& rhs) const -> bool;

        private:

            auto get_name_of_type_def(core::size_type index) const -> type_name_pair;

            core::checked_pointer<metadata::database const> _scope;
        };

        module_type_def_index(module_type_def_index const&);
        auto operator=(module_type_def_index const&) -> module_type_def_index&;

        core::checked_pointer<metadata::database const> _scope;

        // Note that we only index a single database, so all tokens have the same database value.
        // For compactness, we store only the integer token value in the index.  When we need to
        // get a full token with scope, we compose the token value with `_scope`.
        index_type                                      _index;
    };

    template <typename T>
    class initializable_pointer
    {
    public:

        // TODO Determine correct use of acquire/release for less sequential consistency.

        typedef T value_type;

        initializable_pointer()
            : _value()
        {
        }

        auto is_initialized() const -> bool
        {
            return core::atomic_load(_value) != nullptr;
        }

        auto get() const -> value_type
        {
            return core::atomic_load(_value);
        }

        auto set(value_type const value) -> void
        {
            core::assert_true([&]{ return !is_initialized(); });

            core::atomic_store(_value, value);
        }

    private:

        value_type volatile mutable _value;
    };

    template <typename T>
    class initializable_token
    {
    public:

        // We can't atomically read and write 128 bits atomically, at least not using the available
        // x64 intrinsics in Visual C++.  We can, however, read and write 64 bits at a time.  Each
        // value only has one of two states:  not initialized and initialized.  The not initialized
        // state is easily recognizable:  we can simply read the scope pointer and see if it's null.
        //
        // We can enforce correct ordering of reads and writes by always writing the token first,
        // then writing the scope; and by always reading the scope first, then reading the token.
        // This way, the token is always initialized before the scope, so if we ever read a non-null
        // scope pointer, we know that the token is guaranteed to be initialized as well.
        //
        // TODO Determine correct use of acquire/release for less sequential consistency.

        typedef T value_type;

        initializable_token()
            : _scope(), _token()
        {
        }

        auto is_initialized() const-> bool
        {
            return core::atomic_load(_scope) != nullptr;
        }

        auto get() const -> value_type
        {
            metadata::database const* const scope(core::atomic_load(_scope));
            if (scope == nullptr)
                return value_type();

            core::size_type const token(core::atomic_load(_token));
            return value_type(scope, token);
        }

        auto set(value_type const& value) -> void
        {
            if (!value.is_initialized())
                return;

            core::atomic_store(_token, value.value());
            core::atomic_store(_scope, &value.scope());
        }

    private:

        typedef metadata::database const* volatile volatile_database_pointer;

        volatile_database_pointer          mutable _scope;
        core::size_type           volatile mutable _token;
    };

    template <typename Value>
    class module_resolution_cache_types;

    template <typename Value>
    class module_resolution_cache_types<Value*>
    {
    public:

        typedef initializable_pointer<Value*> stored_value_type;
    };

    template <metadata::table_mask Mask, bool WithArithmetic>
    class module_resolution_cache_types<metadata::restricted_token<Mask, WithArithmetic>>
    {
    public:

        typedef initializable_token<metadata::restricted_token<Mask, WithArithmetic>> stored_value_type;
    };

    template <typename Key, typename Value>
    class module_resolution_cache
    {
    public:

        static const metadata::table_id key_token_id = metadata::table_id_for_mask<Key::mask>::value;

        typedef Key   key_type;
        typedef Value value_type;

        typedef typename module_resolution_cache_types<Value>::stored_value_type stored_value_type;

        module_resolution_cache(metadata::database const* const scope)
            : _scope(scope)
        {
            core::assert_not_null(scope);

            _size.get() = scope->tables()[key_token_id].row_count();
            _cache = core::make_unique_array<stored_value_type>(_size.get());
        }

        auto get(key_type const& key) const -> value_type
        {
            core::assert_initialized(key);
            core::assert_true([&]{ return key.scope() == *_scope; });

            core::size_type const index(key.index());
            if (index > _size.get())
                throw core::logic_error(L"attempted to get value for out-of-range reference");

            return _cache[index].get();
        }

        auto set(key_type const& key, value_type const& value) -> void
        {
            core::assert_initialized(key);
            core::assert_true([&]{ return key.scope() == *_scope; });

            core::size_type const index(key.index());
            if (index > _size.get())
                throw core::logic_error(L"attempted to set value for out-of-range reference");

            _cache[index].set(value);
        }

    private:

        module_resolution_cache(module_resolution_cache const&);
        auto operator=(module_resolution_cache const&) -> void;

        typedef std::unique_ptr<stored_value_type[]> cache_type;

        core::checked_pointer<metadata::database const> _scope;
        core::value_initialized<core::size_type>        _size;
        cache_type                                      _cache;
    };

    typedef module_resolution_cache<metadata::assembly_ref_token, metadata::database const*          > module_assembly_ref_cache;
    typedef module_resolution_cache<metadata::module_ref_token,   metadata::database const*          > module_module_ref_cache;
    typedef module_resolution_cache<metadata::type_ref_token,     metadata::type_def_token           > module_type_ref_cache;
    typedef module_resolution_cache<metadata::member_ref_token,   metadata::field_or_method_def_token> module_member_ref_cache;





    /// Represents a module, a single metadata file
    ///
    /// A module consists of a single metadata file, along with related meta-information.  Each
    /// module is part of an assembly.  Each assembly has at least one module. This `module_context`
    /// is noncopyable and nonmovable, so pointers into it or into its database remain valid for the
    /// lifetime of the `module_context`.
    class module_context : public metadata::database_owner
    {
    public:

        module_context(assembly_context const* assembly, module_location const& location);

        auto assembly() const -> assembly_context   const&;
        auto location() const -> module_location    const&;
        auto database() const -> metadata::database const&;

        auto type_def_index()   const -> module_type_def_index const&;

        auto assembly_ref_cache() const -> module_assembly_ref_cache&;
        auto module_ref_cache()   const -> module_module_ref_cache  &;
        auto type_ref_cache()     const -> module_type_ref_cache    &;
        auto member_ref_cache()   const -> module_member_ref_cache  &;
        
        static auto from(metadata::database const& scope) -> module_context const&;

    private:

        module_context(module_context const&);
        auto operator=(module_context const&) -> module_context&;

        auto create_database(module_location const& location) -> metadata::database;

        // Note:  Data member declaration order is important for this type!  The _database must be
        // initialized before the _type_def_index and _type_ref_cache because both of those access
        // the _database to pre-build or initialize cache data.

        core::checked_pointer<assembly_context const> _assembly;
        module_location                               _location;
        metadata::database                            _database;

        module_type_def_index                         _type_def_index;
        module_assembly_ref_cache             mutable _assembly_ref_cache;
        module_module_ref_cache               mutable _module_ref_cache;
        module_type_ref_cache                 mutable _type_ref_cache;
        module_member_ref_cache               mutable _member_ref_cache;
    };

} } }

#endif
