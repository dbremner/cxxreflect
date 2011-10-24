//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataLoader.hpp"

namespace CxxReflect {

    IMetadataResolver::~IMetadataResolver()
    {
    }

    DirectoryBasedMetadataResolver::DirectoryBasedMetadataResolver(DirectorySet const& directories)
        : _directories(std::move(directories))
    {
    }

    String DirectoryBasedMetadataResolver::ResolveAssembly(AssemblyName const& name) const
    {
        using std::begin;
        using std::end;

        wchar_t const* const extensions[] = { L".dll", L".exe" };
        for (auto dir_it(begin(_directories)); dir_it != end(_directories); ++dir_it)
        {
            for (auto ext_it(begin(extensions)); ext_it != end(extensions); ++ext_it)
            {
                std::wstring path(*dir_it + L"/" + name.GetName() + *ext_it);
                if (Detail::FileExists(path.c_str()))
                {
                    return path;
                }
            }
        }

        return L"";
    }

    String DirectoryBasedMetadataResolver::ResolveAssembly(AssemblyName const& name, String const&) const
    {
        // The directory-based resolver does not utilize namespace-based resolution, so we can
        // defer directly to the assembly-based resolution function.
        return ResolveAssembly(name);
    }

    MetadataLoader::MetadataLoader(std::unique_ptr<IMetadataResolver> resolver)
        : _resolver(std::move(resolver))
    {
        Detail::VerifyNotNull(_resolver.get());
    }

    Assembly MetadataLoader::LoadAssembly(String const& path) const
    {
        // TODO PATH NORMALIZATION?
        auto it(_databases.find(path));
        if (it == _databases.end())
            it = _databases.insert(std::make_pair(path, Metadata::Database(path.c_str()))).first;

        return Assembly(it->first.c_str(), this, &it->second);
    }

}
