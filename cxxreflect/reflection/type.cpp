
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/method.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy_utility.hpp"

namespace cxxreflect { namespace reflection { namespace {

    template <typename T>
    auto core_filter_member(metadata::binding_flags const filter, bool const is_declaring_type, T const& current) -> bool
    {
        typedef typename core::identity<decltype(current.element_row().flags())>::type::enumeration_type attribute_type;
        
        auto const current_flags(current.element_row().flags());

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

            core::string_reference const member_name(current.element_row().name());

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

namespace cxxreflect { namespace reflection {

    auto type::filter_event(metadata::binding_flags, type const&, detail::event_context const&) -> bool
    {
        // TODO To filter events, we need to compute the most accessible related method
        return false;
    }

    auto type::filter_field(metadata::binding_flags const  filter,
                     type                     const& reflected_type,
                     detail::field_context    const& current) -> bool
    {
        metadata::type_def_token const declaring_type(metadata::find_owner_of_field(current.element()).token());

        bool const reflected_is_declaring_type(reflected_type.metadata_token() == declaring_type.value());

        if (core_filter_member(filter, reflected_is_declaring_type, current))
            return true;

        return false;
    }

    auto type::filter_interface(metadata::binding_flags const, type const&, detail::interface_context const&) -> bool
    {
        // We do not provide a filtered view of interfaces; all interface implementations are public
        return false;
    }

    auto type::filter_method(metadata::binding_flags const  filter,
                       type                    const& reflected_type,
                       detail::method_context  const& current) -> bool
    {
        metadata::type_def_token const declaring_type(metadata::find_owner_of_method_def(current.element()).token());

        bool const reflected_is_declaring_type(reflected_type.metadata_token() == declaring_type.value());

        if (core_filter_member(filter, reflected_is_declaring_type, current))
            return true;

        core::string_reference const name(current.element_row().name());
        bool const is_constructor(
            current.element_row().flags().is_set(metadata::method_attribute::special_name) && 
            (name == L".ctor" || name == L".cctor"));

        return is_constructor != filter.is_set(metadata::binding_attribute::internal_use_only_constructor);
    }

    auto type::filter_property(metadata::binding_flags, type const&, detail::property_context const&) -> bool
    {
        // TODO To filter properties, we need to compute the most accessible related method
        return false;
    }

    type::type()
    {
    }

    type::type(module                                   const& declaring_module,
               metadata::type_def_ref_spec_or_signature const& type_token,
               core::internal_key)
    {
        core::assert_initialized(declaring_module);
        core::assert_initialized(type_token);

        _type = detail::resolve_type_for_construction(
            detail::type_def_ref_spec_or_signature_with_module(
                &declaring_module.context(core::internal_key()),
                type_token));

       _policy = detail::type_policy::get_for(_type);

        core::assert_initialized(*this);
    }

    type::type(detail::loader_context                   const& owning_loader,
               metadata::type_def_ref_spec_or_signature const& type_token,
               core::internal_key)
    {
        core::assert_initialized(type_token);

        _type = detail::resolve_type_for_construction(
            detail::type_def_ref_spec_or_signature_with_module(
                &owning_loader.module_from_scope(type_token.scope()),
                type_token));

        _policy = detail::type_policy::get_for(_type);

        core::assert_initialized(*this);
    }

    type::type(type                      const& reflected_type,
               detail::interface_context const* context,
               core::internal_key)
    {
        core::assert_initialized(reflected_type);
        core::assert_not_null(context);

        detail::loader_context const& root(detail::loader_context::from(reflected_type));

        metadata::type_signature const signature(context->element_signature(root));
        if (signature.is_initialized())
        {
            _type = resolve_type_for_construction(
                detail::type_def_ref_spec_or_signature_with_module(
                    &root.module_from_scope(signature.scope()),
                    metadata::blob(signature)));
        }
        else
        {
            _type = resolve_type_for_construction(
                detail::type_def_ref_spec_or_signature_with_module(
                    &root.module_from_scope(context->element().scope()),
                    context->element_row().interface()));
        }

        _policy = detail::type_policy::get_for(_type);

        core::assert_initialized(*this);
    }

    auto type::defining_module() const -> module
    {
        core::assert_initialized(*this);
        return _type.module().realize();
    }

    auto type::defining_assembly() const -> assembly
    {
        core::assert_initialized(*this);
        return _type.module().realize().defining_assembly();
    }

    auto type::metadata_token() const -> core::size_type
    {
        core::assert_initialized(*this);
        return _policy->metadata_token(_type);
    }

    auto type::attributes() const -> metadata::type_flags
    {
        core::assert_initialized(*this);
        return _policy->attributes(_type);
    }

    auto type::base_type() const -> type
    {
        core::assert_initialized(*this);
        detail::type_def_or_signature_with_module const base(_policy->base_type(_type));
        if (!base.is_initialized())
            return type();

        return type(base.module().realize(), base.type(), core::internal_key());
    }

    auto type::declaring_type() const -> type
    {
        core::assert_initialized(*this);
        detail::type_def_or_signature_with_module const declarer(_policy->declaring_type(_type));
        if (!declarer.is_initialized())
            return type();

        return type(declarer.module().realize(), declarer.type(), core::internal_key());
    }

    auto type::element_type() const -> type
    {
        core::assert_initialized(*this);
        detail::type_def_or_signature_with_module const element(detail::resolve_element_type(_type));
        if (!element.is_initialized())
            return type();

        return type(element.module().realize(), element.type(), core::internal_key());
    }

    auto type::assembly_qualified_name() const -> core::string
    {
        core::assert_initialized(*this);

        return detail::type_name_builder::build_type_name(
            *this,
            detail::type_name_builder::mode::assembly_qualified_name);
    }

    auto type::full_name() const -> core::string
    {
        core::assert_initialized(*this);

        return detail::type_name_builder::build_type_name(
            *this,
            detail::type_name_builder::mode::full_name);
    }

    auto type::simple_name() const -> core::string
    {
        core::assert_initialized(*this);

        return detail::type_name_builder::build_type_name(
            *this,
            detail::type_name_builder::mode::simple_name);
    }

    auto type::basic_name() const -> core::string_reference
    {
        core::assert_initialized(*this);
        detail::type_def_with_module const definition(detail::resolve_type_def(_type));
        if (!definition.is_initialized())
            return core::string_reference();

        return row_from(definition.type()).name();
    }

    auto type::namespace_name() const -> core::string_reference
    {
        core::assert_initialized(*this);

        bool const is_nested_type(detail::resolve_type_def_and_call(_type, &detail::type_policy::is_nested));

        if (is_type_spec() && get_type_spec_signature().is_method_variable())
        {
            return core::string_reference::from_literal(L"System");
        }
        else if (is_type_spec() && get_type_spec_signature().is_class_variable())
        {
            return declaring_type().namespace_name();
        }

        // TODO How do we need to handle nested-nested types?
        return resolve_type_def_and_call([=](type const& t)
        {
            return is_nested_type
                ? t.declaring_type().namespace_name()
                : t.get_type_def_row().namespace_name();
        });
    }

    auto type::is_abstract() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_abstract(_type);
    }

    auto type::is_ansi_class() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->string_format(_type) == detail::type_attribute_string_format::ansi_string_format;
    }

    auto type::is_array() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_array(_type);
    }

