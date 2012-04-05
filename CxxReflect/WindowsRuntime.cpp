//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/WindowsRuntime.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

// Note:  We cannot include any of the concurrency headers in our headers because they cannot be
// used in a C++/CLI program.  This source file only has functionality usable from a Windows Runtime
// program.   Windows Runtime and C++/CLI cannot be used together, so usage of these Standard 
// Library components here is safe.
#include <atomic>
#include <filesystem>
#include <future>
#include <thread>

#include <hstring.h>
#include <inspectable.h>
#include <roapi.h>
#include <rometadataresolution.h>
#include <windows.h>
#include <winstring.h>
#include <wrl/client.h>





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

        WindowsRuntime::PackageAssemblyLocator const& GetLocator() const
        {
            IAssemblyLocator const& locator(_loader.Get()->GetAssemblyLocator(InternalKey()));

            Detail::Assert([&]
            {
                return dynamic_cast<WindowsRuntime::PackageAssemblyLocator const*>(&locator) != nullptr;
            });

            return static_cast<WindowsRuntime::PackageAssemblyLocator const&>(locator);
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
            Detail::VerifySuccess(::WindowsCreateString(
                s,
                static_cast<UINT32>(::wcslen(s)),
                &_value.Get()));
        }

        SmartHString(StringReference const s)
        {
            Detail::VerifySuccess(::WindowsCreateString(
                s.c_str(),
                static_cast<UINT32>(::wcslen(s.c_str())),
                &_value.Get()));
        }

        SmartHString(String const& s)
        {
            Detail::VerifySuccess(::WindowsCreateString(
                s.c_str(),
                static_cast<UINT32>(::wcslen(s.c_str())),
                &_value.Get()));
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

        const_reverse_iterator rbegin()  const { return const_reverse_iterator(get_buffer_end());    }
        const_reverse_iterator rend()    const { return const_reverse_iterator(get_buffer_begin());  }

        const_reverse_iterator crbegin() const { return const_reverse_iterator(get_buffer_end());    }
        const_reverse_iterator crend()   const { return const_reverse_iterator(get_buffer_begin());  }

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

} } }





//
//
// METADATA FILE DISCOVERY AND GENERAL UTILITIES
//
//

namespace CxxReflect { namespace { namespace Private {

    void EnumerateUniverseMetadataFilesInto(SmartHString const rootNamespace, std::vector<String>& result)
    {
        RaiiHStringArray filePaths;
        RaiiHStringArray nestedNamespaces;

        Detail::VerifySuccess(::RoResolveNamespace(
            rootNamespace.empty() ? nullptr : rootNamespace.value(),
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

    std::vector<String> EnumerateUniverseMetadataFiles(StringReference const packageDirectory)
    {
        std::vector<String> result;

        EnumerateUniverseMetadataFilesInto(SmartHString(), result);

        // TODO This is an ugly workaround:  it appears that RoResolveNamespace doesn't actually
        // give us non-Windows namespaces when we ask for the root namespaces.
        for (std::tr2::sys::wdirectory_iterator it(packageDirectory.c_str()), end; it != end; ++it)
        {
            if (it->path().extension() != L".winmd")
                continue;

            result.push_back(String(packageDirectory.c_str()) + it->path().filename());
        }


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
        typeName.erase(std::find(typeName.rbegin(), typeName.rend(), L'.').base() - 1, typeName.end());
    }





    GUID ToComGuid(Guid const& cxxGuid)
    {
        GUID comGuid;
        Detail::RangeCheckedCopy(
            cxxGuid.AsByteArray().begin(), cxxGuid.AsByteArray().end(),
            Detail::BeginBytes(comGuid), Detail::EndBytes(comGuid));
        return comGuid;
    }

    Guid ToCxxGuid(GUID const& comGuid)
    {
        return Guid(
            comGuid.Data1, comGuid.Data2, comGuid.Data3, 
            comGuid.Data4[0], comGuid.Data4[1], comGuid.Data4[2], comGuid.Data4[3],
            comGuid.Data4[4], comGuid.Data4[5], comGuid.Data4[6], comGuid.Data4[7]);
    }




    Guid GetGuidFromType(Type const& type)
    {
        StringReference const guidAttributeName(L"Windows.Foundation.Metadata.GuidAttribute");

        // TODO We can cache the GUID Type and compare using its identity instead, for performance.
        auto const it(std::find_if(type.BeginCustomAttributes(),
                                   type.EndCustomAttributes(),
                                   [&](CustomAttribute const& attribute)
        {
            return attribute.GetConstructor().GetDeclaringType().GetFullName() == guidAttributeName.c_str();
        }));

        return it != type.EndCustomAttributes() ? it->GetSingleGuidArgument() : Guid();
    }


    // TODO Performance:  We do a linear search of the entire type system for every query, which is,
    // it suffices to say, ridiculously slow.  :-|
    Type GetTypeFromGuid(Assembly const& assembly, GUID const& comGuid)
    {
        Guid const cxxGuid(ToCxxGuid(comGuid));
        
        for (auto typeIt(assembly.BeginTypes()); typeIt != assembly.EndTypes(); ++typeIt)
        {
            Guid const typeGuid(GetGuidFromType(*typeIt));
            if (typeGuid == cxxGuid)
                return*typeIt;
        }

        return Type();
    }

} } }





//
//
// IMPLEMENTATION OF HEADER-DECLARED FUNCTIONS (I.E., THE PUBLIC INTERFACE)
//
//

namespace CxxReflect { namespace WindowsRuntime {

