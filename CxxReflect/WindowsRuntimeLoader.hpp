#ifndef CXXREFLECT_WINDOWSRUNTIMELOADER_HPP_
#define CXXREFLECT_WINDOWSRUNTIMELOADER_HPP_

//               Copyright James P. McNellis (james@jamesmcnellis.com) 2011 - 2012.               //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/WindowsRuntimeCommon.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace CxxReflect { namespace WindowsRuntime {

    // An IAssemblyLocator that finds assemblies in the current package.  Assemblies are resolved
    // using RoResolveNamespace, and system types are redirected to our faux platform metadata.
    class PackageAssemblyLocator : public IAssemblyLocator
    {
    public:

        typedef std::map<String, String> PathMap;

        PackageAssemblyLocator(String const& packageRoot);

        virtual String LocateAssembly(AssemblyName const& assemblyName) const;
        virtual String LocateAssembly(AssemblyName const& assemblyName, String const& fullTypeName) const;

        // TODO We should replace this with something a bit less expensive.  Since we need to sync
        // to access _metadataFiles, direct iterator access is a bit tricky.  This will suffice for
        // the moment.
        PathMap GetMetadataFiles() const;

        String FindMetadataFileForNamespace(String const& namespaceName) const;

    private:

        typedef std::recursive_mutex    Mutex;
        typedef std::unique_lock<Mutex> Lock;

        String          _packageRoot;
        PathMap mutable _metadataFiles;
        Mutex   mutable _sync;
    };





    // Implementation of ILoaderConfiguration used by the Windows Runtime bindings.
    class LoaderConfiguration : public ILoaderConfiguration
    {
    public:

        virtual String TransformNamespace(String const& namespaceName);
    };





    // TODO Documentation
    class LoaderContext
    {
    public:

        typedef PackageAssemblyLocator  Locator;

        LoaderContext(std::unique_ptr<Loader>&& loader);

        Loader  const& GetLoader()  const;
        Locator const& GetLocator() const;

        Type GetType(StringReference typeFullName) const;
        Type GetType(StringReference namespaceName, StringReference typeSimpleName) const;

        std::vector<Type> GetImplementers(Type const& interfaceType) const;

        Type GetActivationFactoryType(Type const& type);

        Guid GetGuid(Type const& type);

        #define CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DECLARE_PROPERTY(XTYPE, XNAME)      \
            private:                                                                        \
                mutable Detail::ValueInitialized<bool> _delayInit ## XNAME ## Initialized;  \
                mutable XTYPE                          _delayInit ## XNAME;                 \
                                                                                            \
            public:                                                                         \
                XTYPE Get ## XNAME() const;

        CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DECLARE_PROPERTY(Type,   ActivatableAttributeType);
        CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DECLARE_PROPERTY(Type,   GuidAttributeType);

        CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DECLARE_PROPERTY(Method, ActivatableAttributeFactoryConstructor);

        #undef CXXREFLECT_WINDOWSRUNTIME_LOADERCONTEXT_DECLARE_PROPERTY

    private:

        typedef std::recursive_mutex    Mutex;
        typedef std::unique_lock<Mutex> Lock;

        LoaderContext(LoaderContext const&);
        LoaderContext& operator=(LoaderContext const&);

        std::unique_ptr<Loader>         _loader;
        Mutex                   mutable _sync;
    };





    // A global instance of the LoaderContext.  Most of the time it only makes sense to have a
    // single LoaderContext that maintains the type system for the current application package.
    // This global instance is used for that.  The static and nonmember type system functions in
    // the WindowsRuntime namespace all use this global instance of the LoaderContext.
    class GlobalLoaderContext
    {
    public:

        typedef std::atomic<bool>                 InitializedFlag;
        typedef std::unique_ptr<LoaderContext>    UniqueContext;
        typedef std::shared_future<UniqueContext> UniqueContextFuture;

        // Called to initialize the global instance.  This can only be called once.  Subsequent
        // calls will throw a LogicError.
        static void Initialize(UniqueContextFuture&& context);

        // Gets the global instance.  If the global instance is not yet ready, this blocks until it
        // is.  If you compile with /CX support and this call tries to block, it will throw (this is
        // the behavior of std::future with /CX support, unfortunately).
        static LoaderContext& Get();

        // Returns true if Initialize() has been called.
        static bool HasInitializationBegun();

        // Returns true if Initialize() has been called and a call to Get() will not block.
        static bool IsInitialized();

    private:

        // This type is not constructable.
        GlobalLoaderContext();
        GlobalLoaderContext(GlobalLoaderContext const&);
        GlobalLoaderContext& operator=(GlobalLoaderContext const&);

        static InitializedFlag     _initialized;
        static UniqueContextFuture _context;
    };





    // This begins initialization of the global Windows Runtime loader.  You must call this function
    // once before using any of the CxxReflect library functionality.  Do not call this multiple
    // times.
    void BeginInitialization();

    // Tests whether BeginInitialization() has been called.
    bool HasInitializationBegun();

    // Tests whether BeginInitialization() has been called and initialization has completed.  If
    // this returns true, then calls to the public CxxReflect API functionality will not block.
    bool IsInitialized();

    // Calls 'callable' on a worker thread after the global loader has initialized.  This can be
    // used to avoid blocking on the STA.
    void WhenInitializedCall(std::function<void()> callable);

    // TODO We should also provide a WhenInitializedMarshal() that marshals back onto the STA before
    // calling the callable object.  This would make for even cleaner usage of this method. Also, we
    // should expose a way to attach tasks to the chain waiting for initialization to complete.  If
    // we could just call GlobalLoaderContext::Then(), that would be much simpler.

} }

#endif // CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif
