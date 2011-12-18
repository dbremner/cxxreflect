//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// MetadataLoader provides bindings between the physical layer implementation of the metadata reader
// (in CxxReflect::Metadata) and the logical layer interface (in CxxReflect).  The MetadataLoader
// itself manages the loading of assemblies and owns all persistent data structures.
#ifndef CXXREFLECT_METADATALOADER_HPP_
#define CXXREFLECT_METADATALOADER_HPP_

#include "CxxReflect/CoreInternals.hpp"

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