    void InspectableDeleter::operator()(IInspectable* inspectable)
    {
        if (inspectable == nullptr)
            return;

        inspectable->Release();
    }





    PackageAssemblyLocator::PackageAssemblyLocator(String const& packageRoot)
        : _packageRoot(packageRoot)
    {
        auto const metadataFiles(Private::EnumerateUniverseMetadataFiles(StringReference(packageRoot.c_str())));

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

    String PackageAssemblyLocator::LocateAssembly(AssemblyName const& assemblyName) const
    {
        String const simpleName(Detail::MakeLowercase(assemblyName.GetName()));

        // The platform metadata and system assembly are special-cased to use our platform metadata:
        if (simpleName == L"platform" || simpleName == L"mscorlib")
        {
            return _packageRoot + PlatformMetadataFileName;
        }

        Detail::AssertFail(L"Not Yet Implemented");
        return String();
    }

    String PackageAssemblyLocator::LocateAssembly(AssemblyName const& assemblyName,
                                                  String       const& fullTypeName) const
    {
        String const simpleName(Detail::MakeLowercase(assemblyName.GetName()));

        // The platform metadata and system assembly are special-cased to use our platform metadata:
        if (simpleName == L"platform" || simpleName == L"mscorlib")
        {
            return _packageRoot + PlatformMetadataFileName;
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

    PackageAssemblyLocator::PathMap::const_iterator PackageAssemblyLocator::BeginMetadataFiles() const
    {
        return _metadataFiles.begin();
    }

    PackageAssemblyLocator::PathMap::const_iterator PackageAssemblyLocator::EndMetadataFiles() const
    {
        return _metadataFiles.end();
    }

    String PackageAssemblyLocator::FindMetadataFileForNamespace(String const& namespaceName) const
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
            return _packageRoot + PlatformMetadataFileName;
        }

        // TODO Should we throw here or return an empty string?
        throw RuntimeError(L"Failed to locate metadata file");
    }





    void BeginInitialization(String const& platformMetadataPath)
    {
        if (Private::GlobalWindowsRuntimeLoader::IsInitialized())
            return;

        Private::GlobalWindowsRuntimeLoader::Initialize(std::async(std::launch::async,
        [=]() -> std::unique_ptr<Loader>
        {
            std::unique_ptr<PackageAssemblyLocator> resolver(new PackageAssemblyLocator(platformMetadataPath));
            PackageAssemblyLocator const& rawResolver(*resolver.get());

            std::unique_ptr<Loader> loader(new Loader(std::move(resolver)));

            typedef PackageAssemblyLocator::PathMap::value_type Element;
            std::for_each(rawResolver.BeginMetadataFiles(), rawResolver.EndMetadataFiles(), [&](Element const& e)
            {
                loader->LoadAssembly(e.second);
            });

            return loader;
        }));
    }

    bool HasInitializationBegun()
    {
        return Private::GlobalWindowsRuntimeLoader::IsInitialized();
    }

    bool IsInitialized()
    {
        return Private::GlobalWindowsRuntimeLoader::IsReady();
    }





    std::vector<Type> GetImplementersOf(Type const interfaceType)
    {
        Detail::Assert([&]{ return interfaceType.IsInitialized(); });

        // We only need to test Windows types if the target interface is from Windows:
        // TODO Is this really correct?
        bool const includeWindowsTypes(String(interfaceType.GetNamespace().c_str()).substr(0, 7) == L"Windows");

        Private::LoaderLease const lease(Private::GlobalWindowsRuntimeLoader::Lease());
        PackageAssemblyLocator const& locator(lease.GetLocator());

        std::vector<Type> implementers;

        // TODO Fix the type of the lambda parameter:
        typedef PackageAssemblyLocator::PathMap::value_type Element;
        std::for_each(locator.BeginMetadataFiles(), locator.EndMetadataFiles(), [&](Element const& f)
        {
            // TODO We need to add some StartsWith function that tests StringReference directly
            if (!includeWindowsTypes && f.first.substr(0, 7) == L"windows")
                return;

            // TODO We can do better filtering than this by checking assembly references.
            // TODO Add caching of the obtained data.
            Assembly const a(lease.Get().LoadAssembly(f.second));
            std::for_each(a.BeginTypes(), a.EndTypes(), [&](Type const& t)
            {
                if (std::find(t.BeginInterfaces(), t.EndInterfaces(), interfaceType) != t.EndInterfaces())
                    implementers.push_back(t);
            });
        });

        return implementers;
    }

    std::vector<Type> GetImplementersOf(GUID const& guid)
    {
        Private::LoaderLease lease(Private::GlobalWindowsRuntimeLoader::Lease());

        PackageAssemblyLocator const& locator(lease.GetLocator());

        Type targetType;
        for (auto it(locator.BeginMetadataFiles()); it != locator.EndMetadataFiles(); ++it)
        {
            Assembly a(lease.Get().LoadAssembly(it->second));

            targetType = Private::GetTypeFromGuid(a, guid);
            if (targetType.IsInitialized())
                break;
        }

        if (!targetType.IsInitialized())
            throw RuntimeError(L"Failed to locate interface type by GUID");

        return GetImplementersOf(targetType);
    }

    std::vector<Type> GetImplementersOf(StringReference const interfaceFullName, bool caseSensitive)
    {
        Type const interfaceType(GetType(interfaceFullName, caseSensitive));
        if (!interfaceType.IsInitialized())
            throw RuntimeError(L"Failed to locate named interface type");

        return GetImplementersOf(interfaceType);
    }

    std::vector<Type> GetImplementersOf(StringReference const namespaceName,
                                        StringReference const interfaceSimpleName,
                                        bool            const caseSensitive)
    {
        Type const interfaceType(GetType(namespaceName, interfaceSimpleName, caseSensitive));
        if (!interfaceType.IsInitialized())
            throw RuntimeError(L"Failed to locate named interface type");

        return GetImplementersOf(interfaceType);
    }






    Type GetType(StringReference const typeFullName, bool const caseSensitive)
    {
        auto const endOfNamespaceIt(std::find(typeFullName.rbegin(), typeFullName.rend(), '.').base());
        Detail::Assert([&]{ return endOfNamespaceIt != typeFullName.rend().base(); });

        String const namespaceName(typeFullName.begin(), std::prev(endOfNamespaceIt));
        String const typeSimpleName(endOfNamespaceIt, typeFullName.end());

        return GetType(namespaceName.c_str(), typeSimpleName.c_str(), caseSensitive);
    }

    Type GetType(StringReference const namespaceName,
                 StringReference const typeSimpleName,
                 bool            const caseSensitive)
    {
        Private::LoaderLease loader(Private::GlobalWindowsRuntimeLoader::Lease());
        
        PackageAssemblyLocator const& assemblyLocator(loader.GetLocator());

        String metadataFileName(assemblyLocator.FindMetadataFileForNamespace(namespaceName.c_str()));
        if (metadataFileName.empty())
            return Type();

        Assembly assembly(loader.Get().LoadAssembly(metadataFileName));
        if (!assembly.IsInitialized())
            return Type();

        return assembly.GetType(namespaceName.c_str(), typeSimpleName.c_str(), !caseSensitive);
    }

    Type GetTypeOf(IInspectable* const object)
    {
        Detail::AssertNotNull(object);

        Private::SmartHString typeNameHString;
        Detail::AssertSuccess(object->GetRuntimeClassName(typeNameHString.proxy()));
        Detail::AssertNotNull(typeNameHString.value());

        return GetType(typeNameHString.c_str());
    }





    UniqueInspectable CreateInspectableInstance(Type const type)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });
        // TODO Verify that 'type' is a valid kind of type to be activated.

