//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_TYPE_HPP_
#define CXXREFLECT_TYPE_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

namespace CxxReflect {

    class Type
    {
    public:

        // The token must be a TypeDef, TypeRef, or TypeSpec.  The assembly must be the assembly
        // that we can use to resolve properties of the type.
        Type(AssemblyHandle assembly, MetadataToken token);

        AssemblyHandle GetAssembly() const { return _assembly; }

        // A type by any other name would be something else...
        String GetAssemblyQualifiedName() const;
        String GetName()                  const;
        String GetNamespace()             const;
        String GetFullName()              const;

        // The base type of this type, or null if the type has no base type
        TypeHandle    GetBaseType()        const { return PrivateGetBaseType();      }
        TypeHandle    GetDeclaringType()   const { return PrivateGetDeclaringType(); }
        MethodHandle  GetDeclaringMethod() const; // TODO

        MetadataToken GetMetadataToken()   const { return _originalToken;            }

        // ContainsGenericParameters
        // GenericParameterAttributes
        // GenericParameterPosition
        // GUID

        std::uint32_t GetAttributes()    const { return PrivateGetTypeFlags(); }
        
        bool HasElementType()            const;

        bool IsAbstract()                const;
        bool IsAnsiClass()               const;
        bool IsArray()                   const;
        bool IsAutoClass()               const;
        bool IsAutoLayout()              const;
        bool IsByRef()                   const;
        bool IsClass()                   const;
        bool IsCOMObject()               const;

        bool IsContextful()              const;
        bool IsEnum()                    const;
        bool IsExplicitLayout()          const;
        bool IsGenericParameter()        const;
        bool IsGenericType()             const;
        bool IsGenericTypeDefinition()   const;
        bool IsImport()                  const;
        bool IsInterface()               const;
        
        bool IsLayoutSequential()        const;
        bool IsMarshalByRef()            const;
        bool IsNested()                  const;
        bool IsNestedAssembly()          const;
        bool IsNestedFamilyAndAssembly() const;
        bool IsNestedFamily()            const;
        bool IsNestedFamilyOrAssembly()  const;
        bool IsNestedPrivate()           const;
        
        bool IsNestedPublic()            const;
        bool IsNotPublic()               const;
        bool IsPointer()                 const;
        bool IsPrimitive()               const;
        bool IsPublic()                  const;
        bool IsSealed()                  const;
        bool IsSerializable()            const;
        bool IsSpecialName()             const;

        bool IsUnicodeClass()            const;
        bool IsValueType()               const;
        bool IsVisible()                 const;
        
        // Module
        // ReflectedType
        // StructLayoutAttribute
        // TypeInitializer
        // UnderlyingSystemType

        // FindInterfaces
        // FindMembers
        // GetArrayRank
        // GetConstructor
        // GetConstructors
        // GetCustomAttributes
        // GetCustomAttributesData
        // GetDefaultMembers
        // GetElementType
        // GetEnumName
        // GetEnumNames
        // GetEnumUnderlyingType
        // GetEnumValues
        // GetEvent
        // GetEvents
        // GetField
        // GetFields
        // GetGenericArguments
        // GetGenericParameterConstraints
        // GetGenericTypeDefinition
        // GetInterface
        // GetInterfaceMap
        // GetInterfaces
        // GetMember
        // GetMembers
        // GetMethod
        // GetMethods
        // GetNestedType
        // GetNestedTypes
        // GetProperties
        // GetProperty
        
        // IsAssignableFrom
        // IsDefined
        // IsEnumDefined
        // IsEquivalentTo
        // IsInstanceOfType
        // IsSubclassOf

        // MakeArrayType
        // MakeByRefType
        // MakeGenericType
        // MakePointerType

        // -- The following members of System.Type are not implemented --
        // DefaultBinder             Requires runtime
        // IsSecurityCritical        } 
        // IsSecuritySafeCritical    } Security properties do not apply for reflection 
        // IsSecurityTransparent     }
        // MemberType                We always know that this is a Type object
        // TypeHandle                No meaning outside of the CLR
        //
        // InvokeMember              Requires runtime

    private:

        CXXREFLECT_NONCOPYABLE(Type);

        enum RealizationState
        {
            RealizedTypeDef           = 0x0001,
            RealizedTypeDefProperties = 0x0002,
            RealizedBaseType          = 0x0004,
            RealizedDeclaringType     = 0x0008
        };

        void RealizeTypeDef()           const;
        void RealizeTypeDefProperties() const;
        void RealizeBaseType()          const;
        void RealizeDeclaringType()     const;

        MetadataToken&  PrivateGetTypeDefToken()  const { RealizeTypeDef();           return _typeDefToken;  }

        String&         PrivateGetTypeName()      const { RealizeTypeDefProperties(); return _typeName;      }
        std::uint32_t&  PrivateGetTypeFlags()     const { RealizeTypeDefProperties(); return _typeFlags;     }
        MetadataToken&  PrivateGetBaseTypeToken() const { RealizeTypeDefProperties(); return _baseTypeToken; }

        TypeHandle&     PrivateGetBaseType()      const { RealizeBaseType();          return _baseType;      }
        TypeHandle&     PrivateGetDeclaringType() const { RealizeDeclaringType();     return _declaringType; }



        // Always valid.  The original token may be a TypeDef, TypeRef, or TypeSpec.  The Assembly
        // is the assembly whence the original token was obtained.
        AssemblyHandle        _assembly;
        MetadataToken         _originalToken;

        mutable Detail::FlagSet<RealizationState> _state;

        // Guarded by RealizedTypeDef.  The TypeDef token is the TypeDef referred to by the original
        // token; it is the same as the original token if the original token is a TypeDef
        mutable MetadataToken _typeDefToken;

        // Guarded by RealizedTypeDefProperties
        mutable String        _typeName;
        mutable std::uint32_t _typeFlags;
        mutable MetadataToken _baseTypeToken;

        // Guarded by RealizedBaseType
        mutable TypeHandle    _baseType;

        // Guarded by RealizedDeclaringType
        mutable TypeHandle    _declaringType;
    };

    bool operator==(Type const& lhs, Type const& rhs); // TODO
    bool operator< (Type const& lhs, Type const& rhs); // TODO

    inline bool operator!=(Type const& lhs, Type const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Type const& lhs, Type const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Type const& lhs, Type const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Type const& lhs, Type const& rhs) { return !(rhs <  lhs); }

}

#endif
