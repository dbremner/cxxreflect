
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_TYPE_HPP_
#define CXXREFLECT_REFLECTION_TYPE_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    template <typename Token, typename DerivedType>
    class common_type_functionality
    {
    public:

        typedef Token       token_type;
        typedef DerivedType derived_type;

        auto context(core::internal_key) const -> token_type const&;

        auto assembly_qualified_name() const -> core::string;
        auto full_name()               const -> core::string;
        auto simple_name()             const -> core::string;
        auto primary_name()            const -> core::string_reference;
        auto namespace_name()          const -> core::string_reference;

        auto declaring_type() const -> derived_type;
        auto element_type()   const -> unresolved_type;

        auto is_array()                      const -> bool;
        auto is_by_ref()                     const -> bool;
        auto is_generic_type_instantiation() const -> bool;
        auto is_nested()                     const -> bool;
        auto is_pointer()                    const -> bool;
        auto is_primitive()                  const -> bool;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(common_type_functionality)

    protected:

        common_type_functionality();
        common_type_functionality(token_type const&);
        ~common_type_functionality();

        auto token()  const -> token_type  const&;
        auto policy() const -> type_policy const&;

    private:

        token_type                               _token;
        core::checked_pointer<type_policy const> _policy;
    };

} } }





namespace cxxreflect { namespace reflection {

    class type;

    class unresolved_type
        : public detail::common_type_functionality<metadata::type_def_ref_or_signature, unresolved_type>
    {
    public:

        unresolved_type();
        unresolved_type(metadata::type_def_ref_spec_or_signature const& token, core::internal_key);
        unresolved_type(type const& reflected_type, detail::interface_table_entry const* context, core::internal_key);

        auto resolve() const -> type;
    };

    class type
        : public detail::common_type_functionality<metadata::type_def_or_signature, type>
    {
    private:

        static auto filter_event    (metadata::binding_flags, type const&, detail::event_table_entry     const* const&) -> bool;
        static auto filter_field    (metadata::binding_flags, type const&, detail::field_table_entry     const* const&) -> bool;
        static auto filter_interface(metadata::binding_flags, type const&, detail::interface_table_entry const* const&) -> bool;
        static auto filter_method   (metadata::binding_flags, type const&, detail::method_table_entry    const* const&) -> bool;
        static auto filter_property (metadata::binding_flags, type const&, detail::property_table_entry  const* const&) -> bool;

    public:

        typedef detail::member_iterator<
            type,
            event,
            detail::event_table_iterator,
            &type::filter_event
        > event_iterator;

        typedef detail::member_iterator<
            type,
            field,
            detail::field_table_iterator,
            &type::filter_field
        > field_iterator;

        typedef detail::member_iterator<
            type,
            unresolved_type,
            detail::interface_table_iterator,
            &type::filter_interface
        > interface_iterator;

        typedef detail::member_iterator<
            type,
            method,
            detail::method_table_iterator,
            &type::filter_method
        > method_iterator;

        typedef detail::member_iterator<
            type,
            property,
            detail::property_table_iterator,
            &type::filter_property
        > property_iterator;

        typedef core::iterator_range<event_iterator    > event_range;
        typedef core::iterator_range<field_iterator    > field_range;
        typedef core::iterator_range<interface_iterator> interface_range;
        typedef core::iterator_range<method_iterator   > method_range;
        typedef core::iterator_range<property_iterator > property_range;

        type();
        type(metadata::type_def_ref_spec_or_signature const& token, core::internal_key);
        type(unresolved_type const& source);
        
        auto defining_assembly() const -> assembly;
        auto defining_module()   const -> module;

        auto metadata_token() const -> core::size_type;
        auto attributes()     const -> metadata::type_flags;

        auto base_type() const -> unresolved_type;

        auto layout()        const -> type_layout;
        auto string_format() const -> type_string_format;
        auto visibility()    const -> type_visibility;

        auto is_abstract()                   const -> bool;
        auto is_class()                      const -> bool;
        auto is_com_object()                 const -> bool;
        auto is_contextful()                 const -> bool;
        auto is_enum()                       const -> bool;
        auto is_generic_parameter()          const -> bool;
        auto is_generic_type()               const -> bool;
        auto is_generic_type_definition()    const -> bool;
        auto is_import()                     const -> bool;
        auto is_interface()                  const -> bool;
        auto is_marshal_by_ref()             const -> bool;
        auto is_sealed()                     const -> bool;
        auto is_serializable()               const -> bool;
        auto is_special_name()               const -> bool;
        auto is_value_type()                 const -> bool;
        auto is_visible()                    const -> bool;

        auto interfaces() const -> interface_range;
        auto constructors(metadata::binding_flags = metadata::binding_attribute::default_) const -> method_range;
        auto events      (metadata::binding_flags = metadata::binding_attribute::default_) const -> event_range;
        auto fields      (metadata::binding_flags = metadata::binding_attribute::default_) const -> field_range;
        auto methods     (metadata::binding_flags = metadata::binding_attribute::default_) const -> method_range;
        auto properties  (metadata::binding_flags = metadata::binding_attribute::default_) const -> property_range;

        auto find_method(core::string_reference name, metadata::binding_flags = metadata::binding_attribute::default_) const -> method;

        auto custom_attributes()         const -> detail::custom_attribute_range;
        auto required_custom_modifiers() const -> custom_modifier_range;
        auto optional_custom_modifiers() const -> custom_modifier_range;
    };

    template <typename TL, typename DL, typename TR, typename DR>
    auto operator==(detail::common_type_functionality<TL, DL> const& lhs, detail::common_type_functionality<TR, DR> const& rhs) -> bool
    {
        if (!lhs.is_initialized() || !rhs.is_initialized())
            return lhs.is_initialized() == rhs.is_initialized();

        return lhs.context(core::internal_key()) == rhs.context(core::internal_key());
    }

    template <typename TL, typename DL, typename TR, typename DR>
    auto operator!=(detail::common_type_functionality<TL, DL> const& lhs, detail::common_type_functionality<TR, DR> const& rhs) -> bool
    {
        return !(lhs == rhs);
    }

    template <typename TL, typename DL, typename TR, typename DR>
    auto operator<(detail::common_type_functionality<TL, DL> const& lhs, detail::common_type_functionality<TR, DR> const& rhs) -> bool
    {
        return lhs.context(core::internal_key()) < rhs.context(core::internal_key());
    }

    template <typename TL, typename DL, typename TR, typename DR>
    auto operator>(detail::common_type_functionality<TL, DL> const& lhs, detail::common_type_functionality<TR, DR> const& rhs) -> bool
    {
        return rhs < lhs;
    }

    template <typename TL, typename DL, typename TR, typename DR>
    auto operator<=(detail::common_type_functionality<TL, DL> const& lhs, detail::common_type_functionality<TR, DR> const& rhs) -> bool
    {
        return !(rhs < lhs);
    }

    template <typename TL, typename DL, typename TR, typename DR>
    auto operator>=(detail::common_type_functionality<TL, DL> const& lhs, detail::common_type_functionality<TR, DR> const& rhs) -> bool
    {
        return !(lhs < rhs);
    }

} }

#endif
