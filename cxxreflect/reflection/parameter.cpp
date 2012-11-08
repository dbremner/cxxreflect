
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/parameter_data.hpp"
#include "cxxreflect/reflection/constant.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/method.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/parameter.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection {

    parameter::parameter()
    {
    }

    parameter::parameter(method                 const& declaring_method,
                         detail::parameter_data const& data,
                         core::internal_key)
        : _reflected_type(declaring_method.reflected_type().context(core::internal_key())),
          _method(&declaring_method.context(core::internal_key())),
          _parameter(data.token()),
          _signature(data.signature())
    {
        core::assert_initialized(declaring_method);
        core::assert_initialized(data.token());
        core::assert_initialized(data.signature());
    }

    parameter::parameter(method                   const& declaring_method,
                         metadata::param_token    const& token,
                         metadata::type_signature const& signature,
                         core::internal_key)
        : _reflected_type(declaring_method.reflected_type().context(core::internal_key())),
          _method(&declaring_method.context(core::internal_key())),
          _parameter(token),
          _signature(signature)
    {
        core::assert_initialized(declaring_method);
        core::assert_initialized(token);
        core::assert_initialized(signature);
    }

    auto parameter::attributes() const -> metadata::parameter_flags
    {
        core::assert_initialized(*this);

        return row().flags();
    }

    auto parameter::is_in() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::parameter_attribute::in);
    }

    auto parameter::is_lcid() const -> bool
    {
        core::assert_initialized(*this);

        core::assert_not_yet_implemented();
    }

    auto parameter::is_optional() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::parameter_attribute::optional);
    }

    auto parameter::is_out() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::parameter_attribute::out);
    }

    auto parameter::is_ret_val() const -> bool
    {
        core::assert_initialized(*this);

        core::assert_not_yet_implemented();
    }

    auto parameter::declaring_method() const -> method
    {
        core::assert_initialized(*this);

        return method(type(_reflected_type, core::internal_key()), _method.get(), core::internal_key());
    }

    auto parameter::metadata_token() const -> core::size_type
    {
        core::assert_initialized(*this);

        return row().token().value();
    }

    auto parameter::name() const -> core::string_reference
    {
        core::assert_initialized(*this);

        return row().name();
    }

    auto parameter::parameter_type() const -> type
    {
        core::assert_initialized(*this);

        return type(metadata::blob(_signature), core::internal_key());
    }

    auto parameter::position() const -> core::size_type
    {
        core::assert_initialized(*this);

        // The sequence is one-based, but we want to return a zero-based position:
        return row().sequence() - 1;
    }

    auto parameter::default_value() const -> constant
    {
        core::assert_initialized(*this);

        return constant(metadata::find_constant(_parameter).token(), core::internal_key());
    }

    auto parameter::custom_attributes() const -> detail::custom_attribute_range
    {
        core::assert_initialized(*this);

        return custom_attribute::get_for(_parameter, core::internal_key());
    }

    auto parameter::is_initialized() const -> bool
    {
        return _parameter.is_initialized() && _signature.is_initialized();
    }

    auto parameter::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(parameter const& lhs, parameter const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._parameter == rhs._parameter;
    }

    auto operator<(parameter const& lhs, parameter const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._parameter < rhs._parameter;
    }

    auto parameter::self_reference(core::internal_key) const -> metadata::param_token
    {
        core::assert_initialized(*this);

        return _parameter;
    }

    auto parameter::self_signature(core::internal_key) const -> metadata::type_signature
    {
        core::assert_initialized(*this);

        return _signature;
    }

    auto parameter::row() const -> metadata::param_row
    {
        core::assert_initialized(*this);

        return row_from(_parameter);
    }

} }
