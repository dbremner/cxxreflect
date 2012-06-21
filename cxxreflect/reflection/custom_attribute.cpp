
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/custom_attribute.hpp"
#include "cxxreflect/reflection/guid.hpp"
#include "cxxreflect/reflection/method.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection {

    custom_attribute::custom_attribute()
    {
    }

    custom_attribute::custom_attribute(module                           const& declaring_module,
                                       metadata::custom_attribute_token const& attribute,
                                       core::internal_key)
    {
        core::assert_initialized(declaring_module);
        core::assert_initialized(attribute);

        metadata::custom_attribute_row const attribute_row(row_from(attribute));

        static metadata::binding_flags const flags(metadata::binding_attribute::all_instance);

        _attribute = attribute;

        switch (attribute_row.type().table())
        {
        case metadata::table_id::method_def:
        {
            metadata::method_def_token const ctor_token(attribute_row.type().as<metadata::method_def_token>());
            metadata::type_def_token   const owner_token(metadata::find_owner_of_method_def(ctor_token).token());

            type const t(declaring_module, owner_token, core::internal_key());

            auto const ctor_it(std::find_if(t.begin_constructors(flags),
                                            t.end_constructors(),
                                            [&](method const& ctor)
            {
                return ctor.metadata_token() == ctor_token.value();
            }));

            if (ctor_it == t.end_constructors())
                throw core::runtime_error(L"failed to find constructor for attribute");

            _constructor =  detail::method_handle(*ctor_it);
            break;
        }

        case metadata::table_id::member_ref:
        {
            metadata::member_ref_row const ref_row(row_from(attribute_row.type().as<metadata::member_ref_token>()));
            metadata::member_ref_parent_token const owner_token(ref_row.parent());

            switch (owner_token.table())
            {
            case metadata::table_id::type_ref:
            {
                type const t(declaring_module, owner_token.as<metadata::type_ref_token>(), core::internal_key());
                if (ref_row.name() == L".ctor")
                {
                    // TODO Correct detection of contructors

                    if (t.begin_constructors(flags) == t.end_constructors())
                        throw core::logic_error(L"not yet implemented");

                    detail::loader_context const& root(detail::loader_context::from(declaring_module));

                    metadata::signature_comparer const compare(&root);

                    auto const ctor_it(std::find_if(t.begin_constructors(flags),
                                                    t.end_constructors(),
                                                    [&](method const& ctor)
                    {
                        return compare(
                            ref_row.signature().as<metadata::method_signature>(),
                            ctor.context(core::internal_key()).element_signature(root));
                    }));

                    if (ctor_it == t.end_constructors())
                        throw core::runtime_error(L"failed to find constructor for attribute");

                    _constructor = detail::method_handle(*ctor_it);
                    break;
                }
                else
                {
                    throw core::logic_error(L"not yet implemented");
                }
            }

            default:
            {
                throw core::logic_error(L"not yet implemented");
            }
            }

            break;
        }

        default:
        {
            core::assert_fail(L"unreachable code");
        }
        }
    }

    auto custom_attribute::metadata_token() const -> core::size_type
    {
        core::assert_initialized(*this);

        return _attribute.value();
    }

    auto custom_attribute::constructor() const -> method
    {
        core::assert_initialized(*this);

        return _constructor.realize();
    }

    // TODO auto custom_attribute::begin_positional_arguments() const -> positional_argument_iterator;
    // TODO auto custom_attribute::end_positional_arguments()   const -> positional_argument_iterator;

    // TODO auto custom_attribute::begin_named_arguments() const -> named_argument_iterator;
    // TODO auto custom_attribute::end_named_arguments()   const -> named_argument_iterator;

    auto custom_attribute::single_string_argument() const -> core::string
    {
        metadata::custom_attribute_row const attribute(row_from(_attribute));

        metadata::blob const value_blob(attribute.value());
        core::const_byte_iterator it(begin(value_blob));

        // All custom attribute signatures begin with a two-byte, little-endian integer with the
        // value '1'.  If the signature doesn't have this prefix, we've screwed up somewhere or
        // the metadata is invalid.
        auto const prefix(metadata::detail::read_sig_element<std::uint16_t>(it, end(value_blob)));
        if (prefix != 1)
            throw core::runtime_error(L"Invalid custom attribute signature");

        // TODO We materialize three separate buffers here; we should look into removing one or two.
        core::size_type const utf8_length(metadata::detail::read_sig_compressed_uint32(it, end(value_blob)));
        if (core::distance(it, end(value_blob)) < utf8_length)
            throw core::runtime_error(L"Invalid custom attribute value");

        std::vector<char> utf8_buffer(it, it + utf8_length);
        utf8_buffer.push_back(L'\0');

        core::size_type const utf16_length(core::externals::compute_utf16_length_of_utf8_string(
            reinterpret_cast<char const*>(it)));

        std::vector<wchar_t> utf16_buffer(utf16_length);

        core::externals::convert_utf8_to_utf16(utf8_buffer.data(), utf16_buffer.data(), utf16_length);
        return core::string(utf16_buffer.data());
    }

    auto custom_attribute::single_guid_argument() const -> guid
    {
        metadata::custom_attribute_row const attribute(row_from(_attribute));

        metadata::blob const value_blob(attribute.value());
        core::const_byte_iterator it(begin(value_blob));

        // All custom attribute signatures begin with a two-byte, little-endian integer with the
        // value '1'.  If the signature doesn't have this prefix, we've screwed up somewhere or
        // the metadata is invalid.
        auto const prefix(metadata::detail::read_sig_element<std::uint16_t>(it, end(value_blob)));
        if (prefix != 1)
            throw core::runtime_error(L"Invalid custom attribute signature");

        auto const a0(metadata::detail::read_sig_element<std::uint32_t>(it, end(value_blob)));

        auto const a1(metadata::detail::read_sig_element<std::uint16_t>(it, end(value_blob)));
        auto const a2(metadata::detail::read_sig_element<std::uint16_t>(it, end(value_blob)));

        auto const a3(metadata::detail::read_sig_element<std::uint8_t>(it, end(value_blob)));
        auto const a4(metadata::detail::read_sig_element<std::uint8_t>(it, end(value_blob)));
        auto const a5(metadata::detail::read_sig_element<std::uint8_t>(it, end(value_blob)));
        auto const a6(metadata::detail::read_sig_element<std::uint8_t>(it, end(value_blob)));
        auto const a7(metadata::detail::read_sig_element<std::uint8_t>(it, end(value_blob)));
        auto const a8(metadata::detail::read_sig_element<std::uint8_t>(it, end(value_blob)));
        auto const a9(metadata::detail::read_sig_element<std::uint8_t>(it, end(value_blob)));
        auto const aa(metadata::detail::read_sig_element<std::uint8_t>(it, end(value_blob)));

        return guid(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, aa);
    }

    auto custom_attribute::is_initialized() const -> bool
    {
        return _attribute.is_initialized() && _constructor.is_initialized();
    }

    auto custom_attribute::operator!() const -> bool
    {
        return !is_initialized();
    }

    auto operator==(custom_attribute const& lhs, custom_attribute const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._attribute == rhs._attribute;
    }

    auto operator<(custom_attribute const& lhs, custom_attribute const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._attribute < rhs._attribute;
    }

    auto custom_attribute::begin_for(module                               const& declaring_module,
                                     metadata::has_custom_attribute_token const& parent,
                                     core::internal_key) -> custom_attribute_iterator
    {
        core::assert_initialized(declaring_module);
        core::assert_initialized(parent);

        metadata::custom_attribute_row_iterator const it(metadata::begin_custom_attributes(parent));
        if (!it.is_initialized())
            return custom_attribute_iterator();

        return custom_attribute_iterator(declaring_module, it.token());
    }

    auto custom_attribute::end_for(module                               const& declaring_module,
                                   metadata::has_custom_attribute_token const& parent,
                                   core::internal_key) -> custom_attribute_iterator
    {
        core::assert_initialized(declaring_module);
        core::assert_initialized(parent);

        metadata::custom_attribute_row_iterator const it(metadata::end_custom_attributes(parent));
        if (!it.is_initialized())
            return custom_attribute_iterator();

        return custom_attribute_iterator(declaring_module, it.token());
    }

} }

// AMDG //
