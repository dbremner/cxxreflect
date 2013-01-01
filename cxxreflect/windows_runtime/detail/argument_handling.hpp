
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_DETAIL_ARGUMENT_HANDLING_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_DETAIL_ARGUMENT_HANDLING_HPP_

#include "cxxreflect/windows_runtime/enumerator.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace cxxreflect { namespace windows_runtime { namespace detail {

    class unresolved_variant_argument
    {
    public:

        unresolved_variant_argument(metadata::element_type type,
                                    core::size_type        value_index,
                                    core::size_type        value_size,
                                    core::size_type        type_name_index,
                                    core::size_type        type_name_size);

        auto element_type()    const -> metadata::element_type;
        auto value_index()     const -> core::size_type;
        auto value_size()      const -> core::size_type;
        auto type_name_index() const -> core::size_type;
        auto type_name_size()  const -> core::size_type;
        
    private:

        core::value_initialized<metadata::element_type> _type;
        core::value_initialized<core::size_type>        _value_index;
        core::value_initialized<core::size_type>        _value_size;
        core::value_initialized<core::size_type>        _type_name_index;
        core::value_initialized<core::size_type>        _type_name_size;
    };





    class resolved_variant_argument
    {
    public:

        resolved_variant_argument(metadata::element_type    type,
                                  core::const_byte_iterator value_first,
                                  core::const_byte_iterator value_last,
                                  core::const_byte_iterator type_name_first,
                                  core::const_byte_iterator type_name_last);

        auto element_type() const -> metadata::element_type;
        auto logical_type() const -> reflection::type;

        auto begin_value()  const -> core::const_byte_iterator;
        auto end_value()    const -> core::const_byte_iterator;
        
        auto type_name()    const -> core::string_reference;

    private:

        core::value_initialized<metadata::element_type>    _type;
        core::value_initialized<core::const_byte_iterator> _value_first;
        core::value_initialized<core::const_byte_iterator> _value_last;
        core::value_initialized<core::const_byte_iterator> _type_name_first;
        core::value_initialized<core::const_byte_iterator> _type_name_last;
    };





    class inspectable_with_type_name
    {
    public:

        inspectable_with_type_name();
        inspectable_with_type_name(IInspectable* inspectable, core::string_reference type_name);

        auto inspectable() const -> IInspectable*;
        auto type_name()   const -> core::string_reference;

    private:

        core::value_initialized<IInspectable*> _inspectable;
        core::string                           _type_name;
    };





    class variant_argument_pack
    {
    public:

        typedef std::vector<unresolved_variant_argument>             unresolved_argument_sequence;
        typedef unresolved_argument_sequence::const_iterator         unresolved_argument_iterator;
        typedef unresolved_argument_sequence::const_reverse_iterator reverse_unresolved_argument_iterator;

        auto arity()  const -> core::size_type;

        auto begin()  const -> unresolved_argument_iterator;
        auto end()    const -> unresolved_argument_iterator;

        auto rbegin() const -> reverse_unresolved_argument_iterator;
        auto rend()   const -> reverse_unresolved_argument_iterator;

        auto resolve(unresolved_variant_argument const&) const -> resolved_variant_argument;

        auto push_argument(bool         ) -> void;
        auto push_argument(wchar_t      ) -> void;
        auto push_argument(std::int8_t  ) -> void;
        auto push_argument(std::uint8_t ) -> void;
        auto push_argument(std::int16_t ) -> void;
        auto push_argument(std::uint16_t) -> void;
        auto push_argument(std::int32_t ) -> void;
        auto push_argument(std::uint32_t) -> void;
        auto push_argument(std::int64_t ) -> void;
        auto push_argument(std::uint64_t) -> void;
        auto push_argument(float        ) -> void;
        auto push_argument(double       ) -> void;

        auto push_argument(inspectable_with_type_name const&) -> void;

        // TODO Add support for strings
        // TODO Add support for arbitrary value types

    private:

        auto push_argument(metadata::element_type    type,
                           core::const_byte_iterator first,
                           core::const_byte_iterator last) -> void;

        unresolved_argument_sequence _arguments;
        std::vector<core::byte>      _data;
    };

    using std::begin;
    using std::end;





    template <typename T>
    auto preprocess_argument(T&& value) -> T
    {
        return std::forward<T>(value);
    }

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW
    template <typename T>
    auto preprocess_argument(T^ const value) -> inspectable_with_type_name
    {
        return inspectable_with_type_name(
            reinterpret_cast<IInspectable*>(value),
            T::typeid->FullName->Data());
    }
    #endif





    template <typename P0>
    auto pack_arguments(P0&& a0)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        return pack;
    }

    template <typename P0, typename P1>
    auto pack_arguments(P0&& a0, P1&& a1)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        return pack;
    }

    template <typename P0, typename P1, typename P2>
    auto pack_arguments(P0&& a0, P1&& a1, P2&& a2)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        pack.push_argument(detail::preprocess_argument(std::forward<P2>(a2)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3>
    auto pack_arguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        pack.push_argument(detail::preprocess_argument(std::forward<P2>(a2)));
        pack.push_argument(detail::preprocess_argument(std::forward<P3>(a3)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    auto pack_arguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        pack.push_argument(detail::preprocess_argument(std::forward<P2>(a2)));
        pack.push_argument(detail::preprocess_argument(std::forward<P3>(a3)));
        pack.push_argument(detail::preprocess_argument(std::forward<P4>(a4)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5>
    auto pack_arguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4, P5&& a5)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        pack.push_argument(detail::preprocess_argument(std::forward<P2>(a2)));
        pack.push_argument(detail::preprocess_argument(std::forward<P3>(a3)));
        pack.push_argument(detail::preprocess_argument(std::forward<P4>(a4)));
        pack.push_argument(detail::preprocess_argument(std::forward<P5>(a5)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6>
    auto pack_arguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4, P5&& a5, P6&& a6)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        pack.push_argument(detail::preprocess_argument(std::forward<P2>(a2)));
        pack.push_argument(detail::preprocess_argument(std::forward<P3>(a3)));
        pack.push_argument(detail::preprocess_argument(std::forward<P4>(a4)));
        pack.push_argument(detail::preprocess_argument(std::forward<P5>(a5)));
        pack.push_argument(detail::preprocess_argument(std::forward<P6>(a6)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7>
    auto pack_arguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4, P5&& a5, P6&& a6, P7&& a7)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        pack.push_argument(detail::preprocess_argument(std::forward<P2>(a2)));
        pack.push_argument(detail::preprocess_argument(std::forward<P3>(a3)));
        pack.push_argument(detail::preprocess_argument(std::forward<P4>(a4)));
        pack.push_argument(detail::preprocess_argument(std::forward<P5>(a5)));
        pack.push_argument(detail::preprocess_argument(std::forward<P6>(a6)));
        pack.push_argument(detail::preprocess_argument(std::forward<P7>(a7)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7, typename P8>
    auto pack_arguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4, P5&& a5, P6&& a6, P7&& a7, P8&& a8)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        pack.push_argument(detail::preprocess_argument(std::forward<P2>(a2)));
        pack.push_argument(detail::preprocess_argument(std::forward<P3>(a3)));
        pack.push_argument(detail::preprocess_argument(std::forward<P4>(a4)));
        pack.push_argument(detail::preprocess_argument(std::forward<P5>(a5)));
        pack.push_argument(detail::preprocess_argument(std::forward<P6>(a6)));
        pack.push_argument(detail::preprocess_argument(std::forward<P7>(a7)));
        pack.push_argument(detail::preprocess_argument(std::forward<P8>(a8)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7, typename P8, typename P9>
    auto pack_arguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4, P5&& a5, P6&& a6, P7&& a7, P8&& a8, P9&& a9)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        pack.push_argument(detail::preprocess_argument(std::forward<P0>(a0)));
        pack.push_argument(detail::preprocess_argument(std::forward<P1>(a1)));
        pack.push_argument(detail::preprocess_argument(std::forward<P2>(a2)));
        pack.push_argument(detail::preprocess_argument(std::forward<P3>(a3)));
        pack.push_argument(detail::preprocess_argument(std::forward<P4>(a4)));
        pack.push_argument(detail::preprocess_argument(std::forward<P5>(a5)));
        pack.push_argument(detail::preprocess_argument(std::forward<P6>(a6)));
        pack.push_argument(detail::preprocess_argument(std::forward<P7>(a7)));
        pack.push_argument(detail::preprocess_argument(std::forward<P8>(a8)));
        pack.push_argument(detail::preprocess_argument(std::forward<P9>(a9)));
        return pack;
    }

    template <typename ForwardIterator>
    auto pack_argument_range(ForwardIterator const first_argument, ForwardIterator const last_argument)
        -> variant_argument_pack
    {
        variant_argument_pack pack;
        std::for_each(first_argument, last_argument, [&](decltype(*first_argument) a)
        {
            pack.push_argument(detail::preprocess_argument(a));
        });
        return pack;
    }





    auto convert_to_i4(resolved_variant_argument const& argument) -> std::int32_t; 
    auto convert_to_i8(resolved_variant_argument const& argument) -> std::int64_t;

    auto convert_to_u4(resolved_variant_argument const& argument) -> std::uint32_t; 
    auto convert_to_u8(resolved_variant_argument const& argument) -> std::uint64_t;

    auto convert_to_r4(resolved_variant_argument const& argument) -> float; 
    auto convert_to_r8(resolved_variant_argument const& argument) -> double;

    auto convert_to_interface(resolved_variant_argument const& argument,
                              reflection::guid          const& interface_guid) -> IInspectable*;

    template <typename T>
    auto reinterpret_as(resolved_variant_argument const& argument) -> T
    {
        if (core::distance(argument.begin_value(), argument.end_value()) != sizeof(T))
            throw core::logic_error(L"invalid reinterpretation target:  size does not match");

        // We pack arguments into the range of bytes without respecting alignment, so when we read
        // them back out we copy the bytes individually to ensure we aren't accessing an
        // insufficiently aligned object:
        T value;
        core::range_checked_copy(argument.begin_value(), argument.end_value(),
                                 core::begin_bytes(value), core::end_bytes(value));
        return value;
    }

    template <typename Target, typename Source>
    auto verify_in_range_and_convert_to(Source const& value) -> Target
    {
        // Only widening conversions are permitted, so this check should never fail:
        if (value < static_cast<Source>(std::numeric_limits<Target>::min()) ||
            value > static_cast<Source>(std::numeric_limits<Target>::max()))
            throw core::logic_error(L"unsupported conversion requested:  argument out of range");

        return static_cast<Target>(value);
    }


} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION

#endif 
