//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/WindowsRuntime.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Type.hpp"

// Note:  We cannot include any of the concurrency headers in our headers because they cannot be
// used in a C++/CLI program.  This source file only has functionality usable from a Windows Runtime
// program.   Windows Runtime and C++/CLI cannot be used together, so usage of these Standard 
// Library components here is safe.
#include <atomic>
#include <future>
#include <thread>

#include <hstring.h>
#include <inspectable.h>
#include <rometadataresolution.h>
#include <windows.h>
#include <winstring.h>





//
//
// GLOBAL WINDOWS RUNTIME METADATA LOADER
//
//

namespace CxxReflect { namespace { namespace Private {

    typedef std::atomic<bool>                           InitializedFlag;
    typedef std::shared_future<std::unique_ptr<Loader>> LoaderFuture;
    typedef std::mutex                                  LoaderMutex;
    typedef std::unique_lock<LoaderMutex>               LoaderLock;

    // To help us to avoid silly mistakes, we encapsulate the synchronized Loader using this
    // LoaderLease class.  It is only possible to get the Loader instance through a lease (the
    // GlobalWindowsRuntimeLoader class, below, ensures this).  This doesn't prevent obviously
    // stupid code, e.g., storing a pointer to the loader, then using that pointer after the
    // lease is released (i.e., destroyed), but it helps to prevent subtle issues.  Perhaps.
    class LoaderLease
    {
    public:

        LoaderLease(Loader& loader, LoaderMutex& mutex)
            : _loader(&loader), _lock(mutex)
        {
        }

        LoaderLease(LoaderLease&& other)
            : _loader(other._loader), _lock(std::move(other._lock))
        {
            other._loader.Reset();
        }

        LoaderLease& operator=(LoaderLease&& other)
        {
            _loader = other._loader;
            _lock = std::move(other._lock);

            other._loader.Reset();

            return *this;
        }

        Loader& Get() const
        {
            return *_loader.Get();
        }

    private:

        LoaderLease(LoaderLease const&);
        LoaderLease& operator=(LoaderLease const&);

        Detail::ValueInitialized<Loader*> _loader;
        LoaderLock                        _lock;
    };

    class GlobalWindowsRuntimeLoader
    {
    public:

        static void Initialize(LoaderFuture&& loader)
        {
            bool expected(false);
            if (!_initialized.compare_exchange_strong(expected, true))
                throw LogicError(L"Global Windows Runtime Loader was already initialized");

            _loader = std::move(loader);
        }

        static LoaderLease Lease()
        {
            Detail::Assert([&]{ return _initialized.load(); });

            Loader* const loader(_loader.get().get());
            Detail::Verify([&]{ return loader != nullptr; }, L"Global Windows Runtime Loader is not valid");

            return LoaderLease(*loader, _mutex);
        }

        static bool IsInitialized() { return _initialized.load(); }
        static bool IsReady()       { return _loader.valid();     }

    private:

        // This type is not constructible.  It exists solely to prevent direct access to _loader.
        GlobalWindowsRuntimeLoader();
        GlobalWindowsRuntimeLoader(GlobalWindowsRuntimeLoader const&);
        GlobalWindowsRuntimeLoader& operator=(GlobalWindowsRuntimeLoader const&);

        static InitializedFlag _initialized;
        static LoaderFuture    _loader;
        static LoaderMutex     _mutex;
    };

    InitializedFlag GlobalWindowsRuntimeLoader::_initialized;
    LoaderFuture    GlobalWindowsRuntimeLoader::_loader;
    LoaderMutex     GlobalWindowsRuntimeLoader::_mutex;

} } }





//
//
// HSTRING CONTAINER WRAPPER AND UTILITIES
//
//

namespace CxxReflect { namespace { namespace Private {

    class SmartHString
    {
    public:

        typedef wchar_t           value_type;
        typedef std::size_t       size_type;
        typedef std::ptrdiff_t    difference_type;

