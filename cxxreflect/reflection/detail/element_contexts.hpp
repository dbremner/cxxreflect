#ifndef CXXREFLECT_REFLECTION_DETAIL_ELEMENT_CONTEXTS_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_ELEMENT_CONTEXTS_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/metadata.hpp"

namespace cxxreflect { namespace reflection {

    class event;
    class field;
    class loader;
    class method;
    class property;
    class type;

} }

namespace cxxreflect { namespace reflection { namespace detail {

    /// \defgroup cxxreflect_detail_element_contexts Implementation Details :: Element Contexts
    ///
    /// These element contexts represent elements that are owned by types:  events, fields, methods,
    /// properties, and interfaces.
    ///
    /// \todo
    ///
    /// @{

    /// Tag for specializing the context types for events
    struct event_context_tag     { };

    /// Tag for specializing the context types for fields
    struct field_context_tag     { };

    /// Tag for specializing the context types for interfaces
    struct interface_context_tag { };

    /// Tag for specializing the context types for methods
    struct method_context_tag    { };

    /// Tag for specializing the context types for properties
    struct property_context_tag  { };

    template <typename ContextTag>
    class element_context;

    // The element_contextTraits defines the types and operations for each instantiation of the
    // element_context class.  These provide all of the specific implementation details that enable
    // use of the extremely general element_context class.
    template <typename ContetTag>
    class element_context_traits
    {
    };

    template <>
    class element_context_traits<event_context_tag>
    {
    public:

        static const metadata::table_id row_table_id = metadata::table_id::event;

        typedef event_context_tag                     tag_type;
        typedef event                                 resolved_type;
        typedef metadata::event_token                 token_type;
        typedef metadata::event_row                   row_type;
        typedef metadata::row_iterator<row_table_id>  row_iterator_type;
        typedef metadata::type_signature              signature_type;

        typedef element_context<tag_type>             context_type;
        typedef std::vector<context_type>             context_sequence_type;

        static auto begin_elements(metadata::type_def_token const& type) -> row_iterator_type;
        static auto end_elements  (metadata::type_def_token const& type) -> row_iterator_type;

        static auto get_signature (metadata::type_resolver const& resolver,
                                   token_type              const& element) -> metadata::blob;

        static auto insert_element(metadata::type_resolver const& resolver,
                                   context_sequence_type        & element_table,
                                   context_type            const& new_element,
                                   core::size_type         const  inherited_element_count) -> void;

        static auto should_recurse(metadata::type_def_token const& type) -> bool;
    };

    template <>
    class element_context_traits<field_context_tag>
    {
    public:

        static const metadata::table_id row_table_id = metadata::table_id::field;

        typedef field_context_tag                    tag_type;
        typedef field                                resolved_type;
        typedef metadata::field_token                token_type;
        typedef metadata::field_row                  row_type;
        typedef metadata::row_iterator<row_table_id> row_iterator_type;
        typedef metadata::field_signature            signature_type;

        typedef element_context<tag_type>            context_type;
        typedef std::vector<context_type>            context_sequence_type;

        static auto begin_elements(metadata::type_def_token const& type) -> row_iterator_type;
        static auto end_elements  (metadata::type_def_token const& type) -> row_iterator_type;

        static auto get_signature (metadata::type_resolver const& resolver,
                                   token_type              const& element) -> metadata::blob;

        static auto insert_element(metadata::type_resolver const& resolver,
                                   context_sequence_type        & element_table,
                                   context_type            const& new_element,
                                   core::size_type         const  inherited_element_count) -> void;

        static auto should_recurse(metadata::type_def_token const& type) -> bool;
    };

    template <>
    class element_context_traits<interface_context_tag>
    {
    public:

        static const metadata::table_id row_table_id = metadata::table_id::interface_impl;

