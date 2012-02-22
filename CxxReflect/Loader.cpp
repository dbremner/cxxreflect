//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    DirectoryBasedAssemblyLocator::DirectoryBasedAssemblyLocator(DirectorySet const& directories)
        : _directories(std::move(directories))
    {
    }

    String DirectoryBasedAssemblyLocator::LocateAssembly(AssemblyName const& name) const
    {
        using std::begin;
        using std::end;

        wchar_t const* const extensions[] = { L".dll", L".exe" };
        for (auto dir_it(begin(_directories)); dir_it != end(_directories); ++dir_it)
        {
            for (auto ext_it(begin(extensions)); ext_it != end(extensions); ++ext_it)
            {
                std::wstring path(*dir_it + L"/" + name.GetName() + *ext_it);
                if (Externals::FileExists(path.c_str()))
                {
                    return path;
                }
            }
        }

        return L"";
    }

    String DirectoryBasedAssemblyLocator::LocateAssembly(AssemblyName const& name, String const&) const
    {
        // The directory-based resolver does not utilize namespace-based resolution, so we can
        // defer directly to the assembly-based resolution function.
        return LocateAssembly(name);
    }

    Loader::Loader(std::unique_ptr<IAssemblyLocator> assemblyLocator)
        : _assemblyLocator(std::move(assemblyLocator))
    {
        Detail::AssertNotNull(_assemblyLocator.get());
    }

    IAssemblyLocator const& Loader::GetAssemblyLocator(InternalKey) const
    {
        return *_assemblyLocator.get();
    }

    Detail::AssemblyContext const& Loader::GetContextForDatabase(Metadata::Database const& database, InternalKey) const
    {
        typedef std::pair<String const, Detail::AssemblyContext> ValueType;
        auto const it(std::find_if(begin(_contexts), end(_contexts), [&](ValueType const& a)
        {
            return a.second.GetDatabase() == database;
        }));

        Detail::Assert([&]{ return it != _contexts.end(); }, L"The database is not owned by this loader");

        return it->second;
    }

    Metadata::FullReference Loader::ResolveType(Metadata::FullReference const& type) const
    {
        using namespace CxxReflect::Metadata;

        // A TypeDef or TypeSpec is already resolved:
        if (type.AsRowReference().GetTable() == TableId::TypeDef ||
            type.AsRowReference().GetTable() == TableId::TypeSpec)
            return type;

        Detail::Assert([&]{ return type.AsRowReference().GetTable() == TableId::TypeRef; });

        // Ok, we have a TypeRef;
        Database const& referenceDatabase(type.GetDatabase());
        SizeType const typeRefIndex(type.AsRowReference().GetIndex());
        TypeRefRow const typeRef(referenceDatabase.GetRow<TableId::TypeRef>(typeRefIndex));

        RowReference const resolutionScope(typeRef.GetResolutionScope());

        // If the resolution scope is null, we look in the ExportedType table for this type.
        if (!resolutionScope.IsValid())
        {
            Detail::AssertFail(L"NYI");
            return Metadata::FullReference();
        }

        switch (resolutionScope.GetTable())
        {
        case TableId::Module:
        {
            // A Module resolution scope means the target type is defined in the current module:
            Assembly const definingAssembly(
                &GetContextForDatabase(type.GetDatabase(), InternalKey()),
                InternalKey());

            Type resolvedType(definingAssembly.GetType(typeRef.GetNamespace(), typeRef.GetName()));
            if (!resolvedType.IsInitialized())
                throw RuntimeError(L"Failed to resolve type in module");

            return Metadata::FullReference(
                &definingAssembly.GetContext(InternalKey()).GetDatabase(),
                RowReference::FromToken(resolvedType.GetMetadataToken()));
        }
        case TableId::ModuleRef:
        {
            throw std::logic_error("NYI");
        }
        case TableId::AssemblyRef:
        {
            AssemblyName const definingAssemblyName(
                Assembly(&GetContextForDatabase(referenceDatabase, InternalKey()), InternalKey()),
                resolutionScope,
                InternalKey());

            Assembly const definingAssembly(LoadAssembly(definingAssemblyName));
            if (!definingAssembly.IsInitialized())
                throw RuntimeError(L"Failed to resolve assembly reference");

            Type const resolvedType(definingAssembly.GetType(typeRef.GetNamespace(), typeRef.GetName()));
            if (!resolvedType.IsInitialized())
                throw RuntimeError(L"Failed to resolve type in assembly");

            return Metadata::FullReference(
                &definingAssembly.GetContext(InternalKey()).GetDatabase(),
                RowReference::FromToken(resolvedType.GetMetadataToken()));
        }
        case TableId::TypeRef:
        {
            throw LogicError(L"NYI");
        }
        default:
        {
            // The resolution scope must be from one of the tables in the switch; if we get here,
            // something is broken in the MetadataDatabase code.
            Detail::AssertFail(L"This is unreachable");
            return Metadata::FullReference();
        }
        }
    }

    Assembly Loader::LoadAssembly(String path) const
    {
        auto it(_contexts.find(path));
        if (it == end(_contexts))
        {
            it = _contexts.insert(std::make_pair(
                Externals::ComputeCanonicalUri(path.c_str()),
                std::move(Detail::AssemblyContext(this, path, std::move(Metadata::Database(path.c_str())))))).first;
        }

        return Assembly(&it->second, InternalKey());
    }

    Assembly Loader::LoadAssembly(AssemblyName const& name) const
    {
        return LoadAssembly(_assemblyLocator->LocateAssembly(name));
    }

}