        typedef value_type const& reference;
        typedef value_type const& const_reference;
        typedef value_type const* pointer;
        typedef value_type const* const_pointer;

        typedef pointer                               iterator;
        typedef const_pointer                         const_iterator;
        typedef std::reverse_iterator<iterator>       reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        SmartHString()
        {
        }

        SmartHString(const_pointer const s)
        {
            Detail::VerifySuccess(::WindowsCreateString(s, static_cast<UINT32>(::wcslen(s)), &_value.Get()));
        }

        SmartHString(StringReference const s)
        {
            Detail::VerifySuccess(::WindowsCreateString(s.c_str(), static_cast<UINT32>(::wcslen(s.c_str())), &_value.Get()));
        }

        SmartHString(String const& s)
        {
            Detail::VerifySuccess(::WindowsCreateString(s.c_str(), static_cast<UINT32>(::wcslen(s.c_str())), &_value.Get()));
        }

        SmartHString(SmartHString const& other)
        {
            Detail::VerifySuccess(::WindowsDuplicateString(other._value.Get(), &_value.Get()));
        }

        SmartHString& operator=(SmartHString other)
        {
            swap(other);
            return *this;
        }

        ~SmartHString()
        {
            Detail::AssertSuccess(::WindowsDeleteString(_value.Get()));
        }

        void swap(SmartHString& other)
        {
            using std::swap;
            swap(_value, other._value);
        }

        const_iterator begin()  const { return get_buffer_begin(); }
        const_iterator end()    const { return get_buffer_end();   }
        const_iterator cbegin() const { return get_buffer_begin(); }
        const_iterator cend()   const { return get_buffer_end();   }

        const_reverse_iterator rbegin()  const { return reverse_iterator(get_buffer_end());    }
        const_reverse_iterator rend()    const { return reverse_iterator(get_buffer_begin());  }

        const_reverse_iterator crbegin() const { return reverse_iterator(get_buffer_end());    }
        const_reverse_iterator crend()   const { return reverse_iterator(get_buffer_begin());  }

        size_type size()     const { return end() - begin();                         }
        size_type length()   const { return size();                                  }
        size_type max_size() const { return std::numeric_limits<std::size_t>::max(); }
        size_type capacity() const { return size();                                  }
        bool      empty()    const { return size() == 0;                             }

        const_reference operator[](size_type const n) const
        {
            return get_buffer_begin()[n];
        }

        const_reference at(size_type const n) const
        {
            if (n >= size())
                throw std::out_of_range("n");

            return get_buffer_begin()[n];
        }

        const_reference front() const { return *get_buffer_begin();     }
        const_reference back()  const { return *(get_buffer_end() - 1); }

        const_pointer c_str() const { return get_buffer_begin(); }
        const_pointer data()  const { return get_buffer_begin(); }

        class ReferenceProxy
        {
        public:

            ReferenceProxy(SmartHString* const value)
                : _value(value), _proxy(value->_value.Get())
            {
                Detail::AssertNotNull(value);
            }

            ~ReferenceProxy()
            {
                if (_value.Get()->_value.Get() == _proxy.Get())
                    return;

                SmartHString newString;
                newString._value.Get() = _proxy.Get();
                
                _value.Get()->swap(newString);
            }

            operator HSTRING*()
            {
                return &_proxy.Get();
            }

        private:

            // Note that this type is copyable though it is not intended to be copied, aside from
            // when it is returned from SmartHString::proxy().
            ReferenceProxy& operator=(ReferenceProxy const&);

            Detail::ValueInitialized<HSTRING>       _proxy;
            Detail::ValueInitialized<SmartHString*> _value;
        };

        ReferenceProxy proxy()
        {
            return ReferenceProxy(this);
        }

        HSTRING value() const
        {
            return _value.Get();
        }

        friend bool operator==(SmartHString const& lhs, SmartHString const& rhs)
        {
            return compare(lhs, rhs) ==  0;
        }

