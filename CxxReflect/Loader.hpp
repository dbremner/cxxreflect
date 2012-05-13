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

    /// \ingroup cxxreflect_public_interface
    ///
    /// @{





    /// A module locator that searches for an assembly in a set of directories.
    ///
    /// This assembly locator is constructed with a set of directories in which it is to search for
    /// assemblies.  It requires that the file name of an assembly matches its simple name, with an
    /// added .dll or .exe extension.
    class DirectoryBasedModuleLocator : public IModuleLocator
    {
    public:

        typedef std::set<String> DirectorySet;

        /// Constructs a new `DirectoryBasedModuleLocator`.
        ///
        /// \param directories The set of directories that are searched when `LocateAssembly` is
        /// called.  The directories are searched in sorted order.  It is expected that an assembly
        /// is located in only one of the provided directories.
        DirectoryBasedModuleLocator(DirectorySet const& directories);

        virtual ModuleLocation LocateAssembly(AssemblyName const& assemblyName) const;
        virtual ModuleLocation LocateAssembly(AssemblyName const& assemblyName, String const& fullTypeName) const;

        virtual ModuleLocation LocateModule(AssemblyName const& requestingAssembly, String const& moduleName) const;

    private:

        DirectorySet _directories;
    };





    /// A `Loader` is responsible for loading assemblies
    class Loader
    {
    public:

        /// Constructs a new `Loader` instance
        ///
        /// This overload is identical to the overload that has unique_ptr paramters.  This overload
        /// is provided for use in C++/CLI only, because C++/CLI does not support move-only types
        /// like unique_ptr.  If you are not using C++/CLI, do not call this overload.
        explicit Loader(std::auto_ptr<IModuleLocator>       locator,
                        std::auto_ptr<ILoaderConfiguration> configuration = std::auto_ptr<ILoaderConfiguration>(nullptr));

        explicit Loader(UniqueModuleLocator locator, UniqueLoaderConfiguration configuration = nullptr);

        Loader(Loader&& other);
        Loader& operator=(Loader&& other);

        Assembly LoadAssembly(String         const& pathOrUri) const;
        Assembly LoadAssembly(ModuleLocation const& location)  const;
        Assembly LoadAssembly(AssemblyName   const& name)      const;

        /// Gets the `IModuleLocator` implementation instance that is used for module locating
        ///
        /// \nothrows
        IModuleLocator const& GetLocator() const;

        /// Tests whether this `Loader` is initialized
        ///
        /// This is `true` for all `Loader` instance except those that have been moved from.
        ///
        /// \nothrows
        bool IsInitialized() const;

    public: // Internal Members

        Detail::LoaderContext const& GetContext(InternalKey) const;

    private:

        Loader(Loader const&);
        Loader& operator=(Loader const&);

        void AssertInitialized() const;

        Detail::UniqueLoaderContext _context;
    };

    /// @}

}

#endif
