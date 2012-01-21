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

        IMetadataResolver const& GetResolver() const;

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
