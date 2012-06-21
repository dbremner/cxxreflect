
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/guid.hpp"

namespace cxxreflect { namespace reflection {

    guid::guid()
    {
    }

    guid::guid(core::string const& guid)
    {
        std::wistringstream iss(guid.c_str());
        if (!(iss >> *this >> std::ws) || !iss.eof())
            throw core::runtime_error(L"failed to parse guid from string");
    }

    guid::guid(u4 const m0,
               u2 const m1a, u2 const m1b,
               u1 const m2a, u1 const m2b, u1 const m2c, u1 const m2d,
               u1 const m2e, u1 const m2f, u1 const m2g, u1 const m2h)
    {
        core::range_checked_copy(core::begin_bytes(m0),  core::end_bytes(m0),  begin(_data.get()),     end(_data.get()));
        core::range_checked_copy(core::begin_bytes(m1a), core::end_bytes(m1a), begin(_data.get()) + 4, end(_data.get()));
        core::range_checked_copy(core::begin_bytes(m1b), core::end_bytes(m1b), begin(_data.get()) + 6, end(_data.get()));

        _data.get()[0x8] = m2a; _data.get()[0x9] = m2b; _data.get()[0xA] = m2c; _data.get()[0xB] = m2d;
        _data.get()[0xC] = m2e; _data.get()[0xD] = m2f; _data.get()[0xE] = m2g; _data.get()[0xF] = m2h;
    }

    auto guid::bytes() const -> byte_array const&
    {
        return _data.get();
    }

    auto operator==(guid const& lhs, guid const& rhs) -> bool
    {
        return lhs._data.get() == rhs._data.get();
    }

    auto operator<(guid const& lhs, guid const& rhs) -> bool
    {
        return lhs._data.get() < rhs._data.get();
    }

    auto operator<<(core::output_stream& os, guid const& x) -> core::output_stream&
    {
        guid::byte_array const& bytes(x.bytes());

        // 32 hexadecimal characters + 4 dashes + 1 null terminator = 37 characters
        std::array<wchar_t, 37> buffer = { { 0 } };
        if (std::swprintf(buffer.data(), buffer.size(),
                     L"%2x%2x%2x%2x-%2x%2x-%2x%2x-%2x%2x-%2x%2x%2x%2x%2x%2x", 
                     bytes[0x0], bytes[0x1], bytes[0x2], bytes[0x3],
                     bytes[0x4], bytes[0x5], bytes[0x6], bytes[0x7],
                     bytes[0x8], bytes[0x9], bytes[0xa], bytes[0xb],
                     bytes[0xc], bytes[0xd], bytes[0xe], bytes[0xf]) != 36)
        {
            throw core::runtime_error(L"failed to parse guid");
        }

        os << buffer.data();
        return os;
    }

    auto operator>>(core::input_stream& is, guid& x) -> core::input_stream&
    {
        // 32 hexadecimal characters + 4 dashes + 1 null terminator = 37 characters
        std::array<wchar_t, 36> buffer = { { 0 } };
        if (!is.read(buffer.data(), buffer.size()))
            return is;

        unsigned int m0(0), m1a(0), m1b(0),
                     m2a(0), m2b(0), m2c(0), m2d(0), m2e(0), m2f(0), m2g(0), m2h(0);

        // Ignore "swscanf is unsafe" warning; our use is safe
        // Ignore narrowing conversion warning; we know it's safe
        #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP
        #    pragma warning(push)
        #    pragma warning(disable: 4996 4244) 
        #endif

        if (std::swscanf(buffer.data(),
                         L"%8x-%4x-%4x-%2x%2x-%2x%2x%2x%2x%2x%2x",
                         &m0, &m1a, &m1b, &m2a, &m2b, &m2c, &m2d, &m2e, &m2f, &m2g, &m2h) != 11)
        {
            throw core::runtime_error(L"failed to parse guid");
        }
        x = guid(m0, m1a, m1b, m2a, m2b, m2c, m2d, m2e, m2f, m2g, m2h);

        #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP
        #    pragma warning(pop)
        #endif

        return is;
    }

    guid guid::empty(0x00000000, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

} }

// AMDG //
