
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_FORWARD_DECLARATIONS_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_FORWARD_DECLARATIONS_HPP_

#include "cxxreflect/metadata/metadata.hpp"

namespace cxxreflect { namespace reflection {

    class assembly;
    class assembly_name;
    class constant;
    class custom_attribute;
    class custom_modifier_iterator;
    class event;
    class field;
    class file;
    class guid;
    class loader;
    class loader_configuration;
    class method;
    class module;
    class module_location;
    class module_locator;
    class parameter;
    class property;
    class type;
    class unresolved_type;

    typedef core::iterator_range<custom_modifier_iterator> custom_modifier_range;

    enum class type_layout
    {
        unknown,
        auto_layout,
        explicit_layout,
        sequential_layout
    };

    enum class type_string_format
    {
        unknown,
        ansi_string_format,
        auto_string_format,
        unicode_string_format
    };

    enum class type_visibility
    {
        unknown,
        not_public,
        public_,
        nested_public,
        nested_private,
        nested_family,
        nested_assembly,
        nested_family_and_assembly,
        nested_family_or_assembly
    };

    using std::begin;
    using std::end;

} }

namespace cxxreflect { namespace reflection { namespace detail {

    class assembly_context;
    class array_type_policy;
    class by_ref_type_policy;
    class definition_type_policy;
    class generic_instantiation_type_policy;
    class generic_variable_type_policy;
    class loader_context;
    class member_table_entry;
    class member_table_entry_with_instantiation;
    class member_table_entry_with_override_slot;
    class membership_context;
    class membership_handle;
    class membership_storage;
    class module_context;
    class module_type_def_index;
    class module_type_def_index_iterator_constructor;
    class module_type_iterator_constructor;
    class parameter_data;
    class pointer_type_policy;
    class specialization_type_policy;
    class reference_type_policy;
    class type_policy;

    typedef core::instantiating_iterator
    <
        metadata::token_with_arithmetic<metadata::custom_attribute_token>::type,
        custom_attribute,
        std::nullptr_t,
        core::internal_constructor_forwarder<custom_attribute>
    > custom_attribute_iterator;

    typedef core::iterator_range<custom_attribute_iterator> custom_attribute_range;

    typedef core::instantiating_iterator
    <
        metadata::type_signature::generic_argument_iterator,
        unresolved_type,
        std::nullptr_t,
        core::internal_constructor_forwarder<unresolved_type>,
        core::identity_transformer,
        std::forward_iterator_tag
    > generic_argument_iterator;

    typedef core::iterator_range<generic_argument_iterator> generic_argument_range;

    template
    <
        typename Type,
        typename Member,
        typename InnerIterator,
        bool (*Filter)(metadata::binding_flags, Type const&, typename InnerIterator::value_type const&)
    >
    class member_iterator;

    typedef core::instantiating_iterator<
        std::vector<core::size_type>::const_iterator,
        metadata::type_def_token,
        metadata::database const*,
        module_type_def_index_iterator_constructor
    > module_type_def_index_iterator;

    typedef core::instantiating_iterator<
        module_type_def_index_iterator,
        type,
        std::nullptr_t,
        module_type_iterator_constructor
    > module_type_iterator;

    typedef core::iterator_range<module_type_def_index_iterator> module_type_def_index_iterator_range;

    CXXREFLECT_DECLARE_INCOMPLETE_DELETE(unique_assembly_context_delete, assembly_context);
    CXXREFLECT_DECLARE_INCOMPLETE_DELETE(unique_loader_context_delete,   loader_context  );
    CXXREFLECT_DECLARE_INCOMPLETE_DELETE(unique_module_context_delete,   module_context  );

    typedef std::unique_ptr<assembly_context, unique_assembly_context_delete> unique_assembly_context;
    typedef std::unique_ptr<loader_context,   unique_loader_context_delete  > unique_loader_context;
    typedef std::unique_ptr<module_context,   unique_module_context_delete  > unique_module_context;

    //
    // Membership
    //

    enum class member_kind : core::byte
    {
        event,
        field,
        interface_,
        method,
        property
    };

    template <member_kind> class member_table_entry_facade;
    template <member_kind> class member_table_iterator_constructor;
    template <member_kind> class member_traits;

    typedef member_table_entry_facade<member_kind::event     > event_table_entry;
    typedef member_table_entry_facade<member_kind::field     > field_table_entry;
    typedef member_table_entry_facade<member_kind::interface_> interface_table_entry;
    typedef member_table_entry_facade<member_kind::method    > method_table_entry;
    typedef member_table_entry_facade<member_kind::property  > property_table_entry;

    typedef member_traits<member_kind::event     > event_traits;
    typedef member_traits<member_kind::field     > field_traits;
    typedef member_traits<member_kind::interface_> interface_traits;
    typedef member_traits<member_kind::method    > method_traits;
    typedef member_traits<member_kind::property  > property_traits;

    typedef core::instantiating_iterator<
        core::stride_iterator,
        member_table_entry_facade<member_kind::event> const*,
        member_kind,
        member_table_iterator_constructor<member_kind::event>
    > event_table_iterator;

    typedef core::instantiating_iterator<
        core::stride_iterator,
        member_table_entry_facade<member_kind::field> const*,
        member_kind,
        member_table_iterator_constructor<member_kind::field>
    > field_table_iterator;

    typedef core::instantiating_iterator<
        core::stride_iterator,
        member_table_entry_facade<member_kind::interface_> const*,
        member_kind,
        member_table_iterator_constructor<member_kind::interface_>
    > interface_table_iterator;

    typedef core::instantiating_iterator<
        core::stride_iterator,
        member_table_entry_facade<member_kind::method> const*,
        member_kind,
        member_table_iterator_constructor<member_kind::method>
    > method_table_iterator;

    typedef core::instantiating_iterator<
        core::stride_iterator,
        member_table_entry_facade<member_kind::property> const*,
        member_kind,
        member_table_iterator_constructor<member_kind::property>
    > property_table_iterator;

    typedef core::iterator_range<event_table_iterator    > event_table_range;
    typedef core::iterator_range<field_table_iterator    > field_table_range;
    typedef core::iterator_range<interface_table_iterator> interface_table_range;
    typedef core::iterator_range<method_table_iterator   > method_table_range;
    typedef core::iterator_range<property_table_iterator > property_table_range;

    using std::begin;
    using std::end;

} } }

#endif
