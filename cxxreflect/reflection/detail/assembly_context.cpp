
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/assembly_context.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/assembly_name.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    assembly_context::assembly_context(loader_context const* const loader, module_location const& manifest_module_location)
        : _loader(loader)
    {
        core::assert_not_null(loader);
        core::assert_initialized(manifest_module_location);

        _modules.emplace_back(core::make_unique_with_delete<unique_module_context_delete, module_context>(this, manifest_module_location));
    }

    auto assembly_context::loader() const -> loader_context const&
    {
        return *_loader;
    }

    auto assembly_context::manifest_module() const -> module_context const&
    {
        return *_modules.front();
    }

    auto assembly_context::modules() const -> module_context_storage_type const&
    {
        realize_modules();
        return _modules;
    }

    auto assembly_context::name() const -> assembly_name const&
    {
        realize_name();
        return *_name;
    }

    auto assembly_context::realize_name() const -> void
    {
        auto const lock(_sync.lock());

        if (_state.is_set(realization_state::name))
            return;

        metadata::assembly_token const token(&manifest_module().database(), metadata::table_id::assembly, 0);

        module_location const location(manifest_module().location());
        if (location.is_file())
            _name = core::make_unique<assembly_name>(token, location.file_path().c_str(), core::internal_key());
        else
            _name = core::make_unique<assembly_name>(nullptr, token, core::internal_key());

        _state.set(realization_state::name);
    }

    auto assembly_context::realize_modules() const -> void
    {
        auto const lock(_sync.lock());

        if (_state.is_set(realization_state::other_modules))
            return;

        metadata::database const& manifest_database(manifest_module().database());

        core::for_all(manifest_database.table<metadata::table_id::file>(), [&](metadata::file_row const& file)
        {
            if (file.flags().is_set(metadata::file_attribute::contains_no_metadata))
                return;

            module_location const location(_loader.get()->locator().locate_module(
                name(),
                file.name().c_str()));

            if (!location.is_initialized())
                throw core::runtime_error(L"failed to locate module");

            _modules.push_back(core::make_unique_with_delete<unique_module_context_delete, module_context>(this, location));
        });

        _state.set(realization_state::other_modules);
    }

    CXXREFLECT_DEFINE_INCOMPLETE_DELETE(unique_assembly_context_delete, assembly_context)

} } }
