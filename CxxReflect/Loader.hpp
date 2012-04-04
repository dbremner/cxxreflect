#ifndef CXXREFLECT_LOADER_HPP_
#define CXXREFLECT_LOADER_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// The Loader provides bindings between the physical layer implementation of the metadata reader
// (in CxxReflect::Metadata) and the logical layer interface (in CxxReflect).  The Loader itself
// manages the loading of assemblies and owns all persistent data structures.

#include "CxxReflect/CoreComponents.hpp"

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

        // Note:  The auto_ptr overload is provided for use from C++/CLI, which does not yet support
        // move-only types.  If you aren't using C++/CLI, use the unique_ptr overload.
        Loader(std::auto_ptr<IAssemblyLocator>   assemblyLocator);
        Loader(std::unique_ptr<IAssemblyLocator> assemblyLocator);

        Loader(Loader&& other);
        Loader& operator=(Loader&& other);

        Assembly LoadAssembly(String path) const;
        Assembly LoadAssembly(AssemblyName const& name) const;

    public: // Internal Members

        // Gets the assembly locator being used by this metadata loader.
        IAssemblyLocator const& GetAssemblyLocator(InternalKey) const;

        // Searches the set of AssemblyContexts for the one that owns 'database'.  In most cases we
        // should try to keep a pointer to the context itself so that we don't need to use this
        // much.  Once case where we really need this is in resolving DatabaseReference elements;
        // to maintain the physical/logical firewall, we cannot store the context in the reference.
        Detail::AssemblyContext const& GetContextForDatabase(Metadata::Database const& database, InternalKey) const;

        Metadata::FullReference ResolveType(Metadata::FullReference const& typeReference) const;

        Type GetFundamentalType(Metadata::ElementType const elementType, InternalKey) const;

        Detail::EventContextTable     GetOrCreateEventTable    (Metadata::FullReference const& typeDef, InternalKey) const;
        Detail::FieldContextTable     GetOrCreateFieldTable    (Metadata::FullReference const& typeDef, InternalKey) const;
        Detail::InterfaceContextTable GetOrCreateInterfaceTable(Metadata::FullReference const& typeDef, InternalKey) const;
        Detail::MethodContextTable    GetOrCreateMethodTable   (Metadata::FullReference const& typeDef, InternalKey) const;
        Detail::PropertyContextTable  GetOrCreatePropertyTable (Metadata::FullReference const& typeDef, InternalKey) const;

    private:

        Loader(Loader const&);
        Loader& operator=(Loader const&);

        std::unique_ptr<IAssemblyLocator>                 _assemblyLocator;
        mutable std::map<String, Detail::AssemblyContext> _contexts;

        enum { FundamentalTypeCount = static_cast<std::size_t>(Metadata::ElementType::ConcreteElementTypeMax) };

        // There must be exactly one system assembly and it must define types for each fundamental
        // element type.  It can be a bit expensive to hunt down the type definitions, especially if
        // we frequently need to look them up (which is common in reflection use cases), so we cache
        // the set of fundamental type definitions for the type universe once, here, in the loader:
        mutable std::array<Detail::TypeHandle, FundamentalTypeCount> _fundamentalTypes;

        Detail::ElementContextTableStorageInstance mutable _contextStorage;

        Detail::EventContextTableCollection        mutable _events;
        Detail::FieldContextTableCollection        mutable _fields;
        Detail::InterfaceContextTableCollection    mutable _interfaces;
        Detail::MethodContextTableCollection       mutable _methods;
        Detail::PropertyContextTableCollection     mutable _properties;
    };

}

#endif