        friend bool operator< (SmartHString const& lhs, SmartHString const& rhs)
        {
            return compare(lhs, rhs) == -1;
        }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(SmartHString)

    private:

        const_pointer get_buffer_begin() const
        {
            const_pointer const result(::WindowsGetStringRawBuffer(_value.Get(), nullptr));
            return result == nullptr ? L"" : result;
        }

        const_pointer get_buffer_end() const
        {
            std::uint32_t length(0);
            const_pointer const first(::WindowsGetStringRawBuffer(_value.Get(), &length));
            return first + length;
        }

        static int compare(SmartHString const& lhs, SmartHString const& rhs)
        {
            std::int32_t result(0);
            Detail::VerifySuccess(::WindowsCompareStringOrdinal(lhs._value.Get(), rhs._value.Get(), &result));
            return result;
        }

        Detail::ValueInitialized<HSTRING> _value;
    };

    String ToString(HSTRING hstring)
    {
        wchar_t const* const buffer(::WindowsGetStringRawBuffer(hstring, nullptr));
        return buffer == nullptr ? L"" : buffer;
    }

    // An RAII wrapper for an array of HSTRINGs.
    class RaiiHStringArray
    {
    public:

        RaiiHStringArray()
        {
        }

        ~RaiiHStringArray()
        {
            Detail::Assert([&]{ return _count.Get() == 0 || _array.Get() != nullptr; });

            // Exception-safety note:  if the deletion fails, something has gone horribly wrong
            for (DWORD i(0); i < _count.Get(); ++i)
                Detail::AssertSuccess(::WindowsDeleteString(_array.Get()[i]));

            ::CoTaskMemFree(_array.Get());
        }

        DWORD&    GetCount() { return _count.Get(); }
        HSTRING*& GetArray() { return _array.Get(); }

        HSTRING*  begin() const { return _array.Get();                }
        HSTRING*  end()   const { return _array.Get() + _count.Get(); }

    private:

        RaiiHStringArray(RaiiHStringArray const&);
        RaiiHStringArray& operator=(RaiiHStringArray const&);

        Detail::ValueInitialized<DWORD>    _count;
        Detail::ValueInitialized<HSTRING*> _array;
    };

} } }





//
//
// LOW-LEVEL DYNAMIC METHOD INVOCATION UTILITIES AND VTABLE HELPERS
//
//

namespace CxxReflect { namespace { namespace Private {

    // Contains logic for invoking a virtual function on an object via vtable lookup.  Currently this
    // only supports functions that take no arguments or that take one or two reference-type arguments.
    // TODO Add support for value-type arguments.  This may or may not be very easy.
    class Invoker
    {
    public:

        // Note that each type has an extra argument for the 'this' pointer:
        typedef HRESULT (__stdcall* ReferenceOnly0Args)(void*);
        typedef HRESULT (__stdcall* ReferenceOnly1Args)(void*, void*);
        typedef HRESULT (__stdcall* ReferenceOnly2Args)(void*, void*, void*);

        static HRESULT VirtualAbiInvokeReferenceOnly(unsigned  const  index,
                                                     IID       const& interfaceId,
                                                     IUnknown* const  originalThis)
        {
            ThisPointer const correctThis(interfaceId, originalThis);
            Detail::AssertNotNull(correctThis.Get());
        
            auto const fnPtr(ComputeFunctionPointer<ReferenceOnly0Args>(index, correctThis.Get()));
            Detail::AssertNotNull(fnPtr);

            return (*fnPtr)(correctThis.Get());
        }

        static HRESULT VirtualAbiInvokeReferenceOnly(unsigned  const  index,
                                                     IID       const& interfaceId,
                                                     IUnknown* const  originalThis,
                                                     void*     const  arg0)
        {
            ThisPointer const correctThis(interfaceId, originalThis);
            Detail::AssertNotNull(correctThis.Get());
        
            auto const fnPtr(ComputeFunctionPointer<ReferenceOnly1Args>(index, correctThis.Get()));
            Detail::AssertNotNull(fnPtr);

            return (*fnPtr)(correctThis.Get(), arg0);
        }

