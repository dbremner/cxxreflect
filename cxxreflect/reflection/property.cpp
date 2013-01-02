
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/member_iterator.hpp"
#include "cxxreflect/reflection/constant.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/method.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/property.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection { namespace {

    auto find_method_token(metadata::property_token const& property, metadata::method_semantics_attribute const desired_semantics)
        -> metadata::method_def_token
    {
       auto const range(metadata::find_method_semantics(property));
       auto const result(std::find_if(begin(range), end(range), [&](metadata::method_semantics_row const& s)
       {
           return s.semantics() == desired_semantics;
       }));

       return result != end(range) ? result->method() : metadata::method_def_token();
    }

    auto find_method(property const& property, metadata::method_semantics_attribute const desired_semantics) -> method
    {
        metadata::property_token   const property_token(property.context(core::internal_key()).member_token());
        metadata::method_def_token const method_token(find_method_token(property_token, desired_semantics));

        if (!method_token.is_initialized())
            return method();

        metadata::property_signature const property_signature(row_from(property_token).signature().as<metadata::property_signature>());

        type const reflected_type(property.reflected_type());
        core::assert_initialized(reflected_type);

        metadata::binding_flags const flags(property_signature.has_this()
            ? metadata::binding_attribute::all_instance
            : metadata::binding_attribute::all_static);

        auto const it(core::find_if(reflected_type.methods(flags), [&](method const& m)
        {
            return m.context(core::internal_key()).member_token() == method_token;
        }));

        if (it == end(reflected_type.methods(flags)))
            throw core::runtime_error(L"failed to find property method with requested semantics");

        return *it;
    }

} } }

namespace cxxreflect { namespace reflection {

    property::property()
    {
    }

    property::property(type const& reflected_type, detail::property_table_entry const* context, core::internal_key)
        : _reflected_type(reflected_type.context(core::internal_key())), _context(context)
    {
        core::assert_initialized(reflected_type);
        core::assert_not_null(context);
    }

    auto property::declaring_type() const -> type
    {
        core::assert_initialized(*this);

        if (_context->has_instantiating_type())
        {
            return type(_context->instantiating_type(), core::internal_key());
        }

        return type(metadata::find_owner_of_property(_context->member_token()).token(), core::internal_key());
    }

    auto property::reflected_type() const -> type
    {
        core::assert_initialized(*this);

        return type(_reflected_type, core::internal_key());
    }

    auto property::attributes() const -> metadata::property_flags
    {
        core::assert_initialized(*this);

        return row().flags();
    }

    auto property::can_read() const -> bool
    {
        core::assert_initialized(*this);

        return find_method_token(_context->member_token(), metadata::method_semantics_attribute::getter).is_initialized();
    }

    auto property::can_write() const -> bool
    {
        core::assert_initialized(*this);

        return find_method_token(_context->member_token(), metadata::method_semantics_attribute::setter).is_initialized();
    }

    auto property::is_special_name() const -> bool
    {
        core::assert_initialized(*this);

        return row().flags().is_set(metadata::property_attribute::special_name);
    }

    auto property::metadata_token() const -> core::size_type
    {
        core::assert_initialized(*this);

        return row().token().value();
    }

    auto property::declaring_module() const -> module
    {
        core::assert_initialized(*this);

        return declaring_type().defining_module();
    }

    auto property::name() const -> core::string_reference
    {
        core::assert_initialized(*this);

        return row().name();
    }

    auto property::property_type() const -> type
    {
        core::assert_initialized(*this);

        metadata::property_signature const signature(row().signature().as<metadata::property_signature>());
        return type(metadata::blob(signature.type()), core::internal_key());
    }

    auto property::custom_attributes() const -> detail::custom_attribute_range
    {
        core::assert_initialized(*this);

        return custom_attribute::get_for(_context->member_token(), core::internal_key());
    }

    auto property::default_value() const -> constant
    {
        core::assert_initialized(*this);

        return constant::create_for(_context->member_token(), core::internal_key());
    }

    auto property::get_method() const -> method
    {
        core::assert_initialized(*this);

        return find_method(*this, metadata::method_semantics_attribute::getter);
    }

    auto property::set_method() const -> method
    {
        core::assert_initialized(*this);

        return find_method(*this, metadata::method_semantics_attribute::setter);
    }

    auto property::is_initialized() const -> bool
    {
        return _context.is_initialized();
    }

    auto property::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(property const& lhs, property const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context != rhs._context;
    }

    auto operator<(property const& lhs, property const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._context < rhs._context;
    }

    auto property::context(core::internal_key) const -> detail::property_table_entry const&
    {
        core::assert_initialized(*this);
        return *_context;
    }

    auto property::row() const -> metadata::property_row
    {
        core::assert_initialized(*this);

        return row_from(_context->member_token());
    }

} }
