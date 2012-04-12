#ifndef CXXREFLECT_WINDOWSRUNTIME_HPP_
#define CXXREFLECT_WINDOWSRUNTIME_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "CxxReflect/Guid.hpp"
#include "CxxReflect/Type.hpp"

// To avoid including any non-standard headers in our header files, we forward declare all COM types
// that we use in various methods.
typedef struct _GUID GUID;
struct IInspectable;
struct IUnknown;

// Throughout this header, many functions are usable only with the C++/CX language extensions.  We
// only compile these when C++/CX is enabled.  These functions are entirely header-only, which means
// we can compile the whole library without C++/CX support, but still use the C++/CX-specific
// functions in C++/CX projects.
#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX

#include <ppl.h>
#include <ppltasks.h>

namespace CxxReflect { namespace Detail {

    // Utility function for making a Windows Runtime async call synchronous.  When we're already
    // offloaded onto a worker thread and we need the answer immediately, the async API is just
    // an unnecessary beating.
    template <typename TCallable>
    auto SyncCall(TCallable&& callable) -> decltype(callable()->GetResults())
    {
        typedef decltype(callable()->GetResults()) Result;
        ::Concurrency::task<Result> asyncTask(callable());
        asyncTask.wait();
        return asyncTask.get();
    }

    template <typename T> struct IsHat         { enum { value = false }; };
    template <typename T> struct IsHat<T^>     { enum { value = true; }; };
    template <typename T> struct AddHat        { typedef T^ Type;        };
    template <typename T> struct AddHat<T^>    {                         };
    template <typename T> struct RemoveHat     {                         };
    template <typename T> struct RemoveHat<T^> { typedef T Type;         };

} }

#endif

namespace CxxReflect { namespace WindowsRuntime {

    // This is the name of the CxxReflect library's Platform metadata file.  This file is compiled
    // when you build CxxReflect.  You must include it in the root of your application package as
    // content in order to use CxxReflect.
    static const wchar_t* const PlatformMetadataFileName(L"CxxReflectPlatform.dat");





    // A deleter for IUnknown and IInspectable objects that calls Release to delete and a unique_ptr
    // instantiation that can be used to own an IInspectable object.
    class InspectableDeleter
    {
    public:

        void operator()(IInspectable* inspectable);
    };

    typedef std::unique_ptr<IInspectable, InspectableDeleter> UniqueInspectable;





    // We encapsulate most of the Windows Runtime-specific functionality into this LoaderContext
    // class.  It is befriended by InternalKey so that it has access to the library internals, and
    // its functionality is exposed via the set of nonmember functions in the WindowsRuntime
    // namespace, which utilize a global instance of LoaderContext.  The global instance is
    // initialized by BeginInitialization().
    class LoaderContext;





    // PACKAGE ASSEMBLY LOCATOR
    //
    // This is an internal component that is used to enumerate and locate assemblies in a package.
    // It uses a combination of the Windows Runtime namespace resolution APIs and filesystem
    // groveling to locate all of the metadata files.  You shouldn't need to construct this on its
    // own; one gets created automatically when you initialize the package metadata (see below).
    class PackageAssemblyLocator : public IAssemblyLocator
    {
    public:

        typedef std::map<String, String> PathMap;

        PackageAssemblyLocator(String const& packageRoot);

        virtual String LocateAssembly(AssemblyName const& assemblyName) const;

        virtual String LocateAssembly(AssemblyName const& assemblyName,
                                      String       const& fullTypeName) const;

        PathMap::const_iterator BeginMetadataFiles() const;
        PathMap::const_iterator EndMetadataFiles()   const;

        String FindMetadataFileForNamespace(String const& namespaceName) const;

    private:

        String          _packageRoot;
        PathMap mutable _metadataFiles;
    };





    // WINDOWS RUNTIME LOADER CONFIGURATION
    //
    // TODO Documentation
    class LoaderConfiguration : public ILoaderConfiguration
    {
    public:

        virtual String TransformNamespace(String const& namespaceName);
    };






