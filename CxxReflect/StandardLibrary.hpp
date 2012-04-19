#ifndef CXXREFLECT_STANDARDLIBRARY_HPP_
#define CXXREFLECT_STANDARDLIBRARY_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// The Standard Library headers that are used by the library.

#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// The C++ Standard Library thread and atomic headers are not supported by C++/CLI
#ifndef __cplusplus_cli

#include <atomic>
#include <future>
#include <mutex>
#include <thread>

#endif

namespace CxxReflect {

    typedef std::uint8_t                                  Byte;
    typedef Byte*                                         ByteIterator;
    typedef Byte const*                                   ConstByteIterator;
    typedef std::reverse_iterator<ByteIterator>           ReverseByteIterator;
    typedef std::reverse_iterator<ConstByteIterator>      ConstReverseByteIterator;

    typedef wchar_t                                       Character;
    typedef wchar_t*                                      CharacterIterator;
    typedef wchar_t const*                                ConstCharacterIterator;
    typedef std::reverse_iterator<CharacterIterator>      ReverseCharacterIterator;
    typedef std::reverse_iterator<ConstCharacterIterator> ConstReverseCharacterIterator;

    typedef std::basic_string<wchar_t>                    String;
    typedef std::basic_istream<wchar_t>                   InputStream;
    typedef std::wostringstream                           OutputStream;

    typedef std::basic_string<char>                       NarrowString;
    typedef std::basic_istream<char>                      NarrowInputStream;
    typedef std::basic_ostream<char>                      NarrowOutputStream;

    typedef long                                          HResult;

    typedef std::uint32_t                                 SizeType;
    typedef std::int32_t                                  DifferenceType;

}

#endif
