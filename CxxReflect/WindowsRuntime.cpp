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
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Type.hpp"

#include <atomic>
#include <filesystem>
#include <future>
#include <thread>

#include <agents.h>
#include <concrt.h>
#include <hstring.h>
#include <inspectable.h>
#include <ppl.h>
#include <ppltasks.h>
#include <roapi.h>
#include <rometadataresolution.h>
#include <windows.h>
#include <winstring.h>
#include <wrl/client.h>





//
//
// WINDOWS RUNTIME LOADER CONTEXT
//
//

namespace CxxReflect { namespace WindowsRuntime {

    class LoaderContext
    {
    public:

        typedef PackageAssemblyLocator  Locator;

        typedef std::recursive_mutex    Mutex;
        typedef std::unique_lock<Mutex> Lock;

        LoaderContext(std::unique_ptr<Loader>&& loader)
            : _loader(std::move(loader))
        {
            Detail::Verify([&]{ return _loader != nullptr; });
        }

        Loader const& GetLoader() const
        {
            return *_loader;
        }

        Locator const& GetLocator() const
        {
            Loader const& loader(GetLoader());

            IAssemblyLocator const& locator(loader.GetAssemblyLocator(InternalKey()));

            Detail::Assert([&]{ return dynamic_cast<Locator const*>(&locator) != nullptr; });
            return *static_cast<Locator const*>(&locator);
        }

        Type GetActivationFactoryTypeFor(Type const& type)
        {
            Method const activatableConstructor(GetActivatableAttributeFactoryConstructor());

            auto const activatableAttributeIt(std::find_if(type.BeginCustomAttributes(), type.EndCustomAttributes(),
            [&](CustomAttribute const& a)
            {
                return a.GetConstructor() == activatableConstructor;
            }));

            Detail::Verify([&]{ return activatableAttributeIt != type.EndCustomAttributes(); });

            String const factoryTypeName(activatableAttributeIt->GetSingleStringArgument());

            return GetType(factoryTypeName.c_str(), true);
        }

        Guid GetGuid(Type const& type)
        {
            Detail::Verify([&]{ return type.IsInitialized(); }, L"Uninitialized type provided as argument");

            Type const guidAttributeType(GetGuidAttributeType());

            // TODO We can cache the GUID Type and compare using its identity instead, for performance.
            auto const it(std::find_if(type.BeginCustomAttributes(),
                                       type.EndCustomAttributes(),
                                       [&](CustomAttribute const& attribute)
            {
                return attribute.GetConstructor().GetDeclaringType() == guidAttributeType;
            }));

            return it != type.EndCustomAttributes() ? it->GetSingleGuidArgument() : Guid();
        }

