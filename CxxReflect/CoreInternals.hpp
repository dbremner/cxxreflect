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

    // Represents the information required to construct a Method object.
    class MethodContext
    {
    public:

        MethodContext();

        MethodContext(Metadata::FullReference const& typeDef,
                      Metadata::RowReference  const& methodDef);

        MethodContext(Metadata::FullReference const& typeDef,
                      Metadata::RowReference  const& methodDef,
                      Metadata::FullReference const& typeSpec,
                      ByteRange               const  instantiatedSignature);

        // Resolves the MethodContext as a Method using the provided reflected type
        Method Resolve(Type const& reflectedType) const;

        Metadata::FullReference   GetDeclaringType()         const;

        Metadata::FullReference   GetMethod()                const;
        Metadata::MethodDefRow    GetMethodDefinition()      const;
        Metadata::MethodSignature GetMethodSignature()       const;

        bool                      HasInstantiatedType()      const;
        Metadata::FullReference   GetInstantiatedType()      const;

        bool                      HasInstantiatedSignature() const;
        ByteRange                 GetInstantiatedSignature() const;

        bool                      IsInitialized()            const;

    private:

        void                      VerifyInitialized()        const;

        // _typeDef is the type that defines the method.  _methodDef is the method definition; it is
        // resolved in the same database as _typeDef.  _typeSpec is the type through which the method
        // is referred; this is usually set for e.g. an instantiated generic type.  If _typeSpec is
        // set and the method uses any of the _typeSpec's generic parameters, _instantiatedSignature
        // will be set and will point to a replacement signature that has all of the variables (Var!0)
        // replaced with their arguments.
        Metadata::FullReference _typeDef;
        Metadata::RowReference  _methodDef;
        Metadata::FullReference _typeSpec;
        ByteRange               _instantiatedSignature;
    };

    typedef Range<MethodContext> MethodTable;





    class MethodTableCollection
    {
    public:

        typedef LinearArrayAllocator<Byte,          (1 << 16)> SignatureAllocator;
        typedef LinearArrayAllocator<MethodContext, (1 << 11)> TableAllocator;
        typedef Metadata::ClassVariableSignatureInstantiator   Instantiator;
        typedef Metadata::FullReference                        FullReference;

        MethodTableCollection(MetadataLoader const* const loader);

        MethodTableCollection(MethodTableCollection&& other);
        MethodTableCollection& operator=(MethodTableCollection&& other);

        void Swap(MethodTableCollection& other);

        MethodTable GetOrCreateMethodTable(FullReference const& type) const;

    private:

        typedef std::pair<FullReference, FullReference> TypeDefAndSpec;

        MethodTableCollection(MethodTableCollection const&);
        MethodTableCollection& operator=(MethodTableCollection const&);

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
        ByteRange Instantiate(Instantiator const& instantiator, Metadata::MethodSignature const& signature) const;

        // Computes the correct override slot for 'newMethod' in the method table being built (in
        // the _buffer).  The 'inheritedMethodCount' is the index of the first new method (i.e., the
        // first method that was defined in the derived class).
        void InsertMethodIntoBuffer(MethodContext const& newMethod, SizeType inheritedMethodCount) const;

        ValueInitialized<MetadataLoader const*>         _loader;
        SignatureAllocator                      mutable _signatureAllocator;
        TableAllocator                          mutable _tableAllocator;
        std::map<FullReference, MethodTable>    mutable _index;
        std::vector<MethodContext>              mutable _buffer;
    };





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

        MethodTable const GetOrCreateMethodTable(Metadata::ElementReference const& type) const;

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
        MethodTableCollection     mutable _methods;
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

#endif
