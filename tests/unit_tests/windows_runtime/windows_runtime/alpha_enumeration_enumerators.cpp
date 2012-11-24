
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/windows_runtime/precompiled_headers.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
    using namespace cxxreflect::windows_runtime;
}

namespace co
{
    using namespace TestComponents::Alpha;
}

namespace cxxreflect_test { namespace unit_tests_windows_runtime {

    CXXREFLECTTEST_DEFINE_TEST(alpha_enumeration_enumerators)
    {
        auto enumerators(cxr::get_enumerators(cxr::get_type(L"TestComponents.Alpha.DayOfWeek")));

        //The order of the enumerators is unspecified, so we'll sort them by value:
        std::sort(begin(enumerators), end(enumerators), cxr::enumerator_unsigned_value_less_than());

        std::map<std::wstring, unsigned> expected_values;
        expected_values.insert(std::make_pair(L"Sunday",    0));
        expected_values.insert(std::make_pair(L"Monday",    1));
        expected_values.insert(std::make_pair(L"Tuesday",   2));
        expected_values.insert(std::make_pair(L"Wednesday", 3));
        expected_values.insert(std::make_pair(L"Thursday",  4));
        expected_values.insert(std::make_pair(L"Friday",    5));
        expected_values.insert(std::make_pair(L"Saturday",  6));

        // Print the enumerators:
        std::for_each(begin(enumerators), end(enumerators), [&](cxr::enumerator const& e)
        {
            c.verify(expected_values.find(e.name().c_str()) != expected_values.end());
            c.verify_equals(expected_values[e.name().c_str()], e.unsigned_value());
        });
    }

} }