    // PACKAGE INITIALIZATION
    //
    // Before you can use the use the WindowsRuntime reflection API defined here you must initialize
    // the type system.  Do this by calling BeginPackageInitialization() once (and only once).  This
    // function starts initialization of the type system and returns immediately.  After beginning
    // initialization, any other calls to the reflection API will block until initialization is done.
    //
    // HasInitializationBegun() and IsInitialized() will both return the current initialization
    // status (they return immediately and do not block).

    void BeginInitialization(String const& platformMetadataPath);
    bool HasInitializationBegun();
    bool IsInitialized();

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    inline void BeginPackageInitialization()
    {
        // TODO Once we implement the WACK-friendly CxxReflect::Platform::WinRT, we need to change
        // this line to initialize the platform externals with that new implementation.
        CxxReflect::Externals::Initialize<CxxReflect::Platform::Win32>();

        String packagePath(Windows::ApplicationModel::Package::Current->InstalledLocation->Path->Data());
        packagePath += L'\\';

        CxxReflect::WindowsRuntime::BeginInitialization(packagePath);
    }
    #endif





    // INTERFACE IMPLEMENTATION QUERIES
    //
    // These functions allow you to get the list of types in the package that implement a given
    // interface.  They grovel the entire set of loaded metadata files for implementers.  If no
    // types implement the interface, an empty sequence is returned.  If the interface cannot
    // be found, a RuntimeError is thrown.
    //
    // TODO Consider adding specific type resolution exceptions.

    std::vector<Type> GetImplementersOf(Type interfaceType);
    std::vector<Type> GetImplementersOf(GUID const& interfaceGuid);
    std::vector<Type> GetImplementersOf(StringReference interfaceFullName, bool caseSensitive = true);
    std::vector<Type> GetImplementersOf(StringReference namespaceName, StringReference interfaceSimpleName, bool caseSensitive = true);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template<typename TInterface>
    std::vector<Type> GetImplementersOf()
    {
        typedef typename Detail::RemoveHat<TInterface>::Type BareHeadedInterfaceType;
        String const interfaceFullName(BareHeadedInterfaceType::typeid->FullName->Data());
        return CxxReflect::WindowsRuntime::GetImplementersOf(StringReference(interfaceFullName.c_str()));
    }
    #endif





    // GET TYPE
    //
    // These functions allow you to get a type from the package given its name or given an inspectable
    // object.
    //
    // TODO We do not handle non-activatable runtime types very well, or types that do not have public
    // metadata.  We need to consider how to distinguish between these and how we can give as much
    // functionality as possible.

    Type GetType(StringReference typeFullName, bool caseSensitive = true);
    Type GetType(StringReference namespaceName, StringReference typeSimpleName, bool caseSensitive = true);
    Type GetTypeOf(IInspectable* object);
    
    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template <typename T>
    Type GetTypeOf(T^ object)
    {
        return CxxReflect::WindowsRuntime::GetTypeOf(reinterpret_cast<IInspectable*>(object));
    }
    #endif





    // TYPE PROPERTIES
    //
    // TODO Documentation

    bool IsDefaultConstructible(Type const& type);
    Guid GetGuid(Type const& type);




    // TYPE INSTANTIATION
    //
    // TODO Documentation

    UniqueInspectable CreateInspectableInstance(Type type);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    inline ::Platform::Object^ CreateObjectInstance(Type type)
    {
        return reinterpret_cast<::Platform::Object^>(CreateInspectableInstance(type).release());
    }

    template <typename T>
    T^ CreateInstance(Type type)
    {
        return dynamic_cast<T^>(CxxReflect::WindowsRuntime::CreateObjectInstance(type));
    }
    #endif

} }




namespace CxxReflect { namespace Detail {
    
    // We use the VariantArgumentPack to pack the arguments to a function, with original type
    // information.  These arguments are then passed into the method invoker or object activator,
    // where they are used during overload resolution to select the best matching function.  They
    // are then transformed into one of the platform-specific argument packs to construct the
    // arguments portion of the activation frame to be passed to the function.
    class VariantArgumentPack
    {
    public:

        class Argument
        {
        public:

            Argument(Metadata::ElementType type,
                     SizeType              valueIndex,
                     SizeType              valueSize,
                     SizeType              nameIndex = 0, 
                     SizeType              nameSize  = 0);

