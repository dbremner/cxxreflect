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
    class IAssemblyLocator;
    class Loader;
    class Method;
    class Module;
    class Parameter;
    class Property;
    class Type;
    class Utility;
    class Version;

}

namespace CxxReflect { namespace Detail {

    // Represents all of the permanent information about an Assembly.  This is the implementation of
    // an 'Assembly' facade and includes parts of the implementation of other facades (e.g., it
    // stores the method tables for each type in the assembly).  This way, the actual facade types
    // are uber-fast to copy and we can treat them as "references" into the metadata database.
    class AssemblyContext
    {
    public:

        AssemblyContext(Loader const* loader, String uri, Metadata::Database&& database);
        AssemblyContext(AssemblyContext&& other);

        AssemblyContext& operator=(AssemblyContext&& other);

        void Swap(AssemblyContext& other);

        Loader             const& GetLoader()       const;
        Metadata::Database const& GetDatabase()     const;
        String             const& GetLocation()     const;
        AssemblyName       const& GetAssemblyName() const;

        bool IsInitialized() const;

    private:

        AssemblyContext(AssemblyContext const&);
        AssemblyContext operator=(AssemblyContext const&);

        void AssertInitialized() const;

        enum RealizationState
        {
            RealizedName = 0x01
        };

        void RealizeName() const;

        ValueInitialized<Loader const*>            _loader;
        String                                     _uri;
        Metadata::Database                         _database;

        FlagSet<RealizationState>          mutable _state;
        std::unique_ptr<AssemblyName>      mutable _name;
    };





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
        MethodHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                     Metadata::ElementReference const& reflectedTypeReference,
                     MethodContext              const* context);
        MethodHandle(Method const& method);

        Method Realize() const;

        bool IsInitialized() const;

