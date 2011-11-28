//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace Detail {

    Method MethodReference::Resolve(Type const& reflectedType) const
    {
        Type const declaringType(
            Assembly(&reflectedType.GetAssembly().GetContext(InternalKey()), InternalKey()),
            _typeDef.GetTableReference(),
            InternalKey());

        return Method(declaringType, reflectedType, _methodDef);
    }

    AssemblyName const& AssemblyContext::GetAssemblyName() const
    {
        RealizeName();
        return _name;
    }

    void AssemblyContext::RealizeName() const
    {
        if (_state.IsSet(RealizedName)) { return; }

        _name = AssemblyName(
            Assembly(this, InternalKey()),
            Metadata::TableReference(Metadata::TableId::Assembly, 0),
            InternalKey());

        _state.Set(RealizedName);
    }

} }

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

    Detail::AssemblyContext const& MetadataLoader::GetContextForDatabase(Metadata::Database const& database, InternalKey) const
    {
        typedef std::pair<String const, Detail::AssemblyContext> ValueType;
        auto const it(std::find_if(begin(_contexts), end(_contexts), [&](ValueType const& a)
        {
            return a.second.GetDatabase() == database;
        }));

        // TODO Convert this back to the ADL 'end' once we get a compiler fix
        Detail::Verify([&]{ return it != _contexts.end(); }, "The database is not owned by this loader");

        return it->second;
    }

    Metadata::DatabaseReference MetadataLoader::ResolveType(Metadata::DatabaseReference const& typeReference, InternalKey) const
    {
        using namespace CxxReflect::Metadata;

        // A TypeDef or TypeSpec is already resolved:
        if (typeReference.GetTableReference().GetTable() == TableId::TypeDef ||
            typeReference.GetTableReference().GetTable() == TableId::TypeSpec)
            return typeReference;

        Detail::Verify([&]{ return typeReference.GetTableReference().GetTable() == TableId::TypeRef; });

        // Ok, we have a TypeRef;
        Database const& referenceDatabase(typeReference.GetDatabase());
        SizeType const typeRefIndex(typeReference.GetTableReference().GetIndex());
        TypeRefRow const typeRef(referenceDatabase.GetRow<TableId::TypeRef>(typeRefIndex));

        TableReference const resolutionScope(typeRef.GetResolutionScope());

        // If the resolution scope is null, we look in the ExportedType table for this type.
        if (!resolutionScope.IsValid())
        {
            Detail::VerifyFail("Not Yet Implemented"); // TODO
            return DatabaseReference();
        }

        switch (resolutionScope.GetTable())
        {
        case TableId::Module:
        {
            // A Module resolution scope means the target type is defined in the current module:
            Assembly const definingAssembly(
                &GetContextForDatabase(typeReference.GetDatabase(), InternalKey()),
                InternalKey());

            Type resolvedType(definingAssembly.GetType(typeRef.GetNamespace(), typeRef.GetName()));
            if (!resolvedType)
                throw RuntimeError("Failed to resolve type in module");

            return DatabaseReference(
                &definingAssembly.GetContext(InternalKey()).GetDatabase(),
                TableReference::FromToken(resolvedType.GetMetadataToken()));
        }
        case TableId::ModuleRef:
        {
            throw std::logic_error("Not Yet Implemented");
        }
        case TableId::AssemblyRef:
        {
            AssemblyName const definingAssemblyName(
                Assembly(&GetContextForDatabase(referenceDatabase, InternalKey()), InternalKey()),
                resolutionScope,
                InternalKey());

            Assembly const definingAssembly(LoadAssembly(definingAssemblyName));
            if (definingAssembly == nullptr)
                throw RuntimeError("Failed to resolve assembly reference");

            Type const resolvedType(definingAssembly.GetType(typeRef.GetNamespace(), typeRef.GetName()));
            if (resolvedType == nullptr)
                throw RuntimeError("Failed to resolve type in assembly");

            return DatabaseReference(
                &definingAssembly.GetContext(InternalKey()).GetDatabase(),
                TableReference::FromToken(resolvedType.GetMetadataToken()));
        }
        case TableId::TypeRef:
        {
            throw std::logic_error("Not Yet Implemented");
        }
        default:
        {
            // The resolution scope must be from one of the tables in the switch; if we get here,
            // something is broken in the MetadataDatabase code.
            Detail::VerifyFail("This is unreachable");
            return DatabaseReference();
        }
        }
    }

    Assembly MetadataLoader::LoadAssembly(String path) const
    {
        // TODO PATH NORMALIZATION?
        auto it(_contexts.find(path));
        if (it == end(_contexts))
        {
            it = _contexts.insert(std::make_pair(
                path,
                std::move(Detail::AssemblyContext(this, path, std::move(Metadata::Database(path.c_str())))))).first;
        }

        return Assembly(&it->second, InternalKey());
    }

    Assembly MetadataLoader::LoadAssembly(AssemblyName const& name) const
    {
        return LoadAssembly(_resolver->ResolveAssembly(name));
    }

}
