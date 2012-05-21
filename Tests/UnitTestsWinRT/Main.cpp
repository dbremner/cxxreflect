
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "Tests/UnitTests/Context.hpp"

#include "CxxReflect/CxxReflect.hpp"

int main(Platform::Array<Platform::String^>^ args)
{
    CxxReflect::WindowsRuntime::BeginInitialization();
    CxxReflectTest::Index::RunAllTests();
}