            Type              GetType   (VariantArgumentPack const& owner) const;
            ConstByteIterator BeginValue(VariantArgumentPack const& owner) const;
            ConstByteIterator EndValue  (VariantArgumentPack const& owner) const;
            StringReference   GetName   (VariantArgumentPack const& owner) const;

        private:

            Detail::ValueInitialized<Metadata::ElementType> _type;
            Detail::ValueInitialized<SizeType>              _valueIndex;
            Detail::ValueInitialized<SizeType>              _valueSize;
            Detail::ValueInitialized<SizeType>              _nameIndex;
            Detail::ValueInitialized<SizeType>              _nameSize;
        };

        typedef std::vector<Argument>            ArgumentSequence;
        typedef ArgumentSequence::const_iterator ArgumentIterator;

        struct InspectableArgument
        {
        public:

            InspectableArgument();
            InspectableArgument(IInspectable* value, StringReference name);

            IInspectable*   GetValue() const;
            StringReference GetName() const;

        private:

            Detail::ValueInitialized<IInspectable*> _value;
            String                                  _name;
        };

        SizeType Arity() const;

        ArgumentIterator Begin() const;
        ArgumentIterator End()   const;

        void Push(bool);

        void Push(wchar_t);

        void Push(std::int8_t  );
        void Push(std::uint8_t );
        void Push(std::int16_t );
        void Push(std::uint16_t);
        void Push(std::int32_t );
        void Push(std::uint32_t);
        void Push(std::int64_t );
        void Push(std::uint64_t);
        
        void Push(float );
        void Push(double);

        void Push(InspectableArgument);

        // TODO Add support for strings (std::string, wchar_t const*, HSTRING, etc.)
        // TODO Add support for arbitrary value types

    private:
        
        void Push(Metadata::ElementType type, ConstByteIterator first, ConstByteIterator last);

        std::vector<Argument> _arguments;
        std::vector<Byte>     _data;
    };

    template <typename T>
    T PreprocessArgument(T const& value)
    {
        return value;
    }

    

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template <typename T>
    VariantArgumentPack::InspectableArgument PreprocessArgument(T^ value)
    {
        return VariantArgumentPack::InspectableArgument(
            reinterpret_cast<IInspectable*>(value),
            T::typeid->FullName->Data());
    }
    #endif

    template <typename P0>
    VariantArgumentPack PackArguments(P0&& a0)
    {
        VariantArgumentPack pack;
        pack.Push(PreprocessArgument(std::forward<P0>(a0)));
        return pack;
    }

    template <typename P0, typename P1>
    VariantArgumentPack PackArguments(P0&& a0, P1&& a1)
    {
        VariantArgumentPack pack;
        pack.Push(PreprocessArgument(std::forward<P0>(a0)));
        pack.Push(PreprocessArgument(std::forward<P1>(a1)));
        return pack;
    }

