
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    DirectoryBasedModuleLocator::DirectoryBasedModuleLocator(DirectorySet const& directories)
        : _directories(std::move(directories))
    {
    }

    ModuleLocation DirectoryBasedModuleLocator::LocateAssembly(AssemblyName const& name) const
    {
        using std::begin;
        using std::end;

        wchar_t const* const extensions[] = { L".dll", L".exe" };
        for (auto dir_it(begin(_directories)); dir_it != end(_directories); ++dir_it)
        {
            for (auto ext_it(begin(extensions)); ext_it != end(extensions); ++ext_it)
            {
                std::wstring path(*dir_it + L"\\" + name.GetName() + *ext_it);
                if (Externals::FileExists(path.c_str()))
                {
                    return ModuleLocation(path);
                }
            }
        }

        return ModuleLocation();
    }

    ModuleLocation DirectoryBasedModuleLocator::LocateAssembly(AssemblyName const& name, String const&) const
    {
        // The directory-based resolver does not utilize namespace-based resolution, so we can
        // defer directly to the assembly-based resolution function.
        return LocateAssembly(name);
    }

    ModuleLocation DirectoryBasedModuleLocator::LocateModule(AssemblyName const& requestingAssembly,
                                                             String       const& moduleName) const
    {
        String const& requesting(requestingAssembly.GetPath());
        auto const rIt(std::find(requesting.rbegin(), requesting.rend(), L'\\'));
        if (rIt == requesting.rend())
            return ModuleLocation();

        String path(requesting.begin(), rIt.base());
        path += moduleName;
        return ModuleLocation(path);
    }





    Loader::Loader(Loader&& other)
        : _context(std::move(other._context))
    {
        AssertInitialized();
    }

    Loader& Loader::operator=(Loader&& other)
    {
        _context = std::move(other._context);
        AssertInitialized();
        return *this;
    }

    bool Loader::IsInitialized() const
    {
        return _context != nullptr;
    }

    void Loader::AssertInitialized() const
    {
        Detail::Assert([&]{ return _context != nullptr; });
    }

    ModuleLocator const& Loader::GetLocator() const
    {
        AssertInitialized();
        return _context->GetLocator();
    }

    Detail::LoaderContext const& Loader::GetContext(InternalKey) const
    {
        AssertInitialized();
        return *_context;
    }

    Assembly Loader::LoadAssembly(String const& pathOrUri) const
    {
        AssertInitialized();
        return LoadAssembly(ModuleLocation(pathOrUri));
    }

    Assembly Loader::LoadAssembly(ModuleLocation const& location) const
    {
        AssertInitialized();
        return Assembly(&_context->GetOrLoadAssembly(location), InternalKey());
    }

    Assembly Loader::LoadAssembly(AssemblyName const& name) const
    {
        AssertInitialized();
        return Assembly(&_context->GetOrLoadAssembly(name), InternalKey());
    }

}
