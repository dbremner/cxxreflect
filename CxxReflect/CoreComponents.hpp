#ifndef CXXREFLECT_CORECOMPONENTS_HPP_
#define CXXREFLECT_CORECOMPONENTS_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/MetadataSignature.hpp"
#include "CxxReflect/OwnedElements.hpp"

namespace CxxReflect {

    class InternalKey;

    class Assembly;
    class AssemblyName;
    class CustomAttribute;
    class Event;
    class Field;
    class File;
    class IAssemblyLocator;
    class Loader;
    class Method;
    class Module;
    class Parameter;
    class Property;
    class Type;
    class Utility;
    class Version;

}

namespace CxxReflect { namespace Detail {

    // Represents all of the permanent information about an Assembly.  This is the implementation of
    // an 'Assembly' facade and includes parts of the implementation of other facades (e.g., it
    // stores the method tables for each type in the assembly).  This way, the actual facade types
    // are uber-fast to copy and we can treat them as "references" into the metadata database.
    class AssemblyContext
    {
    public:

        AssemblyContext(Loader const* loader, String uri, Metadata::Database&& database);
        AssemblyContext(AssemblyContext&& other);

        AssemblyContext& operator=(AssemblyContext&& other);

        void Swap(AssemblyContext& other);

        Loader             const& GetLoader()       const;
        Metadata::Database const& GetDatabase()     const;
        String             const& GetLocation()     const;
        AssemblyName       const& GetAssemblyName() const;

        OwnedEventTable    const GetOrCreateEventTable   (Metadata::ElementReference const& type) const;
        OwnedFieldTable    const GetOrCreateFieldTable   (Metadata::ElementReference const& type) const;
        OwnedMethodTable   const GetOrCreateMethodTable  (Metadata::ElementReference const& type) const;
        OwnedPropertyTable const GetOrCreatePropertyTable(Metadata::ElementReference const& type) const;

        bool IsInitialized() const;

    private:

        AssemblyContext(AssemblyContext const&);
        AssemblyContext operator=(AssemblyContext const&);

        void AssertInitialized() const;

        enum RealizationState
        {
            RealizedName = 0x01
        };

        void RealizeName() const;

        ValueInitialized<Loader const*>            _loader;
        String                                     _uri;
        Metadata::Database                         _database;

        FlagSet<RealizationState>          mutable _state;
        std::unique_ptr<AssemblyName>      mutable _name;
        OwnedEventTableCollection          mutable _events;
        OwnedFieldTableCollection          mutable _fields;
        OwnedMethodTableCollection         mutable _methods;
        OwnedPropertyTableCollection       mutable _properties;
    };





    class AssemblyHandle
    {
    public:

        AssemblyHandle();
        AssemblyHandle(AssemblyContext const* context);
        AssemblyHandle(Assembly const& assembly);

        Assembly Realize() const;

        bool IsInitialized() const;

        friend bool operator==(AssemblyHandle const&, AssemblyHandle const&);
        friend bool operator< (AssemblyHandle const&, AssemblyHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(AssemblyHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<AssemblyContext const*> _context;
    };

    class MethodHandle
    {
    public:

        MethodHandle();
        MethodHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                     Metadata::ElementReference const& reflectedTypeReference,
                     OwnedMethod                const* ownedMethod);
        MethodHandle(Method const& method);

        Method Realize() const;

        bool IsInitialized() const;

        friend bool operator==(MethodHandle const&, MethodHandle const&);
        friend bool operator< (MethodHandle const&, MethodHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(MethodHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<AssemblyContext const*> _reflectedTypeAssemblyContext;
        Metadata::ElementReference               _reflectedTypeReference;
        ValueInitialized<OwnedMethod const*>     _ownedMethod;
    };

    class ParameterHandle
    {
    public:

        ParameterHandle();
        ParameterHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                        Metadata::ElementReference const& reflectedTypeReference,
                        OwnedMethod                const* ownedMethod,
                        Metadata::RowReference     const& parameterReference,
                        Metadata::TypeSignature    const& parameterSignature);
        ParameterHandle(Parameter const& parameter);

        Parameter Realize() const;

        bool IsInitialized() const;

        friend bool operator==(ParameterHandle const&, ParameterHandle const&);
        friend bool operator< (ParameterHandle const&, ParameterHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(ParameterHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<AssemblyContext const*> _reflectedTypeAssemblyContext;
        Metadata::ElementReference               _reflectedTypeReference;
        ValueInitialized<OwnedMethod const*>     _ownedMethod;

        Metadata::RowReference                   _parameterReference;
        Metadata::TypeSignature                  _parameterSignature;
    };

    class TypeHandle
    {
    public:

        TypeHandle();
        TypeHandle(AssemblyContext            const* assemblyContext,
                   Metadata::ElementReference const& typeReference);
        TypeHandle(Type const& type);

        Type Realize() const;

        bool IsInitialized() const;

        friend bool operator==(TypeHandle const&, TypeHandle const&);
        friend bool operator< (TypeHandle const&, TypeHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(TypeHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<AssemblyContext const*> _assemblyContext;
        Metadata::ElementReference               _typeReference;
    };





    class ParameterData
    {
    public:

        ParameterData();
        ParameterData(Metadata::RowReference                       const& parameter,
                      Metadata::MethodSignature::ParameterIterator const& signature,
                      InternalKey);

        Metadata::RowReference  const& GetParameter() const;
        Metadata::TypeSignature const& GetSignature() const;

        bool IsInitialized() const;

        ParameterData& operator++();
        ParameterData  operator++(int);

        friend bool operator==(ParameterData const&, ParameterData const&);
        friend bool operator< (ParameterData const&, ParameterData const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(ParameterData)

    private:

        void AssertInitialized() const;

        Metadata::RowReference                       _parameter;
        Metadata::MethodSignature::ParameterIterator _signature;
    };

    template
    <
        typename TType,
        typename TMember,
        typename TMemberContext,
        bool (*FFilter)(BindingFlags, TType const&, TMemberContext const&)
    >
    class MemberIterator;

} }

namespace CxxReflect {

    class Utility
    {
    public:

        // Tests whether 'assembly' is the system assembly (i.e., the assembly that references no
        // other assemblies).  This is usually mscorlib.dll.
        static bool IsSystemAssembly(Assembly const& assembly);

        // Tests whether 'type' is the system type named 'systemTypeNamespace.systemTypeSimpleName'.
        static bool IsSystemType(Type            const& type,
                                 StringReference const& systemTypeNamespace,
                                 StringReference const& systemTypeSimpleName);

        // Tests whether 'type' is derived from the named system type, optionally including the type
        // itself.  This is used to test whether a type is contextual, an enumeration, etc.
        static bool IsDerivedFromSystemType(Type            const& type,
                                            StringReference const& systemTypeNamespace,
                                            StringReference const& systemTypeSimpleName,
                                            bool                   includeSelf);

        static Assembly GetSystemAssembly(Type const& referenceType);
        static Assembly GetSystemAssembly(Assembly const& referenceAssembly);

        static Type GetSystemObjectType(Type const& referenceType);
        static Type GetSystemObjectType(Assembly const& referenceAssembly);
    };





    struct MetadataTokenEqualsComparer
    {
        template <typename TMember>
        bool operator()(TMember const& lhs, TMember const& rhs) const
        {
            return lhs.GetMetadataToken() == rhs.GetMetadataToken();
        }
    };





    template <typename TTokenGetter>
    class MetadataTokenStrictWeakOrderingImpl
    {
    public:

        MetadataTokenStrictWeakOrderingImpl(TTokenGetter const& getToken)
            : _getToken(getToken)
        {
        }

        template <typename TMember>
        bool operator()(TMember const& lhs, TMember const& rhs) const
        {
            return _getToken(lhs) < _getToken(rhs);
        }

    private:

        TTokenGetter _getToken;
    };

    class MetadataTokenDefaultGetter
    {
    public:

        template <typename TMember>
        SizeType operator()(TMember const& member) const
        {
            return member.GetMetadataToken();
        }
    };

    MetadataTokenStrictWeakOrderingImpl<MetadataTokenDefaultGetter>
    inline MetadataTokenStrictWeakOrdering()
    {
        return MetadataTokenStrictWeakOrderingImpl<MetadataTokenDefaultGetter>(MetadataTokenDefaultGetter());
    }

    template <typename TTokenGetter>
    MetadataTokenStrictWeakOrderingImpl<TTokenGetter>
    MetadataTokenStrictWeakOrdering(TTokenGetter const& getToken)
    {
        return MetadataTokenStrictWeakOrderingImpl<TTokenGetter>(getToken);
    }



    typedef Detail::InstantiatingIterator
    <
        Metadata::RowReference,
        CustomAttribute,
        Assembly
    > CustomAttributeIterator;



    // This interface allows different assembly location logic to be plugged into the Loader.
    class IAssemblyLocator
    {
    public:

        // When the Loader attempts to read metadata from an assembly it has not yet loaded, it calls
        // this member function to locate the assembly file on disk.  An implementer should return an
        // empty string if it cannot find the named assembly.
        virtual String LocateAssembly(AssemblyName const& assemblyName) const = 0;
        
        // If the Loader knows the name of a type from the assembly it is trying to load, it will
        // call this member function instead and pass the full type name (i.e., the namespace-
        // qualified type name).  This allows type resolution by name, which is used e.g. by Windows
        // Runtime, which uses metadata files but does not have a concept of "assemblies."
        virtual String LocateAssembly(AssemblyName const& assemblyName, String const& fullTypeName) const = 0;

        virtual ~IAssemblyLocator();
    };





    // There are many functions that should not be part of the public interface of the library, but
    // which we need to be able to access from other parts of the CxxReflect library.  To do this,
    // all "internal" member functions have a parameter of this "InternalKey" class type, which can
    // only be constructed by a subset of the CxxReflect library types.  This is better than direct
    // befriending, both because it is centralized and because it protects class invariants from
    // bugs elsewhere in the library.
    class InternalKey
    {
    public: // TODO MAKE PRIVATE AGAIN!

        InternalKey() { }

        template
        <
            typename TType,
            typename TMember,
            typename TMemberContext,
            bool (*FFilter)(BindingFlags, TType const&, TMemberContext const&)
        >
        friend class Detail::MemberIterator;

        template
        <
            typename TCurrent,
            typename TResult,
            typename TParameter,
            typename TTransformer,
            typename TCategory
        >
        friend class Detail::InstantiatingIterator;

        friend Assembly;
        friend AssemblyName;
        friend CustomAttribute;
        friend Event;
        friend Field;
        friend File;
        friend Loader;
        friend Method;
        friend Module;
        friend Parameter;
        friend Property;
        friend Type;
        friend Utility;
        friend Version;

        friend Detail::AssemblyContext;

        friend Detail::AssemblyHandle;
        friend Detail::MethodHandle;
        friend Detail::ParameterHandle;
        friend Detail::TypeHandle;

        friend Metadata::ArrayShape;
        friend Metadata::CustomModifier;
        friend Metadata::FieldSignature;
        friend Metadata::MethodSignature;
        friend Metadata::PropertySignature;
        friend Metadata::TypeSignature;
        friend Metadata::SignatureComparer;
    };
}

#endif