        Private::SmartHString const typeFullName(type.GetFullName());

        Microsoft::WRL::ComPtr<IInspectable> instance;

        Detail::VerifySuccess(::RoActivateInstance(typeFullName.value(), instance.ReleaseAndGetAddressOf()));

        if (instance.Get() == nullptr)
            throw RuntimeError(L"Type activation failed");

        return UniqueInspectable(instance.Detach());
    }

} }





namespace CxxReflect { namespace { namespace Private {

    Method GetActivatableAttributeFactoryConstructor()
    {
        Type const activatableType(WindowsRuntime::GetType(
            L"Windows.Foundation.Metadata",
            L"ActivatableAttribute"));

        Detail::Verify([&]{ return activatableType.IsInitialized(); });

        auto const activatableConstructorIt(std::find_if(
            activatableType.BeginConstructors(BindingAttribute::Public | BindingAttribute::Instance), 
            activatableType.EndConstructors(),
            [&](Method const& constructor)
        {
            // TODO We should also check the parameter types.
            return std::distance(constructor.BeginParameters(), constructor.EndParameters()) == 2;
        }));

        Detail::Verify([&]{ return activatableConstructorIt != activatableType.EndConstructors(); });

        return *activatableConstructorIt;
    }

    Type GetActivationFactoryType(Type const type)
    {
        Method const activatableConstructor(GetActivatableAttributeFactoryConstructor());

        auto const activatableAttributeIt(std::find_if(type.BeginCustomAttributes(), type.EndCustomAttributes(),
        [&](CustomAttribute const& a)
        {
            return a.GetConstructor() == activatableConstructor;
        }));

        Detail::Verify([&]{ return activatableAttributeIt != type.EndCustomAttributes(); });

        String const factoryTypeName(activatableAttributeIt->GetSingleStringArgument());

        return WindowsRuntime::GetType(factoryTypeName.c_str());
    }

