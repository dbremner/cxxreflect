
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/method.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/parameter.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection {

    method::method()
    {
    }

    method::method(type const& reflected_type, detail::method_context const* context, core::internal_key)
        : _reflected_type(reflected_type),
          _context(context)
    {
        core::assert_initialized(reflected_type);
        core::assert_not_null(context);
    }

    auto method::declaring_type() const -> type
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(reflected_type()));

        return type(
            root,
            metadata::find_owner_of_method_def(_context.get()->element()).token(),
            core::internal_key());
    }

    auto method::reflected_type() const -> type
    {
        core::assert_initialized(*this);

        return _reflected_type.realize();
    }

    auto method::declaring_module() const -> module
    {
        core::assert_initialized(*this);

        return declaring_type().defining_module();
    }

    auto method::contains_generic_parameters() const -> bool
    {
        core::assert_initialized(*this);

        // TODO contains_generic_parameters not yet implemented
        return false;
    }

    auto method::attributes() const -> metadata::method_flags
    {
        core::assert_initialized(*this);

        return row().flags();
    }

    auto method::calling_convention() const -> metadata::calling_convention
    {
        core::assert_initialized(*this);

        metadata::signature_attribute const convention(_context.get()
            ->element_signature(detail::loader_context::from(reflected_type()))
            .calling_convention());

        return static_cast<metadata::calling_convention>(static_cast<core::size_type>(convention));
    }

    auto method::metadata_token() const -> core::size_type
    {
        core::assert_initialized(*this);

        return _context.get()->element().value();
    }

    auto method::name() const -> core::string_reference
    {
        core::assert_initialized(*this);

        return row().name();
    }

    auto method::is_abstract() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::method_attribute::abstract);
    }

    auto method::is_assembly() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().with_mask(metadata::method_attribute::member_access_mask)
            == metadata::method_attribute::assembly;
    }

    auto method::is_constructor() const -> bool
    {
        core::assert_initialized(*this);

        if (!is_special_name())
            return false;

        core::string_reference const method_name(name());
        return method_name == L".ctor" || method_name == L".cctor";
    }

    auto method::is_family() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().with_mask(metadata::method_attribute::member_access_mask)
            == metadata::method_attribute::family;
    }

    auto method::is_family_and_assembly() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().with_mask(metadata::method_attribute::member_access_mask)
            == metadata::method_attribute::family_and_assembly;
    }

    auto method::is_family_or_assembly() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().with_mask(metadata::method_attribute::member_access_mask)
            == metadata::method_attribute::family_or_assembly;
    }

    auto method::is_final() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::method_attribute::final);
    }

    auto method::is_generic_method() const -> bool
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(reflected_type()));

        auto const generic_parameters(metadata::find_generic_params_range(_context.get()->element()));
        if (generic_parameters.first == generic_parameters.second)
            return false;

        if (_context.get()->element_signature(root).generic_parameter_count() > 0)
            return true;

        return false;
    }

    auto method::is_generic_method_definition() const -> bool
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(reflected_type()));

        auto const generic_parameters(metadata::find_generic_params_range(_context.get()->element()));
        if (generic_parameters.first == generic_parameters.second)
            return false;

        if (_context.get()->element_signature(root).generic_parameter_count() > 0)
            return true;

        return false;
    }

    auto method::is_hide_by_signature() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::method_attribute::hide_by_sig);
    }

    auto method::is_private() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().with_mask(metadata::method_attribute::member_access_mask)
            == metadata::method_attribute::private_;
    }

    auto method::is_public() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().with_mask(metadata::method_attribute::member_access_mask)
            == metadata::method_attribute::public_;
    }

    auto method::is_special_name() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::method_attribute::special_name);
    }

    auto method::is_static() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::method_attribute::static_);
    }

    auto method::is_virtual() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::method_attribute::virtual_);
    }

    auto method::is_initialized() const -> bool
    {
        return _context.get() != nullptr;
    }

    auto method::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto method::begin_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        return custom_attribute::begin_for(declaring_module(), _context.get()->element(), core::internal_key());
    }

    auto method::end_custom_attributes() const -> custom_attribute_iterator
    {
        core::assert_initialized(*this);

        return custom_attribute::end_for(declaring_module(), _context.get()->element(), core::internal_key());
    }

    auto method::begin_parameters() const -> parameter_iterator
    {
        core::assert_initialized(*this);

        typedef metadata::token_with_arithmetic<metadata::param_token>::type incrementable_param_token;

        detail::loader_context const& root(detail::loader_context::from(reflected_type()));

        metadata::method_def_row  const md_row(row_from(_context.get()->element()));
        incrementable_param_token first_parameter(md_row.first_parameter());
        incrementable_param_token const last_parameter(md_row.last_parameter());

        // If this method owns at least one param_row, we must test the first param_row's sequence
        // number.  A param row with a sequence number '0' is not a real parameter; it is used to
        // attach metadata to the return type.
        if (first_parameter != last_parameter && row_from(first_parameter).sequence() == 0)
            ++first_parameter;

        return parameter_iterator(*this, detail::parameter_data(
            first_parameter,
            _context.get()->element_signature(root).begin_parameters(),
            core::internal_key()));
    }

    auto method::end_parameters() const -> parameter_iterator
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(reflected_type()));

        return parameter_iterator(*this, detail::parameter_data(
            row_from(_context.get()->element()).last_parameter(),
            _context.get()->element_signature(root).end_parameters(),
            core::internal_key()));
    }

    auto method::parameter_count() const -> core::size_type
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(reflected_type()));
        return _context.get()->element_signature(root).parameter_count();
    }

    auto method::return_parameter() const -> parameter
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(reflected_type()));

        metadata::method_def_row const md_row(row_from(_context.get()->element()));
        metadata::param_token const first_parameter(md_row.first_parameter());
        metadata::param_token const last_parameter(md_row.last_parameter());

        // This is the same check that we use in begin_parameters:  We only return a return
        // parameter if one exists in metadata.
        if (first_parameter == last_parameter || row_from(first_parameter).sequence() != 0)
            return parameter();

        return parameter(
            *this,
            first_parameter,
            _context.get()->element_signature(root).return_type(),
            core::internal_key());
    }

    auto method::return_type() const -> type
    {
        core::assert_initialized(*this);

        detail::loader_context const& root(detail::loader_context::from(reflected_type()));

        return type(
            declaring_module(),
            metadata::blob(_context.get()->element_signature(root).return_type()),
            core::internal_key());
    }

    auto operator==(method const& lhs, method const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::equal_to<detail::method_context const*>()(lhs._context.get(), rhs._context.get());
    }

    auto operator<(method const& lhs, method const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::less<detail::method_context const*>()(lhs._context.get(), rhs._context.get());
    }

    auto method::context(core::internal_key) const -> detail::method_context const&
    {
        core::assert_initialized(*this);

        return *_context.get();
    }

    auto method::row() const -> metadata::method_def_row
    {
        core::assert_initialized(*this);

        return row_from(_context.get()->element());
    }

} }

// AMDG //
