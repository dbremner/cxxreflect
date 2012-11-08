
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/detail/type_policy_definition.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto assert_definition(type_policy::unresolved_type_context const& t) -> void
    {
        core::assert_true([&]{ return t.is_token() && t.as_token().is<metadata::type_def_token>(); });
    }

    auto definition_from(type_policy::unresolved_type_context const& t) -> metadata::type_def_token
    {
        return t.as_token().as<metadata::type_def_token>();
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto definition_type_policy::is_array(unresolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return false;
    }

    auto definition_type_policy::is_by_ref(unresolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return false;
    }

    auto definition_type_policy::is_generic_type_instantiation(unresolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return false;
    }

    auto definition_type_policy::is_nested(unresolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return row_from(definition_from(t)).flags().with_mask(metadata::type_attribute::visibility_mask)
            >  metadata::type_attribute::public_;
    }

    auto definition_type_policy::is_pointer(unresolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return false;
    }

    auto definition_type_policy::is_primitive(unresolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        if (!is_system_database(t.scope()))
            return false;

        metadata::type_def_row const row(row_from(definition_from(t)));
        if (row.namespace_name() != loader_context::from(t.scope()).system_namespace())
            return false;

        core::string_reference const& name(row.name());
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

    auto definition_type_policy::namespace_name(unresolved_type_context const& t) const -> core::string_reference
    {
        assert_definition(t);

        return row_from(definition_from(t)).namespace_name();
    }

    auto definition_type_policy::primary_name(unresolved_type_context const& t) const -> core::string_reference
    {
        assert_definition(t);

        return row_from(definition_from(t)).name();
    }


    auto definition_type_policy::declaring_type(unresolved_type_context const& t) const -> unresolved_type_context
    {
        assert_definition(t);

        if (!is_nested(t))
            return unresolved_type_context();

        metadata::database const& scope(t.scope());
        auto const it(std::lower_bound(
            scope.begin<metadata::table_id::nested_class>(),
            scope.end<metadata::table_id::nested_class>(),
            definition_from(t),
            [](metadata::nested_class_row const& row, metadata::type_def_token const& token)
        {
            return row.nested_class() < token;
        }));

        if (it == scope.end<metadata::table_id::nested_class>())
            throw core::metadata_error(L"type was identified as nested but had no associated nested class row");

        return it->enclosing_class();
    }





    auto definition_type_policy::attributes(resolved_type_context const& t) const -> metadata::type_flags
    {
        assert_definition(t);

        return row_from(definition_from(t)).flags();
    }

    auto definition_type_policy::base_type(resolved_type_context const& t) const -> unresolved_type_context
    {
        assert_definition(t);

        metadata::type_def_ref_spec_token const extends(row_from(definition_from(t)).extends());
        if (!extends.is_initialized())
            return unresolved_type_context();

        return compute_type(extends);
    }


    auto definition_type_policy::is_abstract(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return row_from(definition_from(t)).flags().is_set(metadata::type_attribute::abstract_);
    }

    auto definition_type_policy::is_com_object(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return is_derived_from_system_type(definition_from(t), L"__ComObject", true);
    }

    auto definition_type_policy::is_contextful(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return is_derived_from_system_type(definition_from(t), L"ContextBoundObject", true);
    }

    auto definition_type_policy::is_enum(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return is_derived_from_system_type(definition_from(t), L"Enum", false);
    }

    auto definition_type_policy::is_generic_parameter(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return false;
    }

    auto definition_type_policy::is_generic_type(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return is_generic_type_definition(t);
    }

    auto definition_type_policy::is_generic_type_definition(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return !metadata::find_generic_params(definition_from(t)).empty();
    }

    auto definition_type_policy::is_import(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return row_from(definition_from(t)).flags().is_set(metadata::type_attribute::import);
    }

    auto definition_type_policy::is_interface(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return row_from(definition_from(t)).flags().with_mask(metadata::type_attribute::class_semantics_mask)
            == metadata::type_attribute::interface_;
    }

    auto definition_type_policy::is_marshal_by_ref(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return is_derived_from_system_type(definition_from(t), L"MarshalByRefObject", true);
    }

    auto definition_type_policy::is_sealed(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return row_from(definition_from(t)).flags().is_set(metadata::type_attribute::sealed);
    }

    auto definition_type_policy::is_serializable(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return row_from(definition_from(t)).flags().is_set(metadata::type_attribute::serializable)
            || is_enum(t)
            || detail::is_derived_from_system_type(definition_from(t), L"MulticastDelegate", true);
    }

    auto definition_type_policy::is_special_name(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return row_from(definition_from(t)).flags().is_set(metadata::type_attribute::special_name);
    }

    auto definition_type_policy::is_value_type(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        return is_derived_from_system_type(definition_from(t), metadata::element_type::value_type, true)
            && !detail::is_system_type(definition_from(t), L"Enum");
    }

    auto definition_type_policy::is_visible(resolved_type_context const& t) const -> bool
    {
        assert_definition(t);

        if (is_nested(t) && !is_visible(declaring_type(t).as_token().as<metadata::type_def_token>()))
            return false;

        switch (row_from(definition_from(t)).flags().with_mask(metadata::type_attribute::visibility_mask).enumerator())
        {
        case metadata::type_attribute::public_:
        case metadata::type_attribute::nested_public:
            return true;

        default:
            return false;
        }
    }

    auto definition_type_policy::layout(resolved_type_context const& t) const -> type_layout
    {
        assert_definition(t);

        switch (row_from(definition_from(t)).flags().with_mask(metadata::type_attribute::layout_mask).enumerator())
        {
        case metadata::type_attribute::auto_layout:       return type_layout::auto_layout;
        case metadata::type_attribute::explicit_layout:   return type_layout::explicit_layout;
        case metadata::type_attribute::sequential_layout: return type_layout::sequential_layout;
        default:                                          return type_layout::unknown;
        }
    }

    auto definition_type_policy::metadata_token(resolved_type_context const& t) const -> core::size_type
    {
        assert_definition(t);

        return definition_from(t).value();
    }

    auto definition_type_policy::string_format(resolved_type_context const& t) const -> type_string_format
    {
        assert_definition(t);

        switch (row_from(definition_from(t)).flags().with_mask(metadata::type_attribute::string_format_mask).enumerator())
        {
        case metadata::type_attribute::ansi_class:    return type_string_format::ansi_string_format;
        case metadata::type_attribute::auto_class:    return type_string_format::auto_string_format;
        case metadata::type_attribute::unicode_class: return type_string_format::unicode_string_format;
        default:                                      return type_string_format::unknown;
        }
    }

    auto definition_type_policy::visibility(resolved_type_context const& t) const -> type_visibility
    {
        assert_definition(t);

        switch (row_from(definition_from(t)).flags().with_mask(metadata::type_attribute::visibility_mask).enumerator())
        {
        case metadata::type_attribute::not_public:                 return type_visibility::not_public;
        case metadata::type_attribute::public_:                    return type_visibility::public_;
        case metadata::type_attribute::nested_public:              return type_visibility::nested_public;
        case metadata::type_attribute::nested_private:             return type_visibility::nested_private;
        case metadata::type_attribute::nested_family:              return type_visibility::nested_family;
        case metadata::type_attribute::nested_assembly:            return type_visibility::nested_assembly;
        case metadata::type_attribute::nested_family_and_assembly: return type_visibility::nested_family_and_assembly;
        case metadata::type_attribute::nested_family_or_assembly:  return type_visibility::nested_family_or_assembly;
        default:                                                   return type_visibility::unknown;
        }
    }

} } }
