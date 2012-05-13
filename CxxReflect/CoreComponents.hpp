#ifndef CXXREFLECT_CORECOMPONENTS_HPP_
#define CXXREFLECT_CORECOMPONENTS_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Guid.hpp"
#include "CxxReflect/ElementContexts.hpp"

namespace CxxReflect {

    class InternalKey;

    class Assembly;
    class AssemblyName;
    class Constant;
    class CustomAttribute;
    class Event;
    class Field;
    class File;
    class Loader;
    class Method;
    class Module;
    class Parameter;
    class Property;
    class Type;
    class Utility;
    class Version;





    /// Represents the location of a module, either on disk (path) or in memory (byte range)
    class ModuleLocation
    {
    public:

        enum class Kind
        {
            Uninitialized,
            File,
            Memory
        };

        ModuleLocation();
        explicit ModuleLocation(ConstByteRange const& memoryRange);
        explicit ModuleLocation(String const& filePath);

        Kind GetKind()       const;
        bool IsFile()        const;
        bool IsMemory()      const;
        bool IsInitialized() const;

        ConstByteRange const& GetMemoryRange() const;
        String         const& GetFilePath()    const;

        String ToString() const;

        friend bool operator==(ModuleLocation const&, ModuleLocation const&);
        friend bool operator< (ModuleLocation const&, ModuleLocation const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(ModuleLocation)

    private:

        ConstByteRange                 _memoryRange;
        String                         _filePath;
        Detail::ValueInitialized<Kind> _kind;
    };





    /// Interface for module and assembly locating
    ///
    /// When a loader needs to load a module or assembly by name it queries this interface to find
    /// the location of the module or assembly.  An implementer must locate the module or assembly
    /// on disk or in memory and return its location.  If the module or assembly cannot be found via
    /// the given information, a default-initialized `ModuleLocation` should be returned to indicate
    /// failure.
    ///
    /// An implementer of this interface should throw no exceptions from any member function.
    class IModuleLocator
    {
    public:
        
        /// Locates the manifest module of an assembly
        ///
        /// This is called whenever an assembly needs to be located by name alone.  If the loader 
        /// has a reference type in the target assembly, it will call the overload that also takes
        /// a full type name.
        ///
        /// \nothrows
        virtual ModuleLocation LocateAssembly(AssemblyName const& assemblyName) const = 0;

        /// Locates the manifest module of an assembly
        ///
        /// This is called whenever an assembly needs to be located when the loader has both the
        /// name of the assembly and the full name of a type in the defining assembly.  A locator
        /// implementation may use the name of the type to resolve the location of the module.
        ///
        /// This overload is provided primarily to support the Windows Runtime type system, which
        /// does not really have assemblies, just metadata files that define types.
        ///
        /// \nothrows
        virtual ModuleLocation LocateAssembly(AssemblyName const& assemblyName,
                                              String       const& fullTypeName) const = 0;

        /// Locates a non-manifest module of an assembly
        ///
        /// When a loader needs to enumerate the modules of a multi-module assembly, it will call
        /// this member function to resolve all of the modules other than the module bearing the
        /// assembly manifest.
        ///
        /// \nothrows
        virtual ModuleLocation LocateModule(AssemblyName const& requestingAssembly,
                                            String       const& moduleName) const = 0;

        virtual ~IModuleLocator();
    };

    typedef std::unique_ptr<IModuleLocator> UniqueModuleLocator;





    /// Interface for loader configuration
    ///
    /// This interface allows configuration of the loader.  It allows its behavior to be changed for
    /// different type systems.
    class ILoaderConfiguration
    {
    public:

        /// Gets the namespace in which the system types are defined
        ///
        /// This should be the top-level system namespace.  E.g., for the CLI, this is "System".
        /// The returned string must be nonempty and must be a valid namespace name.  This is used
        /// by the loader when it resolves fundamental types (like Int32 or Object) and system
        /// infrastructure types (like Array or Enum).
        ///
        /// \nothrows
        virtual StringReference GetSystemNamespace() const = 0;