    template <typename P0, typename P1, typename P2>
    VariantArgumentPack PackArguments(P0&& a0, P1&& a1, P2&& a2)
    {
        VariantArgumentPack pack;
        pack.Push(PreprocessArgument(std::forward<P0>(a0)));
        pack.Push(PreprocessArgument(std::forward<P1>(a1)));
        pack.Push(PreprocessArgument(std::forward<P2>(a2)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3>
    VariantArgumentPack PackArguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3)
    {
        VariantArgumentPack pack;
        pack.Push(PreprocessArgument(std::forward<P0>(a0)));
        pack.Push(PreprocessArgument(std::forward<P1>(a1)));
        pack.Push(PreprocessArgument(std::forward<P2>(a2)));
        pack.Push(PreprocessArgument(std::forward<P3>(a3)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    VariantArgumentPack PackArguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
    {
        VariantArgumentPack pack;
        pack.Push(PreprocessArgument(std::forward<P0>(a0)));
        pack.Push(PreprocessArgument(std::forward<P1>(a1)));
        pack.Push(PreprocessArgument(std::forward<P2>(a2)));
        pack.Push(PreprocessArgument(std::forward<P3>(a3)));
        pack.Push(PreprocessArgument(std::forward<P4>(a4)));
        return pack;
    }

    WindowsRuntime::UniqueInspectable CreateInspectableInstance(Type const type, VariantArgumentPack const& arguments);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    inline ::Platform::Object^ CreateObjectInstance(Type const type, VariantArgumentPack const& arguments)
    {
        return reinterpret_cast<::Platform::Object^>(CreateInspectableInstance(type, arguments).release());
    }

    template <typename TTarget>
    TTarget^ CreateInstance(Type const type, VariantArgumentPack const& arguments)
    {
        return dynamic_cast<TTarget^>(CreateObjectInstance(type, arguments));
    }
    #endif

} }

namespace CxxReflect { namespace WindowsRuntime {

    template <typename P0>
    UniqueInspectable CreateInspectableInstance(Type const type, P0&& a0)
    {
        return Detail::CreateInspectableInstance(type, Detail::PackArguments(
            std::forward<P0>(a0)));
    }

    template <typename P0, typename P1>
    UniqueInspectable CreateInspectableInstance(Type const type, P0&& a0, P1&& a1)
    {
        return Detail::CreateInspectableInstance(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1)));
    }

    template <typename P0, typename P1, typename P2>
    UniqueInspectable CreateInspectableInstance(Type const type, P0&& a0, P1&& a1, P2&& a2)
    {
        return Detail::CreateInspectableInstance(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2)));
    }

    template <typename P0, typename P1, typename P2, typename P3>
    UniqueInspectable CreateInspectableInstance(Type const type, P0&& a0, P1&& a1, P2&& a2, P3&& a3)
    {
        return Detail::CreateInspectableInstance(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    UniqueInspectable CreateInspectableInstance(Type const type, P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
    {
        return Detail::CreateInspectableInstance(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3), 
            std::forward<P4>(a4)));
    }

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template <typename P0>
    ::Platform::Object^ CreateObjectInstance(Type const type, P0&& a0)
    {
        return Detail::CreateObjectInstance(type, Detail::PackArguments(
            std::forward<P0>(a0)));
    }

    template <typename P0, typename P1>
    ::Platform::Object^ CreateObjectInstance(Type const type, P0&& a0, P1&& a1)
    {
        return Detail::CreateObjectInstance(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1)));
    }

    template <typename P0, typename P1, typename P2>
    ::Platform::Object^ CreateObjectInstance(Type const type, P0&& a0, P1&& a1, P2&& a2)
    {
        return Detail::CreateObjectInstance(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2)));
    }

    template <typename P0, typename P1, typename P2, typename P3>
    ::Platform::Object^ CreateObjectInstance(Type const type, P0&& a0, P1&& a1, P2&& a2, P3&& a3)
    {
        return Detail::CreateObjectInstance(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    ::Platform::Object^ CreateObjectInstance(Type const type, P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
    {
        return Detail::CreateObjectInstance(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3), 
            std::forward<P4>(a4)));
    }

    template <typename TTarget, typename P0>
    TTarget^ CreateInstance(Type const type, P0&& a0)
    {
        return Detail::CreateInstance<TTarget>(type, Detail::PackArguments(
            std::forward<P0>(a0)));
    }

    template <typename TTarget, typename P0, typename P1>
    TTarget^ CreateInstance(Type const type, P0&& a0, P1&& a1)
    {
        return Detail::CreateInstance<TTarget>(type, Detail::PackArguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1)));
    }

    template <typename TTarget, typename P0, typename P1, typename P2>
    TTarget^ CreateInstance(Type const type, P0&& a0, P1&& a1, P2&& a2)
    {
        return Detail::CreateInstance<TTarget>(type, Detail::PackArguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2)));
    }

    template <typename TTarget, typename P0, typename P1, typename P2, typename P3>
    TTarget^ CreateInstance(Type const type, P0&& a0, P1&& a1, P2&& a2, P3&& a3)
    {
        return Detail::CreateInstance<TTarget>(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3)));
    }

    template <typename TTarget, typename P0, typename P1, typename P2, typename P3, typename P4>
    TTarget^ CreateInstance(Type const type, P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
    {
        return Detail::CreateInstance<TTarget>(type, Detail::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3), 
            std::forward<P4>(a4)));
    }
    #endif

} }

#endif // CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif
