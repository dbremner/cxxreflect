//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_PROPERTY_HPP_
#define CXXREFLECT_PROPERTY_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

namespace CxxReflect {

    class Property
    {
    public:

        // Attributes
        // CanRead
        // CanWrite
        // DeclaringType
        // IsSpecialName
        // MemberType
        // MetadataToken
        // Module
        // Name
        // PropertyType
        // ReflectedType

        // GetAccessors
        // GetConstantValue
        // GetCustomAttributes
        // GetGetMethod
        // GetIndexParameters
        // GetOptionalCustomModifiers
        // GetRawConstantValue
        // GetRequiredCustomModifiers
        // GetSetMethod
        // GetValue
        // IsDefined
        // SetValue

        // -- The following members of System.Reflection.PropertyInfo are not implemented --

    private:

        CXXREFLECT_NONCOPYABLE(Property);

    };

    bool operator==(Property const& lhs, Property const& rhs); // TODO
    bool operator< (Property const& lhs, Property const& rhs); // TODO

    inline bool operator!=(Property const& lhs, Property const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Property const& lhs, Property const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Property const& lhs, Property const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Property const& lhs, Property const& rhs) { return !(rhs <  lhs); }

}

#endif
