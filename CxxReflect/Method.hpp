//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METHOD_HPP_
#define CXXREFLECT_METHOD_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

namespace CxxReflect {

    class Method
    {
    public:

        // Attributes
        // CallingConvention
        // ContainsGenericParameters
        // DeclaringType
        // IsAbstract
        // IsAssembly
        // IsConstructor
        // IsFamily
        // IsFamilyAndAssembly
        // IsFamilyOrAssembly
        // IsFinal
        // IsGenericMethod
        // IsGenericMethodDefinition
        // IsHideBySig
        // IsPrivate
        // IsPublic
        // IsSpecialName
        // IsStatic
        // IsVirtual
        // MemberType
        // MetadataToken
        // MethodHandle
        // Module
        // Name
        // ReflectedType
        // ReturnParameter            -- Non-constructor only
        // ReturnType                 -- Non-constructor only
        // ReturnTypeCustomAttributes -- Non-constructor only

        // GetBaseDefinition          -- Non-constructor only
        // GetCustomAttributes
        // GetGenericArguments
        // GetGenericMethodDefinition -- Non-constructor only
        // GetMethodBody
        // GetMethodImplementationFlags
        // GetParameters
        
        // IsDefined
        // MakeGenericMethod          -- Non-constructor only


        // -- The following members of System.Reflection.MethodInfo are not implemented --
        // IsSecurityCritical        } 
        // IsSecuritySafeCritical    } Security properties do not apply for reflection 
        // IsSecurityTransparent     }
        //
        // Invoke                    N/A in reflection-only

    private:

        CXXREFLECT_MAKE_NONCOPYABLE(Method);

    };

    bool operator==(Method const& lhs, Method const& rhs); // TODO
    bool operator< (Method const& lhs, Method const& rhs); // TODO

    inline bool operator!=(Method const& lhs, Method const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Method const& lhs, Method const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Method const& lhs, Method const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Method const& lhs, Method const& rhs) { return !(rhs <  lhs); }

}

#endif
