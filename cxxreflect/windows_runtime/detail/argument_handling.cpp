
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/inspection.hpp"
#include "cxxreflect/windows_runtime/loader.hpp"
#include "cxxreflect/windows_runtime/utility.hpp"
#include "cxxreflect/windows_runtime/detail/overload_resolution.hpp"
#include "cxxreflect/windows_runtime/detail/runtime_utility.hpp"

#include <inspectable.h>

using namespace Microsoft::WRL;

namespace cxxreflect { namespace windows_runtime { namespace detail {

    unresolved_variant_argument::unresolved_variant_argument(metadata::element_type const type,
                                                             core::size_type        const value_index,
                                                             core::size_type        const value_size,
                                                             core::size_type        const type_name_index,
                                                             core::size_type        const type_name_size)
        : _type           (type           ),
          _value_index    (value_index    ),
          _value_size     (value_size     ),
          _type_name_index(type_name_index),
          _type_name_size (type_name_size )
    {
    }

    auto unresolved_variant_argument::element_type() const -> metadata::element_type
    {
        return _type.get();
    }

    auto unresolved_variant_argument::value_index() const -> core::size_type
    {
        return _value_index.get();
    }

    auto unresolved_variant_argument::value_size() const -> core::size_type
    {
        return _value_size.get();
    }

    auto unresolved_variant_argument::type_name_index() const -> core::size_type
    {
        return _type_name_index.get();
    }

    auto unresolved_variant_argument::type_name_size() const -> core::size_type
    {
        return _type_name_size.get();
    }





    resolved_variant_argument::resolved_variant_argument(metadata::element_type    const type,
                                                         core::const_byte_iterator const value_first,
                                                         core::const_byte_iterator const value_last,
                                                         core::const_byte_iterator const type_name_first,
                                                         core::const_byte_iterator const type_name_last)
        : _type           (type           ),
          _value_first    (value_first    ),
          _value_last     (value_last     ),
          _type_name_first(type_name_first),
          _type_name_last (type_name_last )
    {
    }

    auto resolved_variant_argument::element_type() const -> metadata::element_type
    {
        return _type.get();
    }

    auto resolved_variant_argument::logical_type() const -> reflection::type
    {
        if (_type.get() == metadata::element_type::class_type)
        {
            // First, we see if we have a known type name (i.e. type_name() returns a string).  If
            // we have one, we use that to get the type of the argument.
            core::string_reference const known_type_name(type_name());
            if (!known_type_name.empty())
            {
                reflection::type const type(get_type(known_type_name));

                // If the static type of the object was Platform::Object, we'll instead try to use
                // its dynamic type for overload resolution:
                if (type && type != get_type(L"Platform", L"Object"))
                    return type;
            }

            // Otherwise, see if we can get the type from the IInspectable argument:
            core::assert_true([&]{ return sizeof(IInspectable*) == core::distance(begin_value(), end_value()); });

            IInspectable* value(nullptr);
            core::range_checked_copy(begin_value(), end_value(), core::begin_bytes(value), core::end_bytes(value));

            // If we have an IInspectable object, try to get its runtime class name:
            if (value != nullptr)
            {
                utility::smart_hstring inspectable_type_name;
                utility::throw_on_failure(value->GetRuntimeClassName(inspectable_type_name.proxy()));

                reflection::type const type(get_type(inspectable_type_name.c_str()));
                if (type)
                    return type;
            }

            // TODO For nullptr, we should probably allow conversion to any interface with equal
            // conversion rank.  How to do this cleanly, though, is a good question.

            // Finally, fall back to use Platform::Object:
            reflection::type const type(get_type(L"Platform", L"Object"));
            if (type)
                return type;

            // Well, that was our last check; if we still failed to get the type, it's time to throw
            throw core::logic_error(L"failed to find type");
        }
        else if (_type.get() == metadata::element_type::value_type)
        {
            core::assert_not_yet_implemented();
        }
        else
        {
            reflection::detail::loader_context const& root(global_package_loader::get()
                .loader()
                .context(core::internal_key()));

            return reflection::type(root.resolve_fundamental_type(_type.get()), core::internal_key());
        }
    }

