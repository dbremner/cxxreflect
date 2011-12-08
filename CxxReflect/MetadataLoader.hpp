//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// MetadataLoader provides bindings between the physical layer implementation of the metadata reader
// (in CxxReflect::Metadata) and the logical layer interface (in CxxReflect).  The MetadataLoader
// itself manages the loading of assemblies and owns all persistent data structures.
#ifndef CXXREFLECT_METADATALOADER_HPP_
#define CXXREFLECT_METADATALOADER_HPP_

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Core.hpp"
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

        AssemblyContext(MetadataLoader const* const loader, String path, Metadata::Database&& database)
            : _loader(loader), _path(std::move(path)), _database(std::move(database)), _methods(loader)
        {
            Detail::VerifyNotNull(_loader.Get());
            Detail::Verify([&]{ return !_path.empty(); });
        }

        AssemblyContext(AssemblyContext&& other)
            : _loader  (std::move(other._loader  )),
              _path    (std::move(other._path    )),
              _database(std::move(other._database)),
              _name    (std::move(other._name    )),
              _state   (std::move(other._state   )),
              _methods (std::move(other._methods ))
        {
            other._loader.Get() = nullptr;
            other._state.Reset();
        }

        AssemblyContext& operator=(AssemblyContext&& other)
        {
            Swap(other);
            return *this;
        }

        void Swap(AssemblyContext& other)
        {
            using std::swap;
            swap(other._loader,   _loader  );
            swap(other._path,     _path    );
            swap(other._database, _database);
            swap(other._name,     _name    );
            swap(other._state,    _state   );
            swap(other._methods,  _methods );
        }

        MetadataLoader     const& GetLoader()       const { VerifyInitialized(); return *_loader.Get(); }
        Metadata::Database const& GetDatabase()     const { VerifyInitialized(); return _database;      }
        String             const& GetPath()         const { VerifyInitialized(); return _path;          }
        AssemblyName       const& GetAssemblyName() const;

        MethodTable const GetOrCreateMethodTable(Metadata::ElementReference const& type) const
        {
            // TODO BLOB REFERENCES COULD END UP HERER TOO
            return _methods.GetOrCreateMethodTable(Metadata::FullReference(&_database, type.AsRowReference()));
        }

        bool IsInitialized() const
        {
            return _loader.Get() != nullptr;
        }

    private:

        AssemblyContext(AssemblyContext const&);
        AssemblyContext operator=(AssemblyContext const&);

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

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

} }

namespace CxxReflect {

    class IMetadataResolver
    {
    public:

        virtual ~IMetadataResolver();

        // When an attempt is made to load an assembly by name, the MetadataLoader calls this
        // overload to resolve the assembly.
        virtual String ResolveAssembly(AssemblyName const& assemblyName) const = 0;

        // When an attempt is made to load an assembly and a type from that assembly is known, this
        // overload is called.  This allows us to support WinRT type universes wherein type
        // resolution is namespace-oriented rather than assembly-oriented.  For non-WinRT resolver
        // implementations, this function may simply defer to the above overload.
        virtual String ResolveAssembly(AssemblyName const& assemblyName,
                                       String       const& namespaceQualifiedTypeName) const = 0;
    };

    #ifdef CXXREFLECT_ENABLE_WINRT_RESOLVER

    class WinRTMetadataResolver : public IMetadataResolver
    {
    public:

        WinRTMetadataResolver();

        String ResolveAssembly(AssemblyName const& assemblyName) const;
        String ResolveAssembly(AssemblyName const& assemblyName, String const& namespaceQualifiedTypeName) const;

    private:

        // TODO Implement
    };

    #endif

    class DirectoryBasedMetadataResolver : public IMetadataResolver
    {
    public:

        typedef std::set<String> DirectorySet;

        DirectoryBasedMetadataResolver(DirectorySet const& directories);

        String ResolveAssembly(AssemblyName const& assemblyName) const;
        String ResolveAssembly(AssemblyName const& assemblyName, String const& namespaceQualifiedTypeName) const;

    private:

        DirectorySet _directories;
    };


    // MetadataLoader is the entry point for the library.  It is used to resolve and load assemblies.
    class MetadataLoader
    {
    public:

        MetadataLoader(std::unique_ptr<IMetadataResolver> resolver);

        Assembly LoadAssembly(String path) const;
        Assembly LoadAssembly(AssemblyName const& name) const;

    public: // internals

        // Searches the set of AssemblyContexts for the one that owns 'database'.  In most cases we
        // should try to keep a pointer to the context itself so that we don't need to use this
        // much.  Once case where we really need this is in resolving DatabaseReference elements;
        // to maintain the physical/logical firewall, we cannot store the context in the reference.
        Detail::AssemblyContext const& GetContextForDatabase(Metadata::Database const& database, InternalKey) const;

        // Resolves a type via a type reference.  The type reference must refer to a TypeDef, TypeRef,
        // or TypeSpec token.  If it is a TypeDef or a TypeRef, the token is returned as-is.  If it is
        // a TypeRef, it is resolved into either a TypeDef or a TypeSpec token in the defining
        // assembly.
        Metadata::FullReference ResolveType(Metadata::FullReference const& typeReference, InternalKey) const;

    private:

        MetadataLoader(MetadataLoader const&);
        MetadataLoader& operator=(MetadataLoader const&);

        std::unique_ptr<IMetadataResolver>                _resolver;
        mutable std::map<String, Detail::AssemblyContext> _contexts;
    };

}

#endif
