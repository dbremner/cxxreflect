
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/file.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection {

    auto assembly::begin_module_types(module const& m) -> inner_type_iterator
    {
        core::assert_initialized(m);

        return m.begin_types();
    }

    auto assembly::end_module_types(module const& m) -> inner_type_iterator
    {
        core::assert_initialized(m);

        return m.end_types();
    }

    assembly::assembly()
    {
    }

    assembly::assembly(detail::assembly_context const* context, core::internal_key)
        : _context(context)
    {
        core::assert_not_null(context);
    }

    auto assembly::name() const -> assembly_name const&
    {
        core::assert_initialized(*this);

        return _context.get()->name();
    }

    auto assembly::location() const -> core::string_reference
    {
        core::assert_initialized(*this);

        module_location const location(_context.get()->manifest_module().location());

        return location.is_file() ? location.file_path() : core::string_reference(L"");
    }

    auto assembly::referenced_assembly_count() const -> core::size_type
    {
        core::assert_initialized(*this);

        return _context.get()->manifest_module()
            .database()
            .tables()[metadata::table_id::assembly_ref]
            .row_count();
    }

    auto assembly::begin_referenced_assembly_names() const -> assembly_name_iterator
    {
        core::assert_initialized(*this);

        metadata::database const& scope(_context.get()->manifest_module().database());
        return assembly_name_iterator(
            *this,
            metadata::assembly_ref_token(
                &scope,
                metadata::table_id::assembly_ref,
                0));
    }

    auto assembly::end_referenced_assembly_names() const -> assembly_name_iterator
    {
        core::assert_initialized(*this);

        metadata::database const& scope(_context.get()->manifest_module().database());
        return assembly_name_iterator(
            *this,
            metadata::assembly_ref_token(
                &scope,
                metadata::table_id::assembly_ref,
                scope.tables()[metadata::table_id::assembly_ref].row_count()));
    }

    auto assembly::begin_files() const -> file_iterator
    {
        core::assert_initialized(*this);

        metadata::database const& scope(_context.get()->manifest_module().database());
        return file_iterator(
            *this,
            metadata::file_token(
                &scope,
                metadata::table_id::file,
                0));
    }

    auto assembly::end_files() const -> file_iterator
    {
        core::assert_initialized(*this);

        metadata::database const& scope(_context.get()->manifest_module().database());
        return file_iterator(
            *this,
            metadata::file_token(
                &scope,
                metadata::table_id::file,
                scope.tables()[metadata::table_id::file].row_count()));
    }

    auto assembly::find_file(core::string_reference const name) const -> file
    {
        core::assert_initialized(*this);

        auto const it(std::find_if(begin_files(), end_files(), [&](file const& f)
        {
            return f.name() == name;
        }));

        return it != end_files() ? *it : file();
    }

    auto assembly::begin_modules() const -> module_iterator
    {
        core::assert_initialized(*this);

        return module_iterator(*this, 0);
    }

    auto assembly::end_modules() const -> module_iterator
    {
        core::assert_initialized(*this);

        return module_iterator(*this, core::convert_integer(_context.get()->modules().size()));
    }

    auto assembly::find_module(core::string_reference const name) const -> module
    {
        core::assert_initialized(*this);

        auto const it(std::find_if(begin_modules(), end_modules(), [&](module const& m)
        {
            return m.name() == name;
        }));

        return it != end_modules() ? *it : module();
    }

    auto assembly::begin_types() const -> type_iterator
    {
        core::assert_initialized(*this);

        return type_iterator(begin_modules(), end_modules());
    }

    auto assembly::end_types() const -> type_iterator
    {
        core::assert_initialized(*this);

        return type_iterator(end_modules());
    }

    auto assembly::find_type(core::string_reference const full_name) const -> type
    {
        core::assert_initialized(*this);

        auto const it(std::find_if(begin_types(), end_types(), [&](type const& t)
        {
            return core::string_reference(t.full_name().c_str()) == full_name;
        }));

        return it != end_types() ? *it : type();
    }

    auto assembly::find_type(core::string_reference const namespace_name,
                             core::string_reference const simple_name) const -> type
    {
        core::assert_initialized(*this);
        
        auto const it(std::find_if(begin_types(), end_types(), [&](type const& t)
        {
            return t.namespace_name() == namespace_name
                && t.simple_name()    == simple_name;
        }));

        return it != end_types() ? *it : type();
    }

    auto assembly::is_initialized() const -> bool
    {
        return _context.get() != nullptr;
    }

    auto assembly::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(assembly const& lhs, assembly const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context.get() == rhs._context.get();
    }

    auto operator<(assembly const& lhs, assembly const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::less<detail::assembly_context const*>()(lhs._context.get(), rhs._context.get());
    }

    auto assembly::context(core::internal_key) const -> detail::assembly_context const&
    {
        core::assert_initialized(*this);

        return *_context.get();
    }
} }

// AMDG //
