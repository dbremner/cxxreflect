
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
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
        : _method(declaring_method), _parameter(data.token()), _signature(data.signature())
    {
        core::assert_initialized(declaring_method);
        core::assert_initialized(data.token());
        core::assert_initialized(data.signature());
    }

    parameter::parameter(method                   const& declaring_method,
                         metadata::param_token    const& token,
                         metadata::type_signature const& signature,
                         core::internal_key)
        : _method(declaring_method), _parameter(token), _signature(signature)
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

        throw core::logic_error(L"not yet implemented");
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

        throw core::logic_error(L"not yet implemented");
    }

    auto parameter::declaring_method() const -> method
    {
        core::assert_initialized(*this);

        return _method.realize();
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

        return type(
            _method.realize().declaring_module(),
            metadata::blob(_signature),
            core::internal_key());
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

    auto parameter::begin_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        return custom_attribute::begin_for(declaring_method().declaring_module(), _parameter, core::internal_key());
    }

    auto parameter::end_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        return custom_attribute::end_for(declaring_method().declaring_module(), _parameter, core::internal_key());
    }

    // TODO auto parameter::begin_optional_custom_modifiers() const -> optional_custom_modifier_iterator;
    // TODO auto parameter::end_optional_custom_modifiers()   const -> optional_custom_modifier_iterator;

    // TODO auto parameter::begin_required_custom_modifiers() const -> required_custom_modifier_iterator;
    // TODO auto parameter::end_required_custom_modifiers()   const -> required_custom_modifier_iterator;

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