        std::vector<Type> GetImplementersOf(Type const& interfaceType)
        {
            Detail::Verify([&]{ return interfaceType.IsInitialized(); }, L"Uninitialized argument");

            // We only need to test Windows types if the target interface is from Windows:
            // TODO Is this really correct?
            bool const includeWindowsTypes(Detail::StartsWith(interfaceType.GetNamespace().c_str(), L"Windows"));

            Loader  const& loader (GetLoader() );
            Locator const& locator(GetLocator());

            std::vector<Type> implementers;

            typedef PackageAssemblyLocator::PathMap::value_type Element;
            std::for_each(locator.BeginMetadataFiles(), locator.EndMetadataFiles(), [&](Element const& f)
            {
                // TODO We need to add some StartsWith function that tests StringReference directly
                if (!includeWindowsTypes && Detail::StartsWith(f.first.c_str(), L"windows"))
                    return;

                // TODO We can do better filtering than this by checking assembly references.
                // TODO Add caching of the obtained data.
                Assembly const a(loader.LoadAssembly(f.second));
                std::for_each(a.BeginTypes(), a.EndTypes(), [&](Type const& t)
                {
                    if (std::find(t.BeginInterfaces(), t.EndInterfaces(), interfaceType) != t.EndInterfaces())
                        implementers.push_back(t);
                });
            });

            return implementers;
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

        // A non-throwing GetType().  Attempts to get the named type, and returns a null type if the
        // named type cannot be found.
        Type GetType(StringReference const namespaceName,
                     StringReference const typeSimpleName,
                     bool            const caseSensitive) const
        {
            Loader  const& loader (GetLoader() );
            Locator const& locator(GetLocator());

            String const metadataFileName(locator.FindMetadataFileForNamespace(namespaceName.c_str()));
            if (metadataFileName.empty())
                return Type();

            // TODO We need a non-throwing LoadAssembly.
            Assembly const assembly(loader.LoadAssembly(metadataFileName));
            if (!assembly.IsInitialized())
                return Type();

            return assembly.GetType(namespaceName.c_str(), typeSimpleName.c_str(), !caseSensitive);
        }


        #define CXXREFLECT_DEFINE_PROPERTY(XTYPE, XNAME, ...)                                   \
            private:                                                                            \
                mutable Detail::ValueInitialized<bool> _delayInit ## XNAME ## Initialized;      \
                mutable XTYPE                          _delayInit ## XNAME;                     \
                                                                                                \
            public:                                                                             \
                                                                                                \
                XTYPE Get ## XNAME() const                                                      \
                {                                                                               \
                    Lock const lock(_sync);                                                     \
                                                                                                \
                    if (!_delayInit ## XNAME ## Initialized.Get())                              \
                    {                                                                           \
                        _delayInit ## XNAME = (__VA_ARGS__)();                                  \
                        _delayInit ## XNAME ## Initialized.Get() = true;                        \
                    }                                                                           \
                                                                                                \
                    return _delayInit ## XNAME;                                                 \
                }

        #define CXXREFLECT_DEFINE_TYPE_PROPERTY(XNAME, XNAMESPACE, XTYPENAME)                   \
            CXXREFLECT_DEFINE_PROPERTY(Type, XNAME, [&]() -> Type                               \
            {                                                                                   \
                Type const type(GetType(XNAMESPACE, XTYPENAME, true));                          \
                                                                                                \
                Detail::Verify([&]{ return type.IsInitialized(); }, L"Failed to find type");    \
                                                                                                \
                return type;                                                                    \
            })

        CXXREFLECT_DEFINE_TYPE_PROPERTY(ActivatableAttributeType, L"Windows.Foundation.Metadata", L"ActivatableAttribute");
        CXXREFLECT_DEFINE_TYPE_PROPERTY(GuidAttributeType,        L"Windows.Foundation.Metadata", L"GuidAttribute");

        CXXREFLECT_DEFINE_PROPERTY(Method, ActivatableAttributeFactoryConstructor, [&]() -> Method
        {
            Type const attributeType(GetActivatableAttributeType());

            BindingFlags const bindingFlags(BindingAttribute::Public | BindingAttribute::Instance);
            auto const firstConstructor(attributeType.BeginConstructors(bindingFlags));
            auto const lastConstructor(attributeType.EndConstructors());

            auto const constructorIt(std::find_if(firstConstructor, lastConstructor, [&](Method const& constructor)
            {
                // TODO We should also check parameter types.
                return Detail::Distance(constructor.BeginParameters(), constructor.EndParameters()) == 2;
            }));

            Detail::Verify([&]{ return constructorIt != attributeType.EndConstructors(); });

            return *constructorIt;

        });

        #undef CXXREFLECT_DEFINE_TYPE_PROPERTY
        #undef CXXREFLECT_DEFINE_PROPERTY

    private:

        LoaderContext(LoaderContext const&);
        LoaderContext& operator=(LoaderContext const&);
   
        std::unique_ptr<Loader>         _loader;
        Mutex                   mutable _sync;
    };

} }





//
//
// GLOBAL WINDOWS RUNTIME METADATA LOADER
//
//

namespace CxxReflect { namespace { namespace Private {

    typedef std::atomic<bool>                              LoaderInitializedFlag;
    typedef std::unique_ptr<WindowsRuntime::LoaderContext> LoaderContextPointer;
    typedef std::shared_future<LoaderContextPointer>       LoaderContextFuture;

    class GlobalLoaderContext
    {
    public:

        static void Initialize(LoaderContextFuture&& context)
        {
            bool expected(false);
            if (!_initialized.compare_exchange_strong(expected, true))
                throw LogicError(L"Global Windows Runtime Loader was already initialized");

            _context = std::move(context);
        }

        static WindowsRuntime::LoaderContext& Get()
        {
            Detail::Assert([&]{ return _initialized.load(); });

            WindowsRuntime::LoaderContext* const context(_context.get().get());
            Detail::Verify([&]{ return context != nullptr; }, L"Global Windows Runtime Loader is not valid");

            return *context;
        }

        static bool IsInitialized() { return _initialized.load(); }
        static bool IsReady()       { return _context.valid();    }

    private:

        // This type is not constructible.  It exists solely to prevent direct access to _loader.
        GlobalLoaderContext();
        GlobalLoaderContext(GlobalLoaderContext const&);
        GlobalLoaderContext& operator=(GlobalLoaderContext const&);

        static LoaderInitializedFlag _initialized;
        static LoaderContextFuture   _context;
    };

    LoaderInitializedFlag GlobalLoaderContext::_initialized;
    LoaderContextFuture   GlobalLoaderContext::_context;

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
        ConstCharacterIterator const buffer(::WindowsGetStringRawBuffer(hstring, nullptr));
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