        static HRESULT VirtualAbiInvokeReferenceOnly(unsigned  const  index,
                                                     IID       const& interfaceId,
                                                     IUnknown* const  originalThis,
                                                     void*     const  arg0,
                                                     void*     const  arg1)
        {
            ThisPointer const correctThis(interfaceId, originalThis);
            Detail::AssertNotNull(correctThis.Get());
        
            auto const fnPtr(ComputeFunctionPointer<ReferenceOnly2Args>(index, correctThis.Get()));
            Detail::AssertNotNull(fnPtr);

            return (*fnPtr)(correctThis.Get(), arg0, arg1);
        }

    private:

        // A smart QI'ing pointer for finding and owning the 'this' pointer for the Invoke functions.
        class ThisPointer
        {
        public:

            ThisPointer(IID const& interfaceId, IUnknown* const unknownThis)
            {
                Detail::AssertNotNull(unknownThis);

                // Note that we cannot direct-initialize here due to a bug in the Visual C++ compiler.
                void** const voidUnknown = reinterpret_cast<void**>(&_unknown.Get());
                Detail::VerifySuccess(unknownThis->QueryInterface(interfaceId, voidUnknown));
                Detail::AssertNotNull(_unknown.Get());
            }

            ~ThisPointer()
            {
                Detail::AssertNotNull(_unknown.Get());
                _unknown.Get()->Release();
            }

            IUnknown* Get() const
            {
                return _unknown.Get();
            }

        private:

            ThisPointer(ThisPointer const&);
            ThisPointer& operator=(ThisPointer const&);

            Detail::ValueInitialized<IUnknown*> _unknown;
        };

        Invoker(Invoker const&);
        Invoker& operator=(Invoker&);

        // Computes a function pointer, given the type of the function pointer (TFunctionPointer),
        // the 'this' pointer of the object on which the function will be invoked, and the index of
        // the function in the vtable.  The 'this' pointer must point to the correct vtable (i.e., 
        // before calling this, you must be sure to QueryInterface to get the right 'this' pointer.
        template <typename TFunctionPointer>
        static TFunctionPointer ComputeFunctionPointer(unsigned const index, void* const thisptr)
        {
            Detail::AssertNotNull(thisptr);

            // There are three levels of indirection:  'this' points to an object, the first element
            // of which points to a vtable, which contains slots that point to functions:
            return reinterpret_cast<TFunctionPointer>((*reinterpret_cast<void***>(thisptr))[index]);
        }
    };

    void EnumerateUniverseMetadataFilesInto(SmartHString const rootNamespace, std::vector<String>& result)
    {
        RaiiHStringArray filePaths;
        RaiiHStringArray nestedNamespaces;

        Detail::VerifySuccess(::RoResolveNamespace(
            rootNamespace.value(),
            nullptr,
            0,
            nullptr,
            rootNamespace.empty() ? nullptr : &filePaths.GetCount(),
            rootNamespace.empty() ? nullptr : &filePaths.GetArray(),
            &nestedNamespaces.GetCount(),
            &nestedNamespaces.GetArray()));

        std::transform(filePaths.begin(), filePaths.end(), std::back_inserter(result), [&](HSTRING path)
        {
            return ToString(path);
        });

        String const baseNamespace(String(rootNamespace.c_str()) + (rootNamespace.empty() ? L"" : L"."));
        std::for_each(nestedNamespaces.begin(), nestedNamespaces.end(), [&](HSTRING nestedNamespace)
        {
            EnumerateUniverseMetadataFilesInto(baseNamespace + ToString(nestedNamespace), result);
        });
    }

    std::vector<String> EnumerateUniverseMetadataFiles()
    {
        std::vector<String> result;

        EnumerateUniverseMetadataFilesInto(SmartHString(), result);

        std::sort(result.begin(), result.end());
        result.erase(std::unique(result.begin(), result.end()), result.end());

        return result;
    }

