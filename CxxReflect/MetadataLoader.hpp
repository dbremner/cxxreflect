//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATALOADER_HPP_
#define CXXREFLECT_METADATALOADER_HPP_

#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataDatabase.hpp"

#include <set>
#include <utility>

namespace CxxReflect {

    class IMetadataResolver
    {
    public:

        virtual ~IMetadataResolver();

        virtual std::wstring ResolveAssembly(AssemblyName const& assemblyName) const = 0;
        virtual std::wstring ResolveAssembly(AssemblyName const& assemblyName,
                                             String       const& typeFullName) const = 0;
    };

    #ifdef CXXREFLECT_ENABLE_WINRT_RESOLVER

    class WinRTMetadataResolver : public IMetadataResolver
    {
    public:

        WinRTMetadataResolver();
        ~WinRTMetadataResolver();

        std::wstring ResolveAssembly(AssemblyName const& assemblyName) const;
        std::wstring ResolveAssembly(AssemblyName const& assemblyName,
                                     String       const& typeFullName) const;

    private:

        // TODO Implement
    };

    #endif

    class DirectoryBasedMetadataResolver : public IMetadataResolver
    {
    public:

        typedef std::set<String> DirectorySet;

        DirectoryBasedMetadataResolver(DirectorySet const& directories);

        String ResolveAssembly(AssemblyName const& name) const;
        String ResolveAssembly(AssemblyName const& assemblyName, String const& typeFullName) const;

    private:

        DirectorySet _directories;
    };

    class MetadataLoader
    {
    public:

        MetadataLoader(std::unique_ptr<IMetadataResolver> resolver);

        Assembly LoadAssembly(String const& filePath) const;

    private:

        MetadataLoader(MetadataLoader const&);
        MetadataLoader& operator=(MetadataLoader const&);

        std::unique_ptr<IMetadataResolver>   _resolver;
        mutable std::map<String, Metadata::Database> _databases;
    };

}

#endif
