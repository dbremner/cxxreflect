
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_STANDARD_LIBRARY_HPP_
#define CXXREFLECT_CORE_STANDARD_LIBRARY_HPP_

#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <functional>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <new>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// We include our configuration after the Standard Library is included, to ensure that we pick up
// any macros that were defined by the library.
#include "cxxreflect/core/configuration.hpp"

namespace cxxreflect { namespace core {

    typedef std::uint8_t                                  byte;
    typedef byte*                                         byte_iterator;
    typedef byte const*                                   const_byte_iterator;

    typedef wchar_t                                       character;
    typedef wchar_t*                                      character_iterator;
    typedef wchar_t const*                                const_character_iterator;

    typedef std::basic_string<character>                  string;
    typedef std::basic_istream<character>                 input_stream;
    typedef std::basic_ostream<character>                 output_stream;

    typedef long                                          hresult;

    typedef std::uint32_t                                 size_type;
    typedef std::int32_t                                  difference_type;

    enum : size_type { max_size_type = static_cast<size_type>(-1) };

} }

#endif
