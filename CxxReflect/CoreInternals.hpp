//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Whereas Core.hpp contains declarations tht are independent of the rest of the library (i.e. it
// has no dependencies on any other CxxReflect headers), this header contains core components that
// require declarations from the Metadata{Database,Signature}.hpp headers.
#ifndef CXXREFLECT_COREINTERNALS_HPP_
#define CXXREFLECT_COREINTERNALS_HPP_

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/MetadataSignature.hpp"

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
                      ByteRange               const& instantiatedSignature);

        MemberType Resolve(Type const& reflectedType) const;

        Metadata::FullReference GetDeclaringType()         const;

        Metadata::FullReference GetMember()                const;
        MemberRowType           GetMemberRow()             const;
        MemberSignatureType     GetMemberSignature()       const;

        bool                    HasInstantiatingType()     const;
        Metadata::FullReference GetInstantiatingType()     const;

        bool                    HasInstantiatedSignature() const;
        ByteRange               GetInstantiatedSignature() const;

        bool                    IsInitialized()            const;

    private:

        void                    VerifyInitialized()        const;

        Metadata::FullReference _declaringType;
        Metadata::RowReference  _member;
        Metadata::FullReference _instantiatingType;
        ByteRange               _instantiatedSignature;
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

        MemberTableCollection(MetadataLoader const* loader);
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
        ByteRange Instantiate(Instantiator const& instantiator, MemberSignatureType const& signature) const;

        // Computes the correct override or hiding slot for 'newMember' in the member table being
        // built (in _buffer).  The 'inheritedMemberCount' is the index of the first new member
        // (i.e., the first member that was defined in the derived class).
        void InsertMemberIntoBuffer(MemberContextType const& newMember, SizeType inheritedMemberCount) const;

        ValueInitialized<MetadataLoader const*>          _loader;
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

        AssemblyContext(MetadataLoader const* loader, String path, Metadata::Database&& database);
        AssemblyContext(AssemblyContext&& other);

        AssemblyContext& operator=(AssemblyContext&& other);

        void Swap(AssemblyContext& other);

        MetadataLoader     const& GetLoader()       const;
        Metadata::Database const& GetDatabase()     const;
        String             const& GetPath()         const;
        AssemblyName       const& GetAssemblyName() const;

        EventTable    const GetOrCreateEventTable   (Metadata::ElementReference const& type) const;
        FieldTable    const GetOrCreateFieldTable   (Metadata::ElementReference const& type) const;
        MethodTable   const GetOrCreateMethodTable  (Metadata::ElementReference const& type) const;
        PropertyTable const GetOrCreatePropertyTable(Metadata::ElementReference const& type) const;

        bool IsInitialized() const;

    private:

        AssemblyContext(AssemblyContext const&);
        AssemblyContext operator=(AssemblyContext const&);

        void VerifyInitialized() const;

        enum RealizationState
        {
            RealizedName = 0x01
        };

        void RealizeName() const;

        ValueInitialized<MetadataLoader const*> _loader;
        String                                  _path;
        Metadata::Database                      _database;

        FlagSet<RealizationState> mutable _state;
        AssemblyName              mutable _name;
        EventTableCollection      mutable _events;
        FieldTableCollection      mutable _fields;
        MethodTableCollection     mutable _methods;
        PropertyTableCollection   mutable _properties;
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

        void VerifyInitialized() const;

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

        void VerifyInitialized() const;

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

        void VerifyInitialized() const;

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

        void VerifyInitialized() const;

        ValueInitialized<AssemblyContext const*> _assemblyContext;
        Metadata::ElementReference               _typeReference;
    };

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

        static Type GetPrimitiveType(Assembly const& referenceAssembly, Metadata::ElementType elementType);
    };

}

#endif