        virtual ~ILoaderConfiguration();
    };

    typedef std::unique_ptr<ILoaderConfiguration> UniqueLoaderConfiguration;

}

namespace CxxReflect { namespace Detail {

    /// \defgroup cxxreflect_detail_corecontexts Implementation Details :: Core Contexts
    ///
    /// These types make up the core of the CxxReflect metadata system.  They own all of the
    /// persistent state and most other types in the library are simply iterator-like references
    /// into an instance of one of these types.
    ///
    ///                         +------------+      +-----------------+
    ///                     +-->| Assembly 0 |----->| Manifest Module |
    ///                     |   +------------+      +-----------------+
    ///      +----------+   |
    ///      | Loader   |---|
    ///      +----------+   |
    ///                     |   +------------+      +-----------------+
    ///                     +-->| Assembly 1 |--+-->| Manifest Module |
    ///                         +------------+  |   +-----------------+
    ///                                         |
    ///                                         |   +-----------------+
    ///                                         +-->| Other Module    |
    ///                                             +-----------------+
    ///
    /// * **LoaderContext**:  There is exactly one loader context for a type universe.  It owns all
    ///   of the assemblies that are loaded through it, and their lifetimes are tied to it.
    ///
    /// * **AssemblyContext**:  An assembly context is created for each assembly that is loaded
    ///   through a loader.  The assembly context is simply a collection of module contexts.  When
    ///   an assembly is loaded, a single module context is created for its manifest module, which
    ///   is the module that contains the assembly manifest (and a database with an assembly row).
    ///   The assembly context will load any other modules for the assembly when they are required.
    ///
    /// * **ModuleContext**:  A module context represents a single module.  It creates and owns the
    ///   metadata database for the module.
    ///
    /// There is a 1:N mapping of loader context to assembly context, and a 1:N mapping of assembly
    /// context to module context.  Most assemblies have exactly one module.
    ///
    /// @{

    class AssemblyContext;
    class LoaderContext;
    class ModuleContext;





    /// Internal data structure to represent a loaded module
    ///
    /// This type is neither copyable nor movable.
    class ModuleContext
    {
    public:

        /// Constructs a new `ModuleContext`
        ///
        /// \param    assembly A pointer to the `AssemblyContext` that owns this module
        /// \param    location The location from which this module is to be loaded
        /// \throws   RuntimeError If the module at the specified location cannot be loaded
        ModuleContext(AssemblyContext const* assembly, ModuleLocation const& location);

        /// Gets the assembly context that owns this module
        ///
        /// \nothrows
        AssemblyContext const& GetAssembly() const;

        /// Gets the location from which this module was loaded
        ///
        /// \nothrows
        ModuleLocation const& GetLocation() const;

        /// Gets the metadata database for this module
        ///
        /// \nothrows
        Metadata::Database const& GetDatabase() const;

        /// Find the named type definition in this module
        ///
        /// If no type by the provided name is defined in this module, this returns an empty row
        /// reference.  This function does not handle nested types.
        ///
        /// \nothrows
        Metadata::RowReference GetTypeDefByName(StringReference namespaceName, StringReference typeName) const;

    private:

        ModuleContext(ModuleContext const&);
        ModuleContext& operator=(ModuleContext const&);

        /// Loads the module at the provided location and returns the metadata database for it
        ///
        /// \param  location The location from which this module is to be loaded
        /// \throws RuntimeError If the module at the specified location cannot be loaded
        static Metadata::Database CreateDatabase(ModuleLocation const& location);

        ValueInitialized<AssemblyContext const*> _assembly;
        ModuleLocation                           _location;
        Metadata::Database                       _database;
    };

    typedef std::unique_ptr<ModuleContext> UniqueModuleContext;





    /// Internal data structure to represent an assembly and its loaded modules
    ///
    /// This type is neither copyable nor movable.
    class AssemblyContext
    {
    public:

