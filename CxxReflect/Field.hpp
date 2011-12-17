//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_FIELD_HPP_
#define CXXREFLECT_FIELD_HPP_

#include "CxxReflect/Core.hpp"

namespace CxxReflect {

    class Field
    {
    public:

        // Attributes
        // DeclaringType
        // FieldHandle
        // FieldType
        // IsAssembly
        // IsFamily
        // IsFamilyAndAssembly
        // IsFamilyOrAssembly
        // IsInitOnly
        // IsLiteral
        // IsNotSerialized
        // IsPinvokeImpl
        // IsPrivate
        // IsPublic
        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent
        // IsSpecialName
        // IsStatic
        // MemberType
        // MetadataToken
        // Module
        // Name
        // ReflectedType

        // GetCustomAttributes
        // GetOptionalCustomModifiers
        // GetRawConstantValue
        // GetRequiredCustomModifiers
        // GetValue
        // GetValueDirect
        // IsDefined
        // SetValue
        // SetValueDirect

    private:

    };

    bool operator==(Field const& lhs, Field const& rhs); // TODO
    bool operator< (Field const& lhs, Field const& rhs); // TODO

    inline bool operator!=(Field const& lhs, Field const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Field const& lhs, Field const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Field const& lhs, Field const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Field const& lhs, Field const& rhs) { return !(rhs <  lhs); }

}

#endif