        typedef interface_context_tag                tag_type;
        typedef type                                 resolved_type;
        typedef metadata::interface_impl_token       token_type;
        typedef metadata::interface_impl_row         row_type;
        typedef metadata::row_iterator<row_table_id> row_iterator_type;
        typedef metadata::type_signature             signature_type;

        typedef element_context<tag_type>            context_type;
        typedef std::vector<context_type>            context_sequence_type;

        static auto begin_elements(metadata::type_def_token const& type) -> row_iterator_type;
        static auto end_elements  (metadata::type_def_token const& type) -> row_iterator_type;

        static auto get_signature (metadata::type_resolver const& resolver,
                                   token_type                  const& element) -> metadata::blob;

        static auto insert_element(metadata::type_resolver const& resolver,
                                   context_sequence_type        & element_table,
                                   context_type            const& new_element,
                                   core::size_type         const  inherited_element_count) -> void;

        static auto should_recurse(metadata::type_def_token const& type) -> bool;
    };

    template <>
    class element_context_traits<method_context_tag>
    {
    public:

        static const metadata::table_id row_table_id = metadata::table_id::method_def;

        typedef method_context_tag                   tag_type;
        typedef method                               resolved_type;
        typedef metadata::method_def_token           token_type;
        typedef metadata::method_def_row             row_type;
        typedef metadata::row_iterator<row_table_id> row_iterator_type;
        typedef metadata::method_signature           signature_type;

        typedef element_context<tag_type>            context_type;
        typedef std::vector<context_type>            context_sequence_type;

        static auto begin_elements(metadata::type_def_token const& type) -> row_iterator_type;
        static auto end_elements  (metadata::type_def_token const& type) -> row_iterator_type;

        static auto get_signature (metadata::type_resolver const& resolver,
                                   token_type              const& element) -> metadata::blob;

        static auto insert_element(metadata::type_resolver const& resolver,
                                   context_sequence_type        & element_table,
                                   context_type            const& new_element,
                                   core::size_type         const  inherited_element_count) -> void;

        static auto should_recurse(metadata::type_def_token const& type) -> bool;
    };

    template <>
    class element_context_traits<property_context_tag>
    {
    public:

        static const metadata::table_id row_table_id = metadata::table_id::property;

        typedef property_context_tag                 tag_type;
        typedef property                             resolved_type;
        typedef metadata::property_token             token_type;
        typedef metadata::property_row               row_type;
        typedef metadata::row_iterator<row_table_id> row_iterator_type;
        typedef metadata::property_signature         signature_type;

        typedef element_context<tag_type>            context_type;
        typedef std::vector<context_type>            context_sequence_type;

        static auto begin_elements(metadata::type_def_token const& type) -> row_iterator_type;
        static auto end_elements  (metadata::type_def_token const& type) -> row_iterator_type;

        static auto get_signature (metadata::type_resolver const& resolver,
                                   token_type              const& element) -> metadata::blob;

        static auto insert_element(metadata::type_resolver const& resolver,
                                   context_sequence_type        & element_table,
                                   context_type            const& new_element,
                                   core::size_type         const  inherited_element_count) -> void;

        static auto should_recurse(metadata::type_def_token const& type) -> bool;
    };





    typedef element_context_traits<event_context_tag    > event_context_traits;
    typedef element_context_traits<field_context_tag    > field_context_traits;
    typedef element_context_traits<interface_context_tag> interface_context_traits;
    typedef element_context_traits<method_context_tag   > method_context_traits;
    typedef element_context_traits<property_context_tag > property_context_traits;





    // The ElementContext represents an "owned" element.  That is, something that is owned by a type
    // in the metadata. For example, one instantiation of ElementContext represents a Method because
    // a type owns a collection of methods (the "collection" part is represented by the next classes
    // ElementContextTable).
    template <typename ContextTag>
    class element_context
    {
    public:

        typedef ContextTag                             tag_type;
        typedef element_context_traits<ContextTag>     traits_type;
        typedef typename traits_type::resolved_type    resolved_type;
        typedef typename traits_type::token_type       token_type;
        typedef typename traits_type::row_type         row_type;
        typedef typename traits_type::signature_type   signature_type;

