
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy_utility.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"

namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_token(type_def_or_signature_with_module const& t) -> void
    {
        core::assert_true([&]{ return t.type().is_token(); });
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto definition_type_policy::attributes(type_def_or_signature_with_module const& t) const -> metadata::type_flags
    {
        assert_token(t);

        return row_from(t.type().as_token()).flags();
    }
    
    auto definition_type_policy::base_type(type_def_or_signature_with_module const& t) const -> type_def_or_signature_with_module
    {
        assert_token(t);

        metadata::type_def_ref_spec_token const extends(row_from(t.type().as_token()).extends());
        if (!extends.is_initialized())
            return type_def_or_signature_with_module();

        detail::loader_context const& root(detail::loader_context::from(t.module().context()));
        return resolve_type_for_construction(type_def_ref_spec_or_signature_with_module(
            &root.module_from_scope(extends.scope()),
            extends));
    }

    auto definition_type_policy::declaring_type(type_def_or_signature_with_module const& t) const -> type_def_or_signature_with_module
    {
        assert_token(t);

        if (is_nested(t))
        {
            metadata::database const& scope(t.type().scope());
            auto const it(std::lower_bound(
                scope.begin<metadata::table_id::nested_class>(),
                scope.end<metadata::table_id::nested_class>(),
                t.type().as_token(),
                [](metadata::nested_class_row const& row, metadata::type_def_token const& token)
            {
                return row.nested_class() < token;
            }));

            if (it == scope.end<metadata::table_id::nested_class>())
                throw core::metadata_error(L"type was identified as nested but had no associated nested class row");

            return type_def_or_signature_with_module(&t.module().context(), it->enclosing_class());
        }

        return type_def_or_signature_with_module();
    }

    auto definition_type_policy::is_abstract(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return row_from(t.type().as_token()).flags().is_set(metadata::type_attribute::abstract_);
    }

    auto definition_type_policy::is_array(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        // An array type is always represented via a TypeSpec
        return false;
    }
    
    auto definition_type_policy::is_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        // A by-ref type is always represented via a TypeSpec
        return false;
    }

    auto definition_type_policy::is_com_object(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return detail::is_derived_from_system_type(type_def_with_module(t.module(), t.type().as_token()), L"__ComObject", true);
    }
    
    auto definition_type_policy::is_contextful(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return detail::is_derived_from_system_type(type_def_with_module(t.module(), t.type().as_token()), L"ContextBoundObject", true);
    }

    auto definition_type_policy::is_enum(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return detail::is_derived_from_system_type(type_def_with_module(t.module(), t.type().as_token()), L"Enum", false);
    }

    auto definition_type_policy::is_generic_parameter(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        // A generic type parameter is always represented via a TypeSpec
        return false;
    }
   
    auto definition_type_policy::is_generic_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        // An instantiated generic type is represented by a TypeSpec, so a TypeDef is a generic type
        // if and only if it is a generic type definition.
        return is_generic_type_definition(t);
    }
    
    auto definition_type_policy::is_generic_type_definition(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        metadata::generic_param_row_iterator_pair const variables(metadata::find_generic_params_range(t.type().as_token()));
        return core::distance(variables.first, variables.second) > 0;
    }

    auto definition_type_policy::is_import(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return row_from(t.type().as_token()).flags().is_set(metadata::type_attribute::import);
    }

    auto definition_type_policy::is_interface(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return row_from(t.type().as_token()).flags().with_mask(metadata::type_attribute::class_semantics_mask)
            == metadata::type_attribute::interface_;
    }

    auto definition_type_policy::is_marshal_by_ref(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return detail::is_derived_from_system_type(type_def_with_module(t.module(), t.type().as_token()), L"MarshalByRefObject", true);
    }

    auto definition_type_policy::is_nested(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return row_from(t.type().as_token())
            .flags()
            .with_mask(metadata::type_attribute::visibility_mask) > metadata::type_attribute::public_;
    }

    auto definition_type_policy::is_pointer(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        // A pointer type is always represented via a TypeSpec
        return false;
    }

    auto definition_type_policy::is_primitive(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        detail::loader_context const& root(detail::loader_context::from(t.module().context()));

        if (&t.module().context() != &root.system_module())
            return false;

        if (row_from(t.type().as_token()).namespace_name() != root.system_namespace())
            return false;

        core::string_reference const& name(row_from(t.type().as_token()).name());
        if (name.size() < 4)
            return false;

        switch (name[0])
        {
        case L'B': return name == L"Boolean" || name == L"Byte";
        case L'C': return name == L"Char";
        case L'D': return name == L"Double";
        case L'I': return name == L"Int16" || name == L"Int32" || name == L"Int64" || name == L"IntPtr";
        case L'S': return name == L"SByte" || name == L"Single";
        case L'U': return name == L"UInt16" || name == L"UInt32" || name == L"UInt64" || name == L"UIntPtr";
        }

        return false;
    }

    auto definition_type_policy::is_sealed(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return row_from(t.type().as_token()).flags().is_set(metadata::type_attribute::sealed);
    }
    
    auto definition_type_policy::is_serializable(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return row_from(t.type().as_token()).flags().is_set(metadata::type_attribute::serializable)
            || is_enum(t)
            || detail::is_derived_from_system_type(type_def_with_module(t.module(), t.type().as_token()), L"MulticastDelegate", true);
    }
    
    auto definition_type_policy::is_special_name(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        return row_from(t.type().as_token()).flags().is_set(metadata::type_attribute::special_name);
    } 
    
    auto definition_type_policy::is_value_type(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        type_def_with_module const u(t.module(), t.type().as_token());

        return detail::is_derived_from_system_type(u, metadata::element_type::value_type, false)
            && !detail::is_system_type(u, L"Enum");
    }
    
    auto definition_type_policy::is_visible(type_def_or_signature_with_module const& t) const -> bool
    {
        assert_token(t);

        if (is_nested(t) && !is_visible(declaring_type(t)))
            return false;

        switch (row_from(t.type().as_token()).flags().with_mask(metadata::type_attribute::visibility_mask).enumerator())
        {
        case metadata::type_attribute::public_:
        case metadata::type_attribute::nested_public:
            return true;

        default:
            return false;
        }
    }

    auto definition_type_policy::layout(type_def_or_signature_with_module const& t) const -> type_attribute_layout
    {
        assert_token(t);

        switch (row_from(t.type().as_token()).flags().with_mask(metadata::type_attribute::layout_mask).enumerator())
        {
        case metadata::type_attribute::auto_layout:       return type_attribute_layout::auto_layout;
        case metadata::type_attribute::explicit_layout:   return type_attribute_layout::explicit_layout;
        case metadata::type_attribute::sequential_layout: return type_attribute_layout::sequential_layout;
        default:                                          return type_attribute_layout::unknown;
        }
    }

    auto definition_type_policy::metadata_token(type_def_or_signature_with_module const& t) const -> core::size_type
    {
        assert_token(t);
        return t.type().as_token().value();
    }

    
    auto definition_type_policy::namespace_name(type_def_or_signature_with_module const& t) const -> core::string_reference
    {
        assert_token(t);

        if (is_nested(t))
        {
            return namespace_name(type_def_or_signature_with_module(t.module(), declaring_type(t).type()));
        }

        return row_from(t.type().as_token()).namespace_name();
    }

    auto definition_type_policy::string_format(type_def_or_signature_with_module const& t) const -> type_attribute_string_format
    {
        assert_token(t);

        switch (row_from(t.type().as_token()).flags().with_mask(metadata::type_attribute::string_format_mask).enumerator())
        {
        case metadata::type_attribute::ansi_class:    return type_attribute_string_format::ansi_string_format;
        case metadata::type_attribute::auto_class:    return type_attribute_string_format::auto_string_format;
        case metadata::type_attribute::unicode_class: return type_attribute_string_format::unicode_string_format;
        default:                                      return type_attribute_string_format::unknown;
        }
    }
    
    auto definition_type_policy::visibility(type_def_or_signature_with_module const& t) const -> type_attribute_visibility
    {
        assert_token(t);

        switch (row_from(t.type().as_token()).flags().with_mask(metadata::type_attribute::visibility_mask).enumerator())
        {
        case metadata::type_attribute::not_public:                 return type_attribute_visibility::not_public;
        case metadata::type_attribute::public_:                    return type_attribute_visibility::public_;
        case metadata::type_attribute::nested_public:              return type_attribute_visibility::nested_public;
        case metadata::type_attribute::nested_private:             return type_attribute_visibility::nested_private;
        case metadata::type_attribute::nested_family:              return type_attribute_visibility::nested_family;
        case metadata::type_attribute::nested_assembly:            return type_attribute_visibility::nested_assembly;
        case metadata::type_attribute::nested_family_and_assembly: return type_attribute_visibility::nested_family_and_assembly;
        case metadata::type_attribute::nested_family_or_assembly:  return type_attribute_visibility::nested_family_or_assembly;
        default:                                                   return type_attribute_visibility::unknown;
        }
    }
    
} } }
