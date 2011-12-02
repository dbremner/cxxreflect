//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATALOADER_HPP_
#define CXXREFLECT_METADATALOADER_HPP_

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/MetadataSignature.hpp"

namespace CxxReflect { namespace Detail {

    typedef Detail::LinearArrayAllocator<Byte, (1 << 16)> MethodSignatureAllocator;

    class MethodReference
    {
    public:

        MethodReference()
        {
        }

        MethodReference(Metadata::DatabaseReference const typeDef,
                        Metadata::TableReference    const methodDef)
            : _typeDef(typeDef),
              _methodDef(methodDef)
        {
            Detail::Verify([&]{ return typeDef.IsInitialized();   });
            Detail::Verify([&]{ return methodDef.IsInitialized(); });
        }

        MethodReference(Metadata::DatabaseReference     const typeDef,
                        Metadata::TableReference        const methodDef,
                        Metadata::DatabaseReference     const typeSpec,
                        MethodSignatureAllocator::Range const instantiatedSignature)
            : _typeDef(typeDef),
              _methodDef(methodDef),
              _typeSpec(typeSpec),
              _instantiatedSignature(instantiatedSignature)
        {
            Detail::Verify([&]{ return typeDef.IsInitialized();   });
            Detail::Verify([&]{ return methodDef.IsInitialized(); });
            Detail::Verify([&]{ return typeSpec.IsInitialized();  });
            // Note: 'instantiatedSignature' may not be initialized
        }

        // Resolves the MethodReference as a Method using the provided reflected type
        Method Resolve(Type const& reflectedType) const;

        Metadata::DatabaseReference GetDeclaringType() const
        {
            VerifyInitialized();
            return _typeDef;
        }
        
        Metadata::DatabaseReference GetMethod() const
        {
            VerifyInitialized();
            return Metadata::DatabaseReference(&_typeDef.GetDatabase(), _methodDef);
        }

        Metadata::MethodDefRow GetMethodDefinition() const
        {
            VerifyInitialized();
            return _typeDef.GetDatabase().GetRow<Metadata::TableId::MethodDef>(_methodDef.GetIndex());
        }

        Metadata::MethodSignature GetMethodSignature() const
        {
            VerifyInitialized();
            if (HasInstantiatedSignature())
            {
                return Metadata::MethodSignature(
                    _instantiatedSignature.Begin(),
                    _instantiatedSignature.End());
            }
            else
            {
                return _typeDef
                    .GetDatabase()
                    .GetBlob(GetMethodDefinition().GetSignature())
                    .As<Metadata::MethodSignature>();
            }
        }

        bool HasInstantiatedType() const
        {
            return _typeSpec.IsInitialized();
        }

        Metadata::DatabaseReference GetInstantiatedType() const
        {
            Verify([&]{ return HasInstantiatedType(); });
            return _typeSpec;
        }

        bool HasInstantiatedSignature() const
        {
            return _instantiatedSignature.IsInitialized();
        }

        MethodSignatureAllocator::Range GetInstantiatedSignature() const
        {
            Verify([&]{ return HasInstantiatedSignature(); });
            return _instantiatedSignature;
        }

        bool IsInitialized() const { return _typeDef.IsInitialized(); }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

        // _typeDef is the type that defines the method.  _methodDef is the method definition; it is
        // resolved in the same database as _typeDef.  _typeSpec is the type through which the method
        // is referred; this is usually set for e.g. an instantiated generic type.  If _typeSpec is
        // set and the method uses any of the _typeSpec's generic parameters, _instantiatedSignature
        // will be set and will point to a replacement signature that has all of the variables (Var!0)
        // replaced with their arguments.
        Metadata::DatabaseReference     _typeDef;
        Metadata::TableReference        _methodDef;
        Metadata::DatabaseReference     _typeSpec;
        MethodSignatureAllocator::Range _instantiatedSignature;
    };

    typedef Detail::LinearArrayAllocator<Detail::MethodReference, 2048> MethodTableAllocator;

    // Represents all of the permanent information about an Assembly.  This is the implementation of
    // an 'Assembly' facade and includes parts of the implementation of other facades (e.g., it
    // stores the method tables for each type in the assembly).  This way, the actual facade types
    // are uber-fast to copy and we can treat them as "references" into the metadata database.
    class AssemblyContext
    {
    public:

        AssemblyContext(MetadataLoader const* const loader, String path, Metadata::Database&& database)
            : _loader(loader), _path(std::move(path)), _database(std::move(database))
        {
            Detail::VerifyNotNull(_loader.Get());
            Detail::Verify([&]{ return !_path.empty(); });
        }

        AssemblyContext(AssemblyContext&& other)
            : _loader(other._loader),
                _path(std::move(other._path)),
                _database(std::move(other._database)),
                _name(std::move(other._name)),
                _state(other._state),
                _methodTableAllocator(std::move(other._methodTableAllocator)),
                _methodTableIndices(std::move(other._methodTableIndices))
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
            swap(other._loader,                _loader);
            swap(other._path,                  _path);
            swap(other._database,              _database);
            swap(other._name,                  _name);
            swap(other._state,                 _state);
            swap(other._methodTableAllocator,  _methodTableAllocator);
            swap(other._methodTableIndices,    _methodTableIndices);
        }

        MetadataLoader     const& GetLoader()       const { VerifyInitialized(); return *_loader.Get(); }
        Metadata::Database const& GetDatabase()     const { VerifyInitialized(); return _database;      }
        String             const& GetPath()         const { VerifyInitialized(); return _path;          }
        AssemblyName       const& GetAssemblyName() const;

        Detail::MethodTableAllocator::Range GetMethodTableForType(SizeType const typeIndex) const
        {
            auto const it(_methodTableIndices.find(typeIndex));
            return it != _methodTableIndices.end()
                ? it->second
                : Detail::MethodTableAllocator::Range();
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

        mutable FlagSet<RealizationState>       _state;
        mutable AssemblyName                    _name;

        mutable MethodTableAllocator                            _methodTableAllocator;
        mutable MethodSignatureAllocator                        _methodSignatureAllocator;
        mutable std::map<SizeType, MethodTableAllocator::Range> _methodTableIndices;
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
        Metadata::DatabaseReference ResolveType(Metadata::DatabaseReference const& typeReference, InternalKey) const;

    private:

        MetadataLoader(MetadataLoader const&);
        MetadataLoader& operator=(MetadataLoader const&);

        std::unique_ptr<IMetadataResolver>                _resolver;
        mutable std::map<String, Detail::AssemblyContext> _contexts;
    };

}

#endif
