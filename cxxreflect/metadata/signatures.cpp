
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/precompiled_headers.hpp"
#include "cxxreflect/metadata/database.hpp"
#include "cxxreflect/metadata/signatures.hpp"
#include "cxxreflect/metadata/rows.hpp"
#include "cxxreflect/metadata/utility.hpp"

namespace cxxreflect { namespace metadata {

    base_signature::base_signature()
    {
    }

    base_signature::base_signature(database const*           const scope,
                                   core::const_byte_iterator const first,
                                   core::const_byte_iterator const last)
        : _scope(scope), _first(first), _last(last)
    {
        core::assert_not_null(scope);
        core::assert_not_null(first);
        core::assert_not_null(last);
    }
        
    base_signature::base_signature(base_signature const& other)
        : _scope(other._scope), _first(other._first), _last(other._last)
    {
    }

    auto base_signature::operator=(base_signature const& other) -> base_signature&
    {
        _scope = other._scope;
        _first = other._first;
        _last  = other._last;
        return *this;
    }

    base_signature::~base_signature()
    {
    }

    auto base_signature::scope() const -> database const&
    {
        core::assert_initialized(*this);
        return *_scope.get();
    }

    auto base_signature::begin_bytes() const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);
        return _first.get();
    }

    auto base_signature::end_bytes() const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);
        return _last.get();
    }

    auto base_signature::is_initialized() const -> bool
    {
        return _scope.get() != nullptr && _first.get() != nullptr && _last.get() != nullptr;
    }





    auto array_shape::read_size(database const&,
                                core::const_byte_iterator&      current,
                                core::const_byte_iterator const last) -> core::size_type
    {
        return detail::read_sig_compressed_uint32(current, last);
    }

    auto array_shape::read_low_bound(database const&,
                                     core::const_byte_iterator&      current, 
                                     core::const_byte_iterator const last) -> core::size_type
    {
        return detail::read_sig_compressed_uint32(current, last);
    }

    array_shape::array_shape()
    {
    }

    array_shape::array_shape(database const*           const scope,
                             core::const_byte_iterator const first,
                             core::const_byte_iterator const last)
        : base_signature(scope, first, last)
    {
    }

    auto array_shape::rank() const -> core::size_type
    {
        core::assert_initialized(*this);
        return detail::peek_sig_compressed_uint32(seek_to(part::rank), end_bytes());
    }

    auto array_shape::size_count() const -> core::size_type
    {
        core::assert_initialized(*this);
        return detail::peek_sig_compressed_uint32(seek_to(part::num_sizes), end_bytes());
    }

    auto array_shape::begin_sizes() const -> size_iterator
    {
        core::assert_initialized(*this);
        return size_iterator(&scope(), seek_to(part::first_size), end_bytes(), 0, size_count());
    }

    auto array_shape::end_sizes() const -> size_iterator
    {
        core::assert_initialized(*this);

        core::size_type const n(size_count());
        return size_iterator(&scope(), nullptr, nullptr, n, n);
    }

    auto array_shape::low_bound_count() const -> core::size_type
    {
        core::assert_initialized(*this);
        return detail::peek_sig_compressed_uint32(seek_to(part::num_low_bounds), end_bytes());
    }

    auto array_shape::begin_low_bounds() const -> low_bound_iterator
    {
        core::assert_initialized(*this);
        return low_bound_iterator(&scope(), seek_to(part::first_low_bound), end_bytes(), 0, low_bound_count());
    }

    auto array_shape::end_low_bounds() const -> low_bound_iterator
    {
        core::assert_initialized(*this);

        core::size_type const n(low_bound_count());
        return low_bound_iterator(&scope(), nullptr, nullptr, n, n);
    }

    auto array_shape::compute_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return core::distance(begin_bytes(), seek_to(part::end));
    }

    auto array_shape::seek_to(part const p) const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);

        core::const_byte_iterator current(begin_bytes());

        if (p > part::rank)
        {
            detail::read_sig_compressed_uint32(current, end_bytes());
        }

        std::uint32_t size_count(0);
        if (p > part::num_sizes)
        {
            size_count = detail::read_sig_compressed_uint32(current, end_bytes());
        }

        if (p > part::first_size)
        {
            for (unsigned i(0); i < size_count; ++i)
                detail::read_sig_compressed_uint32(current, end_bytes());
        }

        std::uint32_t low_bound_count(0);
        if (p > part::num_low_bounds)
        {
            low_bound_count = detail::read_sig_compressed_uint32(current, end_bytes());
        }

        if (p > part::first_low_bound)
        {
            for (unsigned i(0); i < low_bound_count; ++i)
                detail::read_sig_compressed_uint32(current, end_bytes());
        }

        core::assert_true([&]{ return p <= part::end; }, L"invalid signature part requested");

        return current;
    }





    custom_modifier::custom_modifier()
    {
    }

    custom_modifier::custom_modifier(database const*           const scope,
                                     core::const_byte_iterator const first,
                                     core::const_byte_iterator const last)
        : base_signature(scope, first, last)
    {
        core::assert_true([&]{ return is_optional() || is_required(); });
    }

    auto custom_modifier::is_optional() const -> bool
    {
        core::assert_initialized(*this);
        return detail::peek_sig_byte(seek_to(part::req_opt_flag), end_bytes()) == element_type::custom_modifier_optional;
    }

    auto custom_modifier::is_required() const -> bool
    {
        core::assert_initialized(*this);
        return detail::peek_sig_byte(seek_to(part::req_opt_flag), end_bytes()) == element_type::custom_modifier_required;
    }

    auto custom_modifier::type() const -> type_def_ref_spec_token
    {
        core::assert_initialized(*this);
        return type_def_ref_spec_token(
            &scope(),
            detail::peek_sig_type_def_ref_spec(seek_to(part::type), end_bytes()));
    }

    auto custom_modifier::compute_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return core::distance(begin_bytes(), seek_to(part::end));
    }

    auto custom_modifier::seek_to(part const p) const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);

        core::const_byte_iterator current(begin_bytes());

        if (p > part::req_opt_flag)
        {
            detail::read_sig_byte(current, end_bytes());
        }

        if (p > part::type)
        {
            detail::read_sig_type_def_ref_spec(current, end_bytes());
        }

        if (p > part::end)
        {
            core::assert_fail(L"invalid signature part requested");
        }

        return current;
    }





    field_signature::field_signature()
    {
    }

    field_signature::field_signature(database const*           const scope,
                                     core::const_byte_iterator const first,
                                     core::const_byte_iterator const last)
        : base_signature(scope, first, last)
    {
        core::assert_true([&]
        {
            return detail::peek_sig_byte(seek_to(part::field_tag), end_bytes()) == signature_attribute::field;
        });
    }

    auto field_signature::type() const -> type_signature
    {
        core::assert_initialized(*this);
        return type_signature(&scope(), seek_to(part::type), end_bytes());
    }

    auto field_signature::compute_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return core::distance(begin_bytes(), seek_to(part::end));
    }

    auto field_signature::seek_to(part const p) const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);

        core::const_byte_iterator current(begin_bytes());

        if (p > part::field_tag)
        {
            detail::read_sig_byte(current, end_bytes());
        }

        if (p > part::type)
        {
            current += type_signature(&scope(), current, end_bytes()).compute_size();
        }

        if (p > part::end)
        {
            core::assert_fail(L"invalid signature part requested");
        }

        return current;
    }





    auto property_signature::read_parameter(database const&                  scope,
                                            core::const_byte_iterator&       current,
                                            core::const_byte_iterator  const last) -> type_signature
    {
        type_signature const type(&scope, current, last);
        current += type.compute_size();
        return type;
    }

    property_signature::property_signature()
    {
    }

    property_signature::property_signature(database const*           const scope,
                                           core::const_byte_iterator const first,
                                           core::const_byte_iterator const last)
        : base_signature(scope, first, last)
    {
        core::assert_true([&]() -> bool
        {
            core::byte const initial_byte(detail::peek_sig_byte(seek_to(part::property_tag), end_bytes()));
            return initial_byte == signature_attribute::property_
                || initial_byte == (signature_attribute::property_ | signature_attribute::has_this);
        });
    }

    auto property_signature::has_this() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::property_tag), end_bytes()))
            .is_set(signature_attribute::has_this);
    }

    auto property_signature::parameter_count() const -> core::size_type
    {
        core::assert_initialized(*this);
        return detail::peek_sig_compressed_uint32(seek_to(part::parameter_count), end_bytes());
    }

    auto property_signature::begin_parameters() const -> parameter_iterator
    {
        core::assert_initialized(*this);
        return parameter_iterator(&scope(), seek_to(part::first_parameter), end_bytes(), 0, parameter_count());
    }

    auto property_signature::end_parameters() const -> parameter_iterator
    {
        core::assert_initialized(*this);

        core::size_type const n(parameter_count());
        return parameter_iterator(&scope(), nullptr, nullptr, n, n);
    }

    auto property_signature::type() const -> type_signature
    {
        core::assert_initialized(*this);
        return type_signature(&scope(), seek_to(part::type), end_bytes());
    }

    auto property_signature::compute_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return core::distance(begin_bytes(), seek_to(part::end));
    }

    auto property_signature::seek_to(part const p) const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);

        core::const_byte_iterator current(begin_bytes());

        if (p > part::property_tag)
        {
            core::byte const tag_byte(detail::read_sig_byte(current, end_bytes()));
            core::assert_true([&]{ return signature_flags(tag_byte).is_set(signature_attribute::property_); });
        }

        core::size_type parameters(0);
        if (p > part::parameter_count)
        {
            parameters = detail::read_sig_compressed_uint32(current, end_bytes());
        }

        if (p > part::type)
        {
            current += type_signature(&scope(), current, end_bytes()).compute_size();
        }

        if (p > part::first_parameter)
        {
            for (unsigned i(0); i < parameters; ++i)
            {
                current += type_signature(&scope(), current, end_bytes()).compute_size();
            }
        }

        if (p > part::end)
        {
            core::assert_fail(L"invalid signature part requested");
        }

        return current;
    }





    auto method_signature::parameter_end_check(database const&,
                                               core::const_byte_iterator const current,
                                               core::const_byte_iterator const last) -> bool
    {
        return detail::peek_sig_byte(current, last) == element_type::sentinel;
    }

    auto method_signature::read_parameter(database const&                 scope,
                                          core::const_byte_iterator&      current,
                                          core::const_byte_iterator const last) -> type_signature
    {
        type_signature const type(&scope, current, last);
        current += type.compute_size();
        return type;
    }

    method_signature::method_signature()
    {
    }

    method_signature::method_signature(database const*           const scope,
                                       core::const_byte_iterator const first,
                                       core::const_byte_iterator const last)
        : base_signature(scope, first, last)
    {
    }

    auto method_signature::has_this() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::has_this);
    }

    auto method_signature::has_explicit_this() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::explicit_this);
    }

    auto method_signature::calling_convention() const -> signature_attribute
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .with_mask(signature_attribute::calling_convention_mask)
            .enumerator();
    }

    auto method_signature::has_default_convention() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::calling_convention_default);
    }

    auto method_signature::has_vararg_convention() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::calling_convention_varargs);
    }

    auto method_signature::has_c_convention() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::calling_convention_cdecl);
    }

    auto method_signature::has_stdcall_convention() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::calling_convention_stdcall);
    }

    auto method_signature::has_thiscall_convention() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::calling_convention_thiscall);
    }

    auto method_signature::has_fastcall_convention() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::calling_convention_fastcall);
    }

    auto method_signature::is_generic() const -> bool
    {
        core::assert_initialized(*this);
        return signature_flags(detail::peek_sig_byte(seek_to(part::type_tag), end_bytes()))
            .is_set(signature_attribute::generic_);
    }

    auto method_signature::generic_parameter_count() const -> core::size_type
    {
        core::assert_initialized(*this);

        if (!is_generic())
            return 0;

        return detail::peek_sig_compressed_uint32(seek_to(part::gen_param_count), end_bytes());
    }

    auto method_signature::return_type() const -> type_signature
    {
        core::assert_initialized(*this);
        return type_signature(&scope(), seek_to(part::ret_type), end_bytes());
    }

    auto method_signature::parameter_count() const -> core::size_type
    {
        core::assert_initialized(*this);
        return detail::peek_sig_compressed_uint32(seek_to(part::param_count), end_bytes());
    }

    auto method_signature::begin_parameters() const -> parameter_iterator
    {
        core::assert_initialized(*this);
        return parameter_iterator(&scope(), seek_to(part::first_param), end_bytes(), 0, parameter_count());
    }

    auto method_signature::end_parameters() const -> parameter_iterator
    {
        core::assert_initialized(*this);

        core::size_type const n(parameter_count());
        return parameter_iterator(&scope(), nullptr, nullptr, n, n);
    }

    auto method_signature::begin_vararg_parameters() const -> parameter_iterator
    {
        core::assert_initialized(*this);

        core::size_type const total_parameters(parameter_count());
        core::size_type const actual_parameters(core::distance(begin_parameters(), end_parameters()));
        core::size_type const vararg_parameters(total_parameters - actual_parameters);

        return parameter_iterator(&scope(), seek_to(part::first_vararg_param), end_bytes(), 0, vararg_parameters);
    }

    auto method_signature::end_vararg_parameters() const -> parameter_iterator
    {
        core::assert_initialized(*this);

        core::size_type const total_parameters(parameter_count());
        core::size_type const actual_parameters(core::distance(begin_parameters(), end_parameters()));
        core::size_type const vararg_parameters(total_parameters - actual_parameters);

        return parameter_iterator(&scope(), nullptr, nullptr, vararg_parameters, vararg_parameters);
    }

    auto method_signature::compute_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return core::distance(begin_bytes(), seek_to(part::end));
    }

    auto method_signature::seek_to(part const p) const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);

        core::const_byte_iterator current(begin_bytes());

        signature_flags type_flags;
        if (p > part::type_tag)
        {
            type_flags = detail::read_sig_byte(current, end_bytes());
        }

        if (p == part::gen_param_count && !type_flags.is_set(signature_attribute::generic_))
        {
            return nullptr;
        }

        if (p > part::gen_param_count && type_flags.is_set(signature_attribute::generic_))
        {
            detail::read_sig_compressed_uint32(current, end_bytes());
        }

        core::size_type parameters(0);
        if (p > part::param_count)
        {
            parameters = detail::read_sig_compressed_uint32(current, end_bytes());
        }

        if (p > part::ret_type)
        {
            current += type_signature(&scope(), current, end_bytes()).compute_size();
        }

        unsigned parameters_read(0);
        if (p > part::first_param)
        {
            while (parameters_read < parameters &&
                   detail::peek_sig_byte(current, end_bytes()) != element_type::sentinel)
            {
                ++parameters_read;
                current += type_signature(&scope(), current, end_bytes()).compute_size();
            }

            if (current != end_bytes() &&
                detail::peek_sig_byte(current, end_bytes()) == element_type::sentinel)
            {
                current += detail::read_sig_byte(current, end_bytes());
            }
        }

        if (p > part::first_vararg_param && parameters_read < parameters)
        {
            for (unsigned i(parameters_read); i != parameters; ++i)
            {
                current += type_signature(&scope(), current, end_bytes()).compute_size();
            }
        }

        if (p > part::end)
        {
            core::assert_fail(L"invalid signature part requested");
        }

        return current;
    }





    auto type_signature::custom_modifier_end_check(database const&,
                                                   core::const_byte_iterator const current,
                                                   core::const_byte_iterator const last) -> bool
    {
        return current == last || !is_custom_modifier_element_type(detail::peek_sig_byte(current, last));
    }

    auto type_signature::read_custom_modifier(database const&                 scope,
                                              core::const_byte_iterator&      current, 
                                              core::const_byte_iterator const last) -> custom_modifier
    {
        custom_modifier modifier(&scope, current, last);
        current += modifier.compute_size();
        return modifier;
    }

    auto type_signature::read_type(database const&                 scope,
                                   core::const_byte_iterator&      current,
                                   core::const_byte_iterator const last) -> type_signature
    {
        type_signature type(&scope, current, last);
        current += type.compute_size();
        return type;
    }

    type_signature::type_signature()
    {
    }

    type_signature::type_signature(database const*           const scope,
                                   core::const_byte_iterator const first,
                                   core::const_byte_iterator const last)
        : base_signature(scope, first, last)
    {
    }

    auto type_signature::compute_size() const -> core::size_type
    {
        core::assert_initialized(*this);
        return core::distance(begin_bytes(), seek_to(part::end));
    }

    auto type_signature::seek_to(part const p) const -> core::const_byte_iterator
    {
        core::assert_initialized(*this);

        kind const part_kind(static_cast<kind>(static_cast<core::size_type>(p)) & kind::mask);
        part const part_code(p & static_cast<part>(~static_cast<core::size_type>(kind::mask)));

        core::const_byte_iterator current(begin_bytes());

        if (part_code > part::first_custom_mod)
        {
            while (is_custom_modifier_element_type(detail::peek_sig_byte(current, end_bytes())))
                current += custom_modifier(&scope(), current, end_bytes()).compute_size();
        }

        if (part_code > part::by_ref_tag && detail::peek_sig_byte(current, end_bytes()) == element_type::by_ref)
        {
            detail::read_sig_byte(current, end_bytes());
        }

        // When we generate a cross-module type reference, we inject a tag in front of the class
        // type reference so that we can identify it here.  We always want to skip over this tag;
        // the only time it is relevant is here when we are seeking to the correct parts of a
        // signature.
        bool const is_cross_module_type_reference(
            current != end_bytes() &&
            detail::peek_sig_byte(current, end_bytes()) == element_type::cross_module_type_reference);

        if (part_code > part::cross_module_type_reference && is_cross_module_type_reference)
        {
            detail::read_sig_byte(current, end_bytes());
        }

        if (part_code > part::type_code)
        {
            element_type const type_tag(detail::read_sig_element_type(current, end_bytes()));

            if (part_kind != kind::unknown && !is_kind(part_kind))
            {
                core::assert_fail(L"invalid signature part requested");
            }

            auto const extract_part([](part const p)
            {
                return static_cast<part>(static_cast<core::size_type>(p) & ~static_cast<core::size_type>(kind::mask));
            });

            kind const k(get_kind());
            switch (k)
            {
            case kind::primitive:
            {
                break;
            }
            case kind::general_array:
            {
                if (part_code > extract_part(part::general_array_type))
                {
                    current += type_signature(&scope(), current, end_bytes()).compute_size();
                }

                if (part_code > extract_part(part::general_array_shape))
                {
                    current += metadata::array_shape(&scope(), current, end_bytes()).compute_size();
                }

                break;
            }
            case kind::simple_array:
            {
                if (part_code > extract_part(part::simple_array_type))
                {
                    current += type_signature(&scope(), current, end_bytes()).compute_size();
                }

                break;
            }
            case kind::class_type:
            {
                if (part_code > extract_part(part::class_type_type))
                {
                    detail::read_sig_type_def_ref_spec(current, end_bytes());
                }

                if (part_code > extract_part(part::class_type_scope) && is_cross_module_type_reference)
                {
                    detail::read_sig_pointer(current, end_bytes());
                }

                break;
            }
            case kind::function_pointer:
            {
                if (part_code > extract_part(part::function_pointer_type))
                {
                    current += method_signature(&scope(), current, end_bytes()).compute_size();
                }

                break;
            }
            case kind::generic_instance:
            {
                core::byte q;
                if (part_code > extract_part(part::generic_instance_type_code))
                {
                    q = detail::read_sig_byte(current, end_bytes());
                }

                std::uint32_t r;
                if (part_code > extract_part(part::generic_instance_type))
                {
                    r = detail::read_sig_type_def_ref_spec(current, end_bytes());
                }

                core::size_type argument_count(0);
                if (part_code > extract_part(part::generic_instance_argument_count))
                {
                    argument_count = detail::read_sig_compressed_uint32(current, end_bytes());
                }

                if (part_code > extract_part(part::first_generic_instance_argument))
                {
                    for (unsigned i(0); i < argument_count; ++i)
                        current += type_signature(&scope(), current, end_bytes()).compute_size();
                }

                break;
            }
            case kind::pointer:
            {
                if (part_code > extract_part(part::pointer_type))
                {
                    current += type_signature(&scope(), current, end_bytes()).compute_size();
                }

                break;
            }
            case kind::variable:
            {
                if (part_code > extract_part(part::variable_number))
                {
                    detail::read_sig_compressed_uint32(current, end_bytes());
                }

                bool const is_annotated(type_tag == element_type::annotated_mvar || type_tag == element_type::annotated_var);
                
                if (is_annotated && part_code > extract_part(part::variable_context))
                {
                    detail::read_sig_element<core::size_type>(current, end_bytes());
                    detail::read_sig_pointer(current, end_bytes());
                }

                break;
            }
            default:
            {
                core::assert_fail(L"it is impossible to get here");
            }
            }
        }

        if (part_code > part::end)
        {
            core::assert_fail(L"invalid signature part requested");
        }

        return current;
    }

    auto type_signature::get_kind() const -> kind
    {
        core::assert_initialized(*this);

        switch (get_element_type())
        {
        case element_type::void_type:
        case element_type::boolean:
        case element_type::character:
        case element_type::i1:
        case element_type::u1:
        case element_type::i2:
        case element_type::u2:
        case element_type::i4:
        case element_type::u4:
        case element_type::i8:
        case element_type::u8:
        case element_type::r4:
        case element_type::r8:
        case element_type::i:
        case element_type::u:
        case element_type::string:
        case element_type::object:
        case element_type::typed_by_ref:
            return kind::primitive;

        case element_type::array:
            return kind::general_array;

        case element_type::sz_array:
            return kind::simple_array;

        case element_type::class_type:
        case element_type::value_type:
        case element_type::cross_module_type_reference:
            return kind::class_type;

        case element_type::fn_ptr:
            return kind::function_pointer;

        case element_type::generic_inst:
            return kind::generic_instance;

        case element_type::ptr:
            return kind::pointer;

        case element_type::annotated_mvar:
        case element_type::annotated_var:
        case element_type::mvar:
        case element_type::var:
            return kind::variable;

        default:
            return kind::unknown;
        }
    }

    auto type_signature::is_kind(kind const k) const -> bool
    {
        return get_kind() == k;
    }

    auto type_signature::get_element_type() const -> element_type
    {
        core::assert_initialized(*this);

        core::byte const type_tag(detail::peek_sig_byte(seek_to(part::type_code), end_bytes()));
        return is_valid_element_type(type_tag) ? static_cast<element_type>(type_tag) : element_type::end;
    }

    auto type_signature::is_cross_module_type_reference() const -> bool
    {
        core::assert_initialized(*this);

        core::byte const type_tag(detail::peek_sig_byte(seek_to(part::cross_module_type_reference), end_bytes()));
        return type_tag == element_type::cross_module_type_reference;
    }

    auto type_signature::begin_custom_modifiers() const -> custom_modifier_iterator
    {
        core::assert_initialized(*this);
        
        core::const_byte_iterator const first_modifier(seek_to(part::first_custom_mod));
        return custom_modifier_iterator(&scope(), first_modifier, first_modifier == nullptr ? nullptr : end_bytes());
    }

    auto type_signature::end_custom_modifiers()   const -> custom_modifier_iterator
    {
        core::assert_initialized(*this);
        return custom_modifier_iterator();
    }

    auto type_signature::is_by_ref() const -> bool
    {
        core::assert_initialized(*this);

        core::const_byte_iterator const by_ref_tag(seek_to(part::by_ref_tag));
        return by_ref_tag != nullptr && detail::peek_sig_byte(by_ref_tag, end_bytes()) == element_type::by_ref;
    }

    auto type_signature::is_primitive() const -> bool
    {
        core::assert_initialized(*this);
        return primitive_type() != element_type::end;
    }

    auto type_signature::primitive_type() const -> element_type
    {
        core::assert_initialized(*this);

        element_type const type(get_element_type());
        switch (type)
        {
        case element_type::boolean:
        case element_type::character:
        case element_type::i1:
        case element_type::u1:
        case element_type::i2:
        case element_type::u2:
        case element_type::i4:
        case element_type::u4:
        case element_type::i8:
        case element_type::u8:
        case element_type::r4:
        case element_type::r8:
        case element_type::i:
        case element_type::u:
        case element_type::object:
        case element_type::string:
        case element_type::void_type:
        case element_type::typed_by_ref:
            return type;

        default:
            return element_type::end;
        }
    }

    auto type_signature::is_general_array() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::array;
    }

    auto type_signature::is_simple_array() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::sz_array;
    }

    auto type_signature::array_type() const -> type_signature
    {
        core::assert_true([&]{ return get_kind() == kind::general_array || get_kind() == kind::simple_array; });
        return type_signature(
            &scope(),
            is_kind(kind::general_array) ? seek_to(part::general_array_type) : seek_to(part::simple_array_type),
            end_bytes());
    }

    auto type_signature::array_shape() const -> metadata::array_shape
    {
        assert_kind(kind::general_array);
        return metadata::array_shape(&scope(), seek_to(part::general_array_shape), end_bytes());
    }

    auto type_signature::is_class_type() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::class_type;
    }

    auto type_signature::is_value_type() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::value_type;
    }

    auto type_signature::class_type() const -> type_def_ref_spec_token
    {
        assert_kind(kind::class_type);

        database const* other_scope(nullptr);
        if (get_element_type() == element_type::cross_module_type_reference)
        {
            other_scope = reinterpret_cast<database const*>(detail::peek_sig_pointer(
                seek_to(part::class_type_scope),
                end_bytes()));
        }

        database const* const actual_scope(other_scope != nullptr ? other_scope : &scope());
        return type_def_ref_spec_token(
            actual_scope,
            detail::peek_sig_type_def_ref_spec(seek_to(part::class_type_type), end_bytes()));
    }

    auto type_signature::is_function_pointer() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::fn_ptr;
    }

    auto type_signature::function_type() const -> method_signature
    {
        assert_kind(kind::function_pointer);
        return method_signature(&scope(), seek_to(part::function_pointer_type), end_bytes());
    }

    auto type_signature::is_generic_instance() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::generic_inst;
    }

    auto type_signature::is_generic_class_type_instance() const -> bool
    {
        core::assert_initialized(*this);
        return detail::peek_sig_byte(seek_to(part::generic_instance_type_code), end_bytes()) == element_type::class_type;
    }

    auto type_signature::is_generic_value_type_instance() const -> bool
    {
        core::assert_initialized(*this);
        return detail::peek_sig_byte(seek_to(part::generic_instance_type_code), end_bytes()) == element_type::value_type;
    }

    auto type_signature::generic_type() const -> type_def_ref_spec_token
    {
        assert_kind(kind::generic_instance);
        return type_def_ref_spec_token(
            &scope(),
            detail::peek_sig_type_def_ref_spec(seek_to(part::generic_instance_type), end_bytes()));
    }

    auto type_signature::generic_argument_count() const -> core::size_type
    {
        assert_kind(kind::generic_instance);
        return detail::peek_sig_compressed_uint32(seek_to(part::generic_instance_argument_count), end_bytes());
    }

    auto type_signature::begin_generic_arguments() const -> generic_argument_iterator
    {
        assert_kind(kind::generic_instance);

        return generic_argument_iterator(
            &scope(),
            seek_to(part::first_generic_instance_argument),
            end_bytes(),
            0,
            generic_argument_count());
    }

    auto type_signature::end_generic_arguments() const -> generic_argument_iterator
    {
        assert_kind(kind::generic_instance);

        core::size_type const count(detail::peek_sig_compressed_uint32(seek_to(part::generic_instance_argument_count), end_bytes()));
        return generic_argument_iterator(&scope(), nullptr, nullptr, count, count);
    }

    auto type_signature::is_pointer() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::ptr;
    }

    auto type_signature::pointer_type() const -> type_signature
    {
        assert_kind(kind::pointer);
        return type_signature(&scope(), seek_to(part::pointer_type), end_bytes());
    }

    auto type_signature::is_class_variable() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::var;
    }

    auto type_signature::is_method_variable() const -> bool
    {
        core::assert_initialized(*this);
        return get_element_type() == element_type::mvar;
    }

    auto type_signature::variable_number() const -> core::size_type
    {
        assert_kind(kind::variable);
        return detail::peek_sig_compressed_uint32(seek_to(part::variable_number), end_bytes());
    }

    auto type_signature::variable_context() const -> type_or_method_def_token
    {
        assert_kind(kind::variable);

        core::const_byte_iterator it(seek_to(part::variable_context));

        core::size_type const token(detail::read_sig_element<core::size_type>(it, end_bytes()));
        database const* const scope(reinterpret_cast<database const*>(detail::read_sig_pointer(it, end_bytes())));

        return type_or_method_def_token(scope, token);
    }

    auto type_signature::assert_kind(kind const k) const -> void
    {
        core::assert_initialized(*this);
        core::assert_true([&]{ return is_kind(k); });
    }





    signature_comparer::signature_comparer(type_resolver const* resolver)
        : _resolver(resolver)
    {
        core::assert_not_null(resolver);
    }

    auto signature_comparer::operator()(array_shape const& lhs, array_shape const& rhs) const -> bool
    {
        if (lhs.rank() != rhs.rank())
            return false;

        if (!core::range_checked_equal(
                lhs.begin_sizes(), lhs.end_sizes(),
                rhs.begin_sizes(), rhs.end_sizes()))
            return false;

        if (!core::range_checked_equal(
                lhs.begin_low_bounds(), lhs.end_low_bounds(),
                rhs.begin_low_bounds(), rhs.end_low_bounds()))
            return false;

        return true;
    }

    auto signature_comparer::operator()(custom_modifier const& lhs, custom_modifier const& rhs) const -> bool
    {
        if (lhs.is_optional() != rhs.is_optional())
            return false;

        if (!(*this)(lhs.type(), rhs.type()))
            return false;

        return true;
    }

    auto signature_comparer::operator()(field_signature const& lhs, field_signature const& rhs) const -> bool
    {
        if (!(*this)(lhs.type(), rhs.type()))
            return false;

        return true;
    }

    auto signature_comparer::operator()(method_signature const& lhs, method_signature const& rhs) const -> bool
    {
        if (lhs.calling_convention() != rhs.calling_convention())
            return false;

        if (lhs.has_this() != rhs.has_this())
            return false;

        if (lhs.has_explicit_this() != rhs.has_explicit_this())
            return false;

        if (lhs.is_generic() != rhs.is_generic())
            return false;

        if (lhs.generic_parameter_count() != rhs.generic_parameter_count())
            return false;

        // TODO Check assignable-to?  Shouldn't this always be the case for derived classes?

        // There is no need to check the parameter count explicitly; RangeCheckedEqual will do that.
        if (!core::range_checked_equal(
                lhs.begin_parameters(), lhs.end_parameters(),
                rhs.begin_parameters(), rhs.end_parameters(),
                *this))
            return false;

        if (!(*this)(lhs.return_type(), rhs.return_type()))
            return false;

        return true;
    }

    auto signature_comparer::operator()(property_signature const& lhs, property_signature const& rhs) const -> bool
    {
        if (lhs.has_this() != rhs.has_this())
            return false;

        if (!core::range_checked_equal(
                lhs.begin_parameters(), lhs.end_parameters(),
                rhs.begin_parameters(), rhs.end_parameters(),
                *this))
            return false;

        if (!(*this)(lhs.type(), rhs.type()))
            return false;

        return true;
    }

    auto signature_comparer::operator()(type_signature const& lhs, type_signature const& rhs) const -> bool
    {
        // TODO DO WE NEED TO CHECK CUSTOM MODIFIERS?

        if (lhs.get_kind() != rhs.get_kind())
            return false;

        if (lhs.get_kind() == type_signature::kind::unknown || rhs.get_kind() == type_signature::kind::unknown)
            return false;

        switch (lhs.get_kind())
        {
        case type_signature::kind::general_array:
        {
            if (!(*this)(lhs.array_type(), rhs.array_type()))
                return false;

            if (!(*this)(lhs.array_shape(), rhs.array_shape()))
                return false;

            return true;
        }

        case type_signature::kind::class_type:
        {
            if (lhs.is_class_type() != rhs.is_class_type())
                return false;

            if (!(*this)(lhs.class_type(), rhs.class_type()))
                return false;

            return true;
        }

        case type_signature::kind::function_pointer:
        {
            if (!(*this)(lhs.function_type(), rhs.function_type()))
                return false;

            return true;
        }

        case type_signature::kind::generic_instance:
        {
            if (lhs.is_generic_class_type_instance() != rhs.is_generic_class_type_instance())
                return false;

            if (lhs.generic_type() != rhs.generic_type())
                return false;

            if (lhs.generic_argument_count() != rhs.generic_argument_count())
                return false;

            if (!core::range_checked_equal(
                    lhs.begin_generic_arguments(), lhs.end_generic_arguments(),
                    rhs.begin_generic_arguments(), rhs.end_generic_arguments(),
                    *this))
                return false;

            return true;
        }

        case type_signature::kind::primitive:
        {
            if (lhs.primitive_type() != rhs.primitive_type())
                return false;

            return true;
        }

        case type_signature::kind::pointer:
        {
            if (!(*this)(lhs.pointer_type(), rhs.pointer_type()))
                return false;

            return true;
        }

        case type_signature::kind::simple_array:
        {
            if (!(*this)(lhs.array_type(), rhs.array_type()))
                return false;

            return true;
        }

        case type_signature::kind::variable:
        {
            if (lhs.is_class_variable() != rhs.is_class_variable())
                return false;

            if (lhs.variable_number() != rhs.variable_number())
                return false;

            return true;
        }

        default:
        {
            return false;
        }
        }
    }

    auto signature_comparer::operator()(type_def_ref_spec_token const& lhs, type_def_ref_spec_token const& rhs) const -> bool
    {
        type_def_spec_token const lhs_resolved(_resolver.get()->resolve_type(lhs));
        type_def_spec_token const rhs_resolved(_resolver.get()->resolve_type(rhs));

        // If the types are from different tables, they cannot be equal:
        if (lhs_resolved.table() != rhs_resolved.table())
            return false;

        // If we have a pair of TypeDefs, they are only equal if they refer to the same type in the
        // same database; in no other case can they be equal:
        if (lhs_resolved.table() == table_id::type_def)
        {
            if (lhs_resolved.scope() == rhs_resolved.scope() &&
                lhs_resolved.value() == rhs_resolved.value())
                return true;

            return false;
        }

        // Otherwise, we have a pair of TypeSpec tokens and we have to compare them recursively:
        blob const lhs_blob(row_from(lhs_resolved.as<type_spec_token>()).signature());
        blob const rhs_blob(row_from(rhs_resolved.as<type_spec_token>()).signature());

        return (*this)(
            type_signature(&lhs_resolved.scope(), begin(lhs_blob), end(lhs_blob)),
            type_signature(&rhs_resolved.scope(), begin(rhs_blob), end(rhs_blob)));
    }





    class_variable_signature_instantiator::class_variable_signature_instantiator()
    {
    }

    class_variable_signature_instantiator::class_variable_signature_instantiator(
        class_variable_signature_instantiator&& other)
        : _arguments          (std::move(other._arguments          )),
          _scope              (std::move(other._scope              )),
          _argument_signatures(std::move(other._argument_signatures)),
          _buffer             (std::move(other._buffer             ))
    {
        other._scope.get() = nullptr;
        core::assert_initialized(*this);
    }

    auto class_variable_signature_instantiator::operator=(
        class_variable_signature_instantiator&& other) -> class_variable_signature_instantiator&
    {
        _arguments           = std::move(other._arguments          );
        _scope               = std::move(other._scope              );
        _argument_signatures = std::move(other._argument_signatures);
        _buffer              = std::move(other._buffer             );

        other._scope.get() = nullptr;

        return *this;
    }

    auto class_variable_signature_instantiator::is_initialized() const -> bool
    {
        return _scope.get() != nullptr;
    }

    auto class_variable_signature_instantiator::has_arguments() const -> bool
    {
        // Note:  It is okay to call this even if we are not initialized.

        return !_arguments.empty();
    }

    template <typename Signature>
    auto class_variable_signature_instantiator::instantiate(Signature const& signature) const -> Signature
    {
        core::assert_initialized(*this);

        _buffer.clear();
        instantiate_into(_buffer, signature);
        return Signature(_scope.get(), _buffer.data(), _buffer.data() + _buffer.size());
    }

    template auto class_variable_signature_instantiator::instantiate(array_shape        const&) const -> array_shape;
    template auto class_variable_signature_instantiator::instantiate(field_signature    const&) const -> field_signature;
    template auto class_variable_signature_instantiator::instantiate(method_signature   const&) const -> method_signature;
    template auto class_variable_signature_instantiator::instantiate(property_signature const&) const -> property_signature;
    template auto class_variable_signature_instantiator::instantiate(type_signature     const&) const -> type_signature;

    template <typename Signature>
    auto class_variable_signature_instantiator::requires_instantiation(Signature const& signature) -> bool
    {
        // TODO Does this need to handle scope conversion?
        return requires_instantiation_internal(signature);
    }

    template auto class_variable_signature_instantiator::requires_instantiation(array_shape        const&) -> bool;
    template auto class_variable_signature_instantiator::requires_instantiation(field_signature    const&) -> bool;
    template auto class_variable_signature_instantiator::requires_instantiation(method_signature   const&) -> bool;
    template auto class_variable_signature_instantiator::requires_instantiation(property_signature const&) -> bool;
    template auto class_variable_signature_instantiator::requires_instantiation(type_signature     const&) -> bool;

    auto class_variable_signature_instantiator::instantiate_into(internal_buffer      & buffer,
                                                                 array_shape     const& s) const -> void
    {
        core::assert_initialized(*this);

        typedef array_shape::part part;

        copy_bytes_into(buffer, s, part::begin, part::end);
    }

    auto class_variable_signature_instantiator::instantiate_into(internal_buffer      & buffer,
                                                                 field_signature const& s) const -> void
    {
        core::assert_initialized(*this);

        typedef field_signature::part part;

        copy_bytes_into(buffer, s, part::begin, part::type);
        instantiate_into(buffer, s.type());
    }

    auto class_variable_signature_instantiator::instantiate_into(internal_buffer       & buffer,
                                                                 method_signature const& s) const -> void
    {
        core::assert_initialized(*this);

        typedef method_signature::part part;

        copy_bytes_into(buffer, s, part::begin, part::ret_type);
        instantiate_into(buffer, s.return_type());
        instantiate_range_into(buffer, s.begin_parameters(), s.end_parameters());

        if (s.begin_vararg_parameters() == s.end_vararg_parameters())
            return;

        copy_bytes_into(buffer, s, part::sentinel, part::first_vararg_param);
        instantiate_range_into(buffer, s.begin_vararg_parameters(), s.end_vararg_parameters());
    }

    auto class_variable_signature_instantiator::instantiate_into(internal_buffer         & buffer,
                                                                 property_signature const& s) const -> void
    {
        core::assert_initialized(*this);

        typedef property_signature::part part;

        copy_bytes_into(buffer, s, part::begin, part::type);
        instantiate_into(buffer, s.type());
        instantiate_range_into(buffer, s.begin_parameters(), s.end_parameters());
    }

    auto class_variable_signature_instantiator::instantiate_into(internal_buffer      & buffer,
                                                                 type_signature  const& s) const -> void
    {
        core::assert_initialized(*this);

        typedef type_signature::part part;

        switch (s.get_kind())
        {
        case type_signature::kind::primitive:
        {
            copy_bytes_into(buffer, s, part::begin, part::end);
            break;
        }
        case type_signature::kind::class_type:
        {
            if (s.is_cross_module_type_reference())
            {
                copy_bytes_into(buffer, s, part::begin, part::end);
            }
            else
            {
                copy_bytes_into(buffer, s, part::begin, part::type_code);
                buffer.push_back(static_cast<core::byte>(element_type::cross_module_type_reference));
                copy_bytes_into(buffer, s, part::type_code, part::end);
                std::copy(core::begin_bytes(_scope.get()),
                          core::end_bytes(_scope.get()),
                          std::back_inserter(buffer));
            }
            break;
        }
        case type_signature::kind::general_array:
        {
            copy_bytes_into(buffer, s, part::begin, part::general_array_type);
            instantiate_into(buffer, s.array_type());
            copy_bytes_into(buffer, s, part::general_array_shape, part::end);
            break;
        }
        case type_signature::kind::simple_array:
        {
            copy_bytes_into(buffer, s, part::begin, part::simple_array_type);
            instantiate_into(buffer, s.array_type());
            break;
        }
        case type_signature::kind::function_pointer:
        {
            copy_bytes_into(buffer, s, part::begin, part::function_pointer_type);
            instantiate_into(buffer, s.function_type());
            break;
        }
        case type_signature::kind::generic_instance:
        {
            copy_bytes_into(buffer, s, part::begin, part::first_generic_instance_argument);
            instantiate_range_into(buffer, s.begin_generic_arguments(), s.end_generic_arguments());
            break;
        }
        case type_signature::kind::pointer:
        {
            copy_bytes_into(buffer, s, part::begin, part::pointer_type);
            instantiate_into(buffer, s.pointer_type());
            break;
        }
        case type_signature::kind::variable:
        {
            if (_arguments.size() == 0)
            {
                // If there are no arguments, this instantiator is still being constructed.  During
                // construction, we instantiate each of the arguments to normalize all class type
                // signatures as cross-module type references.  Until this process is completed,
                // there are no arguments in the _arguments sequence.
                copy_bytes_into(buffer, s, part::begin, part::end);
            }
            else if (s.is_class_variable())
            {
                core::size_type const variable_number(s.variable_number());

                if (variable_number >= _arguments.size())
                    throw core::runtime_error(L"variable number out of range");

                copy_bytes_into(buffer, _arguments[variable_number], part::begin, part::end);
            }
            else if (s.is_method_variable())
            {
                copy_bytes_into(buffer, s, part::begin, part::end);
            }
            else
            {
                throw core::runtime_error(L"unknown variable type");
            }
            break;
        }
        default:
        {
            throw core::logic_error(L"not yet implemented");
        }
        }
    }

    template <typename Signature, typename Part>
    auto class_variable_signature_instantiator::copy_bytes_into(internal_buffer     & buffer,
                                                                Signature      const& s,
                                                                Part           const  first,
                                                                Part           const  last) const -> void
    {
        core::assert_initialized(*this);

        std::copy(s.seek_to(first), s.seek_to(last), std::back_inserter(buffer));
    }

    template <typename ForwardIterator>
    auto class_variable_signature_instantiator::instantiate_range_into(internal_buffer      & buffer,
                                                                       ForwardIterator const  first,
                                                                       ForwardIterator const  last) const -> void
    {
        core::assert_initialized(*this);

        std::for_each(first, last, [&](decltype(*first) s) { instantiate_into(buffer, s); });
    }

    auto class_variable_signature_instantiator::requires_instantiation_internal(array_shape const& s) -> bool
    {
        return false;
    }

    auto class_variable_signature_instantiator::requires_instantiation_internal(field_signature const& s) -> bool
    {
        return requires_instantiation_internal(s.type());
    }

    auto class_variable_signature_instantiator::requires_instantiation_internal(method_signature const& s) -> bool
    {
        if (requires_instantiation_internal(s.return_type()))
            return true;

        if (any_requires_instantiation_internal(s.begin_parameters(), s.end_parameters()))
            return true;

        if (any_requires_instantiation_internal(s.begin_vararg_parameters(), s.end_vararg_parameters()))
            return true;

        return false;
    }

    auto class_variable_signature_instantiator::requires_instantiation_internal(property_signature const& s) -> bool
    {
        if (requires_instantiation_internal(s.type()))
            return true;

        if (any_requires_instantiation_internal(s.begin_parameters(), s.end_parameters()))
            return true;

        return false;
    }

    auto class_variable_signature_instantiator::requires_instantiation_internal(type_signature const& s) -> bool
    {
        switch (s.get_kind())
        {
        case type_signature::kind::class_type:
        case type_signature::kind::primitive:
            return false;

        case type_signature::kind::general_array:
        case type_signature::kind::simple_array:
            return requires_instantiation_internal(s.array_type());
            
        case type_signature::kind::function_pointer:
            return requires_instantiation_internal(s.function_type());

        case type_signature::kind::generic_instance:
            return any_requires_instantiation_internal(s.begin_generic_arguments(), s.end_generic_arguments());
            
        case type_signature::kind::pointer:
            return requires_instantiation_internal(s.pointer_type());

        case type_signature::kind::variable:
            return s.is_class_variable();

        default:
            throw core::logic_error(L"not yet implemented");
        }
    }

    template <typename ForwardIterator>
    auto class_variable_signature_instantiator::any_requires_instantiation_internal(ForwardIterator const first,
                                                                                    ForwardIterator const last) -> bool
    {
        return std::any_of(first, last, [&](decltype(*first) s)
        {
            return requires_instantiation_internal(s);
        });
    }

} }

// AMDG //
