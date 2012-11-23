
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/assembly_context.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/member_iterator.hpp"
#include "cxxreflect/reflection/detail/membership.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/detail/type_name_builder.hpp"
#include "cxxreflect/reflection/detail/type_policy.hpp"
#include "cxxreflect/reflection/detail/type_resolution.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/custom_modifier_iterator.hpp"
#include "cxxreflect/reflection/loader.hpp"
#include "cxxreflect/reflection/method.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"





namespace cxxreflect { namespace reflection { namespace {

    template <typename T>
    auto core_filter_member(metadata::binding_flags const filter, bool const is_declaring_type, T const& current) -> bool
    {
        core::assert_not_null(current);

        typedef typename core::identity<decltype(row_from(current->member_token()).flags())>::type::enumeration_type attribute_type;
        
        auto const current_flags(row_from(current->member_token()).flags());

        if (current_flags.is_set(attribute_type::static_))
        {
            if (!filter.is_set(metadata::binding_attribute::static_))
                return true;
        }
        else
        {
            if (!filter.is_set(metadata::binding_attribute::instance))
                return true;
        }

        if (current_flags.with_mask(attribute_type::member_access_mask) == attribute_type::public_)
        {
            if (!filter.is_set(metadata::binding_attribute::public_))
                return true;
        }
        else
        {
            if (!filter.is_set(metadata::binding_attribute::non_public))
                return true;
        }

        if (!is_declaring_type)
        {
            if (filter.is_set(metadata::binding_attribute::declared_only))
                return true;

            // Static members are not inherited, but they are returned with FlattenHierarchy
            if (current_flags.is_set(attribute_type::static_) &&
                !filter.is_set(metadata::binding_attribute::flatten_hierarchy))
                return true;

            core::string_reference const member_name(row_from(current->member_token()).name());

            // Nonpublic methods inherited from base classes are never returned, except for
            // explicit interface implementations, which may be returned:
            if (current_flags.with_mask(attribute_type::member_access_mask) == attribute_type::private_)
            {
                if (current_flags.is_set(attribute_type::static_))
                    return true;

                if (!std::any_of(begin(member_name), end(member_name), [](core::character c) { return c == L'.'; }))
                    return true;
            }
        }

        return false;
    }

} } }

namespace cxxreflect { namespace reflection { namespace detail {

