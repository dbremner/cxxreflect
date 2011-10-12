//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_GUID_HPP_
#define CXXREFLECT_GUID_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

#include <array>
#include <cstdint>
#include <iosfwd>

namespace CxxReflect {

    class Guid
    {
    public:

        typedef std::uint32_t U4;
        typedef std::uint16_t U2;
        typedef std::uint8_t  U1;

        // Elem. 0   Elem. 1        Elem.2
        // ~~~~~~~~ ~~~~~~~~~ ~~~~~~~~~~~~~~~~~
        // 00000000-0000-0000-0000-000000000000
        typedef std::uint32_t                Element0;
        typedef std::array<std::uint16_t, 2> Element1;
        typedef std::array<std::uint8_t,  8> Element2;

        // We store and provide access to the GUID via a byte[16]
        typedef std::array<std::uint8_t, 16> ByteArray;

        Guid(String const& guid);

        Guid(Element0 m0, Element1 m1, Element2 m2);

        Guid(U4 m0, U2 m1a, U2 m1b, U1 m2a, U1 m2b, U1 m2c, U1 m2d, U1 m2e, U1 m2f, U1 m2g, U1 m2h);

        ByteArray const& AsByteArray() const
        {
            return _data;
        }

    private:

        ByteArray _data;

        Element0 _m0;
        Element1 _m1;
        Element2 _m2;
    };

    inline bool operator==(Guid const& lhs, Guid const& rhs) { return lhs.AsByteArray() == rhs.AsByteArray(); }
    inline bool operator< (Guid const& lhs, Guid const& rhs) { return lhs.AsByteArray() <  rhs.AsByteArray(); }

    inline bool operator!=(Guid const& lhs, Guid const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Guid const& lhs, Guid const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Guid const& lhs, Guid const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Guid const& lhs, Guid const& rhs) { return !(rhs <  lhs); }

    std::wostream& operator<<(std::wostream&, Guid const& x);
    std::wistream& operator>>(std::wistream&, Guid& x);

}

#endif
