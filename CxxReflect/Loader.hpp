//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// The Loader provides bindings between the physical layer implementation of the metadata reader
// (in CxxReflect::Metadata) and the logical layer interface (in CxxReflect).  The Loader itself
// manages the loading of assemblies and owns all persistent data structures.
#ifndef CXXREFLECT_LOADER_HPP_
#define CXXREFLECT_LOADER_HPP_

#include "CxxReflect/CoreInternals.hpp"

namespace CxxReflect {

    class DirectoryBasedAssemblyLocator : public IAssemblyLocator
    {
    public:

        typedef std::set<String> DirectorySet;

        DirectoryBasedAssemblyLocator(DirectorySet const& directories);

        String LocateAssembly(AssemblyName const& assemblyName) const;
        String LocateAssembly(AssemblyName const& assemblyName, String const& fullTypeName) const;

    private:

        DirectorySet _directories;
    };


    // Loader is the entry point for the library.  It is used to resolve and load assemblies.
    class Loader : public Metadata::ITypeResolver
    {
    public:

        Loader(std::unique_ptr<IAssemblyLocator>   assemblyLocator);

        Loader(Loader&& other);
        Loader& operator=(Loader&& other);

        Assembly LoadAssembly(String path) const;
        Assembly LoadAssembly(AssemblyName const& name) const;

    public: // internals

        // Gets the assembly locator being used by this metadata loader.
        IAssemblyLocator const& GetAssemblyLocator(InternalKey) const;

        // Searches the set of AssemblyContexts for the one that owns 'database'.  In most cases we
        // should try to keep a pointer to the context itself so that we don't need to use this
        // much.  Once case where we really need this is in resolving DatabaseReference elements;
        // to maintain the physical/logical firewall, we cannot store the context in the reference.
        Detail::AssemblyContext const& GetContextForDatabase(Metadata::Database const& database, InternalKey) const;

        Metadata::FullReference ResolveType(Metadata::FullReference const& typeReference) const;

    private:

        Loader(Loader const&);
        Loader& operator=(Loader const&);

        std::unique_ptr<IAssemblyLocator>                 _assemblyLocator;
        mutable std::map<String, Detail::AssemblyContext> _contexts;
    };

}

#endif
