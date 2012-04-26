#ifndef CXXREFLECT_TYPE_HPP_
#define CXXREFLECT_TYPE_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

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
            Assert([&]{ return reflectedType.IsInitialized(); });
            AssertNotNull(current);
            AssertNotNull(last);
            FilterAdvance();
        }

        Reference operator*() const
        {
            AssertDereferenceable();
            return ValueType(_reflectedType, _current.Get(), InternalKey());
        }

        Pointer operator->() const
        {
            AssertDereferenceable();
            return ValueType(_reflectedType, _current.Get(), InternalKey());
        }

        MemberIterator& operator++()
        {
            AssertDereferenceable();
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

        void AssertInitialized()     const { Assert([&]{ return IsInitialized();     }); }
        void AssertDereferenceable() const { Assert([&]{ return IsDereferenceable(); }); }

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

    



    // The TypeNameBuilder is used to build type names.  It can build any standard form of type
    // name (simple, namespace-qualified, or assembly-qualified) and any form of TypeSpec.
    class TypeNameBuilder
    {
    public:
        
        enum class Mode
        {
            SimpleName,
            FullName,
            AssemblyQualifiedName
        };

        static String BuildTypeName(Type const& type, Mode mode);

    private:

        TypeNameBuilder(Type const& type, Mode mode);

        // This type is noncopyable and these are unimplemented.
        TypeNameBuilder(TypeNameBuilder const&);
        TypeNameBuilder& operator=(TypeNameBuilder const&);

        operator String();

        bool AccumulateTypeName               (Type const& type, Mode mode);

        bool AccumulateTypeDefName            (Type const& type, Mode mode);
        bool AccumulateTypeSpecName           (Type const& type, Mode mode);

        bool AccumulateArrayTypeSpecName      (Type const& type, Mode mode);
        bool AccumulateClassTypeSpecName      (Type const& type, Mode mode);
        bool AccumulateFnPtrTypeSpecName      (Type const& type, Mode mode);
        bool AccumulateGenericInstTypeSpecName(Type const& type, Mode mode);
        bool AccumulatePrimitiveTypeSpecName  (Type const& type, Mode mode);
        bool AccumulatePtrTypeSpecName        (Type const& type, Mode mode);
        bool AccumulateSzArrayTypeSpecName    (Type const& type, Mode Mode);
        bool AccumulateVarTypeSpecName        (Type const& type, Mode mode);

        void AccumulateAssemblyQualificationIfRequired(Type const& type, Mode mode);

        static Mode WithoutAssemblyQualification(Mode mode);

        String _buffer;
    };

} }

namespace CxxReflect {

    class Type
    {
    private:

        static bool FilterEvent    (BindingFlags, Type const&, Detail::EventContext     const&);
        static bool FilterField    (BindingFlags, Type const&, Detail::FieldContext     const&);
        static bool FilterInterface(BindingFlags, Type const&, Detail::InterfaceContext const&);
        static bool FilterMethod   (BindingFlags, Type const&, Detail::MethodContext    const&);
        static bool FilterProperty (BindingFlags, Type const&, Detail::PropertyContext  const&);

    public:

        typedef Detail::MemberIterator<Type, Event,    Detail::EventContext,     &Type::FilterEvent    > EventIterator;
        typedef Detail::MemberIterator<Type, Field,    Detail::FieldContext,     &Type::FilterField    > FieldIterator;
        typedef Detail::MemberIterator<Type, Type,     Detail::InterfaceContext, &Type::FilterInterface> InterfaceIterator;
        typedef Detail::MemberIterator<Type, Method,   Detail::MethodContext,    &Type::FilterMethod   > MethodIterator;
        typedef Detail::MemberIterator<Type, Property, Detail::PropertyContext,  &Type::FilterProperty > PropertyIterator;

        Type();

        Assembly GetAssembly() const;

        SizeType  GetMetadataToken() const;
        TypeFlags GetAttributes()    const;

        Type GetBaseType() const;
        Type GetDeclaringType() const;

        Type GetElementType() const;

        InterfaceIterator BeginInterfaces()     const;
        InterfaceIterator EndInterfaces()       const;
        Type GetInterface(StringReference name) const;

        // A type has many different names. GetName(), GetFullName(), and GetAssemblyQualifiedName()
        // all match the CLI Reflection API Type's Name, FullName, and AssemblyQualifiedName.  The
        // GetBasicName() returns the same result as GetName() for type definitions, but it only
        // returns the most fundamental element type name for type specifications (e.g., for A.B*,
        // GetName() would return "A.B*", but GetBasicName() would return "A.B").  GetBasicName()
        // is provided for performance.
        String          GetAssemblyQualifiedName() const;
        String          GetFullName()              const;
        String          GetName()                  const;
        StringReference GetBasicName()             const;
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

        // TODO EventIterator BeginEvents(BindingFlags flags = BindingAttribute::Default) const;
        // TODO EventIterator EndEvents() const;

        FieldIterator BeginFields(BindingFlags flags = BindingAttribute::Default) const;
        FieldIterator EndFields() const;

        MethodIterator BeginMethods(BindingFlags flags = BindingAttribute::Default) const;
        MethodIterator EndMethods() const;
        Method         GetMethod(StringReference name, BindingFlags flags = BindingAttribute::Default) const;

        // TODO PropertyIterator BeginProperties(BindingFlags flags = BindingAttribute::Default) const;
        // TODO PropertyIterator EndProperties() const;

        // TODO Provide ability to return inherited attributes
        CustomAttributeIterator BeginCustomAttributes() const;
        CustomAttributeIterator EndCustomAttributes()   const;

        bool IsInitialized() const;
        bool operator!()     const;

        // ContainsGenericParameters
        // DeclaringMethod
        // [static] DefaultBinder
        // GenericParameterAttributes
        // GenericParameterPosition
        // GUID
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

    public: // Internal Members

        Type(Assembly const& assembly, Metadata::RowReference  const& type, InternalKey);
        Type(Assembly const& assembly, Metadata::BlobReference const& type, InternalKey);

        Type(Type const& reflectedType, Detail::InterfaceContext const* context, InternalKey);

        Metadata::ElementReference GetSelfReference(InternalKey) const;

    private:

        friend Detail::TypeNameBuilder;

        void AssertInitialized() const;

        bool IsTypeDef()  const;
        bool IsTypeSpec() const;

        Metadata::TypeDefRow    GetTypeDefRow()        const;
        Metadata::TypeSignature GetTypeSpecSignature() const;

        typedef std::pair<Metadata::RowReference, Metadata::RowReference> InterfacesRange;

        #define CXXREFLECT_GENERATE decltype(std::declval<TCallback>()(std::declval<Type>()))

        static Type ResolveTypeDef(Type const type);

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
            AssertInitialized();

            Type const typeDefType(ResolveTypeDef(*this));
            if (!typeDefType.IsInitialized())
                return defaultResult;

            return callback(typeDefType);
        }

        #undef CXXREFLECT_GENERATE

        Detail::AssemblyHandle     _assembly;
        Metadata::ElementReference _type;
    };

}

#endif
