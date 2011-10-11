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
        Version v;
        std::wistringstream iss(version);
        if (!(iss >> v >> std::ws) || !iss.eof())
            throw std::logic_error("wtf");
        
        _major = v.GetMajor();
        _minor = v.GetMinor();
        _build = v.GetBuild();
        _revision = v.GetRevision();
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

    AssemblyName::AssemblyName(String const& name)
    {
        std::map<String, String> components;

        bool isReadingName(false);
        String componentName(L"Name");
        String componentValue;

        // TODO Unicode and capitalization
        for (auto it(name.begin()); it != name.end(); ++it)
        {
            if (std::isspace(*it))
                continue;

            if (*it == L'=')
            {
                if (isReadingName)
                {
                    isReadingName = false;
                }
                else
                {
                    throw std::logic_error("wtf");
                }

                continue;
            }

            if (*it == L',')
            {
                if (isReadingName)
                {
                    throw std::logic_error("wtf");
                }
                else
                {
                    components.insert(std::make_pair(componentName, componentValue));
                    componentName = L"";
                    componentValue = L"";
                    isReadingName = true;
                }
            }

            if (isReadingName)
            {
                componentName.push_back(*it);
            }
            else
            {
                componentValue.push_back(*it);
            }
        }

        components.insert(std::make_pair(componentName, componentValue));

        auto const nameIt(components.find(L"Name"));
        _simpleName = nameIt != components.end() ? nameIt->second : L"";

        // TODO auto const versionIt(components.find(L"Version"));
        // TODO _version = versionIt != components.end() ? Version(versionIt->second) : L"";

        auto const cultureIt(components.find(L"Culture"));
        _cultureInfo = cultureIt != components.end() ? cultureIt->second : L"";
        
        // TODO PublicKeyToken
    }

    String AssemblyName::GetFullName() const
    {
        std::wostringstream oss;
        oss << _simpleName << L", Version=" << _version;

        if (!_cultureInfo.empty())
        {
            oss << L", Culture=" << _cultureInfo;
        }

        std::size_t const publicKeyZeroCount(std::count(_publicKeyToken.begin(), _publicKeyToken.end(), 0));
        if (publicKeyZeroCount != _publicKeyToken.size())
        {
            oss << L", PublicKeyToken=";
            std::for_each(_publicKeyToken.begin(), _publicKeyToken.end(), [&](std::uint8_t x)
            {
                std::array<wchar_t, 3> buffer;
                std::swprintf(buffer.data(), buffer.size(), L"%x", x);
                oss << buffer.data();
            });
        }

        return oss.str();
    }

    std::wostream& operator<<(std::wostream& os, AssemblyName const& an)
    {
        os << an.GetFullName();
        return os;
    }

}
