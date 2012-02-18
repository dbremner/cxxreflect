//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_ASSEMBLY_HPP_
#define CXXREFLECT_ASSEMBLY_HPP_

#include "CxxReflect/CoreInternals.hpp"

namespace CxxReflect {

    class Assembly
    {
    public:

        Assembly();

        AssemblyName const& GetName() const;
        String       const& GetPath() const;

        typedef Detail::InstantiatingIterator<Metadata::RowReference, File,         Assembly> FileIterator;
        typedef Detail::InstantiatingIterator<Metadata::RowReference, Module,       Assembly> ModuleIterator;
        typedef Detail::InstantiatingIterator<Metadata::RowReference, Type,         Assembly> TypeIterator;
        typedef Detail::InstantiatingIterator<Metadata::RowReference, AssemblyName, Assembly> AssemblyNameIterator;

        SizeType             GetReferencedAssemblyCount()   const;
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

        Type GetType(StringReference namespaceQualifiedTypeName, bool caseInsensitive = false) const;

        // The namespace and name of a type are stored separately; this function is far more
        // efficient than the GetType() that takes a namespace-qualified type name.
        Type GetType(StringReference namespaceName,
                     StringReference unqualifiedTypeName,
                     bool            caseInsensitive = false) const;

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
        // ReflectionOnlyLoad     Use Loader::LoadAssembly()
        // ReflectionOnlyLoadFrom Use Loader::LoadAssembly()
        // UnsafeLoadFrom         Not applicable outside of runtime

        bool IsInitialized() const;
        bool operator!()     const;

        friend bool operator==(Assembly const& lhs, Assembly const& rhs);
        friend bool operator< (Assembly const& lhs, Assembly const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Assembly)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Assembly)

    public: // Internal Members

        Assembly(Detail::AssemblyContext const* context, InternalKey);

        Detail::AssemblyContext const& GetContext(InternalKey) const;

    private:

        void AssertInitialized() const;

        Metadata::AssemblyRow GetAssemblyRow() const;

        Detail::ValueInitialized<Detail::AssemblyContext const*> _context;
    };

}

#endif