    void EnumeratePackageMetadataFilesRecursive(SmartHString const rootNamespace, std::vector<String>& result)
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

        std::transform(filePaths.begin(), filePaths.end(), std::back_inserter(result), [](HSTRING const path)
        {
            return ToString(path);
        });

        String baseNamespace(rootNamespace.c_str());
        if (!baseNamespace.empty())
            baseNamespace.push_back(L'.');

        std::for_each(nestedNamespaces.begin(), nestedNamespaces.end(), [&](HSTRING const nestedNamespace)
        {
            EnumeratePackageMetadataFilesRecursive(baseNamespace + ToString(nestedNamespace), result);
        });
    }

    std::vector<String> EnumerateUniverseMetadataFiles(StringReference const /* packageDirectory*/)
    {
        std::vector<String> result;

        EnumeratePackageMetadataFilesRecursive(SmartHString(), result);

        // WORKAROUND:  For some Application Packages, RoResolveNamespace doesn't seem to find all
        // metadata files in the package.  This is a brute-force workaround that just enumerates
        // the .winmd files in the package root.
        // for (std::tr2::sys::wdirectory_iterator it(packageDirectory.c_str()), end; it != end; ++it)
        // {
        //     if (it->path().extension() != L".winmd")
        //         continue;
        //
        //     result.push_back(String(packageDirectory.c_str()) + it->path().filename());
        // }

        std::sort(begin(result), end(result));
        result.erase(std::unique(begin(result), end(result)), end(result));

        return result;
    }

