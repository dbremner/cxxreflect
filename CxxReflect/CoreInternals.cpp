//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CoreInternals.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace Detail {

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    MemberContext<TMember, TMemberRow, TMemberSignature>::MemberContext()
    {
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    MemberContext<TMember, TMemberRow, TMemberSignature>::MemberContext(
        Metadata::FullReference const& declaringType,
        Metadata::RowReference  const& member)
        : _declaringType(declaringType),
          _member       (member       )
    {
        VerifyInitialized();
        Verify([&]{ return declaringType.IsRowReference(); });
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    MemberContext<TMember, TMemberRow, TMemberSignature>::MemberContext(
        Metadata::FullReference const& declaringType,
        Metadata::RowReference  const& member,
        Metadata::FullReference const& instantiatingType,
        ByteRange               const& instantiatedSignature)
        : _declaringType        (declaringType        ),
          _member               (member               ),
          _instantiatingType    (instantiatingType    ),
          _instantiatedSignature(instantiatedSignature)
    {
        VerifyInitialized();
        Verify([&]{ return declaringType.IsRowReference(); });
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    TMember MemberContext<TMember, TMemberRow, TMemberSignature>::Resolve(Type const& reflectedType) const
    {
        VerifyInitialized();
        return TMember(reflectedType, this, InternalKey());
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    Metadata::FullReference MemberContext<TMember, TMemberRow, TMemberSignature>::GetDeclaringType() const
    {
        VerifyInitialized();
        return _declaringType;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    Metadata::FullReference MemberContext<TMember, TMemberRow, TMemberSignature>::GetMember() const
    {
        VerifyInitialized();
        return Metadata::FullReference(&_declaringType.GetDatabase(), _member);
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    TMemberRow MemberContext<TMember, TMemberRow, TMemberSignature>::GetMemberRow() const
    {
        Metadata::TableId const TId(static_cast<Metadata::TableId>(Metadata::RowTypeToTableId<TMemberRow>::Value));
        VerifyInitialized();
        return _declaringType
            .GetDatabase()
            .GetRow<TId>(_member);
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    TMemberSignature MemberContext<TMember, TMemberRow, TMemberSignature>::GetMemberSignature() const
    {
        VerifyInitialized();
        if (HasInstantiatedSignature())
        {
            return Metadata::MethodSignature(_instantiatedSignature.Begin(), _instantiatedSignature.End());
        }
        else
        {
            return _declaringType
                .GetDatabase()
                .GetBlob(GetMemberRow().GetSignature())
                .As<TMemberSignature>();
        }
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    bool MemberContext<TMember, TMemberRow, TMemberSignature>::HasInstantiatingType() const
    {
        VerifyInitialized();
        return _instantiatingType.IsInitialized();
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    Metadata::FullReference MemberContext<TMember, TMemberRow, TMemberSignature>::GetInstantiatingType() const
    {
        Verify([&]{ return HasInstantiatingType(); });
        return _instantiatingType;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    bool MemberContext<TMember, TMemberRow, TMemberSignature>::HasInstantiatedSignature() const
    {
        VerifyInitialized();
        return _instantiatedSignature.IsInitialized();
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    ByteRange MemberContext<TMember, TMemberRow, TMemberSignature>::GetInstantiatedSignature() const
    {
        Verify([&]{ return HasInstantiatedSignature(); });
        return _instantiatedSignature;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    bool MemberContext<TMember, TMemberRow, TMemberSignature>::IsInitialized() const
    {
        return _declaringType.IsInitialized() && _member.IsInitialized();
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    void MemberContext<TMember, TMemberRow, TMemberSignature>::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }

    //TODO template class MemberContext<Event,    Metadata::EventRow,     Metadata::TypeSignature    >;
    //TODO template class MemberContext<Field,    Metadata::FieldRow,     Metadata::FieldSignature   >;
    template class MemberContext<Method,   Metadata::MethodDefRow, Metadata::MethodSignature  >;
    //TODO template class MemberContext<Property, Metadata::PropertyRow,  Metadata::PropertySignature>;





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
                if (!instantiator.HasArguments() || !Instantiator::RequiresInstantiation(m.GetMemberSignature()))
                    return m;

                return MethodContext(
                    m.GetDeclaringType(),
                    m.GetMember().AsRowReference(),
                    m.GetInstantiatingType(),
                    Instantiate(instantiator, m.GetMemberSignature()));
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
        Metadata::MethodDefRow    const newMethodDef(newMethod.GetMemberRow());
        Metadata::MethodSignature const newMethodSig(newMethod.GetMemberSignature());

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
            Metadata::MethodDefRow    const oldMethodDef(oldMethod.GetMemberRow());
            Metadata::MethodSignature const oldMethodSig(oldMethod.GetMemberSignature());

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





    AssemblyContext::AssemblyContext(MetadataLoader const* const loader, String path, Metadata::Database&& database)
        : _loader(loader), _path(std::move(path)), _database(std::move(database)), _methods(loader)
    {
        VerifyNotNull(_loader.Get());
        Verify([&]{ return !_path.empty(); });
    }

    AssemblyContext::AssemblyContext(AssemblyContext&& other)
        : _loader  (std::move(other._loader  )),
          _path    (std::move(other._path    )),
          _database(std::move(other._database)),
          _name    (std::move(other._name    )),
          _state   (std::move(other._state   )),
          _methods (std::move(other._methods ))
    {
        other._loader.Get() = nullptr;
        other._state.Reset();
    }

    AssemblyContext& AssemblyContext::operator=(AssemblyContext&& other)
    {
        Swap(other);
        return *this;
    }

    void AssemblyContext::Swap(AssemblyContext& other)
    {
        using std::swap;
        swap(other._loader,   _loader  );
        swap(other._path,     _path    );
        swap(other._database, _database);
        swap(other._name,     _name    );
        swap(other._state,    _state   );
        swap(other._methods,  _methods );
    }

    MetadataLoader const& AssemblyContext::GetLoader() const
    {
        VerifyInitialized();
        return *_loader.Get();
    }

    Metadata::Database const& AssemblyContext::GetDatabase() const
    {
        VerifyInitialized();
        return _database;
    }

    String const& AssemblyContext::GetPath() const
    {
        VerifyInitialized();
        return _path;
    }

    AssemblyName const& AssemblyContext::GetAssemblyName() const
    {
        RealizeName();
        return _name;
    }

    MethodTable const AssemblyContext::GetOrCreateMethodTable(Metadata::ElementReference const& type) const
    {
        // TODO BLOB REFERENCES COULD END UP HERER TOO
        return _methods.GetOrCreateMethodTable(Metadata::FullReference(&_database, type.AsRowReference()));
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

    bool AssemblyContext::IsInitialized() const
    {
        return _loader.Get() != nullptr;
    }

    void AssemblyContext::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }





    AssemblyHandle::AssemblyHandle()
    {
    }

    AssemblyHandle::AssemblyHandle(AssemblyContext const* context)
        : _context(context)
    {
        VerifyInitialized();
    }

    AssemblyHandle::AssemblyHandle(Assembly const& assembly)
        : _context(&assembly.GetContext(InternalKey()))
    {
        VerifyInitialized();
    }

    Assembly AssemblyHandle::Realize() const
    {
        VerifyInitialized();
        return Assembly(_context.Get(), InternalKey());
    }

    bool AssemblyHandle::IsInitialized() const
    {
        return _context.Get() != nullptr;
    }

    void AssemblyHandle::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }

    bool operator==(AssemblyHandle const& lhs, AssemblyHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (AssemblyHandle const& lhs, AssemblyHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





        MethodHandle::MethodHandle()
    {
    }

    MethodHandle::MethodHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                               Metadata::ElementReference const& reflectedTypeReference,
                               MethodContext              const* methodContext)
        : _reflectedTypeAssemblyContext(reflectedTypeAssemblyContext),
          _reflectedTypeReference(reflectedTypeReference),
          _methodContext(methodContext)
    {
        VerifyInitialized();
    }

    MethodHandle::MethodHandle(Method const& method)
        : _reflectedTypeAssemblyContext(&method.GetReflectedType().GetAssembly().GetContext(InternalKey())),
          _reflectedTypeReference(method.GetReflectedType().GetSelfReference(InternalKey())),
          _methodContext(&method.GetContext(InternalKey()))
    {
        VerifyInitialized();
    }

    Method MethodHandle::Realize() const
    {
        VerifyInitialized();
        Assembly const assembly(_reflectedTypeAssemblyContext.Get(), InternalKey());

        Type const reflectedType(_reflectedTypeReference.IsRowReference()
            ? Type(assembly, _reflectedTypeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _reflectedTypeReference.AsBlobReference(), InternalKey()));

        return Method(reflectedType, _methodContext.Get(), InternalKey());
    }

    bool MethodHandle::IsInitialized() const
    {
        return _reflectedTypeAssemblyContext.Get() != nullptr
            && _reflectedTypeReference.IsInitialized()
            && _methodContext.Get() != nullptr;
    }

    void MethodHandle::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }

    bool operator==(MethodHandle const& lhs, MethodHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (MethodHandle const& lhs, MethodHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    ParameterHandle::ParameterHandle()
    {
    }

    ParameterHandle::ParameterHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                                     Metadata::ElementReference const& reflectedTypeReference,
                                     MethodContext              const* methodContext,
                                     Metadata::RowReference     const& parameterReference,
                                     Metadata::TypeSignature    const& parameterSignature)
        : _reflectedTypeAssemblyContext(reflectedTypeAssemblyContext),
          _reflectedTypeReference(reflectedTypeReference),
          _methodContext(methodContext),
          _parameterReference(parameterReference),
          _parameterSignature(parameterSignature)
    {
        VerifyInitialized();
    }

    ParameterHandle::ParameterHandle(Parameter const& parameter)
        : _reflectedTypeAssemblyContext(&parameter.GetDeclaringMethod().GetReflectedType().GetAssembly().GetContext(InternalKey())),
          _reflectedTypeReference(parameter.GetDeclaringMethod().GetReflectedType().GetSelfReference(InternalKey())),
          _methodContext(&parameter.GetDeclaringMethod().GetContext(InternalKey())),
          _parameterReference(parameter.GetSelfReference(InternalKey())),
          _parameterSignature(parameter.GetSelfSignature(InternalKey()))
    {
        VerifyInitialized();
    }

    Parameter ParameterHandle::Realize() const
    {
        VerifyInitialized();
        Assembly const assembly(_reflectedTypeAssemblyContext.Get(), InternalKey());

        Type const reflectedType(_reflectedTypeReference.IsRowReference()
            ? Type(assembly, _reflectedTypeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _reflectedTypeReference.AsBlobReference(), InternalKey()));

        Method const declaringMethod(reflectedType, _methodContext.Get(), InternalKey());

        return Parameter(declaringMethod, _parameterReference, _parameterSignature, InternalKey());
    }

    bool ParameterHandle::IsInitialized() const
    {
        return _reflectedTypeAssemblyContext.Get() != nullptr
            && _reflectedTypeReference.IsInitialized()
            && _methodContext.Get() != nullptr
            && _parameterReference.IsInitialized()
            && _parameterSignature.IsInitialized();
    }

    void ParameterHandle::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }

    bool operator==(ParameterHandle const& lhs, ParameterHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator<(ParameterHandle const& lhs, ParameterHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    TypeHandle::TypeHandle()
    {
    }

    TypeHandle::TypeHandle(AssemblyContext            const* assemblyContext,
                           Metadata::ElementReference const& typeReference)
        : _assemblyContext(assemblyContext),
          _typeReference(typeReference)
    {
        VerifyInitialized();
    }

    TypeHandle::TypeHandle(Type const& type)
        : _assemblyContext(&type.GetAssembly().GetContext(InternalKey())),
          _typeReference(type.GetSelfReference(InternalKey()))
    {
        VerifyInitialized();
    }

    Type TypeHandle::Realize() const
    {
        VerifyInitialized();
        Assembly const assembly(_assemblyContext.Get(), InternalKey());
        return _typeReference.IsRowReference()
            ? Type(assembly, _typeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _typeReference.AsBlobReference(), InternalKey());
    }

    bool TypeHandle::IsInitialized() const
    {
        return _assemblyContext.Get() != nullptr && _typeReference.IsInitialized();
    }

    void TypeHandle::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }

    bool operator==(TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }

} }
