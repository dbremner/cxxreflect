
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    module_context::module_context(assembly_context const* defining_assembly, module_location const& location)
        : _assembly(defining_assembly), _location(location), _database(create_database(location))
    {
        core::assert_not_null(defining_assembly);
        core::assert_initialized(_location);
        core::assert_initialized(_database);

        _assembly.get()->loader().register_module(this);
    }

    auto module_context::assembly() const -> assembly_context const&
    {
        return *_assembly.get();
    }

    auto module_context::location() const -> module_location const&
    {
        return _location;
    }

    auto module_context::database() const -> metadata::database const&
    {
        return _database;
    }

    auto module_context::find_type_def(core::string_reference const namespace_name,
                                       core::string_reference const simple_name) const -> metadata::type_def_token
    {
        auto const it(std::find_if(_database.begin<metadata::table_id::type_def>(),
                                   _database.end<metadata::table_id::type_def>(),
                                   [&](metadata::type_def_row const& type_def)
        {
            return type_def.namespace_name() == namespace_name && type_def.name() == simple_name;
        }));

        return it == _database.end<metadata::table_id::type_def>()
            ? metadata::type_def_token()
            : it->token();
    }

    auto module_context::create_database(module_location const& location) -> metadata::database
    {
        core::assert_initialized(location);

        if (location.is_file())
        {
            return metadata::database::create_from_file(location.file_path().c_str(), this);
        }
        else if (!location.is_memory())
        {
            core::assert_fail(L"unreachable code");
        }

        return metadata::database(
            core::unique_byte_array(
                begin(location.memory_range()),
                end(location.memory_range()),
                nullptr),
            this);
    }





    assembly_context::assembly_context(loader_context const* loader, module_location const& location)
        : _loader(loader)
    {
        core::assert_not_null(loader);
        core::assert_initialized(location);

        _modules.push_back(core::make_unique<module_context>(this, location));

        if (_modules[0]->database().tables()[metadata::table_id::assembly].row_count() != 1)
            throw core::runtime_error(L"the module at the specified location is not an assembly");
    }

    auto assembly_context::loader() const -> loader_context const&
    {
        return *_loader.get();
    }

    auto assembly_context::manifest_module() const -> module_context const&
    {
        return *_modules.front();
    }

    auto assembly_context::modules() const -> module_context_sequence const&
    {
        realize_modules();
        return _modules;
    }

    auto assembly_context::name() const -> assembly_name const&
    {
        realize_name();
        return *_name;
    }

    auto assembly_context::find_type_def(core::string_reference const namespace_name,
                                         core::string_reference const simple_name) const -> metadata::type_def_token
    {
        // First search for the named type in the manifest module:
        metadata::type_def_token const td0(manifest_module().find_type_def(namespace_name, simple_name));
        if (td0.is_initialized())
            return td0;

        // Next, try the rest of the modules:
        metadata::type_def_token td1;
        auto const it(std::find_if(begin(modules()), end(modules()), [&](unique_module_context const& m) -> bool
        {
            td1 = m->find_type_def(namespace_name, simple_name);
            return td1.is_initialized();
        }));

        return td1;
    }

    auto assembly_context::realize_name() const -> void
    {
        if (_state.is_set(realization_state::name))
            return;

        metadata::assembly_token const token(&manifest_module().database(), metadata::table_id::assembly, 0);

        module_location const location(manifest_module().location());
        if (location.is_file())
            _name = core::make_unique<assembly_name>(token, location.file_path().c_str(), core::internal_key());
        else
            _name = core::make_unique<assembly_name>(token, core::internal_key());

        _state.set(realization_state::name);
    }

    auto assembly_context::realize_modules() const -> void
    {
        if (_state.is_set(realization_state::modules))
            return;

        metadata::database const& manifest_database(manifest_module().database());

        std::for_each(manifest_database.begin<metadata::table_id::file>(),
                      manifest_database.end<metadata::table_id::file>(),
                      [&](metadata::file_row const& file)
        {
            if (file.flags().is_set(metadata::file_attribute::contains_no_metadata))
                return;

            module_location const location(_loader.get()->locator().locate_module(
                name(),
                file.name().c_str()));

            if (!location.is_initialized())
                throw core::runtime_error(L"failed to locate module");

            _modules.push_back(core::make_unique<module_context>(this, location));
        });

        _state.set(realization_state::modules);
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
            core::make_unique<assembly_context>(this, location))));

        return *it1.first->second;
    }

    auto loader_context::get_or_load_assembly(assembly_name const& name) const -> assembly_context const&
    {
        return get_or_load_assembly(_locator.locate_assembly(name));
    }

    auto loader_context::resolve_type(metadata::type_def_ref_spec_token const type) const
        -> metadata::type_def_spec_token
    {
        core::assert_initialized(type);

        // First handle the easy case:  we only need to resolve TypeRef tokens, so if this is a
        // TypeDef or TypeSpec token, we can return it directly:
        if (type.is<metadata::type_def_spec_token>())
            return type.as<metadata::type_def_spec_token>();

        metadata::type_ref_token const tr(type.as<metadata::type_ref_token>());
        metadata::type_ref_row   const tr_row(row_from(tr));

        metadata::resolution_scope_token const tr_scope(tr_row.resolution_scope());

        // If the resolution scope is null, we need to look in the exported_type table for this type
        if (!tr_scope.is_initialized())
        {
            throw core::logic_error(L"not yet implemented");
        }

        // Otherwise, each resolution scope kind has different resolution logic:
        switch (tr_scope.table())
        {
        case metadata::table_id::module:
        {
            module_context const& tr_module(module_from_scope(tr.scope()));

            metadata::type_def_token const td(tr_module.find_type_def(tr_row.namespace_name(), tr_row.name()));
            if (!td.is_initialized())
                throw core::runtime_error(L"failed to resolve type in module");

            return td;
        }

        case metadata::table_id::module_ref:
        {
            throw core::logic_error(L"not yet implemented");
        }

        case metadata::table_id::assembly_ref:
        {
            assembly_name const tr_assembly_name(tr_scope.as<metadata::assembly_ref_token>(), core::internal_key());

            core::string_reference const tr_namespace_name(tr_row.namespace_name());
            core::string_reference const tr_simple_name(tr_row.name());

            core::string_reference const namespace_name(tr_namespace_name == L"System"
                ? system_namespace()
                : tr_namespace_name);

            core::string const tr_full_name(namespace_name.empty()
                ? core::string(tr_simple_name.c_str())
                : core::string(namespace_name.c_str()) + L"." + tr_simple_name.c_str());

            module_location const& tr_location(_locator.locate_assembly(tr_assembly_name, tr_full_name));
            if (!tr_location.is_initialized())
                throw core::runtime_error(L"failed to locate assembly for referenced type");

            assembly_context const& tr_assembly(get_or_load_assembly(tr_location));

            metadata::type_def_token const& tr_type(tr_assembly.find_type_def(namespace_name, tr_simple_name));
            if (!tr_type.is_initialized())
                throw core::runtime_error(L"failed to locate referenced type in assembly");

            return tr_type;
        }

        case metadata::table_id::type_ref:
        {
            throw core::logic_error(L"not yet implemented");
        }

        default:
        {
            // The resolution scope must be from one of the four tables in the switch; if we get
            // here, something is wrong in the database code:
            core::assert_fail(L"unreachable code");
            return core::default_value();
        }
        }

    }

    auto loader_context::resolve_fundamental_type(metadata::element_type const type) const -> metadata::type_def_token
    {
        core::assert_true([&]{ return type < metadata::element_type::concrete_element_type_max; });

        auto const lock(_sync.lock());

        if (_fundamental_types[core::as_integer(type)].is_initialized())
            return _fundamental_types[core::as_integer(type)];

        core::string_reference fundamental_type_name;
        switch (type)
        {
        case metadata::element_type::boolean:      fundamental_type_name = L"Boolean";        break;
        case metadata::element_type::character:    fundamental_type_name = L"Char";           break;
        case metadata::element_type::i1:           fundamental_type_name = L"SByte";          break;
        case metadata::element_type::u1:           fundamental_type_name = L"Byte";           break;
        case metadata::element_type::i2:           fundamental_type_name = L"Int16";          break;
        case metadata::element_type::u2:           fundamental_type_name = L"UInt16";         break;
        case metadata::element_type::i4:           fundamental_type_name = L"Int32";          break;
        case metadata::element_type::u4:           fundamental_type_name = L"UInt32";         break;
        case metadata::element_type::i8:           fundamental_type_name = L"Int64";          break;
        case metadata::element_type::u8:           fundamental_type_name = L"UInt64";         break;
        case metadata::element_type::r4:           fundamental_type_name = L"Single";         break;
        case metadata::element_type::r8:           fundamental_type_name = L"Double";         break;
        case metadata::element_type::i:            fundamental_type_name = L"IntPtr";         break;
        case metadata::element_type::u:            fundamental_type_name = L"UIntPtr";        break;
        case metadata::element_type::object:       fundamental_type_name = L"Object";         break;
        case metadata::element_type::string:       fundamental_type_name = L"String";         break;
        case metadata::element_type::array:        fundamental_type_name = L"Array";          break;
        case metadata::element_type::sz_array:     fundamental_type_name = L"Array";          break;
        case metadata::element_type::value_type:   fundamental_type_name = L"ValueType";      break;
        case metadata::element_type::void_type:    fundamental_type_name = L"Void";           break;
        case metadata::element_type::typed_by_ref: fundamental_type_name = L"TypedReference"; break;
        default:
            core::assert_fail(L"unknown primitive type");
            break;
        }

        metadata::type_def_token const token(system_module().find_type_def(system_namespace(), fundamental_type_name));
        if (!token.is_initialized())
            throw core::runtime_error(L"failed to find fundamental type in system assembly");

        _fundamental_types[core::as_integer(type)] = token;
        return token;
    }

    auto loader_context::locator() const -> module_locator const&
    {
        return _locator;
    }

    auto loader_context::module_from_scope(metadata::database const& scope) const -> module_context const&
    {
        auto const lock(_sync.lock());

        auto const it(_module_map.find(&scope));
        if (it == end(_module_map))
            throw core::runtime_error(L"scope is for a module not owned by this loader");

        return *it->second;
    }

    auto loader_context::register_module(module_context const* module) const -> void
    {
        auto const lock(_sync.lock());

        _module_map.insert(std::make_pair(&module->database(), module));
    }

    auto loader_context::system_module() const -> module_context const&
    {
        auto const lock(_sync.lock());

        // First see if we've already found the system module; if we have, use that:
        if (_system_module.get() != nullptr)
            return *_system_module.get();

        // Ok, we haven't loaded the system module yet.  Let's hunt for it...
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

        if (it0 != end(_assemblies))
        {
            _system_module.get() = &it0->second->manifest_module();
            return *_system_module.get();
        }

        // Ok, we haven't loaded the system assembly yet.  Pick an arbitrary type from a loaded
        // assembly and resolve the Object base type from it.  First we need to find an assembly
        // that defines types:
        auto const it1(std::find_if(begin(_assemblies), end(_assemblies), [&](assembly_map_entry const& a)
        {
            // Note that we need more than one row in the TypeDef table because the row at index 0
            // is the faux global entry:
            return (*a.second)
                .manifest_module()
                .database()
                .tables()[metadata::table_id::type_def]
                .row_count() > 1;
        }));

        // TODO We should fall back to enumerating and loading all referenced assemblies
        if (it1 == end(_assemblies))
            throw core::runtime_error(L"no loaded assemblies define types; cannot determine system assembly");

        // TODO FINISH
        throw core::logic_error(L"TODO TODO TODO");
    }

    auto loader_context::system_namespace() const -> core::string_reference
    {
        return _configuration.system_namespace();
    }

    auto loader_context::compute_event_table(metadata::type_def_or_signature const& type) const
        -> event_context_table
    {
        core::assert_initialized(type);
        return _events.get_or_create_table(type);
    }

    auto loader_context::compute_field_table(metadata::type_def_or_signature const& type) const
        -> field_context_table
    {
        core::assert_initialized(type);
        return _fields.get_or_create_table(type);
    }

    auto loader_context::compute_interface_table(metadata::type_def_or_signature const& type) const
        -> interface_context_table
    {
        core::assert_initialized(type);
        return _interfaces.get_or_create_table(type);
    }

    auto loader_context::compute_method_table(metadata::type_def_or_signature const& type) const
        -> method_context_table
    {
        core::assert_initialized(type);
        return _methods.get_or_create_table(type);
    }

    auto loader_context::compute_property_table(metadata::type_def_or_signature const& type) const
        -> property_context_table
    {
        core::assert_initialized(type);
        return _properties.get_or_create_table(type);
    }




    auto loader_context::from(assembly_context const& x) -> loader_context const&
    {
        return x.loader();
    }

    auto loader_context::from(module_context const& x) -> loader_context const&
    {
        return x.assembly().loader();
    }

    auto loader_context::from(assembly const& x) -> loader_context const&
    {
        return x.context(core::internal_key()).loader();
    }

    auto loader_context::from(module const& x) -> loader_context const&
    {
        return x.context(core::internal_key()).assembly().loader();
    }

    auto loader_context::from(type const& x) -> loader_context const&
    {
        return x.defining_module().context(core::internal_key()).assembly().loader();
    }

    auto loader_context::from(metadata::database const& x) -> loader_context const&
    {
        module_context const* const module(dynamic_cast<module_context const*>(&x.owner()));
        if (module == nullptr)
            core::logic_error(L"provided unowned database for discovery");

        return module->assembly().loader();
    }

} } }