    void RemoveRightmostTypeNameComponent(String& typeName)
    {
        Detail::Assert([&]{ return !typeName.empty(); });

        // TODO This does not handle generics.  Does it need to handle generics?
        auto it(std::find(typeName.rbegin(), typeName.rend(), L'.').base());
        if (it == typeName.begin())
            typeName = String();

        typeName.erase(it - 1, typeName.end());
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





    // TODO Performance:  We do a linear search of the entire type system for every query, which is,
    // it suffices to say, ridiculously slow.  :-|
    Type GetTypeFromGuid(Assembly const& assembly, GUID const& comGuid)
    {
        Guid const cxxGuid(ToCxxGuid(comGuid));
        
        for (auto typeIt(assembly.BeginTypes()); typeIt != assembly.EndTypes(); ++typeIt)
        {
            Guid const typeGuid(WindowsRuntime::GetGuid(*typeIt));
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
        if (Detail::StartsWith(lowercaseNamespaceName.c_str(), L"platform") ||
            Detail::StartsWith(lowercaseNamespaceName.c_str(), L"system"))
        {
            return _packageRoot + PlatformMetadataFileName;
        }

        // TODO Should we throw here or return an empty string?
        throw RuntimeError(L"Failed to locate metadata file");
    }





    String LoaderConfiguration::TransformNamespace(String const& namespaceName)
    {
        if (namespaceName == L"System")
            return L"Platform";

        return namespaceName;
    }





    void BeginInitialization(String const& platformMetadataPath)
    {
        if (Private::GlobalLoaderContext::IsInitialized())
            return;

        Private::GlobalLoaderContext::Initialize(std::async(std::launch::async,
        [=]() -> std::unique_ptr<LoaderContext>
        {
            std::unique_ptr<PackageAssemblyLocator> resolver(new PackageAssemblyLocator(platformMetadataPath));
            PackageAssemblyLocator const& rawResolver(*resolver.get());

            std::unique_ptr<ILoaderConfiguration> configuration(new LoaderConfiguration());

            std::unique_ptr<Loader> loader(new Loader(std::move(resolver), std::move(configuration)));

            typedef PackageAssemblyLocator::PathMap::value_type Element;
            std::for_each(rawResolver.BeginMetadataFiles(), rawResolver.EndMetadataFiles(), [&](Element const& e)
            {
                loader->LoadAssembly(e.second);
            });

            std::unique_ptr<LoaderContext> context(new LoaderContext(std::move(loader)));
            return context;
        }));

        return;
    }

    bool HasInitializationBegun()
    {
        return Private::GlobalLoaderContext::IsInitialized();
    }

    void CallWhenInitialized(std::function<void()> const callable)
    {
        Concurrency::task<void> t([&]
        {
            Private::GlobalLoaderContext::Get();
        });

        t.then(callable);
    }

    bool IsInitialized()
    {
        return Private::GlobalLoaderContext::IsReady();
    }





    std::vector<Type> GetImplementersOf(Type const interfaceType)
    {
        return Private::GlobalLoaderContext::Get().GetImplementersOf(interfaceType);
    }

    std::vector<Type> GetImplementersOf(GUID const& guid)
    {
        Loader const& loader(Private::GlobalLoaderContext::Get().GetLoader());

        PackageAssemblyLocator const& locator(Private::GlobalLoaderContext::Get().GetLocator());

        Type targetType;
        for (auto it(locator.BeginMetadataFiles()); it != locator.EndMetadataFiles(); ++it)
        {
            Assembly a(loader.LoadAssembly(it->second));

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
        return Private::GlobalLoaderContext::Get().GetImplementersOf(interfaceFullName, caseSensitive);
    }

    std::vector<Type> GetImplementersOf(StringReference const namespaceName,
                                        StringReference const interfaceSimpleName,
                                        bool            const caseSensitive)
    {
        return Private::GlobalLoaderContext::Get().GetImplementersOf(
            namespaceName,
            interfaceSimpleName,
            caseSensitive);
    }






    Type GetType(StringReference const typeFullName, bool const caseSensitive)
    {
        return Private::GlobalLoaderContext::Get().GetType(typeFullName, caseSensitive);
    }

    Type GetType(StringReference const namespaceName,
                 StringReference const typeSimpleName,
                 bool            const caseSensitive)
    {
        return Private::GlobalLoaderContext::Get().GetType(namespaceName, typeSimpleName, caseSensitive);
    }

    Type GetTypeOf(IInspectable* const object)
    {
        Detail::AssertNotNull(object);

        Private::SmartHString typeNameHString;
        Detail::AssertSuccess(object->GetRuntimeClassName(typeNameHString.proxy()));
        Detail::AssertNotNull(typeNameHString.value());

        return GetType(typeNameHString.c_str());
    }





    bool IsDefaultConstructible(Type const& type)
    {
        BindingFlags const flags(BindingAttribute::Instance | BindingAttribute::Public);
        auto const it(std::find_if(type.BeginConstructors(flags), type.EndConstructors(), [](Method const& c)
        {
            return c.GetParameterCount() == 0;
        }));

        return type.BeginConstructors(flags) == type.EndConstructors() || it != type.EndConstructors();
    }

    Guid GetGuid(Type const& type)
    {
        return Private::GlobalLoaderContext::Get().GetGuid(type);
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

    Type GetActivationFactoryType(Type const type)
    {
        Method const activatableConstructor(GlobalLoaderContext::Get().GetActivatableAttributeFactoryConstructor());

        auto const activatableAttributeIt(std::find_if(type.BeginCustomAttributes(), type.EndCustomAttributes(),
        [&](CustomAttribute const& a)
        {
            return a.GetConstructor() == activatableConstructor;
        }));

        Detail::Verify([&]{ return activatableAttributeIt != type.EndCustomAttributes(); });

        String const factoryTypeName(activatableAttributeIt->GetSingleStringArgument());

        return WindowsRuntime::GetType(factoryTypeName.c_str());
    }

    Method FindMatchingInterfaceMethod(Method const runtimeTypeMethod)
    {
        Detail::Assert([&]{ return runtimeTypeMethod.IsInitialized(); });

        BindingFlags const bindingFlags(BindingAttribute::Public | BindingAttribute::Instance);

        Type const runtimeType(runtimeTypeMethod.GetReflectedType());
        if (runtimeType.IsInterface())
            return runtimeTypeMethod;

        for (auto interfaceIt(runtimeType.BeginInterfaces()); interfaceIt != runtimeType.EndInterfaces(); ++interfaceIt)
        {
            for (auto methodIt(interfaceIt->BeginMethods(bindingFlags)); methodIt != interfaceIt->EndMethods(); ++methodIt)
            {
                if (methodIt->GetName() != runtimeTypeMethod.GetName())
                    continue;

                if (methodIt->GetReturnType() != runtimeTypeMethod.GetReturnType())
                    continue;

                if (!Detail::RangeCheckedEqual(
                        methodIt->BeginParameters(), methodIt->EndParameters(),
                        runtimeTypeMethod.BeginParameters(), runtimeTypeMethod.EndParameters()))
                    continue;

                return *methodIt;
            }
        }

        return Method();
    }

    

    

    // This argument frame accumulates and aligns arguments for a stdcall function call.  Since
    // stdcall arguments are pushed right-to-left, arguments must be added to the frame in reverse
    // order (that is, right-to-left).
    class X86StdCallArgumentFrame
    {
    public:

        void Push(void const* const pointer)
        {
            std::copy(Detail::BeginBytes(pointer), Detail::EndBytes(pointer), std::back_inserter(_frame));
        }

        void Push(ConstByteIterator const first, ConstByteIterator const last)
        {
            std::copy(first, last, std::back_inserter(_frame));
            //std::copy(ConstReverseByteIterator(last), ConstReverseByteIterator(first), std::inserter(_frame, _frame.begin()));
        }

        ConstByteIterator Begin() const
        {
            return _frame.data();
        }

        SizeType Size() const
        {
            return _frame.size();
        }

    private:

        std::vector<Byte> _frame;
    };



    class X86StdCallInvoker
    {
    public:

        static HResult Invoke(Method                      const  method,
                              SizeType                    const  methodIndex,
                              IInspectable*               const  instance, 
                              void*                       const  result,
                              Detail::VariantArgumentPack const& arguments)
        {
            X86StdCallArgumentFrame frame;

            //if (method.GetReturnType() != WindowsRuntime::GetType(L"Platform", L"Void", false))
                frame.Push(instance);

            for (auto it(arguments.ReverseBegin()); it != arguments.ReverseEnd(); ++it)
                frame.Push(it->BeginValue(arguments), it->EndValue(arguments));

            frame.Push(result);

            // TODO QI TO THE RIGHT INTERFACE
            void* fp(ComputeFunctionPointer(methodIndex + 6, instance));

            // TODO We aren't doing any conversions yet :-)
            switch (frame.Size())
            {
            case  4: return InternalInvoke< 4>(fp, frame.Begin());
            case  8: return InternalInvoke< 8>(fp, frame.Begin());
            case 12: return InternalInvoke<12>(fp, frame.Begin());
            case 16: return InternalInvoke<16>(fp, frame.Begin());
            case 20: return InternalInvoke<20>(fp, frame.Begin());
            }

            return -1;
        }

    private:

        template <SizeType FrameSize>
        static HResult InternalInvoke(void* functionPointer, ConstByteIterator const frameBytes)
        {
            typedef std::array<Byte, FrameSize> FrameType;
            typedef HResult (__stdcall* FunctionPointer)(FrameType);

            FrameType frame;
            std::copy(frameBytes, frameBytes + FrameSize, begin(frame));

            FunctionPointer fp(reinterpret_cast<FunctionPointer>(functionPointer));

            return fp(frame);
        }

        static void* ComputeFunctionPointer(unsigned const index, void* const thisptr)
        {
            Detail::AssertNotNull(thisptr);

            // There are three levels of indirection:  'this' points to an object, the first element
            // of which points to a vtable, which contains slots that point to functions:
            return (*reinterpret_cast<void***>(thisptr))[index];
        }

    };

    #if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86
    typedef X86StdCallArgumentFrame ArgumentFrame;
    typedef X86StdCallInvoker       CallInvoker;
    #endif
    // TODO Support for X64 and ARM


} } }

//
//
// ARGUMENT HANDLING
//
//

namespace CxxReflect { namespace Detail {

    VariantArgumentPack::Argument::Argument(Metadata::ElementType const type,
                                            SizeType              const valueIndex,
                                            SizeType              const valueSize,
                                            SizeType              const nameIndex,
                                            SizeType              const nameSize)
        : _type(type), _valueIndex(valueIndex), _valueSize(valueSize), _nameIndex(nameIndex), _nameSize(nameSize)
    {
    }

    Type VariantArgumentPack::Argument::GetType(VariantArgumentPack const& owner) const
    {
        if (_type.Get() == Metadata::ElementType::Class)
        {
            Assert([&]{ return sizeof(IInspectable*) == Distance(BeginValue(owner), EndValue(owner)); });

            IInspectable* value;
            RangeCheckedCopy(BeginValue(owner), EndValue(owner), BeginBytes(value), EndBytes(value));

            Private::SmartHString typeName;
            AssertSuccess(value->GetRuntimeClassName(typeName.proxy()));

            Type const type(WindowsRuntime::GetType(typeName.c_str()));
            if (!type.IsInitialized())
                throw RuntimeError(L"Failed to determine type of runtime object");

            return type;
        }
        else if (_type.Get() == Metadata::ElementType::ValueType)
        {
            throw LogicError(L"Not yet implemented");
        }
        else
        {
            return Private::GlobalLoaderContext::Get()
                .GetLoader()
                .GetFundamentalType(_type.Get(), InternalKey());
        }
    }

    ConstByteIterator VariantArgumentPack::Argument::BeginValue(VariantArgumentPack const& owner) const
    {
        return owner._data.data() + _valueIndex.Get();
    }

    ConstByteIterator VariantArgumentPack::Argument::EndValue(VariantArgumentPack const& owner) const
    {
        return owner._data.data() + _valueIndex.Get() + _valueSize.Get();
    }

    StringReference VariantArgumentPack::Argument::GetName(VariantArgumentPack const& owner) const
    {
        if (_nameIndex.Get() == 0 && _nameSize.Get() == 0)
            return StringReference();

        return StringReference(
            reinterpret_cast<Character const*>(owner._data.data() + _nameIndex.Get()),
            reinterpret_cast<Character const*>(owner._data.data() + _nameIndex.Get() + _nameSize.Get()));
    }

    VariantArgumentPack::InspectableArgument::InspectableArgument()
    {
    }

    VariantArgumentPack::InspectableArgument::InspectableArgument(IInspectable*   const value,
                                                                  StringReference const name)
        : _value(value), _name(name.c_str())
    {
    }

    IInspectable* VariantArgumentPack::InspectableArgument::GetValue() const
    {
        return _value.Get();
    }

    StringReference VariantArgumentPack::InspectableArgument::GetName() const
    {
        return _name.c_str();
    }

    VariantArgumentPack::ArgumentIterator VariantArgumentPack::Begin() const
    {
        return begin(_arguments);
    }

    VariantArgumentPack::ArgumentIterator VariantArgumentPack::End() const
    {
        return end(_arguments);
    }

    VariantArgumentPack::ReverseArgumentIterator VariantArgumentPack::ReverseBegin() const
    {
        return _arguments.rbegin();
    }

    VariantArgumentPack::ReverseArgumentIterator VariantArgumentPack::ReverseEnd() const
    {
        return _arguments.rend();
    }

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

    void VariantArgumentPack::Push(InspectableArgument const argument)
    {
        IInspectable* const value(argument.GetValue());

        SizeType const valueIndex(static_cast<SizeType>(_data.size()));
        std::copy(BeginBytes(value), EndBytes(value), std::back_inserter(_data));

        SizeType const nameIndex(static_cast<SizeType>(_data.size()));
        std::copy(reinterpret_cast<Byte const*>(argument.GetName().begin()),
                  reinterpret_cast<Byte const*>(argument.GetName().end()),
                  std::back_inserter(_data));

        _arguments.push_back(Argument(
            Metadata::ElementType::Class,
            valueIndex, static_cast<SizeType>(sizeof(value)),
            nameIndex, static_cast<SizeType>(argument.GetName().size() + 1)));
    }

    void VariantArgumentPack::Push(Metadata::ElementType const type,
                                   ConstByteIterator     const first,
                                   ConstByteIterator     const last)
    {
        SizeType const index(static_cast<SizeType>(_data.size()));

        std::copy(first, last, std::back_inserter(_data));
        _arguments.push_back(Argument(type, index, index + Distance(first, last)));
    }






    bool ConvertingOverloadResolver::Succeeded() const
    {
        EnsureEvaluated();
        return _state.Get() == State::MatchFound;
    }

    Method ConvertingOverloadResolver::GetResult() const
    {
        EnsureEvaluated();
        if (_state.Get() != State::MatchFound)
            throw LogicError(L"Matching method not found.  Call Succeeded() first.");

        return _result;
    }

    void ConvertingOverloadResolver::EnsureEvaluated() const
    {
        if (_state.Get() != State::NotEvaluated)
            return;

        // Accumulate the argument types once, for performance:
        std::vector<Type> argumentTypes;
        std::transform(_arguments.Begin(), _arguments.End(), std::back_inserter(argumentTypes),
                        [&](Detail::VariantArgumentPack::Argument const& a) -> Type
        {
            return a.GetType(_arguments);
        });

        _state.Get() = State::MatchNotFound;

        auto                        bestMatch(end(_candidates));
        std::vector<ConversionRank> bestMatchRank(argumentTypes.size(), ConversionRank::NoMatch);

        for (auto methodIt(begin(_candidates)); methodIt != end(_candidates); ++methodIt)
        {
            // First, check to see if the arity matches:
            if (Distance(methodIt->BeginParameters(), methodIt->EndParameters()) != argumentTypes.size())
                continue;

            std::vector<ConversionRank> currentRank(argumentTypes.size(), ConversionRank::NoMatch);

            // Compute the conversion rank of this method:
            unsigned parameterNumber(0);
            auto parameterIt(methodIt->BeginParameters());
            for (; parameterIt != methodIt->EndParameters(); ++parameterIt, ++parameterNumber)
            {
                currentRank[parameterNumber] = ComputeConversionRank(
                    parameterIt->GetType(),
                    argumentTypes[parameterNumber]);

                // If any parameter is not a match, the whole method is not a match:
                if (currentRank[parameterNumber] == ConversionRank::NoMatch)
                    break;
            }

            bool betterMatch(false);
            bool worseMatch(false);
            bool noMatch(false);
            for (unsigned i(0); i < argumentTypes.size(); ++i)
            {
                if (currentRank[i] == ConversionRank::NoMatch)
                    noMatch = true;
                else if (currentRank[i] < bestMatchRank[i])
                    betterMatch = true;
                else if (currentRank[i] > bestMatchRank[i])
                    worseMatch = true;
            }

            if (noMatch)
            {
                continue;
            }

            // This is an unambiguously better match than the current best match:
            if (betterMatch && !worseMatch)
            {
                bestMatch     = methodIt;
                bestMatchRank = currentRank;
                continue;
            }

            // This is an unambiguously worse match than the current best match:
            if (worseMatch && !betterMatch)
            {
                continue;
            }

            // There is an ambiguity between this match and the current best match:
            bestMatch = end(_candidates);
            for (unsigned i(0); i < argumentTypes.size(); ++i)
            {
                bestMatchRank[i] = bestMatchRank[i] < currentRank[i] ? bestMatchRank[i] : currentRank[i];
            }
        }

        _result = *bestMatch;

        if (_result.IsInitialized())
            _state.Get() = State::MatchFound;
    }

    Metadata::ElementType ConvertingOverloadResolver::ComputeElementType(Type const& type)
    {
        Assert([&]{ return type.IsInitialized(); });

        // Shortcut:  If 'type' isn't from the system assembly, it isn't one of the system types:
        if (!Utility::IsSystemAssembly(type.GetAssembly()))
            return type.IsValueType() ? Metadata::ElementType::ValueType : Metadata::ElementType::Class;

        Loader const& loader(type.GetAssembly().GetContext(InternalKey()).GetLoader());

        #define CXXREFLECT_GENERATE(A)                                                         \
            if (loader.GetFundamentalType(Metadata::ElementType::A, InternalKey()) == type)    \
            {                                                                                  \
                return Metadata::ElementType::A;                                               \
            }

        CXXREFLECT_GENERATE(Boolean)
        CXXREFLECT_GENERATE(Char)
        CXXREFLECT_GENERATE(I1)
        CXXREFLECT_GENERATE(U1)
        CXXREFLECT_GENERATE(I2)
        CXXREFLECT_GENERATE(U2)
        CXXREFLECT_GENERATE(I4)
        CXXREFLECT_GENERATE(U4)
        CXXREFLECT_GENERATE(I8)
        CXXREFLECT_GENERATE(U8)
        CXXREFLECT_GENERATE(R4)
        CXXREFLECT_GENERATE(R8)

        #undef CXXREFLECT_GENERATE

        return type.IsValueType() ? Metadata::ElementType::ValueType : Metadata::ElementType::Class;
    }

    ConvertingOverloadResolver::ConversionRank
    ConvertingOverloadResolver::ComputeConversionRank(Type const& parameterType, Type const& argumentType)
    {
        Assert([&]{ return parameterType.IsInitialized() && argumentType.IsInitialized(); });

        Metadata::ElementType const pType(ComputeElementType(parameterType));
        Metadata::ElementType const aType(ComputeElementType(argumentType ));

        // Exact match of any kind.
        if (parameterType == argumentType)
        {
            return ConversionRank::ExactMatch;
        }

        // Value Types, Boolean, Char, and String only match exactly; there are no conversions.
        if (pType == Metadata::ElementType::ValueType || aType == Metadata::ElementType::ValueType ||
            pType == Metadata::ElementType::Boolean   || aType == Metadata::ElementType::Boolean   ||
            pType == Metadata::ElementType::Char      || aType == Metadata::ElementType::Char      ||
            pType == Metadata::ElementType::String    || aType == Metadata::ElementType::String)
        {
            return ConversionRank::NoMatch;
        }

        // A Class Type may be converted to another Class Type.
        if (pType == Metadata::ElementType::Class && aType == Metadata::ElementType::Class)
        {
            return ComputeClassConversionRank(parameterType, argumentType);
        }
        else if (pType == Metadata::ElementType::Class || aType == Metadata::ElementType::Class)
        {
            return ConversionRank::NoMatch;
        }

        // Numeric conversions:
        if (IsNumericElementType(pType) && IsNumericElementType(aType))
        {
            return ComputeNumericConversionRank(pType, aType);
        }

        AssertFail(L"Not yet implemented");
        return ConversionRank::NoMatch;
    }

    ConvertingOverloadResolver::ConversionRank
    ConvertingOverloadResolver::ComputeClassConversionRank(Type const& parameterType, Type const& argumentType)
    {
        Assert([&]{ return !parameterType.IsValueType() && !argumentType.IsValueType(); });
        Assert([&]{ return parameterType != argumentType;                               });

        // First check to see if there is a derived-to-base conversion:
        if (parameterType.IsClass())
        {
            unsigned baseDistance(1);
            Type baseType(argumentType.GetBaseType());
            while (baseType.IsInitialized())
            {
                if (baseType == parameterType)
                    return ConversionRank::DerivedToBaseConversion | static_cast<ConversionRank>(baseDistance);

                baseType = baseType.GetBaseType();
                ++baseDistance;
            }
        }

        // Next check to see if there is an interface conversion.  Note that all interface
        // conversions are of equal rank.
        if (parameterType.IsInterface())
        {
            Type currentType(argumentType);
            while (currentType.IsInitialized())
            {
                auto const it(std::find(currentType.BeginInterfaces(), currentType.EndInterfaces(), parameterType));
                if (it != currentType.EndInterfaces())
                    return ConversionRank::DerivedToInterfaceConversion;
            }
        }

        return ConversionRank::NoMatch;
    }

    ConvertingOverloadResolver::ConversionRank
    ConvertingOverloadResolver::ComputeNumericConversionRank(Metadata::ElementType const pType,
                                                             Metadata::ElementType const aType)
    {
        Assert([&]{ return IsNumericElementType(pType) && IsNumericElementType(aType); });
        Assert([&]{ return pType != aType;                                             });

        if (IsIntegralElementType(pType) && IsIntegralElementType(aType))
        {
            // Signed -> Unsigned and Unsigned -> Signed conversions are not permitted.
            if (IsSignedIntegralElementType(pType) != IsSignedIntegralElementType(aType))
                return ConversionRank::NoMatch;

            // Narrowing conversions are not permitted.
            if (pType < aType)
                return ConversionRank::NoMatch;

            unsigned const rawConversionDistance(static_cast<unsigned>(pType) - static_cast<unsigned>(aType));
            Assert([&]{ return rawConversionDistance % 2 == 0; });
            unsigned const conversionDistance(rawConversionDistance / 2);

            return ConversionRank::IntegralPromotion | static_cast<ConversionRank>(conversionDistance);
        }

        // Real -> Integral conversions are not permitted.
        if (IsIntegralElementType(pType))
            return ConversionRank::NoMatch;

        // Integral -> Real conversion is permitted.
        if (IsIntegralElementType(aType))
            return ConversionRank::RealConversion;

        Assert([&]{ return IsRealElementType(pType) && IsRealElementType(aType); });

        // R8 -> R4 narrowing is not permitted.
        if (pType == Metadata::ElementType::R4 && aType == Metadata::ElementType::R8)
            return ConversionRank::NoMatch;

        // R4 -> R8 widening is permitted.
        return ConversionRank::RealConversion;
    }





    WindowsRuntime::UniqueInspectable CreateInspectableInstance(Type                const  type,
                                                                VariantArgumentPack const& arguments)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        Type const factoryType(Private::GetActivationFactoryType(type));
        Guid const factoryGuid(WindowsRuntime::GetGuid(factoryType));

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

        auto const methodFilter([&](Method const& m) { return m.GetName() == L"CreateInstance"; });

        ConvertingOverloadResolver const overloadResolver(
            Detail::Filter(factoryType.BeginMethods(activatorBinding), factoryType.EndMethods(), methodFilter),
            Detail::Filter(factoryType.EndMethods(), methodFilter),
            arguments);

        if (!overloadResolver.Succeeded())
            throw RuntimeError(L"Failed to find activation method matching provided arguments");

        // Yay, we found an overload!  Now we need to find the interface it comes from.  TODO If we
        // were smart, we'd map the runtime type method back to the interface internally, then this
        // check here could be really fast.  Someday we should do that.

        Method const interfaceMethod(Private::FindMatchingInterfaceMethod(overloadResolver.GetResult()));
        if (!interfaceMethod.IsInitialized())
            throw RuntimeError(L"Failed to determine interface from runtime type method");

        // Find the method index (TODO We should add this as a method on Method)
        Type const declaringType(interfaceMethod.GetDeclaringType());
        SizeType slotIndex(0);
        for (auto it(declaringType.BeginMethods(activatorBinding)); it != declaringType.EndMethods(); ++it)
        {
            if (*it == interfaceMethod)
                break;

            ++slotIndex;
        }

        Microsoft::WRL::ComPtr<IInspectable> newInstance;
        HResult result(Private::CallInvoker::Invoke(
            interfaceMethod,
            slotIndex,
            factory.Get(),
            reinterpret_cast<void**>(newInstance.ReleaseAndGetAddressOf()),
            arguments));

        VerifySuccess(result);

        return WindowsRuntime::UniqueInspectable(newInstance.Detach());
    }

} }

#endif