    StringReference RemoveRightmostTypeNameComponent(StringReference const typeName)
    {
        Detail::Assert([&]{ return !typeName.empty(); });

        return StringReference(
            typeName.begin(),
            std::find(typeName.rbegin(), typeName.rend(), L'.').base());
    }

    void RemoveRightmostTypeNameComponent(String& typeName)
    {
        Detail::Assert([&]{ return !typeName.empty(); });

        // TODO This does not handle generics.  Does it need to handle generics?
        typeName.erase(std::find(typeName.rbegin(), typeName.rend(), L'.').base(), typeName.end());
    }

} } }





//
//
// HEADER-DEFINED PUBLIC (AND NOT-SO-PUBLIC) INTERFACE
//
//

namespace CxxReflect {

    WinRTAssemblyLocator::WinRTAssemblyLocator(String const& packageRoot)
        : _packageRoot(packageRoot)
    {
        auto const metadataFiles(Private::EnumerateUniverseMetadataFiles());

        std::transform(metadataFiles.begin(),
                       metadataFiles.end(),
                       std::inserter(_metadataFiles, _metadataFiles.end()),
                       [&](String const& fileName) -> PathMap::value_type
        {
            // TODO This code requires error checking and is really, really hacked up.
            auto first(std::find(fileName.rbegin(), fileName.rend(), L'\\').base());
            auto last(std::find(fileName.rbegin(), fileName.rend(), L'.').base());

            String const simpleName(first, std::prev(last));

            return std::make_pair(Detail::MakeLowercase(simpleName), Detail::MakeLowercase(fileName));
        });
    }

    String WinRTAssemblyLocator::LocateAssembly(AssemblyName const& assemblyName) const
    {
        String const simpleName(Detail::MakeLowercase(assemblyName.GetName()));

        // The platform metadata and system assembly are special-cased to use our platform metadata:
        if (simpleName == L"platform" || simpleName == L"mscorlib")
        {
            return _packageRoot + Detail::PlatformMetadataFileName;
        }

        Detail::AssertFail(L"Not Yet Implemented");
        return String();
    }

    String WinRTAssemblyLocator::LocateAssembly(AssemblyName const& assemblyName, String const& fullTypeName) const
    {
        String const simpleName(Detail::MakeLowercase(assemblyName.GetName()));

        // The platform metadata and system assembly are special-cased to use our platform metadata:
        if (simpleName == L"platform" || simpleName == L"mscorlib")
        {
            return _packageRoot + Detail::PlatformMetadataFileName;
        }

        // The assembly name must be a prefix of the namespace-qualified type name, per WinRT rules:
        Detail::Assert([&]() -> bool
        {
            String const lowercaseFullTypeName(Detail::MakeLowercase(fullTypeName));
            return simpleName.size() <= lowercaseFullTypeName.size()
                && std::equal(simpleName.begin(), simpleName.end(), lowercaseFullTypeName.begin());
        });

        String namespaceName(fullTypeName);
        Private::RemoveRightmostTypeNameComponent(namespaceName);
        return FindMetadataFileForNamespace(namespaceName);
    }

    WinRTAssemblyLocator::PathMap::const_iterator WinRTAssemblyLocator::BeginMetadataFiles() const
    {
        return _metadataFiles.begin();
    }

    WinRTAssemblyLocator::PathMap::const_iterator WinRTAssemblyLocator::EndMetadataFiles() const
    {
        return _metadataFiles.end();
    }

