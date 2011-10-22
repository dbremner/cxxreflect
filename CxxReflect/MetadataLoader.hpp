//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATALOADER_HPP_
#define CXXREFLECT_METADATALOADER_HPP_

#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/Platform.hpp"

#include <set>
#include <utility>

namespace CxxReflect {

    class IMetadataResolver
    {
    public:

        virtual ~IMetadataResolver();

        virtual std::wstring ResolveAssembly(AssemblyName const& assemblyName) const = 0;
        virtual std::wstring ResolveType(AssemblyName const& assemblyName, std::wstring const& typeFullName) const = 0;
    };

    #ifdef CXXREFLECT_ENABLE_WINRT_RESOLVER

    class WinRTMetadataResolver : public IMetadataResolver
    {
    public:

        WinRTMetadataResolver();
        ~WinRTMetadataResolver();

        std::wstring ResolveAssembly(AssemblyName const& assemblyName) const;
        std::wstring ResolveAssembly(AssemblyName const& assemblyName, std::wstring const& typeFullName) const;

    private:

        // TODO Implement
    };

    #endif

    class DirectoryBasedMetadataResolver : public IMetadataResolver
    {
    public:

        typedef std::set<std::wstring> DirectorySet;

        DirectoryBasedMetadataResolver(DirectorySet directories)
            : _directories(std::move(directories))
        {
        }

        std::wstring ResolveAssembly(AssemblyName const& assemblyName) const
        {
            using std::begin;
            using std::end;

            wchar_t const* extensions[] = { L".dll", L".exe" };
            for (auto dir_it(begin(_directories)); dir_it != end(_directories); ++dir_it)
            {
                for (auto ext_it(begin(extensions)); ext_it != end(extensions); ++ext_it)
                {
                    std::wstring path(*dir_it + L"/" + /*assemblyName.GetName() + */*ext_it);
                    if (Platform::FileExists(path.c_str()))
                    {
                        return path;
                    }
                }
            }

            return L"";
        }

        std::wstring ResolveAssembly(AssemblyName const& assemblyName, std::wstring const&) const
        {
            return ResolveAssembly(assemblyName);
        }

    private:

        DirectorySet _directories;
    };

    class MetadataLoader
    {
    public:

        MetadataLoader(std::unique_ptr<IMetadataResolver> resolver);

        // Assembly LoadAssembly(std::wstring const& filePath);

    private:

        MetadataLoader(MetadataLoader const&);
        MetadataLoader& operator=(MetadataLoader const&);

        std::unique_ptr<IMetadataResolver>         _resolver;
        std::map<std::wstring, Metadata::Database> _databases;
    };

}

#endif
