
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Context.hpp"

#include <iomanip>
#include <iostream>

namespace CxxReflectTest
{
    Index::TestRegistry& Index::GetOrCreateRegistry()
    {
        static TestRegistry registry;
        return registry;
    }

    int Index::RegisterTest(String const& name, TestFunction const& function)
    {
        if (!GetOrCreateRegistry().insert(std::make_pair(name, function)).second)
            throw TestError(L"Test name already registered");

        return 0;
    }

    void Index::RunAllTests()
    {
        typedef TestRegistry::const_reference Reference;

        TestRegistry const& registry(GetOrCreateRegistry());
        std::for_each(begin(registry), end(registry), [&](Reference test)
        {
            std::wcout << L"Running test [" << std::setw(40) << std::left << test.first << L"]:  ";
            Context context;
            try
            {
                test.second(context);
                std::wcout << L"PASSED" << std::endl;
            }
            catch (CxxReflectTest::TestError const&)
            {
                throw; // TODO Report error on console instead.
            }
        });
    }
}