    String WinRTAssemblyLocator::FindMetadataFileForNamespace(String const& namespaceName) const
    {
        String const lowercaseNamespaceName(Detail::MakeLowercase(namespaceName));

        // First, search the metadata files we got from RoResolveNamespace:
        String enclosingNamespaceName(lowercaseNamespaceName);
        while (!enclosingNamespaceName.empty())
        {
            auto const it(_metadataFiles.find(enclosingNamespaceName));
            if (it != _metadataFiles.end())
                return it->second;

            Private::RemoveRightmostTypeNameComponent(enclosingNamespaceName);
        }

        // Next, search for metadata files in the package root, using the longest-match-wins rule:
        // TODO We may need to support folders other than the package root.  For example, how do we
        // know where to find the Platform.winmd metadata?
        enclosingNamespaceName = lowercaseNamespaceName;
        while (!enclosingNamespaceName.empty())
        {
            String winmdPath = _packageRoot + enclosingNamespaceName + L".winmd";
            if (Externals::FileExists(winmdPath.c_str()))
            {
                _metadataFiles.insert(std::make_pair(enclosingNamespaceName, winmdPath));
                return winmdPath;
            }

            Private::RemoveRightmostTypeNameComponent(enclosingNamespaceName);
        }

        // If this is the 'Platform' or 'System' namespace, try to use the platform metadata file:
        // TODO This is also suspect:  how do we know to look here?  :'(
        if (lowercaseNamespaceName.substr(0, 8) == L"platform" ||
            lowercaseNamespaceName.substr(0, 6) == L"system")
        {
            return _packageRoot + Detail::PlatformMetadataFileName;
        }

        // TODO Should we throw here or return an empty string?
        throw RuntimeError(L"Failed to locate metadata file");
    }

    void WinRTPackageMetadata::BeginInitialization(String const& platformMetadataPath)
    {
        if (Private::GlobalWindowsRuntimeLoader::IsInitialized())
            return;

        Private::GlobalWindowsRuntimeLoader::Initialize(std::async(std::launch::async,
        [=]() -> std::unique_ptr<Loader>
        {
            std::unique_ptr<WinRTAssemblyLocator> resolver(new WinRTAssemblyLocator(platformMetadataPath));
            WinRTAssemblyLocator const& rawResolver(*resolver.get());

            std::unique_ptr<Loader> loader(new Loader(std::move(resolver)));

            typedef WinRTAssemblyLocator::PathMap::value_type Element;
            std::for_each(rawResolver.BeginMetadataFiles(), rawResolver.EndMetadataFiles(), [&](Element const& e)
            {
                loader->LoadAssembly(e.second);
            });

            return loader;
        }));
    }

    bool WinRTPackageMetadata::HasInitializationBegun()
    {
        return Private::GlobalWindowsRuntimeLoader::IsInitialized();
    }

    bool WinRTPackageMetadata::IsInitialized()
    {
        return Private::GlobalWindowsRuntimeLoader::IsReady();
    }

    Type WinRTPackageMetadata::GetTypeOf(IInspectable* const inspectable)
    {
        // TODO CHANGE TO USE StringReference EVENTUALLY
        Detail::AssertNotNull(inspectable);

        Private::SmartHString typeNameHString;
        Detail::AssertSuccess(inspectable->GetRuntimeClassName(typeNameHString.proxy()));
        String const typeName(typeNameHString.begin(), typeNameHString.end());

        auto const endOfNamespaceIt(std::find(typeName.rbegin(), typeName.rend(), '.').base());
        Detail::Assert([&]{ return endOfNamespaceIt != typeName.rend().base(); });

        String const namespaceName(typeName.begin(), std::prev(endOfNamespaceIt));

        Private::LoaderLease loader(Private::GlobalWindowsRuntimeLoader::Lease());
        
        IAssemblyLocator const& resolver(loader.Get().GetAssemblyLocator(InternalKey()));
        WinRTAssemblyLocator const& winrtResolver(dynamic_cast<WinRTAssemblyLocator const&>(resolver));

        String metadataFileName(winrtResolver.FindMetadataFileForNamespace(namespaceName));
        if (metadataFileName.empty())
            return Type();

        Assembly assembly(loader.Get().LoadAssembly(metadataFileName));
        if (!assembly.IsInitialized())
            return Type();

        return assembly.GetType(StringReference(typeName.c_str()));
    }
}

#endif
