//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_TYPE_HPP_
#define CXXREFLECT_TYPE_HPP_

#include "CxxReflect/CoreInternals.hpp"

namespace CxxReflect { namespace Detail {

    template
    <
        typename TType,
        typename TMember,
        typename TMemberContext,
        bool (*FFilter)(BindingFlags, TType const&, TMemberContext const&)
    >
    class MemberIterator
    {
    public:

        typedef std::forward_iterator_tag iterator_category;
        typedef TMember                   value_type;
        typedef TMember                   reference;
        typedef Dereferenceable<TMember>  pointer;
        typedef std::ptrdiff_t            difference_type;

        typedef value_type                ValueType;
        typedef reference                 Reference;
        typedef pointer                   Pointer;
        typedef TMemberContext const*     InnerIterator;

        MemberIterator()
        {
        }

        MemberIterator(TType         const& reflectedType,
                       InnerIterator const  current,
                       InnerIterator const  last,
                       BindingFlags  const  filter)
            : _reflectedType(reflectedType), _current(current), _last(last), _filter(filter)
        {
            Verify([&]{ return reflectedType.IsInitialized(); });
            VerifyNotNull(current);
            VerifyNotNull(last);
            FilterAdvance();
        }

        Reference operator*() const
        {
            VerifyDereferenceable();
            return ValueType(_reflectedType, _current.Get(), InternalKey());
        }

        Pointer operator->() const
        {
            VerifyDereferenceable();
            return ValueType(_reflectedType, _current.Get(), InternalKey());
        }

        MemberIterator& operator++()
        {
            VerifyDereferenceable();
            ++_current.Get();
            FilterAdvance();
            return *this;
        }

        MemberIterator operator++(int)
        {
            MemberIterator const it(*this);
            ++*this;
            return it;
        }

        bool IsInitialized() const
        {
            return _current.Get() != nullptr && _last.Get() !=  nullptr;
        }

        bool IsDereferenceable() const
        {
            return IsInitialized() && _current.Get() != _last.Get();
        }

        friend bool operator==(MemberIterator const& lhs, MemberIterator const& rhs)
        {
            return !lhs.IsDereferenceable() && !rhs.IsDereferenceable()
                || lhs._current.Get() == rhs._current.Get();
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(MemberIterator)

    private:

        void VerifyInitialized()     const { Verify([&]{ return IsInitialized();     }); }
        void VerifyDereferenceable() const { Verify([&]{ return IsDereferenceable(); }); }

        void FilterAdvance()
        {
            while (_current.Get() != _last.Get() && FFilter(_filter, _reflectedType, *_current.Get()))
                ++_current.Get();
        }

        ValueInitialized<InnerIterator> _current;
        ValueInitialized<InnerIterator> _last;
        TType                           _reflectedType;
        BindingFlags                    _filter;
    };

    struct InterfaceImplTransformer
    {
        Metadata::RowReference operator()(Metadata::FullReference r) const volatile
        {
            Detail::Verify([&]{ return r.AsRowReference().GetTable() == Metadata::TableId::InterfaceImpl; });
            return r
                .GetDatabase()
                .GetRow<Metadata::TableId::InterfaceImpl>(r.AsRowReference().GetIndex())
                .GetInterface();
        }
    };

} }

namespace CxxReflect {

    class Type
    {
    private:

        static bool FilterEvent   (BindingFlags, Type const&, Detail::EventContext    const&);
        static bool FilterField   (BindingFlags, Type const&, Detail::FieldContext    const&);
        static bool FilterMethod  (BindingFlags, Type const&, Detail::MethodContext   const&);
        static bool FilterProperty(BindingFlags, Type const&, Detail::PropertyContext const&);

    public:

        typedef Detail::MemberIterator<Type, Event,    Detail::EventContext,    &Type::FilterEvent   > EventIterator;
        typedef Detail::MemberIterator<Type, Field,    Detail::FieldContext,    &Type::FilterField   > FieldIterator;
        typedef Detail::MemberIterator<Type, Method,   Detail::MethodContext,   &Type::FilterMethod  > MethodIterator;
        typedef Detail::MemberIterator<Type, Property, Detail::PropertyContext, &Type::FilterProperty> PropertyIterator;

        typedef Detail::InstantiatingIterator<
            Metadata::FullReference, Type, Assembly, Detail::InterfaceImplTransformer
        > InterfaceIterator;

        Type();
        Type(Assembly const& assembly, Metadata::RowReference  const& type, InternalKey);
        Type(Assembly const& assembly, Metadata::BlobReference const& type, InternalKey);

        Assembly GetAssembly() const;

        Metadata::ElementReference GetSelfReference(InternalKey) const { return _type; }

        SizeType GetMetadataToken() const
        {
            return _type.IsRowReference() ? _type.AsRowReference().GetToken() : 0;
        }

        Type GetBaseType() const;
        Type GetDeclaringType() const;

        InterfaceIterator BeginInterfaces()     const;
        InterfaceIterator EndInterfaces()       const;
        Type GetInterface(StringReference name) const;

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

        MethodIterator BeginConstructors(BindingFlags flags = BindingAttribute::Default) const;
        MethodIterator EndConstructors() const;

        EventIterator BeginEvents(BindingFlags flags = BindingAttribute::Default) const;
        EventIterator EndEvents() const;

        FieldIterator BeginFields(BindingFlags flags = BindingAttribute::Default) const;
        FieldIterator EndFields() const;

        MethodIterator BeginMethods(BindingFlags flags = BindingAttribute::Default) const;
        MethodIterator EndMethods() const;
        Method         GetMethod(StringReference name, BindingFlags flags = BindingAttribute::Default) const;

        PropertyIterator BeginProperties(BindingFlags flags = BindingAttribute::Default) const;
        PropertyIterator EndProperties() const;

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

        friend bool operator==(Type const&, Type const&);
        friend bool operator< (Type const&, Type const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Type)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Type)

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Type is not initialized");
        }

        bool IsTypeDef()  const { VerifyInitialized(); return _type.IsRowReference();  }
        bool IsTypeSpec() const { VerifyInitialized(); return _type.IsBlobReference(); }

        Metadata::TypeDefRow    GetTypeDefRow()        const;
        Metadata::TypeSignature GetTypeSpecSignature() const;

        typedef std::pair<Metadata::RowReference, Metadata::RowReference> InterfacesRange;

        InterfacesRange GetInterfacesRange() const;

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

        Detail::AssemblyHandle     _assembly;
        Metadata::ElementReference _type;
    };

}

#endif
