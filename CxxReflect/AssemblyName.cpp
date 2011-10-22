//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/AssemblyName.hpp"

#include <cctype>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>

namespace CxxReflect {

    Version::Version(String const& version)
    {
        std::wistringstream iss(version.c_str());
        if (!(iss >> *this >> std::ws) || !iss.eof())
            throw std::logic_error("wtf");
    }

    std::wostream& operator<<(std::wostream& os, Version const& v)
    {
        os << v.GetMajor() << L'.' << v.GetMinor() << L'.' << v.GetBuild() << L'.' << v.GetRevision();
        return os;
    }

    std::wistream& operator>>(std::wistream& is, Version& v)
    {
        std::array<std::int16_t, 4> components = { 0 };
        unsigned componentsRead(0);
        bool TrueValueToSuppressC4127(true);
        while (TrueValueToSuppressC4127)
        {
            if (!(is >> components[componentsRead]))
                return is;

            ++componentsRead;
            if (componentsRead == 4 || is.peek() != L'.')
                break;

            is.ignore(1); // Ignore the . between numbers
        }

        v = Version(components[0], components[1], components[2], components[3]);
        return is;
    }

    std::wostream& operator<<(std::wostream& os, AssemblyName const& an)
    {
        os << an.GetFullName();
        return os;
    }

}
