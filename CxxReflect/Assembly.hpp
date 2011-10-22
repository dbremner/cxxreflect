//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_ASSEMBLY_HPP_
#define CXXREFLECT_ASSEMBLY_HPP_

#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataDatabase.hpp"

namespace CxxReflect {

    class Assembly
    {
    public:

        Assembly()
            : _loader(nullptr), _database(nullptr)
        {
        }

        Assembly(StringReference path, MetadataLoader const* loader, Metadata::Database const* database)
            : _path(path.c_str()), _loader(loader), _database(database)
        {
            Detail::Verify([&] { return !path.empty(); });
            Detail::VerifyNotNull(loader);
            Detail::VerifyNotNull(database);
        }

        bool IsInitialized() const
        {
            return _loader != nullptr && _database != nullptr;
        }

        AssemblyName GetName() const;

        Type GetType() const;

        // EntryPoint
        // ImageRuntimeVersion
        // ManifestModule

        // [static] CreateQualifiedName
        // GetCustomAttributes
        // GetExportedTypes
        // GetFile
        // GetFiles
        // GetManifestResourceInfo
        // GetManifestResourceNames
        // GetManifestResourceStream
        // GetModule
        // GetModules
        // GetReferencedAssemblies
        // GetType
        // GetTypes
        // IsDefined

        // -- These members are not implemented --
        //
        // CodeBase               Use GetName().GetPath()
        // EscapedCodeBase        Use GetName().GetPath()
        // Evidence               Not applicable outside of runtime
        // FullName               Use GetName().GetFullName()
        // GlobalAssemblyCache    TODO Can we implement this?
        // HostContext            Not applicable outside of runtime
        // IsDynamic              Not applicable outside of runtime
        // IsFullyTrusted         Not applicable outside of runtime
        // Location               Use GetName().GetPath()
        // PermissionSet          Not applicable outside of runtime
        // ReflectionOnly         Would always be true
        // SecurityRuleSet        Not applicable outside of runtime
        //
        // CreateInstance         Not applicable outside of runtime
        // GetAssembly            Not applicable outside of runtime
        // GetCallingAssembly     Not applicable outside of runtime
        // GetEntryAssembly       Not applicable outside of runtime
        // GetExecutingAssembly   Not applicable outside of runtime
        // GetLoadedModules       Not applicable outside of runtime
        // GetSatelliteAssembly   TODO Can we implement this?
        // Load                   Not applicable outside of runtime
        // LoadFile               Not applicable outside of runtime
        // LoadModule             Not applicable outside of runtime
        // LoadWithPartialName    Not applicable outside of runtime
        // ReflectionOnlyLoad     Use MetadataLoader::LoadAssembly()
        // ReflectionOnlyLoadFrom Use MetadataLoader::LoadAssembly()
        // UnsafeLoadFrom         Not applicable outside of runtime

        friend bool operator==(Assembly const& lhs, Assembly const& rhs) { return lhs._database == rhs._database; }
        friend bool operator< (Assembly const& lhs, Assembly const& rhs) { return lhs._database <  rhs._database; }

        friend bool operator!=(Assembly const& lhs, Assembly const& rhs) { return !(lhs == rhs); }
        friend bool operator> (Assembly const& lhs, Assembly const& rhs) { return   rhs <  lhs ; }
        friend bool operator<=(Assembly const& lhs, Assembly const& rhs) { return !(rhs <  lhs); }
        friend bool operator>=(Assembly const& lhs, Assembly const& rhs) { return !(lhs <  rhs); }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Assembly not initialized");
        }

        Metadata::AssemblyRow GetAssemblyRow() const;

        String                    _path;
        MetadataLoader     const* _loader;
        Metadata::Database const* _database;
    };

}

#endif
