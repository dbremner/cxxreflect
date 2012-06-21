
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/loader.hpp"

namespace cxxreflect { namespace reflection {

    loader::loader(loader&& other)
        : _context(std::move(other._context))
    {
        core::assert_initialized(*this);
    }

    auto loader::operator=(loader&& other) -> loader&
    {
        _context = std::move(other._context);
        core::assert_initialized(*this);
        return *this;
    }

    auto loader::load_assembly(core::string_reference const& path_or_uri) const -> assembly
    {
        core::assert_initialized(*this);

        return assembly(&_context.get()->get_or_load_assembly(module_location(path_or_uri)), core::internal_key());
    }

    auto loader::load_assembly(module_location const& location) const -> assembly
    {
        core::assert_initialized(*this);

        return assembly(&_context.get()->get_or_load_assembly(location), core::internal_key());
    }

    auto loader::load_assembly(assembly_name const& name) const -> assembly
    {
        core::assert_initialized(*this);

        return assembly(&_context.get()->get_or_load_assembly(name), core::internal_key());
    }

    auto loader::locator() const -> module_locator const&
    {
        core::assert_initialized(*this);

        return _context.get()->locator();
    }

    auto loader::is_initialized() const -> bool
    {
        return _context != nullptr;
    }

    auto loader::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto loader::context(core::internal_key) const -> detail::loader_context const&
    {
        core::assert_initialized(*this);

        return *_context.get();
    }

} }

// AMDG //
