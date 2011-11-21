//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METHOD_HPP_
#define CXXREFLECT_METHOD_HPP_

#include "CxxReflect/Core.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    class Method
    {
    public:

        enum { InvalidVtableSlot = static_cast<SizeType>(-1) };

        Method() { }

        Method(Type const& declaringType, Type const& reflectedType, Metadata::TableReference const& method)
            : _declaringType(declaringType),
              _reflectedType(reflectedType),
              _method(method)
        {
            Detail::Verify([&]{ return declaringType != nullptr; });
            Detail::Verify([&]{ return reflectedType != nullptr; });
            Detail::Verify([&]{ return method.IsValid(); });
        }

        Type GetDeclaringType() const { return _declaringType; }
        Type GetReflectedType() const { return _reflectedType; }

        bool ContainsGenericParameters() const;

        MethodFlags GetAttributes() const;
        CallingConvention GetCallingConvention() const;
        SizeType GetMetadataToken() const;

        StringReference GetName() const;

        bool IsAbstract()                const;
        bool IsAssembly()                const;
        bool IsConstructor()             const;
        bool IsFamily()                  const;
        bool IsFamilyAndAssembly()       const;
        bool IsFamilyOrAssembly()        const;
        bool IsFinal()                   const;
        bool IsGenericMethod()           const;
        bool IsGenericMethodDefinition() const;
        bool IsHideBySig()               const;
        bool IsPrivate()                 const;
        bool IsPublic()                  const;
        bool IsSpecialName()             const;
        bool IsStatic()                  const;
        bool IsVirtual()                 const;

        // Module
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
        // MemberType                N/A with nonvirtual hierarchy
        // MethodHandle              N/A in reflection-only
        //
        // Invoke                    N/A in reflection-only

    private:

        Metadata::MethodDefRow GetMethodDefRow() const;

        Type                     _reflectedType;
        Type                     _declaringType;
        Metadata::TableReference _method;
    };

    //bool operator==(Method const& lhs, Method const& rhs); // TODO
    //bool operator< (Method const& lhs, Method const& rhs); // TODO

    /*inline bool operator!=(Method const& lhs, Method const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Method const& lhs, Method const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Method const& lhs, Method const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Method const& lhs, Method const& rhs) { return !(rhs <  lhs); }*/

}

#endif
