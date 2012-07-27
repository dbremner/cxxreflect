
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/module.hpp"

namespace cxxreflect { namespace reflection {

    module::module()
    {
    }

    module::module(detail::module_context const* const context, core::internal_key)
        : _context(context)
    {
        core::assert_not_null(context);
    }

    module::module(assembly const& defining_assembly, core::size_type const module_index, core::internal_key)
    {
        core::assert_initialized(defining_assembly);

        _context.get() = defining_assembly.context(core::internal_key()).modules().at(module_index).get();
        core::assert_initialized(_context);
    }

    auto module::defining_assembly() const -> assembly
    {
        core::assert_initialized(*this);

        return assembly(&_context->assembly(), core::internal_key());
    }

    auto module::metadata_token() const -> core::size_type
    {
        core::assert_initialized(*this);

        // We can cheat here:  every metadata database contains exactly one module row :-)
        return 0x00000001;
    }

    auto module::name() const -> core::string_reference
    {
        core::assert_initialized(*this);

        metadata::database const& scope(_context->database());

        return scope[metadata::module_token(&scope, metadata::table_id::module, 0)].name();
    }

    auto module::path() const -> core::string_reference
    {
        core::assert_initialized(*this);

        module_location const location(_context->location());

        return location.is_file() ? location.file_path() : core::string_reference(L"");
    }

    auto module::begin_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        throw core::logic_error(L"not yet implemented");
    }

    auto module::end_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        throw core::logic_error(L"not yet implemented");
    }

    auto module::begin_types() const -> type_iterator
    {
        core::assert_initialized(*this);

        // We intentionally skip the type at index 0; this isn't a real type, it's the internal
        // <module> "type" containing module-scope thingies.
        return type_iterator(
            *this,
            metadata::type_def_token(&_context->database(), metadata::table_id::type_def, 1));
    }

    auto module::end_types() const -> type_iterator
    {
        core::assert_initialized(*this);

        return type_iterator(
            *this,
            metadata::type_def_token(
                &_context->database(),
                metadata::table_id::type_def,
                _context->database().tables()[metadata::table_id::type_def].row_count()));
    }

    auto module::is_initialized() const -> bool
    {
        return _context.is_initialized();
    }

    auto module::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(module const& lhs, module const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context == rhs._context;
    }

    auto operator<(module const& lhs, module const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context < rhs._context;
    }

    auto module::context(core::internal_key) const -> detail::module_context const&
    {
        core::assert_initialized(*this);

        return *_context;
    }

} }