    // This argument frame accumulates and aligns arguments for a stdcall function call.  Since
    // stdcall arguments are pushed right-to-left, arguments must be added to the frame in reverse
    // order (that is, right-to-left).
    class X86StdCallArgumentFrame
    {
    public:

    private:

        std::vector<Byte> _frame;
    };

    #if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86
    typedef X86StdCallArgumentFrame ArgumentFrame;
    #endif
    // TODO Support for X64 and ARM

} } }

namespace CxxReflect { namespace Detail {

    SizeType VariantArgumentPack::Arity() const
    {
        return static_cast<SizeType>(_arguments.size());
    }

    void VariantArgumentPack::Push(bool const value)
    {
        Push(Metadata::ElementType::Boolean, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(wchar_t const value)
    {
        Push(Metadata::ElementType::Char, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(std::int8_t const value)
    {
        Push(Metadata::ElementType::I1, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(std::uint8_t const value)
    {
        Push(Metadata::ElementType::U1, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(std::int16_t const value)
    {
        Push(Metadata::ElementType::I2, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(std::uint16_t const value)
    {
        Push(Metadata::ElementType::U2, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(std::int32_t const value)
    {
        Push(Metadata::ElementType::I4, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(std::uint32_t const value)
    {
        Push(Metadata::ElementType::U4, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(std::int64_t const value)
    {
        Push(Metadata::ElementType::I8, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(std::uint64_t const value)
    {
        Push(Metadata::ElementType::U8, BeginBytes(value), EndBytes(value));
    }
        
    void VariantArgumentPack::Push(float const value)
    {
        Push(Metadata::ElementType::R4, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(double const value)
    {
        Push(Metadata::ElementType::R8, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(IInspectable* const value)
    {
        Push(Metadata::ElementType::Class, BeginBytes(value), EndBytes(value));
    }

    void VariantArgumentPack::Push(Metadata::ElementType const type,
                                   ConstByteIterator     const first,
                                   ConstByteIterator     const last)
    {
        SizeType const index(static_cast<SizeType>(_data.size()));

        std::copy(first, last, std::back_inserter(_data));
        _arguments.push_back(Argument(type, index, index + Distance(first, last)));
    }

    WindowsRuntime::UniqueInspectable CreateInspectableInstance(Type                const  type,
                                                                VariantArgumentPack const& arguments)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        Type const factoryType(Private::GetActivationFactoryType(type));
        Guid const factoryGuid(Private::GetGuidFromType(factoryType));

        Private::SmartHString const typeFullName(type.GetFullName().c_str());
       
        Microsoft::WRL::ComPtr<IInspectable> factory;
        Detail::VerifySuccess(::RoGetActivationFactory(
            typeFullName.value(),
            Private::ToComGuid(factoryGuid),
            reinterpret_cast<void**>(factory.ReleaseAndGetAddressOf())));

        // Find the best matching constructor. Right now we just look for the constructor with the
        // same number of arguments.  TODO Add full runtime overload resolution support
        BindingFlags const activatorBinding(
            BindingAttribute::Public    |
            BindingAttribute::NonPublic |
            BindingAttribute::Instance);

        SizeType slotIndex(static_cast<SizeType>(-1)); // TODO We should have the method track this itself :-)
        auto const activatorIt(std::find_if(factoryType.BeginMethods(activatorBinding), factoryType.EndMethods(),
        [&](Method const& method)
        {
            ++slotIndex;

            if (method.GetName() != L"CreateInstance")
                return false;

            if (Detail::Distance(method.BeginParameters(), method.EndParameters()) != arguments.Arity())
                return false;

            return true;
        }));

        if (activatorIt == factoryType.EndMethods())
            throw RuntimeError(L"Failed to find activation method matching provided arguments");

        Method const activatorMethod(*activatorIt);

        return WindowsRuntime::UniqueInspectable();
    }

} }

#endif
