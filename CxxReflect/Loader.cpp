//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
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

    #pragma warning(push)
    #pragma warning(disable: 4355) // Disables "don't use 'this' in initializer list" warning; I know what I'm doing.
    Loader::Loader(std::auto_ptr<IAssemblyLocator> assemblyLocator)
        : _assemblyLocator(assemblyLocator.release()),
          _events         (this, &_signatureAllocator),
          _fields         (this, &_signatureAllocator),
          _interfaces     (this, &_signatureAllocator),
          _methods        (this, &_signatureAllocator),
          _properties     (this, &_signatureAllocator)
    {
        Detail::AssertNotNull(_assemblyLocator.get());
    }

    Loader::Loader(std::unique_ptr<IAssemblyLocator> assemblyLocator)
        : _assemblyLocator(std::move(assemblyLocator)),
          _events         (this, &_signatureAllocator),
          _fields         (this, &_signatureAllocator),
          _interfaces     (this, &_signatureAllocator),
          _methods        (this, &_signatureAllocator),
          _properties     (this, &_signatureAllocator)
    {
        Detail::AssertNotNull(_assemblyLocator.get());
    }


    #pragma warning(pop)

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

            Type const resolvedType(definingAssembly.GetType(typeRef.GetNamespace(), typeRef.GetName()));
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

            // TODO Add a LocateAssembly overload that takes a namespace and simple type name.
            String const& path(_assemblyLocator->LocateAssembly(
                definingAssemblyName,
                String(typeRef.GetNamespace().c_str()) + L"." + typeRef.GetName().c_str()));

            Assembly const definingAssembly(LoadAssembly(path));
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
        String canonicalUri(Externals::ComputeCanonicalUri(path.c_str()));
        auto it(_contexts.find(canonicalUri));
        if (it == end(_contexts))
        {
            it = _contexts.insert(std::make_pair(
                canonicalUri,
                Detail::AssemblyContext(this, path, Metadata::Database(path.c_str())))).first;
        }

        return Assembly(&it->second, InternalKey());
    }

    Assembly Loader::LoadAssembly(AssemblyName const& name) const
    {
        return LoadAssembly(_assemblyLocator->LocateAssembly(name));
    }

    Type Loader::GetFundamentalType(Metadata::ElementType const elementType, InternalKey) const
    {
        Detail::Assert([&]{ return elementType < Metadata::ElementType::ConcreteElementTypeMax; });

        if (_fundamentalTypes[Detail::AsInteger(elementType)].IsInitialized())
            return _fundamentalTypes[Detail::AsInteger(elementType)].Realize();

        StringReference primitiveTypeName;
        switch (elementType)
        {
        case Metadata::ElementType::Boolean:    primitiveTypeName = L"Boolean";        break;
        case Metadata::ElementType::Char:       primitiveTypeName = L"Char";           break;
        case Metadata::ElementType::I1:         primitiveTypeName = L"SByte";          break;
        case Metadata::ElementType::U1:         primitiveTypeName = L"Byte";           break;
        case Metadata::ElementType::I2:         primitiveTypeName = L"Int16";          break;
        case Metadata::ElementType::U2:         primitiveTypeName = L"UInt16";         break;
        case Metadata::ElementType::I4:         primitiveTypeName = L"Int32";          break;
        case Metadata::ElementType::U4:         primitiveTypeName = L"UInt32";         break;
        case Metadata::ElementType::I8:         primitiveTypeName = L"Int64";          break;
        case Metadata::ElementType::U8:         primitiveTypeName = L"UInt64";         break;
        case Metadata::ElementType::R4:         primitiveTypeName = L"Single";         break;
        case Metadata::ElementType::R8:         primitiveTypeName = L"Double";         break;
        case Metadata::ElementType::I:          primitiveTypeName = L"IntPtr";         break;
        case Metadata::ElementType::U:          primitiveTypeName = L"UIntPtr";        break;
        case Metadata::ElementType::Object:     primitiveTypeName = L"Object";         break;
        case Metadata::ElementType::String:     primitiveTypeName = L"String";         break;
        case Metadata::ElementType::ValueType:  primitiveTypeName = L"ValueType";      break;
        case Metadata::ElementType::Void:       primitiveTypeName = L"Void";           break;
        case Metadata::ElementType::TypedByRef: primitiveTypeName = L"TypedReference"; break;
        default:
            Detail::AssertFail(L"Unknown primitive type");
            break;
        }

        Detail::Assert([&]{ return !_contexts.empty(); });

        Assembly const referenceAssembly(&_contexts.begin()->second, InternalKey());
        Assembly const systemAssembly(Utility::GetSystemAssembly(referenceAssembly));
        Detail::Assert([&]{ return systemAssembly.IsInitialized(); });

        Type const primitiveType(systemAssembly.GetType(L"System", primitiveTypeName));
        Detail::Assert([&]{ return primitiveType.IsInitialized(); });

        _fundamentalTypes[Detail::AsInteger(elementType)] = Detail::TypeHandle(primitiveType);
        return primitiveType;
    }

    Detail::EventContextTable Loader::GetOrCreateEventTable(Metadata::FullReference const& typeDef, InternalKey) const
    {
        return _events.GetOrCreateTable(typeDef);
    }

    Detail::FieldContextTable Loader::GetOrCreateFieldTable(Metadata::FullReference const& typeDef, InternalKey) const
    {
        return _fields.GetOrCreateTable(typeDef);
    }

    Detail::InterfaceContextTable Loader::GetOrCreateInterfaceTable(Metadata::FullReference const& typeDef, InternalKey) const
    {
        return _interfaces.GetOrCreateTable(typeDef);
    }

    Detail::MethodContextTable Loader::GetOrCreateMethodTable(Metadata::FullReference const& typeDef, InternalKey) const
    {
        return _methods.GetOrCreateTable(typeDef);
    }

    Detail::PropertyContextTable Loader::GetOrCreatePropertyTable(Metadata::FullReference const& typeDef, InternalKey) const
    {
        return _properties.GetOrCreateTable(typeDef);
    }

}
