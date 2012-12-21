
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/assembly_context.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy.hpp"
#include "cxxreflect/reflection/detail/type_resolution.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/assembly_name.hpp"
#include "cxxreflect/reflection/loader.hpp"
#include "cxxreflect/reflection/type.hpp"





namespace cxxreflect { namespace reflection { namespace detail { namespace {

    /// Tests whether an assembly reference is to a Windows Runtime namespace
    ///
    /// Windows Runtime assembly references are not really assembly references; they are simply
    /// placeholders that indicate that any entities that depend on the assembly reference must
    /// be resolved by namespace.
    auto is_windows_runtime_assembly_ref(metadata::assembly_ref_token const ref) -> bool
    {
        core::assert_initialized(ref);

        return row_from(ref).flags().with_mask(metadata::assembly_attribute::content_type_mask)
            == metadata::assembly_attribute::windows_runtime_content_type;
    }

} } } }





namespace cxxreflect { namespace reflection { namespace detail {

    loader_context::loader_context(module_locator locator, loader_configuration configuration)
        : _locator(std::move(locator)), _configuration(std::move(configuration))
    {
    }

    auto loader_context::get_or_load_assembly(module_location const& location) const -> assembly_context const&
    {
        core::assert_initialized(location);

        // We need to canonicalize the location so that we don't load an assembly multiple times.
        // We use the canonical URI for file-based assemblies, and we use a fake URI containing the
        // base address of an assembly for in-memory assemblies.  Note that this canonicalization is
        // a best-effort.
        core::string const canonical_uri(location.is_file()
            ? core::externals::compute_canonical_uri(location.file_path().c_str())
            : L"memory://" + core::to_string(begin(location.memory_range())));

        auto const lock(_sync.lock());

        // First see if we've already loaded the assembly; if we have, return it:
        auto const it0(_assemblies.find(canonical_uri));
        if (it0 != end(_assemblies))
            return *it0->second;

        // Otherwise, load the assembly and insert it into the loaded assemblies collection:
        auto const it1(_assemblies.insert(std::make_pair(
            canonical_uri,
            core::make_unique_with_delete<unique_assembly_context_delete, assembly_context>(this, location))));

        assembly_context const& assembly(*it1.first->second);

        // Test whether this is the system assembly.  If it is, initialize the system module.  Only
        // one system assembly may be loaded; an attempt to load a second will fail.  This is to
        // ensure identity of the System.Object type and the other System types.
        if (is_system_assembly(assembly))
        {
            if (_system_module.is_initialized())
            {
                // UNDO!!! UNDO!!!
                _assemblies.erase(it1.first);
                throw core::runtime_error(L"attempted to load two system modules");
            }

            _system_module.get() = &assembly.manifest_module();
        }

        return *it1.first->second;
    }

    auto loader_context::get_or_load_assembly(assembly_name const& name) const -> assembly_context const&
    {
        return get_or_load_assembly(_locator.locate_assembly(name));
    }

    auto loader_context::resolve_member(metadata::member_ref_token const member) const -> metadata::field_or_method_def_token
    {
        return resolve_member_ref(member);
    }

    auto loader_context::resolve_type(metadata::type_def_ref_spec_token const type) const -> metadata::type_def_spec_token
    {
        core::assert_initialized(type);

        // If the type is a type def or type spec, we can return it directly:
        if (type.is<metadata::type_def_spec_token>())
            return type.as<metadata::type_def_spec_token>();

        // Otherwise, it is a type ref, which we must resolve:
        return resolve_type_ref(type.as<metadata::type_ref_token>());
    }