        element_context();

        element_context(token_type const& element_token);

        element_context(token_type                      const& element_token,
                        metadata::type_def_or_signature const& instantiating_type_token,
                        core::const_byte_range          const& instantiated_signature_range);

        // TODO auto resolve(type const& reflected_type) const -> resolved_type;
        
        // These return the full reference to the owned element's declaration, the row in which the
        // element is declared, and the signature of the element, if it has one.
        auto element()     const -> token_type;
        auto element_row() const -> row_type;
        auto element_signature(metadata::type_resolver const& resolver) const -> signature_type;

        // If the element is declared as generic but has been instantiated in the reflected type or
        // one of its base classes, these will provide the type that instantiated the element and
        // the instantiated signature.
        auto has_instantiating_type() const -> bool;
        auto instantiating_type()     const -> metadata::type_def_or_signature;

        auto has_instantiated_signature() const -> bool;
        auto instantiated_signature()     const -> core::const_byte_range;

        auto is_initialized() const -> bool;

    private:

        metadata::unrestricted_token    _element;

        metadata::type_def_or_signature _instantiating_type;
        core::const_byte_range          _instantiated_signature;
    };





    // The instantiations for which element_context is explicitly instantiated:
    typedef element_context<event_context_tag    > event_context;
    typedef element_context<field_context_tag    > field_context;
    typedef element_context<interface_context_tag> interface_context;
    typedef element_context<method_context_tag   > method_context;
    typedef element_context<property_context_tag > property_context;

    // Ensure that the element context types are standard layout.  Since they all have the same
    // representation, we will store all kinds of contexts in the same array in memory, and will
    // reinterpret them to the right kind of element when requested.
    static_assert(std::is_standard_layout<event_context>::value,"Element context types must be standard layout");





    // An [element]_context_table represents the sequence of [element]s that are owned by a type.  For
    // example, a MethodContextTable contains all of the methods owned by the type.
    typedef core::array_range<event_context     const> event_context_table;
    typedef core::array_range<field_context     const> field_context_table;
    typedef core::array_range<interface_context const> interface_context_table;
    typedef core::array_range<method_context    const> method_context_table;
    typedef core::array_range<property_context  const> property_context_table;





    // We allocate all of the element contexts in an instance of this class
    class element_context_table_storage
    {
    public:

        typedef core::linear_array_allocator<core::byte,    (1 << 16)> signature_storage;
        typedef core::linear_array_allocator<event_context, (1 << 12)> context_storage;

        struct table_index_value
        {
            event_context_table     _events;
            field_context_table     _fields;
            interface_context_table _interfaces;
            method_context_table    _methods;
            property_context_table  _properties;
        };

        class storage_lock
        {
        public:
            
            storage_lock(storage_lock&& other);
            auto operator=(storage_lock&& other) -> storage_lock&;

            auto allocate_signature(core::const_byte_iterator first,
                                    core::const_byte_iterator last) const -> core::const_byte_range;

            template <typename ContextTag>
            auto allocate_table(metadata::type_def_or_signature const& type,
                                element_context<ContextTag>     const* first,
                                element_context<ContextTag>     const* last) const
                -> core::array_range<element_context<ContextTag> const>
            {
                // The context types are all standard layout and are all layout compatible.  We
                // arbitrarily pick event_context as the common type that we will store.
                event_context const* const r_first(reinterpret_cast<event_context const*>(first));
                event_context const* const r_last (reinterpret_cast<event_context const*>(last ));

                core::size_type const size(core::distance(r_first, r_last));
                if (size == 0)
                {
                    get_table(_storage.get()->_index[type], ContextTag()) = core::default_value();
                    return core::default_value();
                }

                auto const range(_storage.get()->_context_storage.allocate(size));
                core::range_checked_copy(r_first, r_last, begin(range), end(range));

                core::array_range<element_context<ContextTag> const> o_range(
                    reinterpret_cast<element_context<ContextTag> const*>(begin(range)),
                    reinterpret_cast<element_context<ContextTag> const*>(end(range)));

                get_table(_storage.get()->_index[type], ContextTag()) = o_range;
                return o_range;
            }

