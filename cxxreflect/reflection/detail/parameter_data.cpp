
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/parameter_data.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    parameter_data::parameter_data()
    {
    }

    parameter_data::parameter_data(metadata::param_token                          const& token,
                                   metadata::method_signature::parameter_iterator const& signature,
                                   core::internal_key)
        : _token(token), _signature(signature)
    {
        core::assert_initialized(token);
    }

    auto parameter_data::token() const -> metadata::param_token const&
    {
        core::assert_initialized(*this);

        return _token;
    }

    auto parameter_data::signature() const -> metadata::type_signature const&
    {
        core::assert_initialized(*this);

        return *_signature;
    }

    auto parameter_data::is_initialized() const -> bool
    {
        return _token.is_initialized();
    }

    auto parameter_data::operator++() -> parameter_data&
    {
        core::assert_initialized(*this);

        _token = metadata::param_token(&_token.scope(), _token.value() + 1);
        ++_signature;
        return *this;
    }

    auto parameter_data::operator++(int) -> parameter_data
    {
        parameter_data const x(*this);
        ++*this;
        return x;
    }

    auto operator==(parameter_data const& lhs, parameter_data const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._token == rhs._token;
    }

    auto operator<(parameter_data const& lhs, parameter_data const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._token < rhs._token;
    }

} } }
