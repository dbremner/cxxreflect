
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "test/unit_tests/test_driver.hpp"
#include "cxxreflect/cxxreflect.hpp"
#include "cxxreflect/windows_runtime/detail/runtime_utility.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
}

namespace cxxreflect_test { namespace {

    auto verify_compute_sha1_hash(context const& c) -> void
    {
        namespace cxrd = cxxreflect::windows_runtime::detail;

        std::array<cxr::byte, 16> const source_data =
        {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        };

        cxr::sha1_hash const expected_hash =
        {
            0xf7, 0x4f, 0x9f, 0x3f, 0x81, 0x83, 0x1c, 0xe1, 0xac, 0x33,
            0x99, 0x6e, 0x89, 0xe0, 0x34, 0x19, 0x56, 0x5c, 0x7a, 0xb7
        };

        cxr::sha1_hash const actual_hash(cxrd::compute_sha1_hash(source_data.data(), source_data.data() + source_data.size()));

        c.verify_equals(actual_hash, expected_hash);
    }

    CXXREFLECTTEST_REGISTER(runtime_utility_compute_sha1_hash, verify_compute_sha1_hash);

} }