            template <typename ContextTag>
            auto find_table(metadata::type_def_or_signature const& type) const
                -> std::pair<bool, core::array_range<element_context<ContextTag> const>>
            {
                core::assert_initialized(*this);

                auto const it(_storage.get()->_index.find(type));
                if (it == end(_storage.get()->_index))
                    return std::make_pair(false, core::default_value());

                auto const table(get_table(it->second, ContextTag()));
                if (table.empty())
                    return std::make_pair(false, core::default_value());

                return std::make_pair(true, table);
            }

            auto is_initialized() const -> bool;

        private:

            static auto get_table(table_index_value& v, event_context_tag)     -> event_context_table&;
            static auto get_table(table_index_value& v, field_context_tag)     -> field_context_table&;
            static auto get_table(table_index_value& v, interface_context_tag) -> interface_context_table&;
            static auto get_table(table_index_value& v, method_context_tag)    -> method_context_table&;
            static auto get_table(table_index_value& v, property_context_tag)  -> property_context_table&;

            friend element_context_table_storage;

            storage_lock(element_context_table_storage const* storage);

            core::value_initialized<element_context_table_storage const*> _storage;
            core::recursive_mutex_lock                                    _lock;
        };

        element_context_table_storage();

        auto lock() const -> storage_lock;

    private:

        typedef metadata::type_def_or_signature              table_index_key;
        typedef std::map<table_index_key, table_index_value> table_index_type;

        element_context_table_storage(element_context_table_storage const&);
        auto operator=(element_context_table_storage const&) -> element_context_table_storage&;
        
        signature_storage     mutable _signature_storage;
        context_storage       mutable _context_storage;
        table_index_type      mutable _index;
        core::recursive_mutex mutable _sync;
    };

    typedef std::unique_ptr<element_context_table_storage> unique_element_context_table_storage;





    // An [element]context_table_collection is simply a collection of element_context_tables.  It
    // owns the tables (and thus their lifetime is bound to the lifetime of the collection) and
    // caches results of generating the tables for faster lookup.
    template <typename ContextTag>
    class element_context_table_collection
    {
    public:
        
        typedef ContextTag                                      tag_type;
        typedef element_context_traits<ContextTag>              traits_type;
        typedef element_context<ContextTag>                     context_type;
        typedef core::array_range<context_type const>           context_table_type;

        typedef typename traits_type::row_type                  row_type;
        typedef typename traits_type::row_iterator_type         row_iterator_type;
        typedef typename traits_type::signature_type            signature_type;
        typedef typename traits_type::token_type                token_type;

        typedef metadata::class_variable_signature_instantiator instantiator;

        element_context_table_collection(metadata::type_resolver       const* resolver,
                                         element_context_table_storage const* storage);

        element_context_table_collection(element_context_table_collection&& other);
        element_context_table_collection& operator=(element_context_table_collection&& other);

        auto get_or_create_table(metadata::type_def_or_signature const& type) const -> context_table_type;

        auto is_initialized() const -> bool;

    private:

        core::value_initialized<metadata::type_resolver       const*> _resolver;
        core::value_initialized<element_context_table_storage const*> _storage;
    };





    typedef element_context_table_collection<event_context_tag    > event_context_table_collection;
    typedef element_context_table_collection<field_context_tag    > field_context_table_collection;
    typedef element_context_table_collection<interface_context_tag> interface_context_table_collection;
    typedef element_context_table_collection<method_context_tag   > method_context_table_collection;
    typedef element_context_table_collection<property_context_tag > property_context_table_collection;

    /// @}

} } }

#endif

// AMDG //
