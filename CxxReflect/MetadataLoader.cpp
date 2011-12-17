//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace Detail {

    MethodContext::MethodContext()
    {
    }

    MethodContext::MethodContext(Metadata::FullReference const& typeDef,
                                 Metadata::RowReference  const& methodDef)
        : _typeDef  (typeDef  ),
          _methodDef(methodDef)
    {
        Verify([&]{ return typeDef.IsInitialized();   });
        Verify([&]{ return methodDef.IsInitialized(); });
    }

    MethodContext::MethodContext(Metadata::FullReference const& typeDef,
                                 Metadata::RowReference  const& methodDef,
                                 Metadata::FullReference const& typeSpec,
                                 ByteRange               const  instantiatedSignature)
        : _typeDef              (typeDef              ),
          _methodDef            (methodDef            ),
          _typeSpec             (typeSpec             ),
          _instantiatedSignature(instantiatedSignature)
    {
        Verify([&]{ return typeDef.IsInitialized();   });
        Verify([&]{ return methodDef.IsInitialized(); });
        // TODO Verify([&]{ return typeSpec.IsInitialized();  });
        // Note: 'instantiatedSignature' may be uninitialized.
    }

    Method MethodContext::Resolve(Type const& reflectedType) const
    {
        Type const declaringType(
            Assembly(&reflectedType.GetAssembly().GetContext(InternalKey()), InternalKey()),
            _typeDef.AsRowReference(),
            InternalKey());

        return Method(reflectedType, this, InternalKey());
    }

    Metadata::FullReference MethodContext::GetDeclaringType() const
    {
        VerifyInitialized();
        return _typeDef;
    }
        
    Metadata::FullReference MethodContext::GetMethod() const
    {
        VerifyInitialized();
        return Metadata::FullReference(&_typeDef.GetDatabase(), _methodDef);
    }

    Metadata::MethodDefRow MethodContext::GetMethodDefinition() const
    {
        VerifyInitialized();
        return _typeDef.GetDatabase().GetRow<Metadata::TableId::MethodDef>(_methodDef);
    }

    Metadata::MethodSignature MethodContext::GetMethodSignature() const
    {
        VerifyInitialized();
        if (HasInstantiatedSignature())
        {
            return Metadata::MethodSignature(
                _instantiatedSignature.Begin(),
                _instantiatedSignature.End());
        }
        else
        {
            return _typeDef
                .GetDatabase()
                .GetBlob(GetMethodDefinition().GetSignature())
                .As<Metadata::MethodSignature>();
        }
    }

    bool MethodContext::HasInstantiatedType() const
    {
        return _typeSpec.IsInitialized();
    }

    Metadata::FullReference MethodContext::GetInstantiatedType() const
    {
        Verify([&]{ return HasInstantiatedType(); });
        return _typeSpec;
    }

    bool MethodContext::HasInstantiatedSignature() const
    {
        return _instantiatedSignature.IsInitialized();
    }

    ByteRange MethodContext::GetInstantiatedSignature() const
    {
        Verify([&]{ return HasInstantiatedSignature(); });
        return _instantiatedSignature;
    }

    bool MethodContext::IsInitialized() const
    {
        return _typeDef.IsInitialized();
    }

    void MethodContext::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }




    MethodTableCollection::MethodTableCollection(MetadataLoader const* const loader)
        : _loader(loader)
    {
        VerifyNotNull(loader);
    }

    MethodTableCollection::MethodTableCollection(MethodTableCollection&& other)
        : _loader            (std::move(other._loader            )),
          _signatureAllocator(std::move(other._signatureAllocator)),
          _tableAllocator    (std::move(other._tableAllocator    )),
          _index             (std::move(other._index             )),
          _buffer            (std::move(other._buffer            ))
    {
    }

    MethodTableCollection& MethodTableCollection::operator=(MethodTableCollection&& other)
    {
        Swap(other);
        return *this;
    }

    void MethodTableCollection::Swap(MethodTableCollection& other)
    {
        using std::swap;
        swap(other._loader,             _loader            );
        swap(other._signatureAllocator, _signatureAllocator);
        swap(other._tableAllocator,     _tableAllocator    );
        swap(other._index,              _index             );
        swap(other._buffer,             _buffer            );
    }

    MethodTable MethodTableCollection::GetOrCreateMethodTable(FullReference const& type) const
    {
        auto const it(_index.find(type));
        if (it != end(_index))
            return it->second;

        // When this function returns, we want to clear the buffer so it's ready for our next use.
        // We can't just clear it when the function is called because this function is recursive.
        // Note that we use resize(0) because on Visual C++, clear() will destroy the underlying
        // storage.  We want to keep the buffer intact so we can reuse it.
        ScopeGuard const bufferCleanupGuard([&]{ _buffer.resize(0); });

        TypeDefAndSpec const typeDefAndSpec(ResolveTypeDefAndSpec(type));
        FullReference const& typeDefReference(typeDefAndSpec.first);
        FullReference const& typeSpecReference(typeDefAndSpec.second);

        Metadata::Database const& database(typeDefReference.GetDatabase());

        SizeType const typeDefIndex(typeDefReference.AsRowReference().GetIndex());
        Metadata::TypeDefRow const typeDef(database.GetRow<Metadata::TableId::TypeDef>(typeDefIndex));

        Instantiator const instantiator(CreateInstantiator(typeSpecReference));

        Metadata::RowReference const baseTypeReference(typeDef.GetExtends());
        if (baseTypeReference.IsValid())
        {
            MethodTable const table(GetOrCreateMethodTable(Metadata::FullReference(&database, baseTypeReference)));

            std::transform(table.Begin(), table.End(), std::back_inserter(_buffer), [&](MethodContext const& m)
                -> MethodContext
            {
                if (!instantiator.HasArguments() || !Instantiator::RequiresInstantiation(m.GetMethodSignature()))
                    return m;

                return MethodContext(
                    m.GetDeclaringType(),
                    m.GetMethod().AsRowReference(),
                    m.GetInstantiatedType(),
                    Instantiate(instantiator, m.GetMethodSignature()));
            });
        }

        SizeType const inheritedMethodCount(_buffer.size());

        auto const beginMethod(database.Begin<Metadata::TableId::MethodDef>());
        auto const firstMethod(beginMethod + typeDef.GetFirstMethod().GetIndex());
        auto const lastMethod (beginMethod + typeDef.GetLastMethod().GetIndex());

        std::for_each(firstMethod, lastMethod, [&](Metadata::MethodDefRow const& methodDef)
        {
            Metadata::MethodSignature const methodSig(database
                .GetBlob(methodDef.GetSignature())
                .As<Metadata::MethodSignature>());

            ByteRange const instantiatedSig(instantiator.HasArguments() && Instantiator::RequiresInstantiation(methodSig)
                ? Instantiate(instantiator, methodSig)
                : ByteRange(methodSig.BeginBytes(), methodSig.EndBytes()));

            Metadata::RowReference const methodDefReference(methodDef.GetSelfReference());

            MethodContext const MethodContext(instantiatedSig.IsInitialized()
                ? MethodContext(typeDefReference, methodDefReference, typeSpecReference, instantiatedSig)
                : MethodContext(typeDefReference, methodDefReference));

            InsertMethodIntoBuffer(MethodContext, inheritedMethodCount);
        });

        MethodTable const range(_tableAllocator.Allocate(_buffer.size()));
        RangeCheckedCopy(_buffer.begin(), _buffer.end(), range.Begin(), range.End());

        _index.insert(std::make_pair(type, range));
        return range;
    }

    MethodTableCollection::TypeDefAndSpec
    MethodTableCollection::ResolveTypeDefAndSpec(FullReference const& type) const
    {
        FullReference const resolvedType(_loader.Get()->ResolveType(type, InternalKey()));

        // If we have a TypeDef, there is no TypeSpec, so we can just return the TypeDef directly:
        if (resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef)
            return std::make_pair(resolvedType, FullReference());

        // Otherwise, we have a TypeSpec, and we need to resolve the TypeDef to which it refers:
        Verify([&]{ return resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeSpec; });

        Metadata::TypeSpecRow const typeSpec(resolvedType
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeSpec>(resolvedType));

        Metadata::TypeSignature const typeSignature(resolvedType
            .GetDatabase()
            .GetBlob(typeSpec.GetSignature())
            .As<Metadata::TypeSignature>());

        // We aren't expecting any other kinds of type signatures to be used as base classes:
        Verify([&]{ return typeSignature.GetKind() == Metadata::TypeSignature::Kind::GenericInst; });

        FullReference const reResolvedType(_loader.Get()->ResolveType(
            FullReference(&resolvedType.GetDatabase(), typeSignature.GetGenericTypeReference()),
            InternalKey()));

        // A GenericInst should refer to a TypeDef or a TypeRef, never another TypeSpec.  We resolve
        // the TypeRef above, so at this point we should always have a TypeDef:
        Verify([&]{ return reResolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });

        return std::make_pair(reResolvedType, resolvedType);
    }

    MethodTableCollection::Instantiator MethodTableCollection::CreateInstantiator(FullReference const& type) const
    {
        if (!type.IsInitialized() || type.AsRowReference().GetTable() != Metadata::TableId::TypeSpec)
            return Metadata::ClassVariableSignatureInstantiator();

        Metadata::TypeSignature const signature(type
            .GetDatabase()
            .GetBlob(type.GetDatabase().GetRow<Metadata::TableId::TypeSpec>(type).GetSignature())
            .As<Metadata::TypeSignature>());

        Verify([&]{ return signature.GetKind() == Metadata::TypeSignature::Kind::GenericInst; });

        return Metadata::ClassVariableSignatureInstantiator(
            signature.BeginGenericArguments(),
            signature.EndGenericArguments());
    }

    ByteRange MethodTableCollection::Instantiate(Instantiator              const& instantiator,
                                                 Metadata::MethodSignature const& signature) const
    {
        Verify([&]{ return signature.IsInitialized(); });
        Verify([&]{ return Instantiator::RequiresInstantiation(signature); });

        Metadata::MethodSignature const& instantiation(instantiator.Instantiate(signature));
        SizeType const instantiationSize(std::distance(
            instantiation.BeginBytes(),
            instantiation.EndBytes()));

        MutableByteRange const ownedInstantiation(_signatureAllocator.Allocate(instantiationSize));

        RangeCheckedCopy(
            instantiation.BeginBytes(), instantiation.EndBytes(),
            ownedInstantiation.Begin(), ownedInstantiation.End());

        return ownedInstantiation;
    }

    void MethodTableCollection::InsertMethodIntoBuffer(MethodContext const& newMethod,
                                                       SizeType      const  inheritedMethodCount) const
    {
        Metadata::MethodDefRow    const newMethodDef(newMethod.GetMethodDefinition());
        Metadata::MethodSignature const newMethodSig(newMethod.GetMethodSignature());

        // If the method occupies a new slot, it does not override any other method.  A static
        // method is always a new method.
        if (newMethodDef.GetFlags().WithMask(MethodAttribute::VTableLayoutMask) == MethodAttribute::NewSlot ||
            newMethodDef.GetFlags().IsSet(MethodAttribute::Static))
        {
            _buffer.push_back(newMethod);
            return;
        }

        bool isNewMethod(true);
        auto const bufferBegin(_buffer.rbegin() + (_buffer.size() - inheritedMethodCount));
        auto const bufferIt(std::find_if(bufferBegin, _buffer.rend(), [&](MethodContext const& oldMethod)
            -> bool
        {
            Metadata::MethodDefRow    const oldMethodDef(oldMethod.GetMethodDefinition());
            Metadata::MethodSignature const oldMethodSig(oldMethod.GetMethodSignature());

            if (!oldMethodDef.GetFlags().IsSet(MethodAttribute::Virtual))
                return false;

            if (oldMethodDef.GetName() != newMethodDef.GetName())
                return false;

            Metadata::SignatureComparer const compareSignatures(
                _loader.Get(),
                &oldMethod.GetDeclaringType().GetDatabase(),
                &newMethod.GetDeclaringType().GetDatabase());

            // If the signature of the method in the derived class is different from the signature
            // of the method in the base class, it does not replace it, because it is HideBySig:
            if (!compareSignatures(oldMethodSig, newMethodSig))
                return false;

            // If the base class method is final, the derived class method is a new method:
            isNewMethod = oldMethodDef.GetFlags().IsSet(MethodAttribute::Final);
            return true;
        }));

        isNewMethod
            ? (void)_buffer.push_back(newMethod)
            : (void)(*bufferIt = newMethod);
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
            Metadata::RowReference(Metadata::TableId::Assembly, 0),
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

    Metadata::FullReference MetadataLoader::ResolveType(Metadata::FullReference const& type, InternalKey) const
    {
        using namespace CxxReflect::Metadata;

        // A TypeDef or TypeSpec is already resolved:
        if (type.AsRowReference().GetTable() == TableId::TypeDef ||
            type.AsRowReference().GetTable() == TableId::TypeSpec)
            return type;

        Detail::Verify([&]{ return type.AsRowReference().GetTable() == TableId::TypeRef; });

        // Ok, we have a TypeRef;
        Database const& referenceDatabase(type.GetDatabase());
        SizeType const typeRefIndex(type.AsRowReference().GetIndex());
        TypeRefRow const typeRef(referenceDatabase.GetRow<TableId::TypeRef>(typeRefIndex));

        RowReference const resolutionScope(typeRef.GetResolutionScope());

        // If the resolution scope is null, we look in the ExportedType table for this type.
        if (!resolutionScope.IsValid())
        {
            Detail::VerifyFail("Not Yet Implemented"); // TODO
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
                throw RuntimeError("Failed to resolve type in module");

            return Metadata::FullReference(
                &definingAssembly.GetContext(InternalKey()).GetDatabase(),
                RowReference::FromToken(resolvedType.GetMetadataToken()));
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
            if (!definingAssembly.IsInitialized())
                throw RuntimeError("Failed to resolve assembly reference");

            Type const resolvedType(definingAssembly.GetType(typeRef.GetNamespace(), typeRef.GetName()));
            if (!resolvedType.IsInitialized())
                throw RuntimeError("Failed to resolve type in assembly");

            return Metadata::FullReference(
                &definingAssembly.GetContext(InternalKey()).GetDatabase(),
                RowReference::FromToken(resolvedType.GetMetadataToken()));
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
            return Metadata::FullReference();
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
