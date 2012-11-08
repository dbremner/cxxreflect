
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/assembly_context.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/loader.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    auto module_type_iterator_constructor::operator()(std::nullptr_t, detail::module_type_def_index_iterator const it) const
        -> type
    {
        return type(*it, core::internal_key());
    }
    
} } }

namespace cxxreflect { namespace reflection {

    module::module()
    {
    }

    module::module(detail::module_context const* const context, core::internal_key)
        : _context(context)
    {
        core::assert_not_null(context);
    }

    module::module(assembly const& defining_assembly, core::size_type const index, core::internal_key)
        : _context(defining_assembly.context(core::internal_key()).modules().at(index).get())
    {
        core::assert_initialized(defining_assembly);
    }

    auto module::defining_assembly() const -> assembly
    {
        core::assert_initialized(*this);
        return assembly(&_context->assembly(), core::internal_key());
    }

    auto module::location() const -> module_location const&
    {
        core::assert_initialized(*this);
        return _context->location();
    }

    auto module::name() const -> core::string_reference
    {
        core::assert_initialized(*this);
        return row_from(metadata::module_token(&_context->database(), metadata::table_id::module, 0)).name();
    }

    auto module::types() const -> type_range
    {
        core::assert_initialized(*this);

        auto const& underlying_range(_context->type_def_index());
        return type_range(
            type_iterator(nullptr, underlying_range.begin()),
            type_iterator(nullptr, underlying_range.end()));
    }

    auto module::find_type(core::string_reference const& namespace_name,
                           core::string_reference const& simple_name) const -> type
    {
        core::assert_initialized(*this);
        metadata::type_def_token const t(_context->type_def_index().find(namespace_name, simple_name));
        if (!t.is_initialized())
            return type();

        return type(t, core::internal_key());
    }

    auto module::context(core::internal_key) const -> detail::module_context const&
    {
        core::assert_initialized(*this);
        return *_context;
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
        return lhs._context == rhs._context;
    }

    auto operator<(module const& lhs, module const& rhs) -> bool
    {
        return lhs._context < rhs._context;
    }

} }
