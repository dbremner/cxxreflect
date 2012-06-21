
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_GUID_HPP_
#define CXXREFLECT_REFLECTION_GUID_HPP_

#include "cxxreflect/core/core.hpp"

namespace cxxreflect { namespace reflection {

    class guid
    {
    public:

        typedef std::uint32_t u4;
        typedef std::uint16_t u2;
        typedef std::uint8_t  u1;

        // Elem. 0   Elem. 1       Elem. 2
        // ~~~~~~~~ ~~~~~~~~~ ~~~~~~~~~~~~~~~~~
        // 00000000-0000-0000-0000-000000000000
        typedef std::uint32_t                element0;
        typedef std::array<std::uint16_t, 2> element1;
        typedef std::array<std::uint8_t,  8> element2;

        // We store and provide access to the GUID via a byte[16]
        typedef std::array<std::uint8_t, 16> byte_array;

        guid();
        guid(core::string const& guid);
        // TODO guid(element0 m0, element1 m1, element2 m2);
        guid(u4 m0, u2 m1a, u2 m1b, u1 m2a, u1 m2b, u1 m2c, u1 m2d, u1 m2e, u1 m2f, u1 m2g, u1 m2h);

        auto bytes() const -> byte_array const&;

        friend auto operator==(guid const&, guid const&) -> bool;
        friend auto operator< (guid const&, guid const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(guid)

        friend auto operator<<(core::output_stream&, guid const&) -> core::output_stream&;
        friend auto operator>>(core::input_stream&,  guid&)       -> core::input_stream&;

        static guid empty;

    private:

        core::value_initialized<byte_array> _data;
    };

    static_assert(sizeof(guid) == sizeof(guid::byte_array), "guid should have no unnamed padding bytes");

} }

#endif 

// AMDG //