    auto loader_context::resolve_fundamental_type(metadata::element_type const type) const -> metadata::type_def_token
    {
        core::assert_true([&]{ return type < metadata::element_type::concrete_element_type_max; });

        core::recursive_mutex_lock const lock(_sync.lock());

        if (_fundamental_types[core::as_integer(type)].is_initialized())
            return _fundamental_types[core::as_integer(type)];

        core::string_reference const fundamental_type_name([&]() -> core::string_reference
        {
            switch (type)
            {
            case metadata::element_type::boolean:      return L"Boolean";        break;
            case metadata::element_type::character:    return L"Char";           break;
            case metadata::element_type::i1:           return L"SByte";          break;
            case metadata::element_type::u1:           return L"Byte";           break;
            case metadata::element_type::i2:           return L"Int16";          break;
            case metadata::element_type::u2:           return L"UInt16";         break;
            case metadata::element_type::i4:           return L"Int32";          break;
            case metadata::element_type::u4:           return L"UInt32";         break;
            case metadata::element_type::i8:           return L"Int64";          break;
            case metadata::element_type::u8:           return L"UInt64";         break;
            case metadata::element_type::r4:           return L"Single";         break;
            case metadata::element_type::r8:           return L"Double";         break;
            case metadata::element_type::i:            return L"IntPtr";         break;
            case metadata::element_type::u:            return L"UIntPtr";        break;
            case metadata::element_type::object:       return L"Object";         break;
            case metadata::element_type::string:       return L"String";         break;
            case metadata::element_type::array:        return L"Array";          break;
            case metadata::element_type::sz_array:     return L"Array";          break;
            case metadata::element_type::value_type:   return L"ValueType";      break;
            case metadata::element_type::void_type:    return L"Void";           break;
            case metadata::element_type::typed_by_ref: return L"TypedReference"; break;
            default: core::assert_fail(L"unknown primitive type");
            }
        }());

        metadata::type_def_token const token(system_module().type_def_index().find(system_namespace(), fundamental_type_name));
        if (!token.is_initialized())
            throw core::runtime_error(L"failed to find fundamental type in system assembly");

        _fundamental_types[core::as_integer(type)] = token;
        return token;
    }

    auto loader_context::resolve_assembly_ref(metadata::assembly_ref_token const ref) const -> metadata::database const&
    {
        core::assert_initialized(ref);

        // Windows Runtime assembly references must be resolved through resolve_namespace
        core::assert_true([&]{ return !is_windows_runtime_assembly_ref(ref); });

        module_assembly_ref_cache& resolution_cache(module_context::from(ref.scope()).assembly_ref_cache());

        // First check to see if we've already resolved the assembly reference:
        metadata::database const* const cached_result(resolution_cache.get(ref));
        if (cached_result != nullptr)
            return *cached_result;

        // Ok, we don't have a cached result; let's resolve the reference:
        module_location const& location(_locator.locate_assembly(assembly_name(nullptr, ref, core::internal_key())));
        if (!location.is_initialized())
            throw core::runtime_error(L"failed to locate referenced assembly");

        assembly_context   const& assembly(get_or_load_assembly(location));
        metadata::database const& scope(assembly.manifest_module().database());

        // Cache the result and return it:
        resolution_cache.set(ref, &scope);
        return scope;
    }

    auto loader_context::resolve_module_ref(metadata::module_ref_token const ref) const -> metadata::database const&
    {
        core::assert_initialized(ref);

        module_context    const& module(module_context::from(ref.scope()));
        assembly_context  const& assembly(module.assembly());
        module_module_ref_cache& resolution_cache(module.module_ref_cache());

        // First check to see if we've already resolved the module reference:
        metadata::database const* const cached_result(resolution_cache.get(ref));
        if (cached_result != nullptr)
            return *cached_result;

        core::string_reference const& target_name(row_from(ref).name());

        // Hunt for the referenced module:
        auto const it(core::find_if(assembly.modules(), [&](unique_module_context const& m) -> bool
        {
            metadata::database const& scope(m->database());
            return scope[metadata::module_token(&scope, metadata::table_id::module, 0)].name() == target_name;
        }));

        // O noez!
        if (it == end(assembly.modules()))
            throw core::runtime_error(L"failed to locate referenced module");

        resolution_cache.set(ref, &(**it).database());
        return (**it).database();
    }