    template <typename T, typename D>
    auto common_type_functionality<T, D>::context(core::internal_key) const -> token_type const&
    {
        core::assert_initialized(*this);
        return _token;
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::assembly_qualified_name() const -> core::string
    {
        core::assert_initialized(*this);
        return type_name_builder::build_type_name(_token, type_name_builder::mode::assembly_qualified_name);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::full_name() const -> core::string
    {
        core::assert_initialized(*this);
        return type_name_builder::build_type_name(_token, type_name_builder::mode::full_name);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::simple_name() const -> core::string
    {
        core::assert_initialized(*this);
        return type_name_builder::build_type_name(_token, type_name_builder::mode::simple_name);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::primary_name() const -> core::string_reference
    {
        core::assert_initialized(*this);
        return _policy->primary_name(_token);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::namespace_name() const -> core::string_reference
    {
        core::assert_initialized(*this);
        return _policy->namespace_name(_token);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::declaring_type() const -> derived_type
    {
        core::assert_initialized(*this);
        return derived_type(_policy->declaring_type(_token), core::internal_key());
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::element_type() const -> unresolved_type
    {
        core::assert_initialized(*this);
        return unresolved_type(detail::compute_element_type(_token), core::internal_key());
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::is_array() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_array(_token);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::is_by_ref() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_by_ref(_token);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::is_generic_type_instantiation() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_generic_type_instantiation(_token);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::is_nested() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_nested(_token);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::is_pointer() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_pointer(_token);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::is_primitive() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_primitive(_token);
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::is_initialized() const -> bool
    {
        return _token.is_initialized() && _policy.is_initialized();
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::operator!() const -> bool
    {
        return !is_initialized();
    }

    template <typename T, typename D>
    common_type_functionality<T, D>::common_type_functionality()
    {
    }

    template <typename T, typename D>
    common_type_functionality<T, D>::common_type_functionality(token_type const& token)
        : _token(token)
    {
        if (token.is_initialized())
            _policy.get() = &type_policy::get_for(token);
    }

    template <typename T, typename D>
    common_type_functionality<T, D>::~common_type_functionality()
    {
        // Destructor definition required to prevent polymorphic destruction
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::token() const -> token_type const&
    {
        core::assert_initialized(*this);
        return _token;
    }

    template <typename T, typename D>
    auto common_type_functionality<T, D>::policy() const -> type_policy const&
    {
        core::assert_initialized(*this);
        return *_policy;
    }

    template class common_type_functionality<metadata::type_def_ref_or_signature, unresolved_type>;
    template class common_type_functionality<metadata::type_def_or_signature,     type>;

} } }

namespace cxxreflect { namespace reflection {

    unresolved_type::unresolved_type()
    {
    }

    unresolved_type::unresolved_type(metadata::type_def_ref_spec_or_signature const& token, core::internal_key)
        : common_type_functionality(token.is_initialized() ? detail::compute_type(token) : metadata::type_def_ref_or_signature())
    {
    }

    auto unresolved_type::resolve() const -> type
    {
        return type(detail::resolve_type(token()), core::internal_key());
    }

    unresolved_type::unresolved_type(type const& reflected_type, detail::interface_table_entry const* const context, core::internal_key)
        : common_type_functionality([&]() -> metadata::type_def_ref_or_signature
    {
        core::assert_not_null(context);

        metadata::type_signature const signature(context->member_signature());
        if (signature.is_initialized())
            return metadata::blob(signature);

        return detail::compute_type(detail::interface_traits::get_interface_type(context->member_token()));
    }())
    {
    }




    auto type::filter_event(metadata::binding_flags, type const&, detail::event_table_entry const* const&) -> bool
    {
        core::assert_not_yet_implemented();
    }

    auto type::filter_field(metadata::binding_flags          const  filter,
                            type                             const& reflected_type,
                            detail::field_table_entry const* const& current) -> bool
    {
        core::assert_initialized(reflected_type);
        core::assert_not_null(current);

        metadata::type_def_token const declaring_type(metadata::find_owner_of_field(current->member_token()).token());

        bool const reflected_is_declaring_type(reflected_type.context(core::internal_key()) == declaring_type);

        if (core_filter_member(filter, reflected_is_declaring_type, current))
            return true;

        return false;
    }

    auto type::filter_interface(metadata::binding_flags, type const&, detail::interface_table_entry const* const&) -> bool
    {
        // Interfaces are never filtered
        return false;
    }

    auto type::filter_method(metadata::binding_flags           const  filter,
                             type                              const& reflected_type,
                             detail::method_table_entry const* const& current) -> bool
    {
        core::assert_initialized(reflected_type);
        core::assert_not_null(current);

        metadata::type_def_token const declaring_type(metadata::find_owner_of_method_def(current->member_token()).token());

        bool const reflected_is_declaring_type(reflected_type.metadata_token() == declaring_type.value());

        if (core_filter_member(filter, reflected_is_declaring_type, current))
            return true;

        core::string_reference const name(row_from(current->member_token()).name());
        bool const is_constructor(
            row_from(current->member_token()).flags().is_set(metadata::method_attribute::special_name) && 
            (name == L".ctor" || name == L".cctor"));

        return is_constructor != filter.is_set(metadata::binding_attribute::internal_use_only_constructor);
    }

    auto type::filter_property(metadata::binding_flags, type const&, detail::property_table_entry const* const&) -> bool
    {
        core::assert_not_yet_implemented();
    }
    
    type::type()
    {
    }

    type::type(metadata::type_def_ref_spec_or_signature const& token, core::internal_key)
        : common_type_functionality(token.is_initialized()
            ? detail::resolve_type(token)
            : metadata::type_def_or_signature())
    {
    }

    type::type(unresolved_type const& source)
        : common_type_functionality(source.is_initialized()
            ? detail::resolve_type(source.context(core::internal_key()))
            : metadata::type_def_or_signature())
    {
    }

    auto type::defining_assembly() const -> assembly
    {
        return assembly(&detail::module_context::from(token().scope()).assembly(), core::internal_key());
    }

    auto type::defining_module() const -> module
    {
        return module(&detail::module_context::from(token().scope()), core::internal_key());
    }

    auto type::metadata_token() const -> core::size_type
    {
        return policy().metadata_token(token());
    }

    auto type::attributes() const -> metadata::type_flags
    {
        return policy().attributes(token());
    }

    auto type::base_type() const -> unresolved_type
    {
        metadata::type_def_ref_or_signature const base(policy().base_type(token()));
        return base.is_initialized()
            ? unresolved_type(base, core::internal_key())
            : unresolved_type();
    }

    auto type::layout() const -> type_layout
    {
        return policy().layout(token());
    }

    auto type::string_format() const -> type_string_format
    {
        return policy().string_format(token());
    }

    auto type::visibility() const -> type_visibility
    {
        return policy().visibility(token());
    }

    auto type::is_abstract() const -> bool
    {
        return policy().is_abstract(token());
    }

    auto type::is_class() const -> bool
    {
        return !is_interface() && !is_value_type();
    }

    auto type::is_com_object() const -> bool
    {
        return policy().is_com_object(token());
    }

    auto type::is_contextful() const -> bool
    {
        return policy().is_contextful(token());
    }

    auto type::is_enum() const -> bool
    {
        return policy().is_enum(token());
    }

    auto type::is_generic_parameter() const -> bool
    {
        return policy().is_generic_parameter(token());
    }

    auto type::is_generic_type() const -> bool
    {
        return policy().is_generic_type(token());
    }

    auto type::is_generic_type_definition() const -> bool
    {
        return policy().is_generic_type_definition(token());
    }

    auto type::is_import() const -> bool
    {
        return policy().is_import(token());
    }

    auto type::is_interface() const -> bool
    {
        return policy().is_interface(token());
    }

    auto type::is_marshal_by_ref() const -> bool
    {
        return policy().is_marshal_by_ref(token());
    }

    auto type::is_sealed() const -> bool
    {
        return policy().is_sealed(token());
    }

    auto type::is_serializable() const -> bool
    {
        return policy().is_serializable(token());
    }

    auto type::is_special_name() const -> bool
    {
        return policy().is_special_name(token());
    }

    auto type::is_value_type() const -> bool
    {
        return policy().is_value_type(token());
    }

    auto type::is_visible() const -> bool
    {
        return policy().is_visible(token());
    }

    auto type::interfaces() const -> interface_range
    {
        token_type const token(token());
        if (token.is_blob() && token.as_blob().as<metadata::type_signature>().is_by_ref())
            return interface_range();

        auto const& table(detail::loader_context::from(token.scope()).get_membership(token).get_interfaces());
        return interface_range(
            interface_iterator(*this, begin(table), end(table), metadata::binding_flags()),
            interface_iterator());
    }

    auto type::constructors(metadata::binding_flags flags) const -> method_range
    {
        core::assert_initialized(*this);

        flags.set(metadata::binding_attribute::internal_use_only_constructor);
        flags.set(metadata::binding_attribute::declared_only);
        flags.unset(metadata::binding_attribute::flatten_hierarchy);

        auto const& table(detail::loader_context::from(token().scope()).get_membership(token()).get_methods());
        if (table.empty())
            return method_range();

        return method_range(
            method_iterator(*this, begin(table), end(table), flags),
            method_iterator());
    }

    auto type::fields(metadata::binding_flags const flags) const -> field_range
    {
        core::assert_initialized(*this);
        if (is_by_ref())
            return field_range();

        auto const& table(detail::loader_context::from(token().scope()).get_membership(token()).get_fields());
        if (table.empty())
            return field_range();

        return field_range(
            field_iterator(*this, begin(table), end(table), flags),
            field_iterator());
    }

    auto type::methods(metadata::binding_flags const flags) const -> method_range
    {
        core::assert_initialized(*this);
        if (is_by_ref())
            return method_range();

        auto const& table(detail::loader_context::from(token().scope()).get_membership(token()).get_methods());
        if (table.empty())
            return method_range();

        return method_range(
            method_iterator(*this, begin(table), end(table), flags),
            method_iterator());
    }

    auto type::find_method(core::string_reference const name, metadata::binding_flags const flags) const -> method
    {
        core::assert_initialized(*this);

        auto const predicate([&](method const& m) { return m.name() == name; });

        auto const methods(methods(flags));
        auto const it(std::find_if(begin(methods), end(methods), predicate));

        if (it != end(methods) && std::find_if(std::next(it), end(methods), predicate) != end(methods))
            throw core::runtime_error(L"method name is not unique");

        return it != end(methods) ? *it : method();
    }

    auto type::custom_attributes() const -> detail::custom_attribute_range
    {
        core::assert_initialized(*this);
        if (token().is_blob())
            return detail::custom_attribute_range();

        return custom_attribute::get_for(token().as_token(), core::internal_key());
    }

    auto type::required_custom_modifiers() const -> custom_modifier_range
    {
        core::assert_initialized(*this);
        if (!token().is_blob())
            return custom_modifier_range();

        return get_required_custom_modifiers(token().as_blob().as<metadata::type_signature>());
    }

    auto type::optional_custom_modifiers() const -> custom_modifier_range
    {
        core::assert_initialized(*this);
        if (!token().is_blob())
            return custom_modifier_range();

        return get_optional_custom_modifiers(token().as_blob().as<metadata::type_signature>());
    }

} }
