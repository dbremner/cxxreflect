
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/constant.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/field.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection {

    field::field()
    {
    }

    field::field(type const& reflected_type, detail::field_context const* const context, core::internal_key)
        : _reflected_type(reflected_type), _context(context)
    {
        core::assert_initialized(reflected_type);
        core::assert_not_null(context);
    }

    auto field::declaring_type() const -> type
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(_reflected_type.realize()));
        return type(root, metadata::find_owner_of_field(_context.get()->element()).token(), core::internal_key());
    }

    auto field::reflected_type() const -> type
    {
        core::assert_initialized(*this);

        return _reflected_type.realize();
    }

    auto field::attributes() const -> metadata::field_flags
    {
        core::assert_initialized(*this);

        return row().flags();
    }

    auto field::field_type() const -> type
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(_reflected_type.realize()));
        return type(
            declaring_module(),
            metadata::blob(_context.get()->element_signature(root)),
            core::internal_key());
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

        return _context.get()->element().value();
    }

    auto field::constant_value() const -> constant
    {
        core::assert_initialized(*this);

        return constant(metadata::find_constant(_context.get()->element()).token(), core::internal_key());
    }

    auto field::name() const -> core::string_reference
    {
        core::assert_initialized(*this);

        return row().name();
    }

    auto field::begin_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        return custom_attribute::begin_for(declaring_module(), _context.get()->element(), core::internal_key());
    }

    auto field::end_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        return custom_attribute::end_for(declaring_module(), _context.get()->element(), core::internal_key());
    }

    auto field::is_initialized() const -> bool
    {
        return _context.get() != nullptr;
    }

    auto field::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(field const& lhs, field const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::equal_to<detail::field_context const*>()(lhs._context.get(), rhs._context.get());
    }

    auto operator<(field const& lhs, field const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::less<detail::field_context const*>()(lhs._context.get(), rhs._context.get());
    }

    auto field::context(core::internal_key) const -> detail::field_context const&
    {
        core::assert_initialized(*this);

        return *_context.get();
    }

    auto field::row() const -> metadata::field_row
    {
        core::assert_initialized(*this);

        return _context.get()->element_row();
    }

} }

// AMDG //