    auto loader_context::resolve_member_ref(metadata::member_ref_token const ref) const -> metadata::field_or_method_def_token
    {
        core::assert_initialized(ref);

        module_context    const& module(module_context::from(ref.scope()));
        module_member_ref_cache& resolution_cache(module.member_ref_cache());

        // First check to see if we've already resolved the member reference:
        metadata::field_or_method_def_token const cached_result(resolution_cache.get(ref));
        if (cached_result.is_initialized())
            return cached_result;

        // Ok, we don't have a cached result; let's resolve the reference:
        metadata::member_ref_row const ref_row(row_from(ref));

        metadata::type_def_token const parent_type([&]() -> metadata::type_def_token
        {
            metadata::member_ref_parent_token const parent(ref_row.parent());
            switch (parent.table())
            {
            case metadata::table_id::type_ref:
                return resolve_type_ref(parent.as<metadata::type_ref_token>());

            case metadata::table_id::module_ref:
                // This class table is used only for global members
                core::assert_not_yet_implemented();

            case metadata::table_id::method_def:
                // This class table is only used for call site varargs usage
                core::assert_not_yet_implemented();

            case metadata::table_id::type_spec:
                return resolve_primary_type(compute_type(parent.as<metadata::type_spec_token>()));

            default:
                core::assert_unreachable();
            }
        }());

        metadata::blob const member_blob(ref_row.signature());
        if (member_blob.begin() == member_blob.end())
            throw core::metadata_error(L"invalid metadata:  signature is empty");

        bool const is_field_signature(metadata::signature_flags(*member_blob.begin())
            .with_mask(metadata::signature_attribute::calling_convention_mask) == metadata::signature_attribute::field);

        metadata::signature_comparer  const signatures_are_equal(this);

        metadata::type_ref_spec_token const unresolved_parent(ref_row.parent().as<metadata::type_ref_spec_token>());
        metadata::type_def_token      const resolved_parent  (resolve_primary_type(compute_type(unresolved_parent)));

        auto const membership(_membership.get_membership(resolved_parent));
        if (is_field_signature)
        {
            auto const fields(find_fields(resolved_parent));
            auto const field(core::find_if(fields, [&](metadata::field_row const& f) -> bool
            {
                if (f.name() != ref_row.name())
                    return false;

                metadata::field_signature const row_signature(f.signature().as<metadata::field_signature>());
                metadata::field_signature const ref_signature(ref_row.signature().as<metadata::field_signature>());

                return signatures_are_equal(row_signature, ref_signature);
            }));

            if (field == end(fields))
                throw core::metadata_error(L"referenced field does not exist");

            resolution_cache.set(ref, field->token());
            return field->token();
        }
        else // it's a method signature
        {
            auto const methods(find_method_defs(resolved_parent));
            auto const method(core::find_if(methods, [&](metadata::method_def_row const& m) -> bool
            {
                if (m.name() != ref_row.name())
                    return false;

                metadata::method_signature const row_signature(m.signature().as<metadata::method_signature>());
                metadata::method_signature const ref_signature(ref_row.signature().as<metadata::method_signature>());

                return signatures_are_equal(row_signature, ref_signature);
            }));

            if (method == end(methods))
                throw core::metadata_error(L"referenced method does not exist");

            resolution_cache.set(ref, method->token());
            return method->token();
        }
    }

    auto loader_context::resolve_type_ref(metadata::type_ref_token const ref) const -> metadata::type_def_token
    {
        core::assert_initialized(ref);

        module_context  const& module(module_context::from(ref.scope()));
        module_type_ref_cache& resolution_cache(module.type_ref_cache());

        // First check to see if we've already resolved the type reference:
        metadata::type_def_token const cached_result(resolution_cache.get(ref));
        if (cached_result.is_initialized())
            return cached_result;

        // Ok, we don't have a cached result; let's resolve the reference:
        metadata::type_ref_row const ref_row(row_from(ref));

        // Select the namespace to be used for resolution:
        core::string_reference const acutal_namespace(ref_row.namespace_name());
        core::string_reference const usable_namespace(ref_row.namespace_name() == core::string_reference::from_literal(L"System")
            ? system_namespace()
            : ref_row.namespace_name());

        metadata::resolution_scope_token const resolution_scope(ref_row.resolution_scope());

        // If the resolution scope is null, we need to look in the exported_type table for this type
        if (!resolution_scope.is_initialized())
            core::assert_not_yet_implemented();

        // Otherwise, we need to resolve the target scope; the logic is different for each kind of
        // resolution scope, so this is a bit of work...
        metadata::database const& target_scope([&]() -> metadata::database const&
        {
            switch (resolution_scope.table())
            {
            // If we have a module, then the type def is defined in the same scope as the type ref:
            case metadata::table_id::module:
            {
                return module.database();
            }

            // If we have a module ref, then the type def is in another module of this assembly.
            // (Actually, it could also be defined in this module, too, but that would be weird.
            // The resolution here does the right thing, regardless:
            case metadata::table_id::module_ref:
            {
                return resolve_module_ref(resolution_scope.as<metadata::module_ref_token>());
            }

            // If we have an assembly ref, then the type def is in another assembly, which we must
            // resolve.
            case metadata::table_id::assembly_ref:
            {
                metadata::assembly_ref_token const assembly_ref_scope(resolution_scope.as<metadata::assembly_ref_token>());
                return is_windows_runtime_assembly_ref(assembly_ref_scope)
                    ? resolve_namespace(usable_namespace)
                    : resolve_assembly_ref(assembly_ref_scope);
            }

            // If we have a type ref, this is a nested type.  We need to resolve the target type ref
            // to find the enclosing type.  The nested type will be defined in the same scope.
            case metadata::table_id::type_ref:
            {
                core::assert_not_yet_implemented();
            }

            // There are no other valid resolution scope tables:
            default:
            {
                core::assert_unreachable();
            }
            }
        }());

        module_context const& target_module(module_context::from(target_scope));

        // Find the target type in the module:
        metadata::type_def_token const result(target_module.type_def_index().find(usable_namespace, ref_row.name()));
        if (!result.is_initialized())
            throw core::runtime_error(L"failed to locate referenced type in scope");

        // Finally, cache the result and return it:
        resolution_cache.set(ref, result);
        return result;
    }

