
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/loader.hpp"
#include "cxxreflect/reflection/loader_configuration.hpp"
#include "cxxreflect/reflection/type.hpp"





namespace cxxreflect { namespace reflection {

    loader::loader()
    {
    }

    loader::loader(detail::loader_context const* context, core::internal_key)
        : _context(context)
    {
        core::assert_not_null(context);
    }

    auto loader::load_assembly(core::string_reference const& path_or_uri) const -> assembly
    {
        core::assert_initialized(*this);
        return assembly(&_context->get_or_load_assembly(module_location(path_or_uri)), core::internal_key());
    }

    auto loader::load_assembly(module_location const& location) const -> assembly
    {
        core::assert_initialized(*this);
        return assembly(&_context->get_or_load_assembly(location), core::internal_key());
    }

    auto loader::load_assembly(assembly_name const& name) const -> assembly
    {
        core::assert_initialized(*this);
        return assembly(&_context->get_or_load_assembly(name), core::internal_key());
    }

    auto loader::locator() const -> module_locator const&
    {
        core::assert_initialized(*this);
        return _context->locator();
    }

    auto loader::context(core::internal_key) const -> detail::loader_context const&
    {
        core::assert_initialized(*this);
        return *_context;
    }

    auto loader::is_initialized() const -> bool
    {
        return _context.is_initialized();
    }

    auto loader::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(loader const& lhs, loader const& rhs) -> bool
    {
        return lhs._context == rhs._context;
    }

    auto operator<(loader const& lhs, loader const& rhs) -> bool
    {
        return lhs._context < rhs._context;
    }





    loader_root::loader_root(detail::unique_loader_context context, core::internal_key)
        : _context(std::move(context))
    {
        core::assert_initialized(*this);
    }

    loader_root::loader_root(loader_root&& other)
        : _context(std::move(other._context))
    {
        core::assert_initialized(*this);
    }

    auto loader_root::operator=(loader_root&& other) -> loader_root&
    {
        _context = std::move(other._context);
        core::assert_initialized(*this);
        return *this;
    }

    auto loader_root::get() const -> loader
    {
        core::assert_initialized(*this);
        return loader(_context.get(), core::internal_key());
    }

    auto loader_root::is_initialized() const -> bool
    {
        return _context != nullptr;
    }

    auto create_loader_root(module_locator locator, loader_configuration configuration) -> loader_root
    {
        return loader_root(
            core::make_unique<detail::loader_context>(std::move(locator), std::move(configuration)), core::internal_key());
    }

} }
