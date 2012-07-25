
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_TYPE_HPP_
#define CXXREFLECT_REFLECTION_TYPE_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/independent_handles.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"
#include "cxxreflect/reflection/detail/member_iterator.hpp"
#include "cxxreflect/reflection/detail/type_name_builder.hpp"
#include "cxxreflect/reflection/detail/type_policy.hpp"

namespace cxxreflect { namespace reflection {

    class type
    {
    private:

        static auto filter_event    (metadata::binding_flags, type const&, detail::event_context     const&) -> bool;
        static auto filter_field    (metadata::binding_flags, type const&, detail::field_context     const&) -> bool;
        static auto filter_interface(metadata::binding_flags, type const&, detail::interface_context const&) -> bool;
        static auto filter_method   (metadata::binding_flags, type const&, detail::method_context    const&) -> bool;
        static auto filter_property (metadata::binding_flags, type const&, detail::property_context  const&) -> bool;

    public:

        typedef detail::member_iterator<
            type,
            event,
            detail::event_context,
            &type::filter_event
        > event_iterator;

        typedef detail::member_iterator<
            type,
            field,
            detail::field_context,
            &type::filter_field
        > field_iterator;

        typedef detail::member_iterator<
            type,
            type,
            detail::interface_context,
            &type::filter_interface
        > interface_iterator;

        typedef detail::member_iterator<
            type,
            method,
            detail::method_context,
            &type::filter_method
        > method_iterator;

        typedef detail::member_iterator<
            type,
            property,
            detail::property_context,
            &type::filter_property
        > property_iterator;

        type();

        auto defining_module()   const -> module;
        auto defining_assembly() const -> assembly;

        auto metadata_token() const -> core::size_type;
        auto attributes()     const -> metadata::type_flags;

        auto base_type()      const -> type;
        auto declaring_type() const -> type;
        auto element_type()   const -> type;

        auto assembly_qualified_name() const -> core::string;
        auto full_name()               const -> core::string;
        auto simple_name()             const -> core::string;
        auto basic_name()              const -> core::string_reference;
        auto namespace_name()          const -> core::string_reference;

        auto is_abstract()                   const -> bool;
        auto is_ansi_class()                 const -> bool;
        auto is_array()                      const -> bool;
        auto is_auto_class()                 const -> bool;
        auto is_auto_layout()                const -> bool;
        auto is_by_ref()                     const -> bool;
        auto is_class()                      const -> bool;
        auto is_com_object()                 const -> bool;
        auto is_contextful()                 const -> bool;
        auto is_enum()                       const -> bool;
        auto is_explicit_layout()            const -> bool;
        auto is_generic_parameter()          const -> bool;
        auto is_generic_type()               const -> bool;
        auto is_generic_type_definition()    const -> bool;
        auto is_import()                     const -> bool;
        auto is_interface()                  const -> bool;
        auto is_layout_sequential()          const -> bool;
        auto is_marshal_by_ref()             const -> bool;
        auto is_nested()                     const -> bool;
        auto is_nested_assembly()            const -> bool;
        auto is_nested_family_and_assembly() const -> bool;
        auto is_nested_family()              const -> bool;
        auto is_nested_family_or_assembly()  const -> bool;
        auto is_nested_private()             const -> bool;
        auto is_nested_public()              const -> bool;
        auto is_not_public()                 const -> bool;
        auto is_pointer()                    const -> bool;
        auto is_primitive()                  const -> bool;
        auto is_public()                     const -> bool;
        auto is_sealed()                     const -> bool;
        auto is_serializable()               const -> bool;
        auto is_special_name()               const -> bool;
        auto is_unicode_class()              const -> bool;
        auto is_value_type()                 const -> bool;
        auto is_visible()                    const -> bool;

        auto begin_interfaces() const -> interface_iterator;
        auto end_interfaces()   const -> interface_iterator;
        auto find_interface(core::string_reference name) const -> type;

        auto begin_constructors(metadata::binding_flags = metadata::binding_attribute::default_) const -> method_iterator;
        auto end_constructors() const -> method_iterator;

        // TODO EventIterator BeginEvents(BindingFlags flags = BindingAttribute::Default) const;
        // TODO EventIterator EndEvents() const;

        auto begin_fields(metadata::binding_flags = metadata::binding_attribute::default_) const -> field_iterator;
        auto end_fields() const -> field_iterator;

        auto begin_methods(metadata::binding_flags = metadata::binding_attribute::default_) const -> method_iterator;
        auto end_methods() const -> method_iterator;
        auto find_method(core::string_reference name, metadata::binding_flags = metadata::binding_attribute::default_) const -> method;

        // TODO PropertyIterator BeginProperties(BindingFlags flags = BindingAttribute::Default) const;
        // TODO PropertyIterator EndProperties() const;

        // TODO Provide ability to return inherited attributes
        auto begin_custom_attributes() const -> custom_attribute_iterator;
        auto end_custom_attributes()   const -> custom_attribute_iterator;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        // ContainsGenericParameters
        // DeclaringMethod
        // [static] DefaultBinder
        // GenericParameterAttributes
        // GenericParameterPosition
        // GUID
        // Module
        // ReflectedType
        // StructLayoutAttribute
        // TypeHandle
        // TypeInitializer
        // UnderlyingSystemType

        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent

        friend auto operator==(type const&, type const&) -> bool;
        friend auto operator< (type const&, type const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(type)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(type)

    public: // internal members

        type(module                                   const& declaring_module,
             metadata::type_def_ref_spec_or_signature const& type_token,
             core::internal_key);

        type(detail::loader_context                   const& owning_loader,
             metadata::type_def_ref_spec_or_signature const& type_token,
             core::internal_key);

        type(type                      const& reflected_type,
             detail::interface_context const* context,
             core::internal_key);

        auto self_reference(core::internal_key) const -> metadata::type_def_or_signature;

    private:

        friend detail::type_name_builder;
        friend class debug_type;

        auto is_type_def()  const -> bool;
        auto is_type_spec() const -> bool;

        auto get_type_def_row()        const -> metadata::type_def_row;
        auto get_type_spec_signature() const -> metadata::type_signature;

        #define CXXREFLECT_GENERATE decltype(std::declval<Callback>()(std::declval<type>()))

        static auto resolve_type_def(type const&) -> type;

        // Resolves the TypeDef associated with this type.  If this type is itself a TypeDef, it
        // returns itself.  If this type is a TypeSpec, it parses the TypeSpec to find the
        // primary TypeDef referenced by the TypeSpec; note that in this case the TypeDef may be
        // in a different module or assembly.
        template <typename Callback>
        auto resolve_type_def_and_call(
            Callback callback,
            CXXREFLECT_GENERATE default_result = typename core::identity<CXXREFLECT_GENERATE>::type()
        ) const -> CXXREFLECT_GENERATE
        {
            core::assert_initialized(*this);

            type const type_def(resolve_type_def(*this));
            if (!type_def.is_initialized())
                return default_result;

            return callback(type_def);
        }

        #undef CXXREFLECT_GENERATE

        detail::type_policy_handle                _policy;
        detail::type_def_or_signature_with_module _type;
    };





    class debug_type
    {
    public:

        debug_type(type const&);

    private:

        bool            _is_initialized;
        bool            _is_type_def;
        core::size_type _metadata_token;
        core::size_type _attributes;
        core::string    _simple_name;
        core::string    _namespace_name;
        core::string    _full_name;
        core::string    _assembly_qualified_name;
        core::string    _base_type_aqn;
    };

} }

#endif 
