//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// All calls to platform functions, third-party functions, or non-standard library functions are
// encapsulated here in CxxReflect::Platform to make it uber-easy to replace said components with
// others.
#ifndef CXXREFLECT_PLATFORM_HPP_
#define CXXREFLECT_PLATFORM_HPP_

#include <array>
#include <cstdint>

namespace CxxReflect { namespace Platform {

    unsigned ComputeUtf16LengthOfUtf8String(char const* source);
    bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength);

    typedef std::array<std::uint8_t, 20> Sha1Hash;

    // Computes the 20 byte SHA1 hash for the bytes in the range [first, last).
    Sha1Hash ComputeSha1Hash(std::uint8_t const* first, std::uint8_t const* last);

} }

#endif
