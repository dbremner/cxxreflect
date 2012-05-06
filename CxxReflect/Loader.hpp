#ifndef CXXREFLECT_LOADER_HPP_
#define CXXREFLECT_LOADER_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// The Loader provides bindings between the physical layer implementation of the metadata reader
// (in CxxReflect::Metadata) and the logical layer interface (in CxxReflect).  The Loader itself
// manages the loading of assemblies and owns all persistent data structures.

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect { namespace Detail {

    /// Provides a synchronization object for the `Loader`.
    ///
    /// We cannot include <mutex> in the public interface headers because a C++/CLI translation unit
    /// will fail to compile if <mutex> is included.  Therefore, all usage of <mutex> in the type
    /// system interface is pimpl'ed, in this case by using this synchronization context.
    ///
    /// Thanks, C++/CLI!
    class LoaderSynchronizationContext;

    ConstByteIterator BeginCliTypeSystemSupportEmbedded();
    ConstByteIterator EndCliTypeSystemSupportEmbedded();

} }

namespace CxxReflect {

    /// An assembly locator that searches for an assembly in a set of directories.
    ///
    /// This assembly locator is constructed with a set of directories in which it is to search for
    /// assemblies.  It requires that the file name of an assembly matches its simple name, with an
    /// added .dll or .exe extension.
    class DirectoryBasedAssemblyLocator : public IAssemblyLocator
    {
    public:

        typedef std::set<String> DirectorySet;

        /// Constructs a new `DirectoryBasedAssemblyLocator`.
        ///
        /// \param directories The set of directories that are searched when `LocateAssembly` is
        /// called.  The directories are searched in sorted order.  It is expected that an assembly
        /// is located in only one of the provided directories.
        DirectoryBasedAssemblyLocator(DirectorySet const& directories);

        virtual AssemblyLocation LocateAssembly(AssemblyName const& assemblyName) const;
        virtual AssemblyLocation LocateAssembly(AssemblyName const& assemblyName, String const& fullTypeName) const;

    private:

        DirectorySet _directories;
    };





    /// A `Loader` is responsible for loading assemblies and serves as the root of a type universe.
    ///
    /// Note that all of the membr functions of `Loader` are const-qualified.  Many of the member
    /// functions do modify internal state of the `Loader`, but none of them mutate observable state.
    /// A type system, rooted in a `Loader`, is immutable.  Assemblies, types, methods, and other
    /// kinds of entities do not change.
    class Loader : public Metadata::ITypeResolver
    {
    public:

        /// Constructs a new `Loader` instance.
        ///
        /// This overload is identical to the overload that has unique_ptr paramters.  This overload
        /// is provided for use in C++/CLI only, because C++/CLI does not support move-only types
        /// like unique_ptr.  If you are not using C++/CLI, do not call this overload.
        explicit Loader(std::auto_ptr<IAssemblyLocator>       assemblyLocator,
                        std::auto_ptr<ILoaderConfiguration>   loaderConfiguration = std::auto_ptr<ILoaderConfiguration>(nullptr));

        explicit Loader(std::unique_ptr<IAssemblyLocator>     assemblyLocator,
                        std::unique_ptr<ILoaderConfiguration> loaderConfiguration = nullptr);

        ~Loader();

        Loader(Loader&& other);
        Loader& operator=(Loader&& other);

        Assembly LoadAssembly(AssemblyLocation const& location) const;
        Assembly LoadAssembly(AssemblyName     const& name)     const;

    public: // Internal Members

        // Gets the assembly locator being used by this metadata loader.
        IAssemblyLocator const& GetAssemblyLocator(InternalKey) const;

        // Searches the set of AssemblyContexts for the one that owns 'database'.  In most cases we
        // should try to keep a pointer to the context itself so that we don't need to use this
        // much.  Once case where we really need this is in resolving DatabaseReference elements;
        // to maintain the physical/logical firewall, we cannot store the context in the reference.
        Detail::AssemblyContext const& GetContextForDatabase(Metadata::Database const& database, InternalKey) const;

        Metadata::FullReference ResolveType(Metadata::FullReference const& typeReference) const;
        Metadata::FullReference ResolveFundamentalType(Metadata::ElementType elementType) const;
        Metadata::FullReference Loader::ResolveReplacementType(Metadata::FullReference const& type) const;

        Type GetFundamentalType(Metadata::ElementType elementType, InternalKey) const;

        String TransformNamespace(String const& namespaceName, InternalKey) const;

        Detail::EventContextTable     GetOrCreateEventTable    (Metadata::FullReference const& typeDef, InternalKey) const;
        Detail::FieldContextTable     GetOrCreateFieldTable    (Metadata::FullReference const& typeDef, InternalKey) const;
        Detail::InterfaceContextTable GetOrCreateInterfaceTable(Metadata::FullReference const& typeDef, InternalKey) const;
        Detail::MethodContextTable    GetOrCreateMethodTable   (Metadata::FullReference const& typeDef, InternalKey) const;
        Detail::PropertyContextTable  GetOrCreatePropertyTable (Metadata::FullReference const& typeDef, InternalKey) const;

    private:

        Loader(Loader const&);
        Loader& operator=(Loader const&);

        std::unique_ptr<IAssemblyLocator>                 _assemblyLocator;
        std::unique_ptr<ILoaderConfiguration>             _loaderConfiguration;
        mutable std::map<String, Detail::AssemblyContext> _contexts;

        enum { FundamentalTypeCount = static_cast<std::size_t>(Metadata::ElementType::ConcreteElementTypeMax) };

        // There must be exactly one system assembly and it must define types for each fundamental
        // element type.  It can be a bit expensive to hunt down the type definitions, especially if
        // we frequently need to look them up (which is common in reflection use cases), so we cache
        // the set of fundamental type definitions for the type universe once, here, in the loader:
        mutable std::array<Detail::TypeHandle, FundamentalTypeCount> _fundamentalTypes;

        Detail::ElementContextTableStorageInstance    mutable _contextStorage;

        Detail::EventContextTableCollection           mutable _events;
        Detail::FieldContextTableCollection           mutable _fields;
        Detail::InterfaceContextTableCollection       mutable _interfaces;
        Detail::MethodContextTableCollection          mutable _methods;
        Detail::PropertyContextTableCollection        mutable _properties;

        std::unique_ptr<Detail::LoaderSynchronizationContext> _sync;
    };

}

#endif
