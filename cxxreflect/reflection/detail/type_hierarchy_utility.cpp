
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy_utility.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    auto is_system_assembly(assembly_context const& a) -> bool
    {
        return is_system_module(a.manifest_module());
    }

    auto is_system_assembly(assembly const& a) -> bool
    {
        core::assert_initialized(a);

        return is_system_assembly(a.context(core::internal_key()));
    }

    auto is_system_module(module_context const& m) -> bool
    {
        return m.database().tables()[metadata::table_id::assembly_ref].row_count() == 0;
    }

    auto is_system_module(module const& m) -> bool
    {
        core::assert_initialized(m);

        return is_system_module(m.context(core::internal_key()));
    }

    auto is_system_type(loader_context           const& root,
                        metadata::type_def_token const& t,
                        core::string_reference   const  simple_name) -> bool
    {
        core::assert_initialized(t);
        core::assert_true([&]{ return !simple_name.empty(); });

        metadata::type_def_token const system_type(root
            .system_module()
            .find_type_def(root.system_namespace(), simple_name));

        return t == system_type;
    }

    auto is_system_type(type_def_with_module const& t, core::string_reference const simple_name) -> bool
    {
        core::assert_initialized(t);

        return is_system_type(
            loader_context::from(t.module().context()),
            t.type(),
            simple_name);
    }

    auto is_system_type(type const& t, core::string_reference const simple_name) -> bool
    {
        core::assert_initialized(t);

        metadata::type_def_or_signature const ts(t.self_reference(core::internal_key()));
        if (ts.is_blob())
            return false;

        return is_system_type(
            loader_context::from(t),
            ts.as_token(),
            simple_name);
    }
    
    auto is_derived_from_system_type(type_def_with_module const& t, metadata::element_type const system_type, bool const include_self) -> bool
    {
        // TODO Rewrite to revert the usage of this so the higher level functions defer to the lower level
        return is_derived_from_system_type(type(t.module().realize(), t.type(), core::internal_key()), system_type, include_self);
    }

    auto is_derived_from_system_type(type_def_with_module const& t, core::string_reference const simple_name, bool const include_self) -> bool
    {
        // TODO Rewrite to revert the usage of this so the higher level functions defer to the lower level
        return is_derived_from_system_type(type(t.module().realize(), t.type(), core::internal_key()), simple_name, include_self);
    }

    auto is_derived_from_system_type(type                   const& t,
                                     metadata::element_type const  system_type,
                                     bool                   const  include_self) -> bool
    {
        core::assert_initialized(t);

        metadata::type_def_token const target_type(loader_context::from(t).resolve_fundamental_type(system_type));

        type current_type(t);
        if (!include_self && current_type)
            current_type = current_type.base_type();

        while (current_type)
        {
            if (current_type.self_reference(core::internal_key()) == target_type)
                return true;

            current_type = current_type.base_type();
        }

        return false;
    }

    auto is_derived_from_system_type(type                   const& t,
                                     core::string_reference const  simple_name,
                                     bool                   const  include_self) -> bool
    {
        core::assert_initialized(t);
        core::assert_true([&]{ return !simple_name.empty(); });

        type current_type(t);
        if (!include_self && current_type)
            current_type = current_type.base_type();

        while (current_type)
        {
            if (is_system_type(current_type, simple_name))
                return true;

            current_type = current_type.base_type();
        }

        return false;
    }

} } }
