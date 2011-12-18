//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CUSTOMATTRIBUTE_HPP_
#define CXXREFLECT_CUSTOMATTRIBUTE_HPP_

#include "CxxReflect/CoreInternals.hpp"

namespace CxxReflect {

    class CustomAttribute
    {
    public:

        typedef void /* TODO */ PositionalArgumentIterator;
        typedef void /* TODO */ NamedArgumentIterator;

        CustomAttribute();

        Method GetConstructor() const;

        PositionalArgumentIterator BeginPositionalArguments() const;
        PositionalArgumentIterator EndPositionalArguments()   const;

        NamedArgumentIterator BeginNamedArguments() const;
        NamedArgumentIterator EndNamedArguments()   const;

    private:

    };

}

#endif
