//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_PARAMETER_HPP_
#define CXXREFLECT_PARAMETER_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

namespace CxxReflect {

    class Parameter
    {
    public:

        // Attributes
        // DefaultValue
        // IsIn
        // IsLcid
        // IsOptional
        // IsOut
        // IsRetval
        // Member
        // MetadataToken
        // Name
        // ParameterType
        // Position
        // RawDefaultValue

        // GetCustomAttributes
        // GetOptionalCustomModifiers
        // GetRequiredCustomModifiers
        // IsDefined

        // -- The following members of System.Reflection.ParameterInfo are not implemented --

    private:

        CXXREFLECT_MAKE_NONCOPYABLE(Parameter);

    };

    bool operator==(Parameter const& lhs, Parameter const& rhs); // TODO
    bool operator< (Parameter const& lhs, Parameter const& rhs); // TODO

    inline bool operator!=(Parameter const& lhs, Parameter const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Parameter const& lhs, Parameter const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Parameter const& lhs, Parameter const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Parameter const& lhs, Parameter const& rhs) { return !(rhs <  lhs); }

}

#endif
