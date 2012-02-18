//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTINTEGRATION_HPP_
#define CXXREFLECT_WINRTINTEGRATION_HPP_

#include "CxxReflect/CoreInternals.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "CxxReflect/Type.hpp"

#include <ppl.h>

struct IInspectable;
struct IUnknown;

namespace CxxReflect { namespace Detail {

    static const wchar_t* const PlatformMetadataFileName(L"CxxReflectPlatform.dat");

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

} }

namespace CxxReflect {

    class WinRTMetadataResolver : public IMetadataResolver
    {
    public:

        typedef std::map<String, String> PathMap;

        WinRTMetadataResolver(String const& packageRoot);

        // TODO We should also provide support for out-of-package resolution.

        virtual String ResolveAssembly(AssemblyName const& assemblyName) const;

        virtual String ResolveAssembly(AssemblyName const& assemblyName,
                                       String       const& namespaceQualifiedTypeName) const;

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

// There is no support for C++/CX language extentions in static libraries.  So, in order not to
// require a consumer of the library to also include a source file with this definition, we just
// define it here, inline, if and only if it is actually being included by a consumer (and not by
// the library proper).
#ifdef __cplusplus_winrt

namespace CxxReflect { namespace Detail {

    inline String GetCxxReflectPlatformMetadataPath()
    {
        // TODO We should catch any Windows Runtime exceptions here and throw our own instead:
        Windows::Foundation::Uri^ fileUri(
            ref new Windows::Foundation::Uri(L"ms-appx:///CxxReflectPlatform.winmd"));

        Windows::Storage::StorageFile^ file(Detail::SyncCall([&]
        {
            return Windows::Storage::StorageFile::GetFileFromApplicationUriAsync(fileUri);
        }));

        String path(file->Path->Data());
        path.resize(path.size() - file->Name->Length());
        return path;
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