    auto resolved_variant_argument::begin_value() const -> core::const_byte_iterator
    {
        return _value_first.get();
    }

    auto resolved_variant_argument::end_value() const -> core::const_byte_iterator
    {
        return _value_last.get();
    }
        
    auto resolved_variant_argument::type_name() const -> core::string_reference
    {
        if (_type_name_first.get() == _type_name_last.get())
            return core::string_reference();

        return core::string_reference(
            reinterpret_cast<core::const_character_iterator>(_type_name_first.get()),
            reinterpret_cast<core::const_character_iterator>(_type_name_last.get()));
    }





    inspectable_with_type_name::inspectable_with_type_name()
    {
    }

    inspectable_with_type_name::inspectable_with_type_name(IInspectable*          const inspectable,
                                                           core::string_reference const type_name)
        : _inspectable(inspectable), _type_name(type_name.c_str())
    {
    }

    auto inspectable_with_type_name::inspectable() const -> IInspectable*
    {
        return _inspectable.get();
    }

    auto inspectable_with_type_name::type_name() const -> core::string_reference
    {
        return _type_name.c_str();
    }





    auto variant_argument_pack::arity() const -> core::size_type
    {
        return core::convert_integer(_arguments.size());
    }

    auto variant_argument_pack::begin() const -> unresolved_argument_iterator
    {
        return _arguments.begin();
    }

    auto variant_argument_pack::end() const -> unresolved_argument_iterator
    {
        return _arguments.end();
    }

    auto variant_argument_pack::rbegin() const -> reverse_unresolved_argument_iterator
    {
        return _arguments.rbegin();
    }

    auto variant_argument_pack::rend() const -> reverse_unresolved_argument_iterator
    {
        return _arguments.rend();
    }

    auto variant_argument_pack::resolve(unresolved_variant_argument const& argument) const
        -> resolved_variant_argument
    {
        core::const_byte_iterator const type_name_first(argument.type_name_size() != 0
            ? _data.data() + argument.type_name_index()
            : nullptr);

        core::const_byte_iterator const type_name_last(argument.type_name_size() != 0
            ? _data.data() + argument.type_name_index() + argument.type_name_size()
            : nullptr);

        return resolved_variant_argument(
            argument.element_type(),
            _data.data() + argument.value_index(),
            _data.data() + argument.value_index() + argument.value_size(),
            type_name_first,
            type_name_last);
    }

