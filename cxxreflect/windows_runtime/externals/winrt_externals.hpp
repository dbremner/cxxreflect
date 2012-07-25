
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_EXTERNALS_WINRT_EXTERNALS_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_EXTERNALS_WINRT_EXTERNALS_HPP_

#include "cxxreflect/windows_runtime/common.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace cxxreflect { namespace externals {

    class winrt_externals : public base_win32_externals
    {
    public:

        auto compute_canonical_uri(wchar_t const* const path_or_uri) const -> core::string;

        auto file_exists(wchar_t const* const file_path) const -> bool;
    };

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 