    auto loader_context::resolve_namespace(core::string_reference const namespace_name) const
        -> metadata::database const&
    {
        // First check to see if we've already resolved the namespace:
        {
            core::recursive_mutex_lock const lock(_sync.lock());
            auto const it(_namespaces.find(namespace_name.c_str()));
            if (it != end(_namespaces))
                return *it->second;
        }

        // Swap out "System" for the real system namespace (e.g. "Platform" for the Windows Runtime
        // C++/CX langauge projection):
        core::string_reference const real_namespace_name(namespace_name == L"System" ? system_namespace() : namespace_name);

        // The namespace hasn't been resolved, so let's resolve it:
        module_location const location(_locator.locate_namespace(real_namespace_name));
        if (!location.is_initialized())
            throw core::runtime_error(L"failed to locate metadata for namespace");

        assembly_context   const& assembly(get_or_load_assembly(location));
        metadata::database const& scope(assembly.manifest_module().database());

        // Finally, cache the result:
        {
            core::recursive_mutex_lock const lock(_sync.lock());
            _namespaces.insert(std::make_pair(namespace_name.c_str(), &scope));
        }

        return scope;
    }

    auto loader_context::locator() const -> module_locator const&
    {
        return _locator;
    }

    auto loader_context::system_module() const -> module_context const&
    {
        core::recursive_mutex_lock const lock(_sync.lock());

        // First see if we've already found the system module; if we have, use that:
        if (_system_module.get() != nullptr)
            return *_system_module.get();

        // Ok, we haven't identified the system module yet.  Let's hunt for it...
        if (_assemblies.empty())
            throw core::runtime_error(L"no assemblies have been loaded; cannot determine system assembly");

        // Check to see if the system assembly has already been loaded:
        auto const it0(std::find_if(begin(_assemblies), end(_assemblies), [&](assembly_map_entry const& a)
        {
            return (*a.second)
                .manifest_module()
                .database()
                .tables()[metadata::table_id::assembly_ref]
                .row_count() == 0;
        }));

        // Oh, the system assembly isn't loaded.  What's up with that?  User error:
        if (it0 == end(_assemblies))
            throw core::runtime_error(L"the system assembly has not been loaded");

        _system_module.get() = &it0->second->manifest_module();
        return *_system_module.get();
    }

    auto loader_context::system_namespace() const -> core::string_reference
    {
        return _configuration.system_namespace();
    }

    auto loader_context::is_filtered_type(metadata::type_def_token const& type) const -> bool
    {
        return _configuration.is_filtered_type(type);
    }

    auto loader_context::get_membership(metadata::type_def_or_signature const& type) const -> membership_handle
    {
        return _membership.get_membership(type);
    }

    auto loader_context::from(metadata::database const& scope) -> loader_context const&
    {
        return module_context::from(scope).assembly().loader();
    }

    CXXREFLECT_DEFINE_INCOMPLETE_DELETE(unique_loader_context_delete, loader_context)

} } }