        typedef std::vector<UniqueModuleContext> ModuleContextSequence;

        /// Constructs a new `AssemblyContext`
        ///
        /// \param    loader   A pointer to the loader that loaded and owns this assembly
        /// \param    location The location of the manifest module of the assembly
        /// \nothrows
        AssemblyContext(LoaderContext const* loader, ModuleLocation const& location);

        /// Gets the loader that owns this assembly
        ///
        /// \nothrows
        LoaderContext const& GetLoader() const;

        /// Gets the context for the manifest module of this assembly
        ///
        /// \nothrows
        ModuleContext const& GetManifestModule() const;
        
        /// Gets the modules of this assembly, including the manifest module
        ///
        /// If all of the modules for this assembly have not yet been loaded, this will realize them
        /// before returning.  If realization of any module fails, this function will not return.
        ///
        /// \throws RuntimeError If module resolution fails or if any module cannot be loaded.
        ModuleContextSequence const& GetModules() const;

        /// Gets the name of this assembly
        ///
        /// \todo throws
        AssemblyName const& GetAssemblyName() const;

    private:

        AssemblyContext(AssemblyContext const&);
        AssemblyContext operator=(AssemblyContext const&);

        enum class RealizationState
        {
            Name    = 0x01,
            Modules = 0x02
        };

        void RealizeName()    const;
        void RealizeModules() const;

        ValueInitialized<LoaderContext const*>         _loader;
        std::vector<UniqueModuleContext>       mutable _modules;

        FlagSet<RealizationState>              mutable _state;
        std::unique_ptr<AssemblyName>          mutable _name;
    };

    typedef std::unique_ptr<AssemblyContext> UniqueAssemblyContext;





    /// Synchronization object used by the `LoaderContext`
    ///
    /// This type is pimpl'ed to avoid including `<mutex>` in the public interface headers.
    class LoaderContextSynchronizer;

    typedef std::unique_ptr<LoaderContextSynchronizer> UniqueLoaderContextSynchronizer;





    /// Internal data structure to represent a loader and its infrastructure functionality
    ///
    /// This type is neither copyable nor movable.
    class LoaderContext : public Metadata::ITypeResolver
    {
    public:

        /// Constructs a new loader context, using the provided locator and configuration
        ///
        /// \param locator       The `IModuleLocator` implementation to use for locating modules and
        ///                      assemblies.  The argument may not be null.
        /// \param configuration The `ILoaderConfiguration` implementation with which the loader is
        ///                      to be configured.  The argument may be null; if it is null, a
        ///                      default implementation will be used, which is suitable for use in
        ///                      many cases.
        /// \nothrows
        LoaderContext(UniqueModuleLocator locator, UniqueLoaderConfiguration configuration);

        /// A user-declared destructor is required to clean up the `LoaderContextSynchronizer`
        ///
        /// \nothrows
        ~LoaderContext();

        /// Gets or creates an assembly context for the assembly at the specified location
        ///
        /// The loader will first search to see whether it has already loaded the assembly from the
        /// specified location.  If it has, it will return the assembly context that it already
        /// loaded.  Otherwise, it will attempt to load the assembly's manifest module and create a
        /// new assembly context for it.
        ///
        /// The provided location must be valid; its validity is only checked internally in debug
        /// builds.
        ///
        /// \todo throws 
        AssemblyContext const& GetOrLoadAssembly(ModuleLocation const& location) const;
        AssemblyContext const& GetOrLoadAssembly(AssemblyName const& name) const;
        
        // ITypeResolver implementation
        Metadata::FullReference ResolveType(Metadata::FullReference const& typeReference)   const;
        Metadata::FullReference ResolveFundamentalType(Metadata::ElementType elementType)   const;
        Metadata::FullReference ResolveReplacementType(Metadata::FullReference const& type) const;

        /// Gets the `IModuleLocator` implementation instance that is used for module locating
        ///
        /// \nothrows
        IModuleLocator const& GetLocator() const;

