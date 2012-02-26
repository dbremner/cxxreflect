//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Whereas Core.hpp contains declarations tht are independent of the rest of the library (i.e. it
// has no dependencies on any other CxxReflect headers), this header contains core components that
// require declarations from the Metadata{Database,Signature}.hpp headers.
#ifndef CXXREFLECT_CORECOMPONENTS_HPP_
#define CXXREFLECT_CORECOMPONENTS_HPP_

#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/MetadataSignature.hpp"

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

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    class MemberContext
    {
    public:

        typedef TMember          MemberType;
        typedef TMemberRow       MemberRowType;
        typedef TMemberSignature MemberSignatureType;

        MemberContext();

        MemberContext(Metadata::FullReference const& declaringType,
                      Metadata::RowReference  const& member);

        MemberContext(Metadata::FullReference const& declaringType,
                      Metadata::RowReference  const& member,
                      Metadata::FullReference const& instantiatingType,
                      ConstByteRange          const& instantiatedSignature);

        MemberType Resolve(Type const& reflectedType) const;

        Metadata::FullReference GetDeclaringType()         const;

        Metadata::FullReference GetMember()                const;
        MemberRowType           GetMemberRow()             const;
        MemberSignatureType     GetMemberSignature()       const;

        bool                    HasInstantiatingType()     const;
        Metadata::FullReference GetInstantiatingType()     const;

        bool                    HasInstantiatedSignature() const;
        ConstByteRange          GetInstantiatedSignature() const;

        bool                    IsInitialized()            const;

    private:

        void                    AssertInitialized()        const;

        Metadata::FullReference _declaringType;
        Metadata::RowReference  _member;
        Metadata::FullReference _instantiatingType;
        ConstByteRange          _instantiatedSignature;
    };

    typedef MemberContext<Event,    Metadata::EventRow,     Metadata::TypeSignature    > EventContext;
    typedef MemberContext<Field,    Metadata::FieldRow,     Metadata::FieldSignature   > FieldContext;
    typedef MemberContext<Method,   Metadata::MethodDefRow, Metadata::MethodSignature  > MethodContext;
    typedef MemberContext<Property, Metadata::PropertyRow,  Metadata::PropertySignature> PropertyContext;

    typedef Range<EventContext>    EventTable;
    typedef Range<FieldContext>    FieldTable;
    typedef Range<MethodContext>   MethodTable;
    typedef Range<PropertyContext> PropertyTable;

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    class MemberTableCollection
    {
    public:

        typedef TMember                                              MemberType;
        typedef TMemberRow                                           MemberRowType;
        typedef TMemberSignature                                     MemberSignatureType;

        typedef MemberContext<TMember, TMemberRow, TMemberSignature> MemberContextType;
        typedef Range<MemberContextType>                             MemberTableType;

        typedef LinearArrayAllocator<Byte,              (1 << 16)>   SignatureAllocator;
        typedef LinearArrayAllocator<MemberContextType, (1 << 11)>   TableAllocator;

        typedef Metadata::ClassVariableSignatureInstantiator         Instantiator;
        typedef Metadata::FullReference                              FullReference;

        MemberTableCollection(Loader const* loader);
        MemberTableCollection(MemberTableCollection&& other);
        MemberTableCollection& operator=(MemberTableCollection&& other);

        void Swap(MemberTableCollection& other);
        
        MemberTableType GetOrCreateMemberTable(FullReference const& type) const;

    private:

        typedef std::pair<FullReference, FullReference> TypeDefAndSpec;

        MemberTableCollection(MemberTableCollection const&);
        MemberTableCollection& operator=(MemberTableCollection const&);

        // The provided 'type' may be a TypeDef, TypeRef, or TypeSpec.  This function returns a
        // single TypeDef if 'type' is resolved to be a TypeDef, or resolves a pair containing the
        // TypeSpec (in .second) and the primary TypeDef from the TypeSpec (in .first).  If it is
        // a TypeSpec, it must be a GenericInst.
        TypeDefAndSpec ResolveTypeDefAndSpec(FullReference const& type) const;

        // The provided 'type' must be a GenericInst TypeSpec.  This function creates and returns a
        // generic class variable instantiator from the arguments of the GenericInst.
        Instantiator CreateInstantiator(FullReference const& type) const;

        // Instantiates 'signature' using 'instantiator', allocates space for it in the signature
        // allocator, and returns the result.
        ConstByteRange Instantiate(Instantiator const& instantiator, MemberSignatureType const& signature) const;

        // Computes the correct override or hiding slot for 'newMember' in the member table being
        // built (in _buffer).  The 'inheritedMemberCount' is the index of the first new member
        // (i.e., the first member that was defined in the derived class).
        void InsertMemberIntoBuffer(MemberContextType const& newMember, SizeType inheritedMemberCount) const;

        ValueInitialized<Loader const*>                  _loader;
        SignatureAllocator                       mutable _signatureAllocator;
        TableAllocator                           mutable _tableAllocator;
        std::map<FullReference, MemberTableType> mutable _index;
        std::vector<MemberContextType>           mutable _buffer;
    };

    typedef MemberTableCollection<Event,    Metadata::EventRow,     Metadata::TypeSignature    > EventTableCollection;
    typedef MemberTableCollection<Field,    Metadata::FieldRow,     Metadata::FieldSignature   > FieldTableCollection;
    typedef MemberTableCollection<Method,   Metadata::MethodDefRow, Metadata::MethodSignature  > MethodTableCollection;
    typedef MemberTableCollection<Property, Metadata::PropertyRow,  Metadata::PropertySignature> PropertyTableCollection;





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

        EventTable    const GetOrCreateEventTable   (Metadata::ElementReference const& type) const;
        FieldTable    const GetOrCreateFieldTable   (Metadata::ElementReference const& type) const;
        MethodTable   const GetOrCreateMethodTable  (Metadata::ElementReference const& type) const;
        PropertyTable const GetOrCreatePropertyTable(Metadata::ElementReference const& type) const;

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

        ValueInitialized<Loader const*>       _loader;
        String                                _uri;
        Metadata::Database                    _database;

        FlagSet<RealizationState>     mutable _state;
        std::unique_ptr<AssemblyName> mutable _name;
        EventTableCollection          mutable _events;
        FieldTableCollection          mutable _fields;
        MethodTableCollection         mutable _methods;
        PropertyTableCollection       mutable _properties;
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
                     MethodContext              const* methodContext);
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
        ValueInitialized<MethodContext const*>   _methodContext;
    };

    class ParameterHandle
    {
    public:

        ParameterHandle();
        ParameterHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                        Metadata::ElementReference const& reflectedTypeReference,
                        MethodContext              const* methodContext,
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
        ValueInitialized<MethodContext const*>   _methodContext;

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

    struct MetadataTokenLessThanComparer
    {
        template <typename TMember>
        bool operator()(TMember const& lhs, TMember const& rhs) const
        {
            return lhs.GetMetadataToken() < rhs.GetMetadataToken();
        }
    };


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

        template <typename TMember, typename TMemberRow, typename TMemberSignature>
        friend class Detail::MemberContext;

        template <typename TMember, typename TMemberRow, typename TMemberSignature>
        friend class Detail::MemberTableCollection;

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
