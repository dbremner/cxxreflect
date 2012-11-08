
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/assembly_context.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy.hpp"
#include "cxxreflect/reflection/detail/type_resolution.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    auto is_derived_from_system_type_internal(metadata::type_def_token const& source_type,
                                              metadata::type_def_token const& system_type,
                                              bool                     const  include_self) -> bool
    {
        if (!source_type.is_initialized() || !system_type.is_initialized())
            return false;

        metadata::type_def_token current(source_type);
        if (!include_self)
        {
            metadata::type_def_ref_spec_token const extends(row_from(current).extends());
            if (!extends.is_initialized())
                return false;

            metadata::type_def_or_signature const resolved_extends(resolve_type(extends));
            if (!resolved_extends.is_token())
                return false;

            current = resolved_extends.as_token();
        }

        // TODO Cycle detection
        while (current.is_initialized())
        {
            if (current == system_type)
                return true;

            metadata::type_def_ref_spec_token const extends(row_from(current).extends());
            if (!extends.is_initialized())
                return false;

            metadata::type_def_or_signature const resolved_extends(resolve_type(extends));
            if (!resolved_extends.is_token())
                return false;

            current = resolved_extends.as_token();
        }

        return false;
    }

    auto resolve_system_type(metadata::database     const& source_scope,
                             metadata::element_type const  target_system_type) -> metadata::type_def_token
    {
        return loader_context::from(source_scope).resolve_fundamental_type(target_system_type);
    }

    auto resolve_system_type(metadata::database     const& source_scope,
                             core::string_reference const& target_simple_name) -> metadata::type_def_token
    {
        loader_context const& root(loader_context::from(source_scope));

        return root.system_module().type_def_index().find(root.system_namespace(), target_simple_name);
    }

} } } }

namespace cxxreflect { namespace reflection { namespace detail {

    auto is_system_assembly(assembly_context const& source_assembly) -> bool
    {
        return is_system_module(source_assembly.manifest_module());
    }

    auto is_system_module(module_context const& source_module) -> bool
    {
        return is_system_database(source_module.database());
    }

    auto is_system_database(metadata::database const& source_database) -> bool
    {
        return source_database.table<metadata::table_id::assembly_ref>().empty();
    }

    auto is_system_type(metadata::type_def_token const& source_type, metadata::element_type const target_system_type) -> bool
    {
        core::assert_initialized(source_type);

        if (!is_system_database(source_type.scope()))
            return false;

        return source_type == resolve_system_type(source_type.scope(), target_system_type);
    }

    auto is_system_type(metadata::type_def_token const& source_type, core::string_reference const& target_simple_name) -> bool
    {
        core::assert_initialized(source_type);
        core::assert_true([&]{ return !target_simple_name.empty(); });

        if (!is_system_database(source_type.scope()))
            return false;

        return source_type == resolve_system_type(source_type.scope(), target_simple_name);
    }

    auto is_derived_from_system_type(metadata::type_def_token const& source_type,
                                     metadata::element_type   const  target_system_type,
                                     bool                     const  include_self) -> bool
    {
        core::assert_initialized(source_type);

        return is_derived_from_system_type_internal(
            source_type,
            resolve_system_type(source_type.scope(), target_system_type),
            include_self);
    }

    auto is_derived_from_system_type(metadata::type_def_token const& source_type,
                                     core::string_reference   const& target_simple_name,
                                     bool                            include_self) -> bool
    {
        core::assert_initialized(source_type);
        core::assert_true([&]{ return !target_simple_name.empty(); });

        return is_derived_from_system_type_internal(
            source_type,
            resolve_system_type(source_type.scope(), target_simple_name),
            include_self);
    }

} } }