        /// Finds the module that owns the provided database
        ///
        /// The provided database must be owned by one of the modules loaded by this loader.  You
        /// cannot mix-and-match databases, loaders, assemblies, and modules.  It won't work and
        /// you'll be sorry.
        ///
        /// \throws RuntimeError If the database is not owned by this loader.
        ModuleContext const& GetContextForDatabase(Metadata::Database const& database) const;

        /// Registers a module so it can be cached for rapid lookup
        ///
        /// Any time a new module context is created that is owned by this loader, it must be
        /// registered by calling this method.  This registration enables the loader to perform
        /// rapid reverse lookups during type resolution, where we only have a pointer to the 
        /// database, not to its module or assembly.
        ///
        /// This should only be called by the `ModuleContext` constructor.
        ///
        /// \param    module The new module to be registered; must not be null
        /// \nothrows
        void RegisterModule(ModuleContext const* module) const;

        /// Gets the system module that has been loaded by this loader
        ///
        /// This is the manifest module of the assembly that refers to no other assemblies.  In .NET
        /// this is mscorlib.  In other type universes, this may be some other assembly.  For this
        /// function to work, at least one assembly must have been loaded.  If it is not the system
        /// assembly, this function will realize the loaded assembly's dependencies until it finds 
        /// the system assembly.  It is therefore recommended that you load the system assembly
        /// yourself, for optimal performance.
        ///
        /// \throws RuntimeError If no assemblies have yet been loaded.
        ModuleContext const& GetSystemModule() const;

        /// Gets the system namespace associated with this loader
        ///
        /// The loader gets this value from the `ILoaderConfiguration` implementationwith which it
        /// was constructed.  For .NET this is `System`.  In other type universes, this may be some
        /// other namespace (notably, in Windows Runtime, we use `Platform` to match what the C++
        /// build system does and to avoid limitations of ilasm).
        ///
        /// \nothrows
        StringReference GetSystemNamespace() const;




        EventContextTable     GetOrCreateEventTable    (Metadata::FullReference const& type) const;
        FieldContextTable     GetOrCreateFieldTable    (Metadata::FullReference const& type) const;
        InterfaceContextTable GetOrCreateInterfaceTable(Metadata::FullReference const& type) const;
        MethodContextTable    GetOrCreateMethodTable   (Metadata::FullReference const& type) const;
        PropertyContextTable  GetOrCreatePropertyTable (Metadata::FullReference const& type) const;

        static LoaderContext const& From(AssemblyContext const&);
        static LoaderContext const& From(ModuleContext   const&);
        static LoaderContext const& From(Assembly        const&);
        static LoaderContext const& From(Module          const&);
        static LoaderContext const& From(Type            const&);

    private:

        enum
        {
            FundamentalTypeCount = static_cast<SizeType>(Metadata::ElementType::ConcreteElementTypeMax)
        };

        typedef std::map<String, UniqueAssemblyContext>                   AssemblyMap;
        typedef AssemblyMap::value_type                                   AssemblyMapEntry;
        typedef std::map<Metadata::Database const*, ModuleContext const*> ModuleMap;
        typedef ModuleMap::value_type                                     ModuleMapEntry;

        LoaderContext(LoaderContext const&);
        LoaderContext& operator=(LoaderContext const&);

        UniqueModuleLocator       _locator;
        UniqueLoaderConfiguration _configuration;

        /// The set of loaded assemblies, mapped by Absolute URI
        AssemblyMap mutable _assemblies;

        /// A map of each database to the module that owns it, used for rapid reverse lookup
        ModuleMap mutable _moduleMap;

        std::array<Metadata::FullReference, FundamentalTypeCount> _fundamentalTypes;

        ValueInitialized<ModuleContext const*> mutable _systemModule;

