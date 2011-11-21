//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATALOADER_HPP_
#define CXXREFLECT_METADATALOADER_HPP_

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataDatabase.hpp"

#include <set>
#include <utility>

namespace CxxReflect { namespace Detail {

    class MethodReference
    {
    public:

        MethodReference()
        {
        }

        MethodReference(Metadata::Database const* const database,
                        Metadata::TableReference const declaringType,
                        Metadata::TableReference const method)
            : _database(database), _declaringType(declaringType), _method(method)
        {
            Detail::VerifyNotNull(database);
            Detail::Verify([&]{ return declaringType.IsInitialized(); });
            Detail::Verify([&]{ return method.IsInitialized(); });
        }

        // Resolves the MethodReference as a Method using the provided reflected type
        Method Resolve(Type const& reflectedType) const;

    private:

        Detail::ValueInitialized<Metadata::Database const*> _database;
        Metadata::TableReference                            _declaringType; // Reference into TypeDef table
        Metadata::TableReference                            _method;        // Reference into MethodDef table
    };

    typedef Detail::LinearArrayAllocator<Detail::MethodReference, 2048> MethodTableAllocator;

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

        Detail::ValueInitialized<MetadataLoader const*> _loader;
        String                                          _path;
        Metadata::Database                              _database;

        mutable Detail::FlagSet<RealizationState>       _state; 
        mutable AssemblyName                            _name; 

        mutable Detail::MethodTableAllocator                            _methodTableAllocator;
        mutable std::map<SizeType, Detail::MethodTableAllocator::Range> _methodTableIndices;
    };

} }

namespace CxxReflect {

    class IMetadataResolver
    {
    public:

        virtual ~IMetadataResolver();

        virtual std::wstring ResolveAssembly(AssemblyName const& assemblyName) const = 0;
        virtual std::wstring ResolveAssembly(AssemblyName const& assemblyName,
                                             String       const& typeFullName) const = 0;
    };

    #ifdef CXXREFLECT_ENABLE_WINRT_RESOLVER

    class WinRTMetadataResolver : public IMetadataResolver
    {
    public:

        WinRTMetadataResolver();
        ~WinRTMetadataResolver();

        std::wstring ResolveAssembly(AssemblyName const& assemblyName) const;
        std::wstring ResolveAssembly(AssemblyName const& assemblyName,
                                     String       const& typeFullName) const;

    private:

        // TODO Implement
    };

    #endif

    class DirectoryBasedMetadataResolver : public IMetadataResolver
    {
    public:

        typedef std::set<String> DirectorySet;

        DirectoryBasedMetadataResolver(DirectorySet const& directories);

        String ResolveAssembly(AssemblyName const& name) const;
        String ResolveAssembly(AssemblyName const& assemblyName, String const& typeFullName) const;

    private:

        DirectorySet _directories;
    };

    class MetadataLoader
    {
    public:

        MetadataLoader(std::unique_ptr<IMetadataResolver> resolver);

        Assembly LoadAssembly(String path) const;
        Assembly LoadAssembly(AssemblyName const& name) const;

    public: // internals

        // Searches the set of AssemblyContexts for the one that owns 'database'.
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
