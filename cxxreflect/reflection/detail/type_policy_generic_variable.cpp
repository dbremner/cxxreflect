
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_policy_generic_variable.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_generic_variable(type_policy::unresolved_type_context const& t) -> void
    {
        core::assert_true([&]() -> bool
        {
            metadata::type_signature const signature(t.as_blob().as<metadata::type_signature>());
            return signature.is_class_variable() || signature.is_method_variable();
        });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto generic_variable_type_policy::is_nested(unresolved_type_context const& t) const -> bool
    {
        assert_generic_variable(t);

        return true;
    }

    auto generic_variable_type_policy::namespace_name(unresolved_type_context const& t) const -> core::string_reference
    {
        assert_generic_variable(t);

        unresolved_type_context const declarer(declaring_type(t));
        if (!declarer.is_initialized())
            return core::string_reference::from_literal(L"");

        return type_policy::get_for(declarer).namespace_name(declarer);
    }

    auto generic_variable_type_policy::declaring_type(unresolved_type_context const& t) const -> unresolved_type_context
    {
        assert_generic_variable(t);

        metadata::type_signature           const signature(t.as_blob().as<metadata::type_signature>());
        metadata::type_or_method_def_token const variable_context(signature.variable_context());
        if (variable_context.is<metadata::type_def_token>())
        {
            return variable_context.as<metadata::type_def_token>();
        }
        else if (variable_context.is<metadata::method_def_token>())
        {
            metadata::method_def_token const method_context(variable_context.as<metadata::method_def_token>());
            return metadata::find_owner_of_method_def(method_context).token();
        }

        core::assert_unreachable();
    }





    auto generic_variable_type_policy::attributes(resolved_type_context const& t) const -> metadata::type_flags
    {
        assert_generic_variable(t);

        return metadata::type_attribute::public_;
    }

    auto generic_variable_type_policy::is_generic_parameter(resolved_type_context const& t) const -> bool
    {
        assert_generic_variable(t);

        return true;
    }

    auto generic_variable_type_policy::is_value_type(resolved_type_context const& t) const -> bool
    {
        assert_generic_variable(t);

        metadata::type_signature           const signature(t.as_blob().as<metadata::type_signature>());
        metadata::type_or_method_def_token const variable_context(signature.variable_context());
        core::size_type                    const variable_number(signature.variable_number());

        metadata::generic_param_row_range const range(metadata::find_generic_params(variable_context));
        if (range.size() < variable_number)
            throw core::runtime_error(L"generic parameter index out of range");

        metadata::generic_param_row const row(*std::next(range.begin(), variable_number));

        return row.flags()
            .with_mask(metadata::generic_parameter_attribute::special_constraint_mask)
            .is_set(metadata::generic_parameter_attribute::non_nullable_value_type_constraint);
    }

    auto generic_variable_type_policy::is_visible(resolved_type_context const& t) const -> bool
    {
        assert_generic_variable(t);

        return true;
    }

    auto generic_variable_type_policy::layout(resolved_type_context const& t) const -> type_layout
    {
        assert_generic_variable(t);

        return type_layout::auto_layout;
    }

    auto generic_variable_type_policy::string_format(resolved_type_context const& t) const -> type_string_format
    {
        assert_generic_variable(t);

        return type_string_format::ansi_string_format;
    }

    auto generic_variable_type_policy::visibility(resolved_type_context const& t) const -> type_visibility
    {
        assert_generic_variable(t);

        return type_visibility::public_;
    }

} } }
