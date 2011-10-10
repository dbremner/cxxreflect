//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_ASSEMBLY_HPP_
#define CXXREFLECT_ASSEMBLY_HPP_

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/CoreDeclarations.hpp"
#include "CxxReflect/Utility.hpp"
#include "CxxReflect/Type.hpp"

#include <memory>

namespace CxxReflect {

    class Assembly
    {
    public:

        Assembly(MetadataReader const* reader, String const& path, IMetaDataImport2* import)
            : _reader(reader), _path(path), _import(import)
        {
            Detail::VerifyNotNull(reader);
            Detail::VerifyNotNull(import);
        }

        MetadataReader const* GetMetadataReader() const { return _reader; }
        String const&         GetPath()           const { return _path;   }
        IMetaDataImport2*     UnsafeGetImport()   const { return _import; }

        AssemblyName const& GetName() const { return PrivateGetName(); }

        AssemblyNameIterator BeginReferencedAssemblies() const { return PrivateGetReferencedAssemblies().begin(); }
        AssemblyNameIterator EndReferencedAssemblies()   const { return PrivateGetReferencedAssemblies().end();   }

        Type const* BeginTypes() const { return PrivateGetTypes().Get(); }
        Type const* EndTypes()   const { return PrivateGetTypes().Get() + PrivateGetTypes().GetSize();   }

        // Resolves a type from this assembly by name, optionally ignoring case.  If no type with
        // the specified name is found, a null pointer is returned.
        Type const* GetType(String const& name, bool ignoreCase) const;

        // Resolves a type from this assembly by its metadata token.  If no type matching the token
        // is found, a null pointer is returned.
        Type const* GetType(MetadataToken typeDef) const;

        // EntryPoint
        // ImageRuntimeVersion
        // ManifestModule

        // GetCustomAttributes
        // GetExportedTypes
        // GetFile
        // GetFiles
        // GetManifestResourceInfo
        // GetManifestResourceNames
        // GetManifestResourceStream
        // GetModule
        // GetModules
        // GetSatelliteAssembly
        // GetType
        // IsDefined

        // -- The following members of System.Reflection.Assembly are not implemented --
        // CodeBase               Replaced by GetPath()
        // EscapedCodeBase        Replaced by GetPath()
        // Evidence               N/A
        // FullName               Replaced by GetName().GetFullName()
        // GlobalAssemblyCache    Check GetPath() vs. actual GAC directory
        // HostContext            N/A
        // IsDynamic              Would always be false
        // IsFullyTrusted         N/A
        // Location               Replaced by GetPath()
        // PermissionSet          N/A
        // ReflectionOnly         Would always be true
        // SecurityRuleSet        N/A
        //
        // CreateInstance         N/A - Execution required
        // GetLoadedModules       N/A

        bool IsSystemAssembly() const
        {
            return PrivateGetReferencedAssemblies().empty();
        }

    private:

        CXXREFLECT_MAKE_NONCOPYABLE(Assembly);

        enum RealizationState
        {
            RealizedName                 = 0x0001,
            RealizedReferencedAssemblies = 0x0002,
            RealizedTypes                = 0x0004
        };

        void RealizeName()                 const;
        void RealizeReferencedAssemblies() const;
        void RealizeTypes()                const;

        AssemblyName& PrivateGetName() const
        {
            RealizeName();
            return _name;
        }

        AssemblyNameSequence& PrivateGetReferencedAssemblies() const
        {
            RealizeReferencedAssemblies();
            return _referencedAssemblies;
        }

        Detail::AllocatorBasedArray<Type>& PrivateGetTypes() const
        {
            RealizeTypes();
            return _types;
        }

        // These are set in the constructor, never change, and are guaranteed to be valid
        MetadataReader const* _reader;
        String                _path;
        IMetaDataImport2*     _import;

        mutable Detail::FlagSet<RealizationState> _state;

        mutable AssemblyName         _name;
        mutable AssemblyNameSequence _referencedAssemblies;

        mutable Detail::AllocatorBasedArray<Type> _types;
    };

    bool operator==(Assembly const&, Assembly const&); // TODO
    bool operator< (Assembly const&, Assembly const&); // TODO

    inline bool operator!=(Assembly const& lhs, Assembly const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Assembly const& lhs, Assembly const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Assembly const& lhs, Assembly const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Assembly const& lhs, Assembly const& rhs) { return !(rhs <  lhs); }

}

#endif
