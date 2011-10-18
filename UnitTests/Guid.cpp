//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Guid.hpp"
#include "CxxReflect/Utility.hpp"

#include "UnitTests/Index.hpp"

namespace CxxReflectTest {

    void Index::VerifyGuid()
    {
        using CxxReflect::Guid;
        using CxxReflect::String;

        Guid g(0x12345678, 0x90ab, 0xcdef, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef);

        String s(CxxReflect::Utility::ToString(g));

        Guid h(s);

        std::wistringstream oss(s);
        if (!(oss >> g >> std::ws) || !oss.eof())
            throw std::logic_error("wtf");

        String t(oss.str());

        bool seq(s == t);
        bool geq(g == h);

    }

}
