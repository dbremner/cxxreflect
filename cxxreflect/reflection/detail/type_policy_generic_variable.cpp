
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_variable.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_generic_variable(type_def_or_signature_with_module const& t) -> void
    {
        core::assert_true([&]() -> bool
        {
            metadata::type_signature const signature(t.type().as_blob().as<metadata::type_signature>());
            return signature.is_class_variable() || signature.is_method_variable();
        });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto generic_variable_type_policy::attributes(type_def_or_signature_with_module const& t) const -> metadata::type_flags
    {
        assert_generic_variable(t);
        return 0x00000001; // The public flag, and just the public flag
    }
    
    auto generic_variable_type_policy::declaring_type(type_def_or_signature_with_module const& t) const
        -> type_def_or_signature_with_module
    {
        assert_generic_variable(t);

        metadata::type_signature           const signature(t.type().as_blob().as<metadata::type_signature>());
        metadata::type_or_method_def_token const variable_context(signature.variable_context());
        if (variable_context.is<metadata::type_def_token>())
        {
            metadata::type_def_token const type_context(variable_context.as<metadata::type_def_token>());
            return type_def_or_signature_with_module(t.module(), type_context);
        }
        else if (variable_context.is<metadata::method_def_token>())
        {
            metadata::method_def_token const method_context(variable_context.as<metadata::method_def_token>());
            metadata::type_def_token   const method_owner(metadata::find_owner_of_method_def(method_context).token());
            return type_def_or_signature_with_module(t.module(), method_owner);
        }
        else
        {
            core::assert_fail(L"unreachable code");
            return type_def_or_signature_with_module();
        }
    }

    auto generic_variable_type_policy::is_generic_parameter(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_generic_variable(t);
        return true;
    }
    
    auto generic_variable_type_policy::is_nested(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_generic_variable(t);
        return true;
    }

    auto generic_variable_type_policy::is_value_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_generic_variable(t);

        metadata::type_signature           const signature(t.type().as_blob().as<metadata::type_signature>());
        metadata::type_or_method_def_token const variable_context(signature.variable_context());
        core::size_type                    const variable_number(signature.variable_number());

        auto const range(metadata::find_generic_params_range(variable_context));
        if (core::distance(range.first, range.second) < variable_number)
            throw core::runtime_error(L"generic parameter index out of range");

        metadata::generic_param_row const row(*std::next(range.first, variable_number));

        return row.flags()
            .with_mask(metadata::generic_parameter_attribute::special_constraint_mask)
            .is_set(metadata::generic_parameter_attribute::non_nullable_value_type_constraint);
    }
    
    auto generic_variable_type_policy::is_visible(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_generic_variable(t);
        return true;
    }
    
    auto generic_variable_type_policy::layout(type_def_or_signature_with_module const& t) const -> type_attribute_layout
    {
        assert_generic_variable(t);
        return type_attribute_layout::auto_layout;
    }

    auto generic_variable_type_policy::namespace_name(type_def_or_signature_with_module const& t) const -> core::string_reference
    {
        assert_generic_variable(t);

        type_def_or_signature_with_module const declarer(declaring_type(t));
        if (!declarer.is_initialized())
            return core::string_reference::from_literal(L"");

        return type_policy::get_for(declarer)->namespace_name(declarer);

        // core::string_reference const name();
        // return name.empty() ? core::string_reference::from_literal(L"System") : name;
    }

    auto generic_variable_type_policy::string_format(type_def_or_signature_with_module const& t) const -> type_attribute_string_format
    {
        assert_generic_variable(t);
        return type_attribute_string_format::ansi_string_format;
    }
    
    auto generic_variable_type_policy::visibility(type_def_or_signature_with_module const& t) const -> type_attribute_visibility
    {
        assert_generic_variable(t);
        return type_attribute_visibility::public_;
    }
    
} } }
