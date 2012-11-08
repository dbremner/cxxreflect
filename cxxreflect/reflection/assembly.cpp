
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/assembly_context.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/file.hpp"
#include "cxxreflect/reflection/loader.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"





namespace cxxreflect { namespace reflection {

    auto assembly::begin_module_types(module const& m) -> detail::module_type_iterator
    {
        return begin(m.types());
    }

    auto assembly::end_module_types(module const& m) -> detail::module_type_iterator
    {
        return end(m.types());
    }

    assembly::assembly()
    {
    }

    assembly::assembly(detail::assembly_context const* const context, core::internal_key)
        : _context(context)
    {
        core::assert_not_null(context);
    }

    auto assembly::owning_loader() const -> loader
    {
        core::assert_initialized(*this);
        return loader(&_context->loader(), core::internal_key());
    }

    auto assembly::name() const -> assembly_name const&
    {
        core::assert_initialized(*this);
        return _context->name();
    }

    auto assembly::location() const -> core::string_reference
    {
        core::assert_initialized(*this);
        if (!_context->manifest_module().location().is_file())
            return core::string_reference::from_literal(L"in-memory module");

        return _context->manifest_module().location().file_path();
    }

    auto assembly::referenced_assembly_names() const -> assembly_name_range
    {
        core::assert_initialized(*this);

        metadata::database const& scope(_context->manifest_module().database());
        core::size_type    const  row_count(scope.tables()[metadata::table_id::assembly_ref].row_count());

        return assembly_name_range(
            assembly_name_iterator(nullptr, metadata::assembly_ref_token(&scope, metadata::table_id::assembly_ref, 0)),
            assembly_name_iterator(nullptr, metadata::assembly_ref_token(&scope, metadata::table_id::assembly_ref, row_count)));
    }

    auto assembly::files() const -> file_range
    {
        metadata::database const& scope(_context->manifest_module().database());
        core::size_type    const  row_count(scope.tables()[metadata::table_id::assembly_ref].row_count());

        return file_range(
            file_iterator(*this, metadata::file_token(&scope, metadata::table_id::file, 0)),
            file_iterator(*this, metadata::file_token(&scope, metadata::table_id::file, row_count)));
    }

    auto assembly::modules() const -> module_range
    {
        core::assert_initialized(*this);

        return module_range(
            module_iterator(*this, 0),
            module_iterator(*this, core::convert_integer(_context->modules().size())));
    }

    auto assembly::types() const -> type_range
    {
        core::assert_initialized(*this);

        module_range const inner(modules());
        return type_range(type_iterator(begin(inner), end(inner)), type_iterator(end(inner)));
    }

    auto assembly::find_file(core::string_reference const& name) const -> file
    {
        core::assert_initialized(*this);

        auto const range(files());
        auto const it(core::find_if(range, [&](file const& f) { return f.name() == name; }));
        return it != end(range) ? *it : file();
    }

    auto assembly::find_module(core::string_reference const& name) const -> module
    {
        core::assert_initialized(*this);

        auto const range(modules());
        auto const it(core::find_if(range, [&](module const& m) { return m.name() == name; }));
        return it != end(range) ? *it : module();
    }

    auto assembly::find_type(core::string_reference const& namespace_name,
                             core::string_reference const& simple_name) const -> type
    {
        core::assert_initialized(*this);

        // First check the manifest module:
        metadata::type_def_token token;
        core::find_if(_context->modules(), [&](detail::unique_module_context const& module)
        {
            token = module->type_def_index().find(namespace_name, simple_name);
            return token.is_initialized();
        });

        return token.is_initialized() ? type(token, core::internal_key()) : type();
    }

    auto assembly::manifest_module() const -> module
    {
        core::assert_initialized(*this);
        return module(&_context->manifest_module(), core::internal_key());
    }

    auto assembly::context(core::internal_key) const -> detail::assembly_context const&
    {
        core::assert_initialized(*this);
        return *_context;
    }

    auto assembly::is_initialized() const -> bool
    {
        return _context.is_initialized();
    }

    auto assembly::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(assembly const& lhs, assembly const& rhs) -> bool
    {
        return lhs._context == rhs._context;
    }

    auto operator<(assembly const& lhs, assembly const& rhs) -> bool
    {
        return lhs._context < rhs._context;
    }

} }
