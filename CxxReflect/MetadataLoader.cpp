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
        MetadataLoader const& loader(reflectedType.GetAssembly().GetLoader(InternalKey()));

        Type const declaringType(
            Assembly(&loader, _database, InternalKey()),
            Metadata::TableReference(Metadata::TableId::TypeDef, _declaringType));

        return Method(
            declaringType,
            reflectedType,
            Metadata::TableReference(Metadata::TableId::MethodDef, _method));
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



    AssemblyName const& MetadataLoader::GetAssemblyName(Metadata::Database const& database, InternalKey) const
    {
        auto const it(std::find_if(begin(_contexts), end(_contexts), [&](std::pair<String const, AssemblyContext> const& a)
        {
            return &a.second.GetDatabase() == &database;
        }));

        if (it == end(_contexts))
            throw std::logic_error("wtf");

        return it->second.GetAssemblyName();
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
            Detail::VerifyFail("wtf");
            return DatabaseReference(); // TODO WE SHOULD THROW HERE
        }

        switch (resolutionScope.GetTable())
        {
        case TableId::Module:
        {
            Detail::VerifyFail("wtf");
            break;
        }

        case TableId::ModuleRef:
        {
            Detail::VerifyFail("wtf");
            break;
        }
        case TableId::AssemblyRef:
        {
            AssemblyName const definingAssemblyName(
                Assembly(this, &referenceDatabase, InternalKey()),
                resolutionScope);

            Assembly const definingAssembly(LoadAssembly(definingAssemblyName));
            if (definingAssembly == nullptr)
                throw std::logic_error("wtf");

            Type const resolvedType(definingAssembly.GetType(typeRef.GetNamespace(), typeRef.GetName()));
            if (resolvedType == nullptr)
                throw std::logic_error("wtf");

            return DatabaseReference(
                &definingAssembly.GetDatabase(InternalKey()), 
                TableReference::FromToken(resolvedType.GetMetadataToken()));
        }

        case TableId::TypeRef:
        {
            Detail::VerifyFail("wtf");
            break;
        }

        default:
        {
            Detail::VerifyFail("wtf");
            break;
        }
        }

        return DatabaseReference(); // TODO THIS IS UNREACHABLE
    }

    Assembly MetadataLoader::LoadAssembly(String path) const
    {
        // TODO PATH NORMALIZATION?
        auto it(_contexts.find(path));
        if (it == end(_contexts))
        {
            it = _contexts.insert(std::make_pair(
                path,
                std::move(AssemblyContext(this, std::move(path), std::move(Metadata::Database(path.c_str())))))).first;
        }

        return Assembly(this, &it->second.GetDatabase(), InternalKey());
    }

    Assembly MetadataLoader::LoadAssembly(AssemblyName const& name) const
    {
        return LoadAssembly(_resolver->ResolveAssembly(name, String()));
    }

    AssemblyName const& MetadataLoader::AssemblyContext::GetAssemblyName() const
    {
        RealizeName();
        return _name;
    }

    void MetadataLoader::AssemblyContext::RealizeName() const
    {
        if (_state.IsSet(RealizedName)) { return; }

        _name = AssemblyName(
            Assembly(_loader.Get(), &_database, InternalKey()),
            Metadata::TableReference(Metadata::TableId::Assembly, 0));

        _state.Set(RealizedName);
    }

}
