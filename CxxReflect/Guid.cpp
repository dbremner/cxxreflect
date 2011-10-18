//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Guid.hpp"
#include "CxxReflect/Utility.hpp"

#include <algorithm>
#include <cstdio>
#include <sstream>

using CxxReflect::Utility::BeginBigEndianBytes;
using CxxReflect::Utility::EndBigEndianBytes;

namespace CxxReflect {

    Guid::Guid(String const& guid)
    {
        std::wistringstream iss(guid);
        if (!(iss >> *this >> std::ws) || !iss.eof())
            throw std::logic_error("wtf");
    }

    Guid::Guid(Element0 m0, Element1 m1, Element2 m2)
    {
        std::copy(BeginBigEndianBytes(m0), EndBigEndianBytes(m0), _data.data());
        std::copy(BeginBigEndianBytes(m1.data() + m1.size()),
                  EndBigEndianBytes(m1.data()),
                  _data.data() + 4);

        std::copy(m2.begin(), m2.end(), _data.data() + 8);
    }

    Guid::Guid(U4 m0, U2 m1a, U2 m1b, U1 m2a, U1 m2b, U1 m2c, U1 m2d, U1 m2e, U1 m2f, U1 m2g, U1 m2h)
    {
        std::copy(BeginBigEndianBytes(m0),  EndBigEndianBytes(m0),  _data.data());
        std::copy(BeginBigEndianBytes(m1a), EndBigEndianBytes(m1a), _data.data() + 4);
        std::copy(BeginBigEndianBytes(m1b), EndBigEndianBytes(m1b), _data.data() + 6);

        _data[0x8] = m2a; _data[0x9] = m2b; _data[0xA] = m2c; _data[0xB] = m2d;
        _data[0xC] = m2e; _data[0xD] = m2f; _data[0xE] = m2g; _data[0xF] = m2h;
    }

    std::wostream& operator<<(std::wostream& os, Guid const& x)
    {
        Guid::ByteArray const& bytes(x.AsByteArray());

        // 32 hexadecimal characters + 4 dashes + 1 null terminator = 37 characters
        std::array<wchar_t, 37> buffer = { 0 };
        if (std::swprintf(buffer.data(), buffer.size(),
                          L"%2x%2x%2x%2x-%2x%2x-%2x%2x-%2x%2x-%2x%2x%2x%2x%2x%2x", 
                          bytes[0x0], bytes[0x1], bytes[0x2], bytes[0x3],
                          bytes[0x4], bytes[0x5], bytes[0x6], bytes[0x7],
                          bytes[0x8], bytes[0x9], bytes[0xa], bytes[0xb],
                          bytes[0xc], bytes[0xd], bytes[0xe], bytes[0xf]) != 36)
        {
            throw std::logic_error("wtf");
        }
        os << buffer.data();
        return os;
    }

    std::wistream& operator>>(std::wistream& is, Guid& x)
    {
        // 32 hexadecimal characters + 4 dashes + 1 null terminator = 37 characters
        std::array<wchar_t, 36> buffer = { 0 };
        if (!is.read(buffer.data(), buffer.size()))
            return is;

        unsigned int m0(0), m1a(0), m1b(0),
                     m2a(0), m2b(0), m2c(0), m2d(0), m2e(0), m2f(0), m2g(0), m2h(0);

        #pragma warning(push)
        #pragma warning(disable: 4996) // Ignore "swscanf is unsafe" warning; our use is safe
        if (std::swscanf(buffer.data(),
                         L"%8x-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x",
                         &m0, &m1a, &m1b, &m2a, &m2b, &m2c, &m2d, &m2e, &m2f, &m2g, &m2h) != 11)
        {
            throw std::logic_error("wtf");
        }
        #pragma warning(pop)

        #pragma warning(push)
        #pragma warning(disable: 4244) // Ignore narrowing conversion warning; we know it's safe
        x = Guid(m0, m1a, m1b, m2a, m2b, m2c, m2d, m2e, m2f, m2g, m2h);
        #pragma warning(pop)
        return is;
    }

    Guid Guid::Empty(0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

}
