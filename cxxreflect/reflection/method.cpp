
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/parameter_data.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/method.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/parameter.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection {

    method::method()
    {
    }

    method::method(type const& reflected_type, detail::method_table_entry const* context, core::internal_key)
        : _reflected_type(reflected_type.context(core::internal_key())),
          _context(context)
    {
        core::assert_initialized(reflected_type);
        core::assert_not_null(context);
    }

    auto method::declaring_type() const -> type
    {
        core::assert_initialized(*this);
        return type(metadata::find_owner_of_method_def(_context->member_token()).token(), core::internal_key());
    }

    auto method::reflected_type() const -> type
    {
        core::assert_initialized(*this);
        return type(_reflected_type, core::internal_key());
    }

    auto method::declaring_module() const -> module
    {
        core::assert_initialized(*this);
        return declaring_type().defining_module();
    }

    auto method::contains_generic_parameters() const -> bool
    {
        core::assert_not_yet_implemented();
    }

    auto method::attributes() const -> metadata::method_flags
    {
        core::assert_initialized(*this);
        return row().flags();
    }

    auto method::calling_convention() const -> metadata::calling_convention
    {
        core::assert_initialized(*this);
        metadata::signature_attribute const convention(_context
            ->member_signature()
            .calling_convention());
        return static_cast<metadata::calling_convention>(static_cast<core::size_type>(convention));
    }

    auto method::metadata_token() const -> core::size_type
    {
        core::assert_initialized(*this);

        return _context->member_token().value();
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

        auto const generic_parameters(metadata::find_generic_params(_context->member_token()));
        if (generic_parameters.empty())
            return false;

        if (_context->member_signature().generic_parameter_count() > 0)
            return true;

        return false;
    }

    auto method::is_generic_method_definition() const -> bool
    {
        core::assert_initialized(*this);

        auto const generic_parameters(metadata::find_generic_params(_context->member_token()));
        if (generic_parameters.empty())
            return false;

        if (_context->member_signature().generic_parameter_count() > 0)
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
        return _context.is_initialized();
    }

    auto method::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto method::custom_attributes() const -> detail::custom_attribute_range
    {
        core::assert_initialized(*this);

        return custom_attribute::get_for(_context->member_token(), core::internal_key());
    }

    auto method::parameters() const -> parameter_range
    {
        core::assert_initialized(*this);

        typedef metadata::token_with_arithmetic<metadata::param_token>::type incrementable_param_token;

        metadata::method_def_row  const md_row(row_from(_context->member_token()));
        incrementable_param_token first_parameter(md_row.first_parameter());
        incrementable_param_token const last_parameter(md_row.last_parameter());

        // If this method owns at least one param_row, we must test the first param_row's sequence
        // number.  A param row with a sequence number '0' is not a real parameter; it is used to
        // attach metadata to the return type.
        if (first_parameter != last_parameter && row_from(first_parameter).sequence() == 0)
            ++first_parameter;

        auto const signatures(_context->member_signature().parameters());

        return parameter_range(
            parameter_iterator(*this, detail::parameter_data(first_parameter, begin(signatures), core::internal_key())),
            parameter_iterator(*this, detail::parameter_data(last_parameter, end(signatures), core::internal_key())));
    }

    auto method::parameter_count() const -> core::size_type
    {
        core::assert_initialized(*this);

        return _context->member_signature().parameter_count();
    }

    auto method::return_parameter() const -> parameter
    {
        core::assert_initialized(*this);

        metadata::method_def_row const md_row(row_from(_context->member_token()));
        metadata::param_token const first_parameter(md_row.first_parameter());
        metadata::param_token const last_parameter(md_row.last_parameter());

        // This is the same check that we use in begin_parameters:  We only return a return
        // parameter if one exists in metadata.
        if (first_parameter == last_parameter || row_from(first_parameter).sequence() != 0)
            return parameter();

        return parameter(
            *this,
            first_parameter,
            _context->member_signature().return_type(),
            core::internal_key());
    }

    auto method::return_type() const -> type
    {
        core::assert_initialized(*this);

        return type(metadata::blob(_context->member_signature().return_type()), core::internal_key());
    }

    auto operator==(method const& lhs, method const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context == rhs._context;
    }

    auto operator<(method const& lhs, method const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context < rhs._context;
    }

    auto method::context(core::internal_key) const -> detail::method_table_entry const&
    {
        core::assert_initialized(*this);

        return *_context;
    }

    auto method::row() const -> metadata::method_def_row
    {
        core::assert_initialized(*this);

        return row_from(_context->member_token());
    }

} }