    auto type::is_auto_class() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->string_format(_type) == detail::type_attribute_string_format::auto_string_format;
    }

    auto type::is_auto_layout() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->layout(_type) == detail::type_attribute_layout::auto_layout;
    }

    auto type::is_by_ref() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_by_ref(_type);
    }

    auto type::is_class() const -> bool
    {
        core::assert_initialized(*this);
        return !is_interface() && !is_value_type();
    }

    auto type::is_com_object() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_com_object(_type);
    }

    auto type::is_contextful() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_contextful(_type);
    }

    auto type::is_enum() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_enum(_type);
    }

    auto type::is_explicit_layout() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->layout(_type) == detail::type_attribute_layout::explicit_layout;
    }

    auto type::is_generic_parameter() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_generic_parameter(_type);
    }

    auto type::is_generic_type() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_generic_type(_type);
    }

    auto type::is_generic_type_definition() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_generic_type_definition(_type);
    }

    auto type::is_import() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_import(_type);
    }

    auto type::is_interface() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_interface(_type);
    }

    auto type::is_layout_sequential() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->layout(_type) == detail::type_attribute_layout::sequential_layout;
    }

    auto type::is_marshal_by_ref() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_marshal_by_ref(_type);
    }

    auto type::is_nested() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_nested(_type);
    }

    auto type::is_nested_assembly() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->visibility(_type) == detail::type_attribute_visibility::nested_assembly;
    }

    auto type::is_nested_family_and_assembly() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->visibility(_type) == detail::type_attribute_visibility::nested_family_and_assembly;
    }

    auto type::is_nested_family() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->visibility(_type) == detail::type_attribute_visibility::nested_family;
    }

    auto type::is_nested_family_or_assembly()  const -> bool
    {
        core::assert_initialized(*this);
        return _policy->visibility(_type) == detail::type_attribute_visibility::nested_family_or_assembly;
    }

    auto type::is_nested_private() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->visibility(_type) == detail::type_attribute_visibility::nested_private;
    }

    auto type::is_nested_public() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->visibility(_type) == detail::type_attribute_visibility::nested_public;
    }

    auto type::is_not_public() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->visibility(_type) == detail::type_attribute_visibility::not_public;
    }

    auto type::is_pointer() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_pointer(_type);
    }

    auto type::is_primitive() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_primitive(_type);
    }

    auto type::is_public() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->visibility(_type) == detail::type_attribute_visibility::public_;
    }

    auto type::is_sealed() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_sealed(_type);
    }

    auto type::is_serializable() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_serializable(_type);
    }

    auto type::is_special_name() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_special_name(_type);
    }

    auto type::is_unicode_class() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->string_format(_type) == detail::type_attribute_string_format::unicode_string_format;
    }

    auto type::is_value_type() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_value_type(_type);
    }

    auto type::is_visible() const -> bool
    {
        core::assert_initialized(*this);
        return _policy->is_visible(_type);
    }

    auto type::begin_interfaces() const -> interface_iterator
    {
        core::assert_initialized(*this);

        if (is_type_spec() && get_type_spec_signature().is_by_ref())
        {
            return interface_iterator();
        }

        auto const& table(detail::loader_context::from(*this).compute_interface_table(_type.type()));
        if (!table.is_initialized())
            return interface_iterator();

        return interface_iterator(*this, begin(table), end(table), metadata::binding_flags());
    }

    auto type::end_interfaces() const -> interface_iterator
    {
        core::assert_initialized(*this);
        return interface_iterator();
    }

    auto type::find_interface(core::string_reference const name) const -> type
    {
        core::assert_initialized(*this);

        auto const it(std::find_if(begin_interfaces(), end_interfaces(), [&](type const& i)
        {
            return i.full_name().c_str() == name;
        }));

        return it != end_interfaces() ? *it : type();
    }

    auto type::begin_constructors(metadata::binding_flags flags) const -> method_iterator
    {
        core::assert_initialized(*this);

        flags.set(metadata::binding_attribute::internal_use_only_constructor);
        flags.set(metadata::binding_attribute::declared_only);
        flags.unset(metadata::binding_attribute::flatten_hierarchy);

        auto const& table(detail::loader_context::from(*this).compute_method_table(_type.type()));
        if (!table.is_initialized())
            return method_iterator();

        return method_iterator(*this, begin(table), end(table), flags);
    }

    auto type::end_constructors() const -> method_iterator
    {
        core::assert_initialized(*this);
        return method_iterator();
    }

    auto type::begin_fields(metadata::binding_flags const flags) const -> field_iterator
    {
        core::assert_initialized(*this);

        if (is_by_ref())
        {
            return field_iterator();
        }

        auto const& table(detail::loader_context::from(*this).compute_field_table(_type.type()));
        if (!table.is_initialized())
            return field_iterator();

        return field_iterator(*this, begin(table), end(table), flags);
    }

    auto type::end_fields() const -> field_iterator
    {
        core::assert_initialized(*this);
        return field_iterator();
    }

    auto type::begin_methods(metadata::binding_flags const flags) const -> method_iterator
    {
        core::assert_initialized(*this);

        if (is_by_ref())
        {
            return method_iterator();
        }

        auto const& table(detail::loader_context::from(*this).compute_method_table(_type.type()));
        if (!table.is_initialized())
            return method_iterator();

        return method_iterator(*this, begin(table), end(table), flags);
    }

    auto type::end_methods() const -> method_iterator
    {
        core::assert_initialized(*this);
        return method_iterator();
    }

    auto type::find_method(core::string_reference name, metadata::binding_flags) const -> method
    {
        core::assert_initialized(*this);
        throw core::logic_error(L"not yet implemented");
    }

    auto type::begin_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        if (is_by_ref() || is_pointer())
        {
            return end_custom_attributes();
        }

        // TODO Handle custom attributes for TypeRef and TypeSpec
        return resolve_type_def_and_call([](type const& t)
        {
            return custom_attribute::begin_for(t.defining_module(), t._type.type().as_token(), core::internal_key());
        });
    }

    auto type::end_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        // TODO Handle custom attributes for TypeRef and TypeSpec
        return resolve_type_def_and_call([](type const& t)
        {
            return custom_attribute::end_for(t.defining_module(), t._type.type().as_token(), core::internal_key());
        });
    }

    auto type::is_initialized() const -> bool
    {
        return _type.is_initialized();
    }

    auto type::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(type const& lhs, type const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        core::assert_true([&]{ return &detail::loader_context::from(lhs) == &detail::loader_context::from(rhs); });

        if (&lhs._type.module().context() != &rhs._type.module().context())
            return false;

        if (lhs.is_type_def() != rhs.is_type_def())
            return false;

        if (lhs.is_type_def())
        {
            return lhs._type.type().as_token() == rhs._type.type().as_token();
        }
        else
        {
            return metadata::signature_comparer(&detail::loader_context::from(lhs))(
                lhs.get_type_spec_signature(),
                rhs.get_type_spec_signature());
        }
    }

    auto operator<(type const& lhs, type const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        if (std::less<detail::module_context const*>()(&lhs._type.module().context(), &rhs._type.module().context()))
            return true;

        if (std::less<detail::module_context const*>()(&rhs._type.module().context(), &lhs._type.module().context()))
            return false;

        // Arbitrarily order all TypeDefs before TypeSpecs
        if (lhs.is_type_def() != rhs.is_type_def())
            return lhs.is_type_def();

        if (lhs.is_type_def())
        {
            return lhs._type.type().as_token() < rhs._type.type().as_token();
        }
        else
        {
            // TODO Handle correct type signature ordering
            return std::less<core::const_byte_iterator>()(
                begin(lhs._type.type().as_blob()),
                end(rhs._type.type().as_blob()));
        }
    }

    auto type::self_reference(core::internal_key) const -> metadata::type_def_or_signature
    {
        core::assert_initialized(*this);
        return _type.type();
    }

    auto type::is_type_def() const -> bool
    {
        core::assert_initialized(*this);
        return _type.type().is_token();
    }

    auto type::is_type_spec() const -> bool
    {
        core::assert_initialized(*this);
        return _type.type().is_blob();
    }

    auto type::get_type_def_row() const -> metadata::type_def_row
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return is_type_def(); });

        return row_from(_type.type().as_token());
    }

    auto type::get_type_spec_signature() const -> metadata::type_signature
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return is_type_spec(); });

        return _type.type().as_blob().as<metadata::type_signature>();
    }

    auto type::resolve_type_def(type const& t) -> type
    {
        core::assert_initialized(t);

        detail::loader_context const& root(detail::loader_context::from(t));

        if (!t.is_initialized() || t.is_type_def())
            return t;

        auto const& signature(t.get_type_spec_signature());

        metadata::type_def_ref_spec_or_signature const next_type([&]() -> metadata::type_def_ref_spec_or_signature
        {
            switch (signature.get_kind())
            {
            case metadata::type_signature::kind::general_array:
                return metadata::blob(signature.array_type());

            case metadata::type_signature::kind::class_type:
                return signature.class_type();

            case metadata::type_signature::kind::function_pointer:
                throw core::logic_error(L"not yet implemented");

            case metadata::type_signature::kind::generic_instance:
                return signature.generic_type();

            case metadata::type_signature::kind::primitive:
                return root.resolve_fundamental_type(signature.primitive_type());

            case metadata::type_signature::kind::pointer:
                return metadata::blob(signature.pointer_type());

            case metadata::type_signature::kind::simple_array:
                return metadata::blob(signature.array_type());

            case metadata::type_signature::kind::variable:
                // A Class or Method Variable is never itself a TypeDef:
                return metadata::type_def_ref_spec_or_signature();

            default:
                throw core::runtime_error(L"unknown signature type");
            }
        }());

        if (!next_type.is_initialized())
            return type();

        // Recursively resolve the next type. Note that 'type' and 'next_type' will always be in the
        // same assembly because we haven't yet resolved the 'next_type' into another assembly:
        return resolve_type_def(type(t.defining_module(), next_type, core::internal_key()));
    }





    debug_type::debug_type(type const& t)
    {
        if (!t.is_initialized())
        {
            _is_initialized = false;
            return;
        }

        _is_initialized          = true;
        _is_type_def             = t.is_type_def();
        _metadata_token          = t._type.type().is_token() ? t._type.type().as_token().value() : 0;
        _attributes              = t.attributes().integer();
        _simple_name             = t.simple_name();
        _namespace_name          = t.namespace_name().c_str();
        _full_name               = t.full_name();
        _assembly_qualified_name = t.assembly_qualified_name();
        _base_type_aqn           = t.base_type().is_initialized() ? t.base_type().assembly_qualified_name() : core::string();
    }

} }

// AMDG //
