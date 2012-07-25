
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/loader_configuration.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    auto default_loader_configuration::system_namespace() const -> core::string_reference
    {
        return L"System";
    }

} } }

namespace cxxreflect { namespace reflection {

    loader_configuration::loader_configuration()
    {
    }

    loader_configuration::loader_configuration(loader_configuration const& other)
        : _x(other.is_initialized() ? other._x->copy() : nullptr)
    {
    }

    loader_configuration::loader_configuration(loader_configuration&& other)
        : _x(std::move(other._x))
    {
    }

    auto loader_configuration::operator=(loader_configuration const& other) -> loader_configuration&
    {
        _x = other.is_initialized() ? other._x->copy() : nullptr;
        return *this;
    }

    auto loader_configuration::operator=(loader_configuration&& other) -> loader_configuration&
    {
        _x = std::move(other._x);
        return *this;
    }

    auto loader_configuration::system_namespace() const -> core::string_reference
    {
        core::assert_initialized(*this);
        return _x->system_namespace();
    }

    auto loader_configuration::is_initialized() const -> bool
    {
        return _x != nullptr;
    }

} }
