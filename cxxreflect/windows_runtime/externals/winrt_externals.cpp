
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/detail/runtime_utility.hpp"
#include "cxxreflect/windows_runtime/externals/winrt_externals.hpp"

namespace cxxreflect { namespace externals {

    auto winrt_externals::compute_canonical_uri(wchar_t const* const path_or_uri) const -> core::string
    {
        return windows_runtime::detail::compute_canonical_uri(path_or_uri);
    }

    auto winrt_externals::file_exists(wchar_t const* const file_path) const -> bool
    {
        FILE* handle(nullptr);

        errno_t const error(_wfopen_s(&handle, file_path, L"rb"));
        if (error == 0)
        {
            std::fclose(handle);
            return true;
        }
        else
        {
            return false;
        }
    }

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION

// AMDG //