        friend bool operator==(MethodHandle const&, MethodHandle const&);
        friend bool operator< (MethodHandle const&, MethodHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(MethodHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<AssemblyContext const*> _reflectedTypeAssemblyContext;
        Metadata::ElementReference               _reflectedTypeReference;
        ValueInitialized<MethodContext const*>   _context;
    };

    class ParameterHandle
    {
    public:

        ParameterHandle();
        ParameterHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                        Metadata::ElementReference const& reflectedTypeReference,
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

        ValueInitialized<AssemblyContext const*> _reflectedTypeAssemblyContext;
        Metadata::ElementReference               _reflectedTypeReference;
        ValueInitialized<MethodContext const*>   _context;

        Metadata::RowReference                   _parameterReference;
        Metadata::TypeSignature                  _parameterSignature;
    };

    class TypeHandle
    {
    public:

        TypeHandle();
        TypeHandle(AssemblyContext            const* assemblyContext,
                   Metadata::ElementReference const& typeReference);
        TypeHandle(Type const& type);

        Type Realize() const;

        bool IsInitialized() const;

        friend bool operator==(TypeHandle const&, TypeHandle const&);
        friend bool operator< (TypeHandle const&, TypeHandle const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(TypeHandle)

    private:

        void AssertInitialized() const;

        ValueInitialized<AssemblyContext const*> _assemblyContext;
        Metadata::ElementReference               _typeReference;
    };





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

    Assembly GetSystemAssembly(Type const& referenceType);
    Assembly GetSystemAssembly(Assembly const& referenceAssembly);

    Type GetSystemObjectType(Type const& referenceType);
    Type GetSystemObjectType(Assembly const& referenceAssembly);

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





    template <typename TTokenGetter>
    class MetadataTokenStrictWeakOrderingImpl
    {
    public:

        MetadataTokenStrictWeakOrderingImpl(TTokenGetter const& getToken)
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

    class MetadataTokenDefaultGetter
    {
    public:

        template <typename TMember>
        SizeType operator()(TMember const& member) const
        {
            return member.GetMetadataToken();
        }
    };

    MetadataTokenStrictWeakOrderingImpl<MetadataTokenDefaultGetter>
    inline MetadataTokenStrictWeakOrdering()
    {
        return MetadataTokenStrictWeakOrderingImpl<MetadataTokenDefaultGetter>(MetadataTokenDefaultGetter());
    }

    template <typename TTokenGetter>
    MetadataTokenStrictWeakOrderingImpl<TTokenGetter>
    MetadataTokenStrictWeakOrdering(TTokenGetter const& getToken)
    {
        return MetadataTokenStrictWeakOrderingImpl<TTokenGetter>(getToken);
    }



    typedef Detail::InstantiatingIterator
    <
        Metadata::RowReference,
        CustomAttribute,
        Assembly
    > CustomAttributeIterator;





    /// Represents an assembly location, as returned by an IAssemblyLocator implementation.
    class AssemblyLocation
    {
    public:

        enum class Kind
        {
            Uninitialized,
            File,
            Memory
        };

        AssemblyLocation();
        explicit AssemblyLocation(ConstByteRange const& memoryRange);
        explicit AssemblyLocation(String const& filePath);

        Kind GetKind()    const;
        bool IsInFile()   const;
        bool IsInMemory() const;

        ConstByteRange const& GetMemoryRange() const;
        String         const& GetFilePath()    const;

    private:

        ConstByteRange                 _memoryRange;
        String                         _filePath;
        Detail::ValueInitialized<Kind> _kind;

    };




    /// Interface for assembly location.
    ///
    /// An assembly locator is responsible for locating an assembly given an assembly name or a
    /// reference type.  This interface allows different assembly resolution logic to be plugged
    /// into a `Loader`.
    class IAssemblyLocator
    {
    public:

        /// Called to locate an assembly by name.
        ///
        /// When a `Loader` attempts to read metadata from an assembly it has not yet loaded, it
        /// calls this member function to locate the assembly file on disk so it can be loaded.  An
        /// implementer should return an empty string if it cannot find the named assembly.
        ///
        /// \param assemblyName The name of the assembly that needs to be loaded.
        ///
        /// \returns The resolved path to the named assembly, or an empty string if the assembly
        /// could not be located by the assembly locator.
        ///
        /// \nothrows
        virtual AssemblyLocation LocateAssembly(AssemblyName const& assemblyName) const = 0;
        
        /// Called to locate an assembly by name, with a known reference type.
        ///
        /// If the `Loader` knows the name of a type from an assembly that it needs to load, it
        /// will call this member function instead of the assembly name-only `LocateAssembly()`.
        /// This allows type resolution by name, which is used e.g. by the Windows Runtime
        /// integration, which uses metadata files but does not have logical assemblies.
        ///
        /// \param assemblyName The name of the assembly that needs to be loaded.
        /// \param fullTypeName The full name (namespace-qualified name) of a reference type from
        /// the assembly that needs to be loaded.  If the assembly locator supports type resolution
        /// by name, it should use this reference type to resolve the assembly.
        ///
        /// \returns The resolved path to the named assembly, or an empty string if the assembly
        /// could not be located by the assembly locator.
        ///
        /// \nothrows
        virtual AssemblyLocation LocateAssembly(AssemblyName const& assemblyName, String const& fullTypeName) const = 0;

        /// The destructor is virtual, to ensure correct interface semantics.
        virtual ~IAssemblyLocator();
    };





    /// Interface for loader configuration functionality
    ///
    /// The functions of this interface allow different types system implementations to configure
    /// the loader differently (notably, we need special handling of a few Windows Runtime types).
    class ILoaderConfiguration
    {
    public:

        /// Transforms a namespace prior to resolution of a system type
        ///
        /// Prior to resolving a system type, the Loader will call this function to transform the
        /// namespace of the system type.  It should return the original namespace unchanged if it
        /// does not need to be transformed.  This allows us to transform, e.g., System -> Platform.
        ///
        /// \todo This could be better implemented as GetSystemNamespace() or something like that.
        virtual String TransformNamespace(String const& namespaceName) = 0;

        virtual ~ILoaderConfiguration();
    };





    // This class is used to identify member functions (or functions) that are considered "internal"
    // and are not really part of the public interface of the library.  A user of the library should
    // not call internal methods.  However, other libraries or utilities built atop CxxReflect might
    // want to use these internal members, e.g. as we have done in the Windows Runtime integrations.
    class InternalKey
    {
    };
}

#endif