        ElementContextTableStorageInstance     mutable _contextStorage;
        EventContextTableCollection            mutable _events;
        FieldContextTableCollection            mutable _fields;
        InterfaceContextTableCollection        mutable _interfaces;
        MethodContextTableCollection           mutable _methods;
        PropertyContextTableCollection         mutable _properties;

        UniqueLoaderContextSynchronizer        mutable _sync;
    };

    typedef std::unique_ptr<LoaderContext> UniqueLoaderContext;


    /// @}





    /// \defgroup cxxreflect_detail_handles Implementation Details :: Interface Handles
    ///
    /// These handle types encapsulate all of the information required to instantiate the
    /// corresponding public interface types, but without being size- or layout-dependent on the 
    /// public interface types.
    ///
    /// This allows us to represent the public interface types without including the actual public
    /// interface headers.  This is important to avoid recursive dependencies between the headers,
    /// and effectively allows us to avoid having to include most of the public interface headers
    /// in other interface headers.
    ///
    /// @{

    class AssemblyHandle
    {
    public:

        AssemblyHandle();
        AssemblyHandle(AssemblyContext const* context);
        AssemblyHandle(Assembly const& assembly);

        Assembly Realize() const;

        bool IsInitialized() const;

        friend bool operator==(AssemblyHandle const&, AssemblyHandle const&);
        friend bool operator< (AssemblyHandle const&, AssemblyHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(AssemblyHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<AssemblyContext const*> _context;
    };

    class MethodHandle
    {
    public:

        MethodHandle();
        MethodHandle(ModuleContext              const* reflectedTypeModule,
                     Metadata::ElementReference const& reflectedType,
                     MethodContext              const* context);
        MethodHandle(Method const& method);

        Method Realize() const;

        bool IsInitialized() const;

        friend bool operator==(MethodHandle const&, MethodHandle const&);
        friend bool operator< (MethodHandle const&, MethodHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(MethodHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<ModuleContext const*> _reflectedTypeModule;
        Metadata::ElementReference             _reflectedType;
        ValueInitialized<MethodContext const*> _context;
    };

    class ModuleHandle
    {
    public:

        ModuleHandle();
        ModuleHandle(ModuleContext const* context);
        ModuleHandle(Module const& module);

        Module Realize() const;
        ModuleContext const& GetContext() const;

        bool IsInitialized() const;

        friend bool operator==(ModuleHandle const&, ModuleHandle const&);
        friend bool operator< (ModuleHandle const&, ModuleHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(ModuleHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<ModuleContext const*> _context;
    };

    class ParameterHandle
    {
    public:

        ParameterHandle();
        ParameterHandle(ModuleContext              const* reflectedTypeModule,
                        Metadata::ElementReference const& reflectedType,
                        MethodContext              const* context,
                        Metadata::RowReference     const& parameterReference,
                        Metadata::TypeSignature    const& parameterSignature);
        ParameterHandle(Parameter const& parameter);

        Parameter Realize() const;

        bool IsInitialized() const;

        friend bool operator==(ParameterHandle const&, ParameterHandle const&);
        friend bool operator< (ParameterHandle const&, ParameterHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(ParameterHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<ModuleContext const*>   _reflectedTypeModule;
        Metadata::ElementReference               _reflectedType;
        ValueInitialized<MethodContext const*>   _context;

        Metadata::RowReference                   _parameterReference;
        Metadata::TypeSignature                  _parameterSignature;
    };

    class TypeHandle
    {
    public:

        TypeHandle();
        TypeHandle(ModuleContext const* module, Metadata::ElementReference const& type);
        TypeHandle(Type const& type);

        Type Realize() const;

        bool IsInitialized() const;

        friend bool operator==(TypeHandle const&, TypeHandle const&);
        friend bool operator< (TypeHandle const&, TypeHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(TypeHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<ModuleContext const*> _module;
        Metadata::ElementReference             _type;
    };

    /// @}





    class ParameterData
    {
    public:

        ParameterData();

        // Note:  This constructor takes an InternalKey only so that it matches other constructors
        // of types with which the parameter iterator is instantiated.
        ParameterData(Metadata::RowReference                       const& parameter,
                      Metadata::MethodSignature::ParameterIterator const& signature,
                      InternalKey);

        Metadata::RowReference  const& GetParameter() const;
        Metadata::TypeSignature const& GetSignature() const;

        bool IsInitialized() const;

        ParameterData& operator++();
        ParameterData  operator++(int);

        friend bool operator==(ParameterData const&, ParameterData const&);
        friend bool operator< (ParameterData const&, ParameterData const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(ParameterData)

    private:

        void AssertInitialized() const;

        Metadata::RowReference                       _parameter;
        Metadata::MethodSignature::ParameterIterator _signature;
    };

    template
    <
        typename TType,
        typename TMember,
        typename TMemberContext,
        bool (*FFilter)(BindingFlags, TType const&, TMemberContext const&)
    >
    class MemberIterator;





    // Tests whether 'assembly' is the system assembly (i.e., the assembly that references no
    // other assemblies).  This is usually mscorlib.dll.
    bool IsSystemAssembly(Assembly const& assembly);

    // Tests whether 'type' is the system type named 'systemTypeNamespace.systemTypeSimpleName'.
    bool IsSystemType(Type            const& type,
                      StringReference const& systemTypeNamespace,
                      StringReference const& systemTypeSimpleName);

    bool IsDerivedFromSystemType(Type const&, Metadata::ElementType, bool);

    // Tests whether 'type' is derived from the named system type, optionally including the type
    // itself.  This is used to test whether a type is contextual, an enumeration, etc.
    bool IsDerivedFromSystemType(Type            const& type,
                                 StringReference const& systemTypeNamespace,
                                 StringReference const& systemTypeSimpleName,
                                 bool                   includeSelf);





    class MetadataTokenDefaultGetter
    {
    public:

        template <typename TMember>
        SizeType operator()(TMember const& member) const
        {
            return member.GetMetadataToken();
        }
    };





    template <typename TTokenGetter>
    class MetadataTokenStrictWeakOrderingImplementation
    {
    public:

        MetadataTokenStrictWeakOrderingImplementation(TTokenGetter const& getToken = TTokenGetter())
            : _getToken(getToken)
        {
        }

        template <typename TMember>
        bool operator()(TMember const& lhs, TMember const& rhs) const
        {
            return _getToken(lhs) < _getToken(rhs);
        }

    private:

        TTokenGetter _getToken;
    };

} }

namespace CxxReflect {





    struct MetadataTokenEqualsComparer
    {
        template <typename TMember>
        bool operator()(TMember const& lhs, TMember const& rhs) const
        {
            return lhs.GetMetadataToken() == rhs.GetMetadataToken();
        }
    };

    Detail::MetadataTokenStrictWeakOrderingImplementation<Detail::MetadataTokenDefaultGetter>
    inline MetadataTokenStrictWeakOrdering()
    {
        return Detail::MetadataTokenStrictWeakOrderingImplementation<Detail::MetadataTokenDefaultGetter>();
    }

    template <typename TTokenGetter>
    Detail::MetadataTokenStrictWeakOrderingImplementation<TTokenGetter>
    MetadataTokenStrictWeakOrdering(TTokenGetter const& getToken)
    {
        return Detail::MetadataTokenStrictWeakOrderingImplementation<TTokenGetter>(getToken);
    }



    typedef Detail::InstantiatingIterator
    <
        Metadata::RowReference,
        CustomAttribute,
        Module
    > CustomAttributeIterator;





    /// Key type for internal member functions
    ///
    /// This class type is the type of the last parameter of any public member functions of public
    /// interface types that expose internal implementation details.  These functions are not really
    /// part of the public interface of the library.  A user of the library should not call these
    /// internal methods.  However, libraries or utilities that build atop CxxReflect might want to
    /// use these internal members, e.g. as we have done in the Windows Runtime integration.
    class InternalKey
    {
    };
}

#endif
