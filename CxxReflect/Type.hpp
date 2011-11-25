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
        : public Detail::SafeBoolConvertible<Type>,
          public Detail::Comparable<Type>
    {
    private:

        typedef std::tuple<
            Type,
            Metadata::TableReference,
            Metadata::TableReference
        > NextMethodScopeResult;

        static NextMethodScopeResult InternalNextMethodScope(Type const& currentScope);
        static bool InternalFilterMethod(Method const& method, BindingFlags const& flags);

    public:

        typedef Detail::NestedTableTransformIterator<
            Metadata::TableReference,
            Method,
            Type,
            BindingFlags,
            NextMethodScopeResult,
            &Type::InternalNextMethodScope,
            &Type::InternalFilterMethod
        > MethodIterator;

        Type();
        Type(Assembly const& assembly, Metadata::TableReference const& type, InternalKey);
        Type(Assembly const& assembly, Metadata::BlobReference  const& type, InternalKey);

        Assembly const& GetAssembly() const { return _assembly; }

        SizeType GetMetadataToken() const
        {
            return _type.IsTableReference() ? _type.AsTableReference().GetToken() : 0;
        }

        Type GetBaseType() const;
        Type GetDeclaringType() const;

        String          GetAssemblyQualifiedName() const;
        String          GetFullName()              const;
        StringReference GetName()                  const;
        StringReference GetNamespace()             const;

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

        MethodIterator BeginMethods(BindingFlags flags = BindingAttribute::Default) const;
        MethodIterator EndMethods()   const;

        // TODO This interface is very incomplete

        bool IsInitialized() const
        {
            return _assembly.IsInitialized() && _type.IsValid();
        }

        bool operator!() const { return !IsInitialized(); }

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
        // Module
        // ReflectedType
        // StructLayoutAttribute
        // TypeHandle
        // TypeInitializer
        // UnderlyingSystemType

        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent

        friend bool operator==(Type const& lhs, Type const& rhs)
        {
            return lhs.GetAssembly() == rhs.GetAssembly()
                && lhs.GetMetadataToken() == rhs.GetMetadataToken();
        }

        // We provide a total ordering of Types across all loaded assemblies.  Types within a given
        // assembly are ordered by metadata token; types in different assemblies have an unspecified
        // total ordering.
        friend bool operator<(Type const& lhs, Type const& rhs)
        {
            if (lhs.GetAssembly() < rhs.GetAssembly())
                return true;

            return lhs.GetAssembly() == rhs.GetAssembly()
                && lhs.GetMetadataToken() == rhs.GetMetadataToken();
        }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Type is not initialized");
        }

        bool IsTypeDef()  const { VerifyInitialized(); return _type.IsTableReference(); }
        bool IsTypeSpec() const { VerifyInitialized(); return _type.IsBlobReference();  }

        Metadata::TypeDefRow    GetTypeDefRow()        const;
        Metadata::TypeSignature GetTypeSpecSignature() const;

        #define CXXREFLECT_GENERATE decltype(std::declval<TCallback>()(std::declval<Type>()))

        // Resolves the TypeDef associated with this type.  If this type is itself a TypeDef, it
        // returns itself.  If this type is a TypeSpec, it parses the TypeSpec to find the
        // primary TypeDef referenced by the TypeSpec; note that in this case the TypeDef may be
        // in a different module or assembly.
        template <typename TCallback>
        auto ResolveTypeDefTypeAndCall(
            TCallback callback,
            CXXREFLECT_GENERATE defaultResult = Detail::Identity<CXXREFLECT_GENERATE>::Type()
        ) const -> CXXREFLECT_GENERATE
        {
            VerifyInitialized();

            // If this type is itself a TypeDef, we can directly call the callback and return:
            if (IsTypeDef())
                return callback(*this);

            // Otherwise, we need to visit the TypeSpec to find the primary TypeDef or TypeRef
            // to which it refers; if it refers to a TypeRef, we must resolve it.

            return defaultResult; // TODO TYPESPEC FIRST
        }

        #undef CXXREFLECT_GENERATE

        bool AccumulateFullNameInto(OutputStream& os) const;
        void AccumulateAssemblyQualifiedNameInto(OutputStream& os) const;

        Detail::MethodTableAllocator::Range GetOrCreateMethodTable() const;

        Assembly                       _assembly;
        Metadata::TableOrBlobReference _type;
    };

}

#endif
