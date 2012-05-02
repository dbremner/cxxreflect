#ifndef CXXREFLECT_STANDARDLIBRARY_HPP_
#define CXXREFLECT_STANDARDLIBRARY_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// These are all of the C++ Standard Library headers that are used in the library.  We include them
// all here to avoid repeating them everywhere (most of them are used in a few core headers anyway,
// so we wouldn't really help compilation time by including Standard Library headers more locally.

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
#include <new>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// The C++ Standard Library threading and synchronization headers cannot be included in C++/CLI
// translation units.  We can use these headers in CxxReflect translation units and in the Windows
// Runtime integration headers, but we cannot include them in the CxxReflect library's public
// interface headers.
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

    /// Represents an object size in the CxxReflect library
    ///
    /// CxxReflect deals almost exclusively with objects read out of PE binaries.  Because these
    /// binaries can be no larger than 4GB in size (and in practice are far smaller), we use a
    /// 32-bit unsigned integer to represent sizes.  This helps us to save some space when running
    /// in a 64-bit process.
    typedef std::uint32_t                                 SizeType;

    /// Represents a difference between two sizes, pointers, or iterators in the CxxReflect library.
    ///
    /// This is the signed type corresponding to the unsigned `SizeType`.
    typedef std::int32_t                                  DifferenceType;

}

#endif
