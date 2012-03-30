#ifndef CXXREFLECT_WINDOWSRUNTIME_HPP_
#define CXXREFLECT_WINDOWSRUNTIME_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "CxxReflect/Type.hpp"

struct IInspectable;
struct IUnknown;

namespace CxxReflect { namespace Detail {

    static const wchar_t* const PlatformMetadataFileName(L"CxxReflectPlatform.dat");

} }

namespace CxxReflect {

    class WinRTAssemblyLocator : public IAssemblyLocator
    {
    public:

        typedef std::map<String, String> PathMap;

        WinRTAssemblyLocator(String const& packageRoot);

        // TODO We should also provide support for out-of-package resolution.

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

    class WinRTPackageMetadata
    {
    public:

        static void BeginInitialization(String const& platformMetadataPath);
        static bool HasInitializationBegun();
        static bool IsInitialized();

        // These functions will block until the universe is fully initialized:
        // TODO PERF We could try loading individual assemblies, too
        static Assembly GetAssembly(StringReference simpleName);
        static Type GetType(StringReference fullName, bool caseInsensitive = false);

        static Type GetTypeOf(IInspectable* inspectable);

        template <typename T>
        static Type GetTypeOf(T object)
        {
            return GetTypeOf(reinterpret_cast<IInspectable*>(object));
        }

    private:

        WinRTPackageMetadata(WinRTPackageMetadata const&);
        WinRTPackageMetadata& operator=(WinRTPackageMetadata const&);
    };
}

namespace CxxReflect { namespace WindowsRuntime {

    std::vector<Type> GetImplementersOf(decltype(__uuidof(0)) const& guid);

    template<typename T>
    std::vector<Type> GetImplementersOf()
    {
        return GetImplementersOf(__uuidof(T));
    }

} }

// There is no support for C++/CX language extentions in static libraries.  So, in order not to
// require a consumer of the library to also include a source file with this definition, we just
// define it here, inline, if and only if it is actually being included by a consumer (and not by
// the library proper).
#ifdef __cplusplus_winrt

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

    inline String GetCxxReflectPlatformMetadataPath()
    {
        return String(Windows::ApplicationModel::Package::Current->InstalledLocation->Path->Data()) + L"\\";
    }

    template <typename T> struct IsCarrot         { enum { value = false }; };
    template <typename T> struct IsCarrot<T^>     { enum { value = true; }; };
    template <typename T> struct AddCarrot        { typedef T^ Type;        };
    template <typename T> struct AddCarrot<T^>    {                         };
    template <typename T> struct RemoveCarrot     {                         };
    template <typename T> struct RemoveCarrot<T^> { typedef T Type;         };

} }

namespace CxxReflect {

    inline void BeginWinRTPackageMetadataInitialization()
    {
        WinRTPackageMetadata::BeginInitialization(Detail::GetCxxReflectPlatformMetadataPath());
    }

}

#endif // __cplusplus_winrt
#endif // CXXREFLECT_ENABLE_FEATURE_WINRT
#endif
