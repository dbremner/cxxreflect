
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/constant.hpp"

namespace cxxreflect { namespace reflection { namespace {

    template <typename T>
    auto check_and_read_single_primitive(metadata::blob const& x) -> T
    {
        if (core::distance(begin(x), end(x)) != sizeof(T))
            throw core::runtime_error(L"attempted an invalid reinterpretation");

        return *reinterpret_cast<T const*>(begin(x));
    }

} } }

namespace cxxreflect { namespace reflection {

    constant::constant()
    {
    }

    constant::constant(metadata::constant_token const& element, core::internal_key)
        : _constant(element)
    {
        core::assert_initialized(element);
    }

    auto constant::get_kind() const -> kind
    {
        if (!is_initialized())
            return kind::unknown;

        switch (row().type())
        {
        case metadata::element_type::boolean:    return kind::boolean;
        case metadata::element_type::character:  return kind::character;
        case metadata::element_type::i1:         return kind::int8;
        case metadata::element_type::u1:         return kind::uint8;
        case metadata::element_type::i2:         return kind::int16;
        case metadata::element_type::u2:         return kind::uint16;
        case metadata::element_type::i4:         return kind::int32;
        case metadata::element_type::u4:         return kind::uint32;
        case metadata::element_type::i8:         return kind::int64;
        case metadata::element_type::u8:         return kind::uint64;
        case metadata::element_type::r4:         return kind::single_precision;
        case metadata::element_type::r8:         return kind::double_precision;
        case metadata::element_type::string:     return kind::string;
        case metadata::element_type::class_type: return kind::class_type;
        default:                                 return kind::unknown;
        }
    }

    auto constant::as_boolean() const -> bool
    {
        return check_and_read_single_primitive<std::uint8_t>(row().value()) != 0;
    }

    auto constant::as_character() const -> wchar_t
    {
        return check_and_read_single_primitive<wchar_t>(row().value());
    }

    auto constant::as_int8() const -> std::int8_t
    {
        return check_and_read_single_primitive<std::int8_t>(row().value());
    }

    auto constant::as_uint8() const -> std::uint8_t
    {
        return check_and_read_single_primitive<std::uint8_t>(row().value());
    }

    auto constant::as_int16() const -> std::int16_t
    {
        return check_and_read_single_primitive<std::int16_t>(row().value());
    }

    auto constant::as_uint16() const -> std::uint16_t
    {
        return check_and_read_single_primitive<std::uint16_t>(row().value());
    }

    auto constant::as_int32() const -> std::int32_t
    {
        return check_and_read_single_primitive<std::int32_t>(row().value());
    }

    auto constant::as_uint32() const -> std::uint32_t
    {
        return check_and_read_single_primitive<std::uint32_t>(row().value());
    }

    auto constant::as_int64() const -> std::int64_t
    {
        return check_and_read_single_primitive<std::int64_t>(row().value());
    }

    auto constant::as_uint64() const -> std::uint64_t
    {
        return check_and_read_single_primitive<std::uint64_t>(row().value());
    }

    auto constant::as_float() const -> float
    {
        return check_and_read_single_primitive<float>(row().value());
    }

    auto constant::as_double() const -> double
    {
        return check_and_read_single_primitive<double>(row().value());
    }
        
    auto constant::is_initialized() const -> bool
    {
        return _constant.is_initialized();
    }

    auto constant::row() const -> metadata::constant_row
    {
        core::assert_initialized(*this);
        return row_from(_constant);
    }

    auto constant::create_for(metadata::has_constant_token const& parent, core::internal_key) -> constant
    {
        core::assert_initialized(parent);

        metadata::constant_row const row(metadata::find_constant(parent));
        return row.is_initialized() ? constant(row.token(), core::internal_key()) : constant();
    }

    auto operator==(constant const& lhs, constant const& rhs) -> bool
    {
        return lhs._constant == rhs._constant;
    }

} }
