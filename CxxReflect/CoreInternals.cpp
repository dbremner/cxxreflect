//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/CoreInternals.hpp"
#include "CxxReflect/Event.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace Detail { namespace { namespace Private {

    // TODO We could refactor much of the code in these four 'PlaceMemberIntoBuffer' functions.

    void PlaceMemberIntoBuffer(Loader                    const&,
                               std::vector<EventContext>      & buffer,
                               EventContext              const& newEvent,
                               SizeType                  const )
    {
        // TODO
        buffer.push_back(newEvent);
    }

    void PlaceMemberIntoBuffer(Loader                    const&,
                               std::vector<FieldContext>      & buffer,
                               FieldContext              const& newField,
                               SizeType                  const )
    {
        // Fields are hide by name and sig (in theory), but the .NET Reflection API will return all
        // of them, regardless of whether they are technically hidden.
        buffer.push_back(newField);
    }

    void PlaceMemberIntoBuffer(Loader                     const& loader,
                               std::vector<MethodContext>      & buffer,
                               MethodContext              const& newMethod,
                               SizeType                   const  inheritedMethodCount)
    {
        Metadata::MethodDefRow    const newMethodDef(newMethod.GetMemberRow());
        Metadata::MethodSignature const newMethodSig(newMethod.GetMemberSignature());

        // If the method occupies a new slot, it does not override any other method.  A static
        // method is always a new method.
        if (newMethodDef.GetFlags().WithMask(MethodAttribute::VTableLayoutMask) == MethodAttribute::NewSlot ||
            newMethodDef.GetFlags().IsSet(MethodAttribute::Static))
        {
            buffer.push_back(newMethod);
            return;
        }

        bool isNewMethod(true);
        auto const bufferBegin(buffer.rbegin() + (buffer.size() - inheritedMethodCount));
        auto const bufferIt(std::find_if(bufferBegin, buffer.rend(), [&](MethodContext const& oldMethod)
            -> bool
        {
            Metadata::MethodDefRow    const oldMethodDef(oldMethod.GetMemberRow());
            Metadata::MethodSignature const oldMethodSig(oldMethod.GetMemberSignature());

            // TODO Does this correctly handle hiding of base-class methods?
            if (!oldMethodDef.GetFlags().IsSet(MethodAttribute::Virtual))
                return false;

            if (oldMethodDef.GetName() != newMethodDef.GetName())
                return false;

            Metadata::SignatureComparer const compareSignatures(
                &loader,
                &oldMethod.GetDeclaringType().GetDatabase(),
                &newMethod.GetDeclaringType().GetDatabase());

            // If the signature of the method in the derived class is different from the signature
            // of the method in the base class, it is not an override:
            if (!compareSignatures(oldMethodSig, newMethodSig))
                return false;

            // If the base class method is final, the derived class method is a new method:
            isNewMethod = oldMethodDef.GetFlags().IsSet(MethodAttribute::Final);
            return true;
        }));

        isNewMethod
            ? (void)buffer.push_back(newMethod)
            : (void)(*bufferIt = newMethod);
    }

    void PlaceMemberIntoBuffer(Loader                       const&,
                               std::vector<PropertyContext>      & buffer,
                               PropertyContext              const& newProperty,
                               SizeType                     const  )
    {
        // TODO
        buffer.push_back(newProperty);
    }

    template <typename TMemberRow>
    auto GetSignatureFromRow(TMemberRow const& row) -> decltype(row.GetSignature())
    {
        return row.GetSignature();
    }

    Metadata::BlobReference GetSignatureFromRow(Metadata::EventRow const&)
    {
        return Metadata::BlobReference(); // TODO
    }

    template <typename TMemberRow>
    SizeType GetFirstMemberIndex(Metadata::Database const& database, Metadata::TypeDefRow const& type);

    template <typename TMemberRow>
    SizeType GetLastMemberIndex(Metadata::Database const& database, Metadata::TypeDefRow const& type);

    template <>
    SizeType GetFirstMemberIndex<Metadata::EventRow>(Metadata::Database   const&,
                                                     Metadata::TypeDefRow const&)
    {
        return 0; // TODO
    }

    template <>
    SizeType GetLastMemberIndex<Metadata::EventRow>(Metadata::Database   const&,
                                                    Metadata::TypeDefRow const&)
    {
        return 0; // TODO
    }

    template <>
    SizeType GetFirstMemberIndex<Metadata::FieldRow>(Metadata::Database   const&,
                                                     Metadata::TypeDefRow const& type)
    {
        return type.GetFirstField().GetIndex();
    }

    template <>
    SizeType GetLastMemberIndex<Metadata::FieldRow>(Metadata::Database   const&,
                                                    Metadata::TypeDefRow const& type)
    {
        return type.GetLastField().GetIndex();
    }

    template <>
    SizeType GetFirstMemberIndex<Metadata::MethodDefRow>(Metadata::Database   const&,
                                                         Metadata::TypeDefRow const& type)
    {
        return type.GetFirstMethod().GetIndex();
    }

    template <>
    SizeType GetLastMemberIndex<Metadata::MethodDefRow>(Metadata::Database   const&,
                                                        Metadata::TypeDefRow const& type)
    {
        return type.GetLastMethod().GetIndex();
    }

    template <>
    SizeType GetFirstMemberIndex<Metadata::PropertyRow>(Metadata::Database   const&,
                                                        Metadata::TypeDefRow const&)
    {
        return 0; // TODO
    }

    template <>
    SizeType GetLastMemberIndex<Metadata::PropertyRow>(Metadata::Database   const&,
                                                       Metadata::TypeDefRow const&)
    {
        return 0; // TODO
    }

} } } }

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
        AssertInitialized();
        Assert([&]{ return declaringType.IsRowReference(); });
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    MemberContext<TMember, TMemberRow, TMemberSignature>::MemberContext(
        Metadata::FullReference const& declaringType,
        Metadata::RowReference  const& member,
        Metadata::FullReference const& instantiatingType,
        ConstByteRange          const& instantiatedSignature)
        : _declaringType        (declaringType        ),
          _member               (member               ),
          _instantiatingType    (instantiatingType    ),
          _instantiatedSignature(instantiatedSignature)
    {
        AssertInitialized();
        Assert([&]{ return declaringType.IsRowReference(); });
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    TMember MemberContext<TMember, TMemberRow, TMemberSignature>::Resolve(Type const& reflectedType) const
    {
        AssertInitialized();
        return TMember(reflectedType, this, InternalKey());
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    Metadata::FullReference MemberContext<TMember, TMemberRow, TMemberSignature>::GetDeclaringType() const
    {
        AssertInitialized();
        return _declaringType;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    Metadata::FullReference MemberContext<TMember, TMemberRow, TMemberSignature>::GetMember() const
    {
        AssertInitialized();
        return Metadata::FullReference(&_declaringType.GetDatabase(), _member);
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    TMemberRow MemberContext<TMember, TMemberRow, TMemberSignature>::GetMemberRow() const
    {
        Metadata::TableId const TId(static_cast<Metadata::TableId>(Metadata::RowTypeToTableId<TMemberRow>::Value));
        AssertInitialized();
        return _declaringType
            .GetDatabase()
            .GetRow<TId>(_member);
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    TMemberSignature MemberContext<TMember, TMemberRow, TMemberSignature>::GetMemberSignature() const
    {
        AssertInitialized();
        if (HasInstantiatedSignature())
        {
            return TMemberSignature(_instantiatedSignature.Begin(), _instantiatedSignature.End());
        }
        else
        {
            return _declaringType
                .GetDatabase()
                .GetBlob(Private::GetSignatureFromRow(GetMemberRow()))
                .As<TMemberSignature>();
        }
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    bool MemberContext<TMember, TMemberRow, TMemberSignature>::HasInstantiatingType() const
    {
        AssertInitialized();
        return _instantiatingType.IsInitialized();
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    Metadata::FullReference MemberContext<TMember, TMemberRow, TMemberSignature>::GetInstantiatingType() const
    {
        Assert([&]{ return HasInstantiatingType(); });
        return _instantiatingType;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    bool MemberContext<TMember, TMemberRow, TMemberSignature>::HasInstantiatedSignature() const
    {
        AssertInitialized();
        return _instantiatedSignature.IsInitialized();
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    ConstByteRange MemberContext<TMember, TMemberRow, TMemberSignature>::GetInstantiatedSignature() const
    {
        Assert([&]{ return HasInstantiatedSignature(); });
        return _instantiatedSignature;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    bool MemberContext<TMember, TMemberRow, TMemberSignature>::IsInitialized() const
    {
        return _declaringType.IsInitialized() && _member.IsInitialized();
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    void MemberContext<TMember, TMemberRow, TMemberSignature>::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    template class MemberContext<Event,    Metadata::EventRow,     Metadata::TypeSignature    >;
    template class MemberContext<Field,    Metadata::FieldRow,     Metadata::FieldSignature   >;
    template class MemberContext<Method,   Metadata::MethodDefRow, Metadata::MethodSignature  >;
    template class MemberContext<Property, Metadata::PropertyRow,  Metadata::PropertySignature>;





    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    MemberTableCollection<TMember, TMemberRow, TMemberSignature>::MemberTableCollection(Loader const* const loader)
        : _loader(loader)
    {
        AssertNotNull(loader);
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    MemberTableCollection<TMember, TMemberRow, TMemberSignature>::MemberTableCollection(
        MemberTableCollection&& other)
        : _loader            (std::move(other._loader            )),
          _signatureAllocator(std::move(other._signatureAllocator)),
          _tableAllocator    (std::move(other._tableAllocator    )),
          _index             (std::move(other._index             )),
          _buffer            (std::move(other._buffer            ))
    {
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    MemberTableCollection<TMember, TMemberRow, TMemberSignature>&
    MemberTableCollection<TMember, TMemberRow, TMemberSignature>::operator=(MemberTableCollection&& other)
    {
        Swap(other);
        return *this;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    void MemberTableCollection<TMember, TMemberRow, TMemberSignature>::Swap(MemberTableCollection& other)
    {
        using std::swap;
        swap(other._loader,             _loader            );
        swap(other._signatureAllocator, _signatureAllocator);
        swap(other._tableAllocator,     _tableAllocator    );
        swap(other._index,              _index             );
        swap(other._buffer,             _buffer            );
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    typename MemberTableCollection<TMember, TMemberRow, TMemberSignature>::MemberTableType
    MemberTableCollection<TMember, TMemberRow, TMemberSignature>::GetOrCreateMemberTable(FullReference const& type) const
    {
        auto const it(_index.find(type));
        if (it != end(_index))
            return it->second;

        // When this function returns, we want to clear the buffer so it's ready for our next use.
        // Note that we use resize(0) because on Visual C++, clear() will destroy the underlying
        // storage.  We want to keep the buffer intact so we can reuse it.
        ScopeGuard const bufferCleanupGuard([&]{ _buffer.resize(0); });

        TypeDefAndSpec const  typeDefAndSpec(ResolveTypeDefAndSpec(type));
        FullReference  const& typeDefReference(typeDefAndSpec.first);
        FullReference  const& typeSpecReference(typeDefAndSpec.second);

        Metadata::Database const& database(typeDefReference.GetDatabase());

        SizeType const typeDefIndex(typeDefReference.AsRowReference().GetIndex());
        Metadata::TypeDefRow const typeDef(database.GetRow<Metadata::TableId::TypeDef>(typeDefIndex));

        Instantiator const instantiator(CreateInstantiator(typeSpecReference));

        Metadata::RowReference const baseTypeReference(typeDef.GetExtends());
        if (baseTypeReference.IsValid())
        {
            MemberTableType const table(GetOrCreateMemberTable(FullReference(&database, baseTypeReference)));

            std::transform(table.Begin(), table.End(), std::back_inserter(_buffer), [&](MemberContextType const& m)
                -> MemberContextType
            {
                if (!instantiator.HasArguments() || !Instantiator::RequiresInstantiation(m.GetMemberSignature()))
                    return m;

                return MemberContextType(
                    m.GetDeclaringType(),
                    m.GetMember().AsRowReference(),
                    m.GetInstantiatingType(),
                    Instantiate(instantiator, m.GetMemberSignature()));
            });
        }

        SizeType const inheritedMemberCount(static_cast<SizeType>(_buffer.size()));

        Metadata::TableId const TId(static_cast<Metadata::TableId>(Metadata::RowTypeToTableId<MemberRowType>::Value));
        auto const beginMember(database.Begin<TId>());
        auto const firstMember(beginMember + Private::GetFirstMemberIndex<MemberRowType>(database, typeDef));
        auto const lastMember (beginMember + Private::GetLastMemberIndex<MemberRowType>(database, typeDef));

        std::for_each(firstMember, lastMember, [&](MemberRowType const& memberDef)
        {
            MemberSignatureType const memberSig(database
                .GetBlob(Private::GetSignatureFromRow(memberDef))
                .As<MemberSignatureType>());

            bool const requiresInstantiation(
                instantiator.HasArguments() &&
                Instantiator::RequiresInstantiation(memberSig));

            ConstByteRange const instantiatedSig(requiresInstantiation
                ? Instantiate(instantiator, memberSig)
                : ConstByteRange(memberSig.BeginBytes(), memberSig.EndBytes()));

            Metadata::RowReference const memberDefReference(memberDef.GetSelfReference());

            MemberContextType const memberContext(instantiatedSig.IsInitialized()
                ? MemberContextType(typeDefReference, memberDefReference, typeSpecReference, instantiatedSig)
                : MemberContextType(typeDefReference, memberDefReference));

            InsertMemberIntoBuffer(memberContext, inheritedMemberCount);
        });

        MemberTableType const range(_tableAllocator.Allocate(_buffer.size()));
        RangeCheckedCopy(_buffer.begin(), _buffer.end(), range.Begin(), range.End());

        _index.insert(std::make_pair(type, range));
        return range;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    typename MemberTableCollection<TMember, TMemberRow, TMemberSignature>::TypeDefAndSpec
    MemberTableCollection<TMember, TMemberRow, TMemberSignature>::ResolveTypeDefAndSpec(FullReference const& type) const
    {
        FullReference const resolvedType(_loader.Get()->ResolveType(type));

        // If we have a TypeDef, there is no TypeSpec, so we can just return the TypeDef directly:
        if (resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef)
            return std::make_pair(resolvedType, FullReference());

        // Otherwise, we have a TypeSpec, and we need to resolve the TypeDef to which it refers:
        Assert([&]{ return resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeSpec; });

        Metadata::TypeSpecRow const typeSpec(resolvedType
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeSpec>(resolvedType));

        Metadata::TypeSignature const typeSignature(resolvedType
            .GetDatabase()
            .GetBlob(typeSpec.GetSignature())
            .As<Metadata::TypeSignature>());

        // We aren't expecting any other kinds of type signatures to be used as base classes:
        Assert([&]{ return typeSignature.GetKind() == Metadata::TypeSignature::Kind::GenericInst; });

        FullReference const reResolvedType(_loader.Get()->ResolveType(
            FullReference(&resolvedType.GetDatabase(), typeSignature.GetGenericTypeReference())));

        // A GenericInst should refer to a TypeDef or a TypeRef, never another TypeSpec.  We resolve
        // the TypeRef above, so at this point we should always have a TypeDef:
        Assert([&]{ return reResolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });

        return std::make_pair(reResolvedType, resolvedType);
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    typename MemberTableCollection<TMember, TMemberRow, TMemberSignature>::Instantiator
    MemberTableCollection<TMember, TMemberRow, TMemberSignature>::CreateInstantiator(FullReference const& type) const
    {
        if (!type.IsInitialized() || type.AsRowReference().GetTable() != Metadata::TableId::TypeSpec)
            return Metadata::ClassVariableSignatureInstantiator();

        Metadata::TypeSignature const signature(type
            .GetDatabase()
            .GetBlob(type.GetDatabase().GetRow<Metadata::TableId::TypeSpec>(type).GetSignature())
            .As<Metadata::TypeSignature>());

        Assert([&]{ return signature.GetKind() == Metadata::TypeSignature::Kind::GenericInst; });

        return Metadata::ClassVariableSignatureInstantiator(
            signature.BeginGenericArguments(),
            signature.EndGenericArguments());
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    ConstByteRange MemberTableCollection<TMember, TMemberRow, TMemberSignature>::Instantiate(
        Instantiator        const& instantiator,
        MemberSignatureType const& signature) const
    {
        Assert([&]{ return signature.IsInitialized(); });
        Assert([&]{ return Instantiator::RequiresInstantiation(signature); });

        MemberSignatureType const& instantiation(instantiator.Instantiate(signature));
        SizeType const instantiationSize(
            static_cast<SizeType>(std::distance(instantiation.BeginBytes(), instantiation.EndBytes())));

        ByteRange const ownedInstantiation(_signatureAllocator.Allocate(instantiationSize));

        RangeCheckedCopy(
            instantiation.BeginBytes(), instantiation.EndBytes(),
            ownedInstantiation.Begin(), ownedInstantiation.End());

        return ownedInstantiation;
    }

    template <typename TMember, typename TMemberRow, typename TMemberSignature>
    void MemberTableCollection<TMember, TMemberRow, TMemberSignature>::InsertMemberIntoBuffer(
        MemberContextType const& newMember,
        SizeType          const  inheritedMemberCount) const
    {
        return Private::PlaceMemberIntoBuffer(*_loader.Get(), _buffer, newMember, inheritedMemberCount);
    }

    template class MemberTableCollection<Event,    Metadata::EventRow,     Metadata::TypeSignature    >;
    template class MemberTableCollection<Field,    Metadata::FieldRow,     Metadata::FieldSignature   >;
    template class MemberTableCollection<Method,   Metadata::MethodDefRow, Metadata::MethodSignature  >;
    template class MemberTableCollection<Property, Metadata::PropertyRow,  Metadata::PropertySignature>;





    AssemblyContext::AssemblyContext(Loader const* const loader, String uri, Metadata::Database&& database)
        : _loader(loader),
          _uri(std::move(uri)),
          _database(std::move(database)),
          _events(loader),
          _fields(loader),
          _methods(loader),
          _properties(loader)
    {
        AssertNotNull(_loader.Get());
        Assert([&]{ return !_uri.empty(); });
    }

    AssemblyContext::AssemblyContext(AssemblyContext&& other)
        : _loader    (std::move(other._loader    )),
          _uri       (std::move(other._uri       )),
          _database  (std::move(other._database  )),
          _name      (std::move(other._name      )),
          _state     (std::move(other._state     )),
          _events    (std::move(other._events    )),
          _fields    (std::move(other._fields    )),
          _methods   (std::move(other._methods   )),
          _properties(std::move(other._properties))
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
        swap(other._loader,     _loader    );
        swap(other._uri,        _uri       );
        swap(other._database,   _database  );
        swap(other._name,       _name      );
        swap(other._state,      _state     );
        swap(other._events,     _events    );
        swap(other._fields,     _fields    );
        swap(other._methods,    _methods   );
        swap(other._properties, _properties);
    }

    Loader const& AssemblyContext::GetLoader() const
    {
        AssertInitialized();
        return *_loader.Get();
    }

    Metadata::Database const& AssemblyContext::GetDatabase() const
    {
        AssertInitialized();
        return _database;
    }

    String const& AssemblyContext::GetLocation() const
    {
        AssertInitialized();
        return _uri;
    }

    AssemblyName const& AssemblyContext::GetAssemblyName() const
    {
        RealizeName();
        return *_name;
    }

    EventTable const AssemblyContext::GetOrCreateEventTable(Metadata::ElementReference const& type) const
    {
        return _events.GetOrCreateMemberTable(Metadata::FullReference(&_database, type));
    }

    FieldTable const AssemblyContext::GetOrCreateFieldTable(Metadata::ElementReference const& type) const
    {
        return _fields.GetOrCreateMemberTable(Metadata::FullReference(&_database, type));
    }

    MethodTable const AssemblyContext::GetOrCreateMethodTable(Metadata::ElementReference const& type) const
    {
        return _methods.GetOrCreateMemberTable(Metadata::FullReference(&_database, type));
    }

    PropertyTable const AssemblyContext::GetOrCreatePropertyTable(Metadata::ElementReference const& type) const
    {
        return _properties.GetOrCreateMemberTable(Metadata::FullReference(&_database, type));
    }

    void AssemblyContext::RealizeName() const
    {
        if (_state.IsSet(RealizedName)) { return; }

        _name.reset(new AssemblyName(
            Assembly(this, InternalKey()),
            Metadata::RowReference(Metadata::TableId::Assembly, 0),
            InternalKey()));

        _state.Set(RealizedName);
    }

    bool AssemblyContext::IsInitialized() const
    {
        return _loader.Get() != nullptr;
    }

    void AssemblyContext::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }





    AssemblyHandle::AssemblyHandle()
    {
    }

    AssemblyHandle::AssemblyHandle(AssemblyContext const* context)
        : _context(context)
    {
        AssertInitialized();
    }

    AssemblyHandle::AssemblyHandle(Assembly const& assembly)
        : _context(&assembly.GetContext(InternalKey()))
    {
        AssertInitialized();
    }

    Assembly AssemblyHandle::Realize() const
    {
        AssertInitialized();
        return Assembly(_context.Get(), InternalKey());
    }

    bool AssemblyHandle::IsInitialized() const
    {
        return _context.Get() != nullptr;
    }

    void AssemblyHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
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
        AssertInitialized();
    }

    MethodHandle::MethodHandle(Method const& method)
        : _reflectedTypeAssemblyContext(&method.GetReflectedType().GetAssembly().GetContext(InternalKey())),
          _reflectedTypeReference(method.GetReflectedType().GetSelfReference(InternalKey())),
          _methodContext(&method.GetContext(InternalKey()))
    {
        AssertInitialized();
    }

    Method MethodHandle::Realize() const
    {
        AssertInitialized();
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

    void MethodHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
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
        AssertInitialized();
    }

    ParameterHandle::ParameterHandle(Parameter const& parameter)
        : _reflectedTypeAssemblyContext(&parameter.GetDeclaringMethod().GetReflectedType().GetAssembly().GetContext(InternalKey())),
          _reflectedTypeReference(parameter.GetDeclaringMethod().GetReflectedType().GetSelfReference(InternalKey())),
          _methodContext(&parameter.GetDeclaringMethod().GetContext(InternalKey())),
          _parameterReference(parameter.GetSelfReference(InternalKey())),
          _parameterSignature(parameter.GetSelfSignature(InternalKey()))
    {
        AssertInitialized();
    }

    Parameter ParameterHandle::Realize() const
    {
        AssertInitialized();
        Assembly const assembly(_reflectedTypeAssemblyContext.Get(), InternalKey());

        Type const reflectedType(_reflectedTypeReference.IsRowReference()
            ? Type(assembly, _reflectedTypeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _reflectedTypeReference.AsBlobReference(), InternalKey()));

        Method const declaringMethod(reflectedType, _methodContext.Get(), InternalKey());

        /* TODO return Parameter(
            declaringMethod,
            ParameterData(_parameterReference, _parameterSignature, InternalKey()),
            InternalKey());*/

        return Parameter();
    }

    bool ParameterHandle::IsInitialized() const
    {
        return _reflectedTypeAssemblyContext.Get() != nullptr
            && _reflectedTypeReference.IsInitialized()
            && _methodContext.Get() != nullptr
            && _parameterReference.IsInitialized()
            && _parameterSignature.IsInitialized();
    }

    void ParameterHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
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
        AssertInitialized();
    }

    TypeHandle::TypeHandle(Type const& type)
        : _assemblyContext(&type.GetAssembly().GetContext(InternalKey())),
          _typeReference(type.GetSelfReference(InternalKey()))
    {
        AssertInitialized();
    }

    Type TypeHandle::Realize() const
    {
        AssertInitialized();
        Assembly const assembly(_assemblyContext.Get(), InternalKey());
        return _typeReference.IsRowReference()
            ? Type(assembly, _typeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _typeReference.AsBlobReference(), InternalKey());
    }

    bool TypeHandle::IsInitialized() const
    {
        return _assemblyContext.Get() != nullptr && _typeReference.IsInitialized();
    }

    void TypeHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    ParameterData::ParameterData()
    {
    }

    ParameterData::ParameterData(Metadata::RowReference                       const& parameter,
                                 Metadata::MethodSignature::ParameterIterator const& signature,
                                 InternalKey)
        : _parameter(parameter), _signature(signature)
    {
        AssertInitialized();
    }

    bool ParameterData::IsInitialized() const
    {
        return _parameter.IsInitialized();
    }

    void ParameterData::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    ParameterData& ParameterData::operator++()
    {
        AssertInitialized();

        ++_parameter;
        ++_signature;
        return *this;
    }

    ParameterData ParameterData::operator++(int)
    {
        ParameterData const it(*this);
        ++*this;
        return it;
    }

    bool operator==(ParameterData const& lhs, ParameterData const& rhs)
    {
        lhs.AssertInitialized();
        rhs.AssertInitialized();

        return lhs._parameter == rhs._parameter;
    }

    bool operator< (ParameterData const& lhs, ParameterData const& rhs)
    {
        lhs.AssertInitialized();
        rhs.AssertInitialized();

        return lhs._parameter < rhs._parameter;
    }

    Metadata::RowReference const& ParameterData::GetParameter() const
    {
        AssertInitialized();

        return _parameter;
    }

    Metadata::TypeSignature const& ParameterData::GetSignature() const
    {
        AssertInitialized();

        return *_signature;
    }

} }

namespace CxxReflect {

    IAssemblyLocator::~IAssemblyLocator()
    {
        // Virtual destructor required for interface class
    }

    bool Utility::IsSystemAssembly(Assembly const& assembly)
    {
        Detail::Assert([&]{ return assembly.IsInitialized(); });

        return assembly.GetReferencedAssemblyCount() == 0;
    }

    bool Utility::IsSystemType(Type            const& type,
                               StringReference const& systemTypeNamespace,
                               StringReference const& systemTypeSimpleName)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        return IsSystemAssembly(type.GetAssembly())
            && type.GetNamespace() == systemTypeNamespace
            && type.GetName()      == systemTypeSimpleName;
    }

    bool Utility::IsDerivedFromSystemType(Type            const& type,
                                          StringReference const& systemTypeNamespace,
                                          StringReference const& systemTypeSimpleName,
                                          bool                   includeSelf)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        Type currentType(type);
        if (!includeSelf && currentType)
            currentType = type.GetBaseType();

        while (currentType)
        {
            if (IsSystemType(currentType, systemTypeNamespace, systemTypeSimpleName))
                return true;

            currentType = currentType.GetBaseType();
        }

        return false;
    }

    Assembly Utility::GetSystemAssembly(Type const& referenceType)
    {
        Detail::Assert([&]{ return referenceType.IsInitialized(); });

        return GetSystemObjectType(referenceType).GetAssembly();
    }

    Assembly Utility::GetSystemAssembly(Assembly const& referenceAssembly)
    {
        Detail::Assert([&]{ return referenceAssembly.IsInitialized(); });

        return GetSystemObjectType(referenceAssembly).GetAssembly();
    }

    Type Utility::GetSystemObjectType(Type const& referenceType)
    {
        Detail::Assert([&]{ return referenceType.IsInitialized(); });

        Type currentType(referenceType);
        while (currentType.GetBaseType())
            currentType = currentType.GetBaseType();

        Detail::Assert([&]{ return currentType.GetFullName() == L"System.Object"; });
        Detail::Assert([&]{ return IsSystemAssembly(currentType.GetAssembly());   });
        // TODO This should be a hard-verified error condition:  an ill-formed assembly might have a
        // type not derived from the One True Object type.

        return currentType;
    }

    Type Utility::GetSystemObjectType(Assembly const& referenceAssembly)
    {
        Detail::Assert([&]{ return referenceAssembly.IsInitialized(); });

        if (referenceAssembly.BeginTypes() != referenceAssembly.EndTypes())
            return GetSystemObjectType(*referenceAssembly.BeginTypes());

        Loader const& loader(referenceAssembly.GetContext(InternalKey()).GetLoader());

        auto const it(std::find_if(referenceAssembly.BeginReferencedAssemblyNames(),
                                   referenceAssembly.EndReferencedAssemblyNames(),
                                   [&](AssemblyName const& assemblyName) -> bool
        {
            Assembly const assembly(loader.LoadAssembly(assemblyName));
            Detail::Assert([&]{ return assembly.IsInitialized(); });
            // TODO Should this be a hard-verified error?

            return assembly.BeginTypes() != assembly.EndTypes();
        }));

        Detail::Assert([&]{ return it != referenceAssembly.EndReferencedAssemblyNames(); });
        // TODO That should probably be hard-verified, to handle ill-formed assemblies.

        Assembly const foundReferenceAssembly(loader.LoadAssembly(*it));
        return GetSystemObjectType(*foundReferenceAssembly.BeginTypes());
    }

    Type Utility::GetPrimitiveType(Assembly const& referenceAssembly, Metadata::ElementType const elementType)
    {
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
        case Metadata::ElementType::Void:       primitiveTypeName = L"Void";           break;
        case Metadata::ElementType::TypedByRef: primitiveTypeName = L"TypedReference"; break;
        default:
            Detail::AssertFail(L"Unknown primitive type");
            break;
        }

        Assembly const systemAssembly(GetSystemAssembly(referenceAssembly));
        Detail::Assert([&]{ return systemAssembly.IsInitialized(); });

        Type const primitiveType(systemAssembly.GetType(L"System", primitiveTypeName));
        Detail::Assert([&]{ return primitiveType.IsInitialized(); });

        return primitiveType;
    }

}
