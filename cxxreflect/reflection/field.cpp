
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/constant.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/field.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection {

    field::field()
    {
    }

    field::field(type const& reflected_type, detail::field_table_entry const* context, core::internal_key)
        : _reflected_type(reflected_type.context(core::internal_key())), _context(context)
    {
        core::assert_initialized(reflected_type);
        core::assert_not_null(context);
    }

    auto field::declaring_type() const -> type
    {
        core::assert_initialized(*this);

        if (_context->has_instantiating_type())
        {
            return type(_context->instantiating_type(), core::internal_key());
        }

        return type(metadata::find_owner_of_field(_context->member_token()).token(), core::internal_key());
    }

    auto field::reflected_type() const -> type
    {
        core::assert_initialized(*this);
        return type(_reflected_type, core::internal_key());
    }

    auto field::attributes() const -> metadata::field_flags
    {
        core::assert_initialized(*this);
        return row().flags();
    }

    auto field::field_type() const -> type
    {
        core::assert_initialized(*this);
        return type(metadata::blob(_context->member_signature().type()), core::internal_key());
    }

    auto field::declaring_module() const -> module
    {
        core::assert_initialized(*this);
        return declaring_type().defining_module();
    }

    auto field::is_assembly() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().with_mask(metadata::field_attribute::field_access_mask)
            == metadata::field_attribute::assembly;
    }

    auto field::is_family() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().with_mask(metadata::field_attribute::field_access_mask)
            == metadata::field_attribute::family;
    }

    auto field::is_family_and_assembly() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().with_mask(metadata::field_attribute::field_access_mask)
            == metadata::field_attribute::family_and_assembly;
    }

    auto field::is_family_or_assembly() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().with_mask(metadata::field_attribute::field_access_mask)
            == metadata::field_attribute::family_or_assembly;
    }

    auto field::is_init_only() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().is_set(metadata::field_attribute::init_only);
    }

    auto field::is_literal() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().is_set(metadata::field_attribute::literal);
    }

    auto field::is_not_serialized() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().is_set(metadata::field_attribute::not_serialized);
    }

    auto field::is_pinvoke_impl() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().is_set(metadata::field_attribute::pinvoke_impl);
    }

    auto field::is_private() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().with_mask(metadata::field_attribute::field_access_mask)
            == metadata::field_attribute::private_;
    }

    auto field::is_public() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().with_mask(metadata::field_attribute::field_access_mask)
            == metadata::field_attribute::public_;
    }

    auto field::is_special_name() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().is_set(metadata::field_attribute::special_name);
    }

    auto field::is_static() const -> bool
    {
        core::assert_initialized(*this);
        return row().flags().is_set(metadata::field_attribute::static_);
    }

    auto field::metadata_token() const -> core::size_type
    {
        core::assert_initialized(*this);
        return _context->member_token().value();
    }

    auto field::constant_value() const -> constant
    {
        core::assert_initialized(*this);

        return constant(metadata::find_constant(_context->member_token()).token(), core::internal_key());
    }

    auto field::name() const -> core::string_reference
    {
        core::assert_initialized(*this);

        return row().name();
    }

    auto field::custom_attributes() const -> detail::custom_attribute_range
    {
        core::assert_initialized(*this);
        return custom_attribute::get_for(_context->member_token(), core::internal_key());
    }

    auto field::is_initialized() const -> bool
    {
        return _context.is_initialized();
    }

    auto field::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(field const& lhs, field const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context->member_token() == rhs._context->member_token(); // TODO Revert to pointer compraisons
    }

    auto operator<(field const& lhs, field const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context->member_token() < rhs._context->member_token(); // TODO Revert to pointer comparisons
    }

    auto field::context(core::internal_key) const -> detail::field_table_entry const&
    {
        core::assert_initialized(*this);

        return *_context;
    }

    auto field::row() const -> metadata::field_row
    {
        core::assert_initialized(*this);

        return row_from(_context->member_token());
    }

} }