    auto variant_argument_pack::push_argument(bool const value) -> void
    {
        push_argument(metadata::element_type::boolean, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(wchar_t const value) -> void
    {
        push_argument(metadata::element_type::character, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(std::int8_t const value) -> void
    {
        push_argument(metadata::element_type::i1, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(std::uint8_t const value) -> void
    {
        push_argument(metadata::element_type::u1, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(std::int16_t const value) -> void
    {
        push_argument(metadata::element_type::i2, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(std::uint16_t const value) -> void
    {
        push_argument(metadata::element_type::u2, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(std::int32_t const value) -> void
    {
        push_argument(metadata::element_type::i4, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(std::uint32_t const value) -> void
    {
        push_argument(metadata::element_type::u4, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(std::int64_t const value) -> void
    {
        push_argument(metadata::element_type::i8, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(std::uint64_t const value) -> void
    {
        push_argument(metadata::element_type::u8, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(float const value) -> void
    {
        push_argument(metadata::element_type::r4, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(double const value) -> void
    {
        push_argument(metadata::element_type::r8, core::begin_bytes(value), core::end_bytes(value));
    }

    auto variant_argument_pack::push_argument(inspectable_with_type_name const& argument) -> void
    {
        IInspectable* const value(argument.inspectable());

        core::size_type const value_index(core::convert_integer(_data.size()));
        std::copy(core::begin_bytes(value), core::end_bytes(value), std::back_inserter(_data));

        core::size_type const name_index(core::convert_integer(_data.size()));
        std::copy(reinterpret_cast<core::const_byte_iterator>(argument.type_name().begin()),
                  reinterpret_cast<core::const_byte_iterator>(argument.type_name().end()),
                  std::back_inserter(_data));

        // Null-terminate the buffer:
        _data.resize(_data.size() + sizeof(wchar_t));

        _arguments.push_back(unresolved_variant_argument(
            metadata::element_type::class_type,
            value_index,
            core::convert_integer(sizeof(value)),
            name_index,
            core::convert_integer((argument.type_name().size() + 1) * sizeof(wchar_t))));
    }

    auto variant_argument_pack::push_argument(metadata::element_type    const type,
                                              core::const_byte_iterator const first,
                                              core::const_byte_iterator const last) -> void
    {
        core::size_type const index(core::convert_integer(_data.size()));

        std::copy(first, last, std::back_inserter(_data));
        _arguments.push_back(unresolved_variant_argument(type, index, core::distance(first, last), 0, 0));
    }





    auto convert_to_i4(resolved_variant_argument const& argument) -> std::int32_t
    {
        return verify_in_range_and_convert_to<std::int32_t>(convert_to_i8(argument));
    }

    auto convert_to_i8(resolved_variant_argument const& argument) -> std::int64_t
    {
        switch (argument.element_type())
        {
        case metadata::element_type::i1: return reinterpret_as<std::int8_t >(argument);
        case metadata::element_type::i2: return reinterpret_as<std::int16_t>(argument);
        case metadata::element_type::i4: return reinterpret_as<std::int32_t>(argument);
        case metadata::element_type::i8: return reinterpret_as<std::int64_t>(argument);
        default: throw core::logic_error(L"unsupported conversion requested");
        }
    }

    auto convert_to_u4(resolved_variant_argument const& argument) -> std::uint32_t
    {
        return verify_in_range_and_convert_to<std::uint32_t>(convert_to_u8(argument));
    }

    auto convert_to_u8(resolved_variant_argument const& argument) -> std::uint64_t
    {
        switch (argument.element_type())
        {
        case metadata::element_type::u1: return reinterpret_as<std::uint8_t >(argument);
        case metadata::element_type::u2: return reinterpret_as<std::uint16_t>(argument);
        case metadata::element_type::u4: return reinterpret_as<std::uint32_t>(argument);
        case metadata::element_type::u8: return reinterpret_as<std::uint64_t>(argument);
        default: throw core::logic_error(L"unsupported conversion requested");
        }
    }

    auto convert_to_r4(resolved_variant_argument const& argument) -> float
    {
        return verify_in_range_and_convert_to<float>(convert_to_r8(argument));
    }

    auto convert_to_r8(resolved_variant_argument const& argument) -> double
    {
        switch (argument.element_type())
        {
        case metadata::element_type::r4: return reinterpret_as<float >(argument);
        case metadata::element_type::r8: return reinterpret_as<double>(argument);
        default: throw core::logic_error(L"unsupported conversion requested");
        }
    }

    auto convert_to_interface(resolved_variant_argument const& argument,
                              reflection::guid          const& interface_guid) -> IInspectable*
    {
        if (argument.element_type() != metadata::element_type::class_type)
            throw core::logic_error(L"invalid source argument:  argument must be a runtime class");

        IInspectable* const inspectable_object(reinterpret_as<IInspectable*>(argument));

        // A nullptr argument is valid:
        if (inspectable_object == nullptr)
            return nullptr;

        ComPtr<IInspectable> inspectable_interface;
        core::hresult const hr(inspectable_object->QueryInterface(
            to_com_guid(interface_guid),
            reinterpret_cast<void**>(inspectable_interface.ReleaseAndGetAddressOf())));

        if (FAILED(hr))
            throw core::logic_error(L"unsupported conversion requested:  interface not implemented");

        // Reference counting note:  our reference to the QI'ed interface pointer will be
        // released when this function returns.  In order for this to work, we rely on there
        // being One True IUnknown for the runtime object.  We are relying on the upstream
        // caller to keep a reference to the inspectable runtime object so that it is not
        // destroyed prematurely.
        return inspectable_interface.Get();
    }

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
