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
        {
        }

        Assembly(StringReference                            const path,
                 Detail::NonNull<MetadataLoader     const*> const loader,
                 Detail::NonNull<Metadata::Database const*> const database)
            : _path(path.c_str()), _loader(loader), _database(database)
        {
            Detail::Verify([&] { return !path.empty(); });
        }

        bool IsInitialized() const
        {
            return _loader != nullptr && _database != nullptr;
        }

        AssemblyName  GetName() const;
        String const& GetPath() const;

        typedef Detail::TableTransformIterator<Metadata::TableReference, File,   Assembly>       FileIterator;
        typedef Detail::TableTransformIterator<Metadata::TableReference, Module, Assembly>       ModuleIterator;
        typedef Detail::TableTransformIterator<Metadata::TableReference, Type,   Assembly>       TypeIterator;
        typedef Detail::TableTransformIterator<Metadata::TableReference, AssemblyName, Assembly> AssemblyNameIterator;

        AssemblyNameIterator BeginReferencedAssemblyNames() const;
        AssemblyNameIterator EndReferencedAssemblyNames()   const;

        FileIterator BeginFiles() const;
        FileIterator EndFiles()   const;

        File GetFile(StringReference name) const;

        ModuleIterator BeginModules() const;
        ModuleIterator EndModules()   const;

        Module GetModule(StringReference name) const;

        TypeIterator BeginTypes() const;
        TypeIterator EndTypes()   const;

        Type GetType(StringReference name, bool ignoreCase = false) const;


        Metadata::Database const& GetDatabase() const { VerifyInitialized(); return *_database; }

        // EntryPoint
        // ImageRuntimeVersion
        // ManifestModule

        // [static] CreateQualifiedName
        // GetCustomAttributes
        // GetExportedTypes
        // GetManifestResourceInfo
        // GetManifestResourceNames
        // GetManifestResourceStream
        // GetReferencedAssemblies
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

        String                                     _path;
        Detail::NonNull<MetadataLoader     const*> _loader;
        Detail::NonNull<Metadata::Database const*> _database;
    };

}

#endif
