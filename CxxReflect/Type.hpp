//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_TYPE_HPP_
#define CXXREFLECT_TYPE_HPP_

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataDatabase.hpp"

namespace CxxReflect {

    class Type
    {
    public:

        Type()
            : _assembly(), _type()
        {
        }

        Type(Assembly const& assembly, Metadata::TableReference const& type)
            : _assembly(assembly), _type(type)
        {
            // Note that a null type index is valid here:  it indicates no type
            Detail::Verify([&] { return assembly.IsInitialized();    });
            Detail::Verify([&] { return IsTypeDef() || IsTypeSpec(); });
        }

        Assembly GetAssembly() const { return _assembly; }

        bool HasBaseType() const;
        Type GetBaseType() const;

        String          GetFullName()  const;
        StringReference GetName()      const;
        StringReference GetNamespace() const;

        bool IsAbstract()                const;
        bool IsAnsiClass()               const;
        bool IsArray()                   const;
        bool IsAutoClass()               const;
        bool IsAutoLayout()              const;
        bool IsByRef()                   const;
        bool IsClass()                   const;
        bool IsComObject()               const;
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

        // TODO This interface is very incomplete

        bool IsInitialized() const
        {
            return _assembly.IsInitialized()
                && _type.GetIndex() != Metadata::InvalidTableIndex;
        }

        // AssemblyQualifiedName
        // Attributes
        // ContainsGenericParameters
        // DeclaringMethod
        // DeclaringType
        // [static] DefaultBinder
        // FullName
        // GenericParameterAttributes
        // GenericParameterPosition
        // GUID
        // HasElementType
        // MemberType
        // MetadataToken
        // Module
        // ReflectedType
        // StructLayoutAttribute
        // TypeHandle
        // TypeInitializer
        // UnderlyingSystemType

        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Type is not initialized");
        }

        bool IsTypeDef()  const { return _type.GetTable() == Metadata::TableId::TypeDef;  }
        bool IsTypeSpec() const { return _type.GetTable() == Metadata::TableId::TypeSpec; }

        Metadata::TypeDefRow  GetTypeDefRow() const
        {
            Detail::Verify([&] { return IsTypeDef(); });
            return _assembly.GetDatabase().GetRow<Metadata::TableId::TypeDef>(_type.GetIndex());
        }

        Metadata::TypeSpecRow GetTypeSpecRow() const
        {
            Detail::Verify([&] { return IsTypeSpec(); });
            return _assembly.GetDatabase().GetRow<Metadata::TableId::TypeSpec>(_type.GetIndex());
        }

        #define CXXREFLECT_GENERATE decltype(std::declval<TCallback>()(std::declval<Metadata::TypeDefRow>()))

        template <typename TCallback>
        auto ResolveTypeDefAndCall(
            TCallback callback,
            CXXREFLECT_GENERATE defaultResult = std::declval<CXXREFLECT_GENERATE>()
        ) const -> CXXREFLECT_GENERATE
        {
            VerifyInitialized();

            // If this type is itself a TypeDef, we can directly call the callback and return:
            if (IsTypeDef())
                return callback(GetTypeDefRow());

            // Otherwise, we need to visit the TypeSpec to find the primary TypeDef or TypeRef
            // to which it refers; if it refers to a TypeRef, we must resolve it.

            return defaultResult; // TODO TYPESPEC FIRST
        }

        #undef CXXREFLECT_GENERATE

        Assembly                 _assembly;
        Metadata::TableReference _type;
    };

}

#endif
