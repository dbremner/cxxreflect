//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Event.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/OwnedElements.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

// TODO REMOVE CORE COMPONENTS
#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect { namespace Detail { namespace { namespace Private {

    // A helper function which, given a TypeSpec, returns its TypeSignature.
    Metadata::TypeSignature GetTypeSpecSignature(Metadata::FullReference const& type)
    {
        Assert([&]{ return type.IsInitialized() && type.IsRowReference();                   });
        Assert([&]{ return type.AsRowReference().GetTable() == Metadata::TableId::TypeSpec; });

        Metadata::TypeSpecRow const typeSpec(type
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeSpec>(type));

        Metadata::TypeSignature const typeSignature(type
            .GetDatabase()
            .GetBlob(typeSpec.GetSignature())
            .As<Metadata::TypeSignature>());

        return typeSignature;
    }





    // A pair that contains a TypeDef and a TypeSpec.  The TypeDef is guaranteed to be present, but
    // the TypeSpec may or may not be present.  This pair type is returned by ResolveTypeDefAndSpec.
    class TypeDefAndSpec
    {
    public:

        TypeDefAndSpec(Metadata::FullReference const& typeDef)
            : _typeDef(typeDef)
        {
            Assert([&]{ return typeDef.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });
        }

        TypeDefAndSpec(Metadata::FullReference const& typeDef, Metadata::FullReference const& typeSpec)
            : _typeDef (typeDef ),
              _typeSpec(typeSpec)
        {
            Assert([&]{ return typeDef .AsRowReference().GetTable() == Metadata::TableId::TypeDef;  });
            Assert([&]{ return typeSpec.AsRowReference().GetTable() == Metadata::TableId::TypeSpec; });
        }

        Metadata::FullReference const& GetTypeDef()  const { return _typeDef;                  }
        Metadata::FullReference const& GetTypeSpec() const { return _typeSpec;                 }
        bool                           HasTypeSpec() const { return _typeSpec.IsInitialized(); }

    private:

        Metadata::FullReference _typeDef;
        Metadata::FullReference _typeSpec;
    };





    // Resolves 'originalType' to its TypeSpec and primary TypeDef components.  The behavior of this
    // function depends on what 'originalType' is.  If it is a...
    //  * ...TypeDef, it is returned unchanged (no TypeSpec is returned).
    //  * ...TypeSpec, it must be a GenericInst, and the GenericInst's GenericTypeReference is
    //       returned as the TypeDef and the TypeSpec is returned as the TypeSpec.
    //  * ...TypeRef, it is resolved to the TypeDef or TypeSpec to which it refers.  The function
    //       then behaves as if that TypeDef or TypeSpec was passed directly into this function.
    TypeDefAndSpec ResolveTypeDefAndSpec(Metadata::ITypeResolver const& typeResolver,
                                         Metadata::FullReference const& originalType)
    {
        Assert([&]{ return originalType.IsInitialized(); });

        // Resolve the original type; this will give us either a TypeDef or a TypeSpec:
        Metadata::FullReference const resolvedType(typeResolver.ResolveType(originalType));

        // If we resolve 'type' to a TypeDef, there is no TypeSpec so we can just return the TypeDef:
        if (resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef)
            return TypeDefAndSpec(resolvedType);

        // Otherwise, we must have a TypeSpec, and we need to resolve the TypeDef to which it refers:
        Verify([&]{ return resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeSpec; });

        Metadata::TypeSignature const typeSignature(GetTypeSpecSignature(resolvedType));

        // We are only expecting to resolve to a base class, so we only expect a GenericInst:
        Verify([&]{ return typeSignature.GetKind() == Metadata::TypeSignature::Kind::GenericInst; });

        // Re-resolve the generic type reference to the TypeDef it instantiates:
        Metadata::FullReference const reResolvedType(typeResolver.ResolveType(Metadata::FullReference(
            &resolvedType.GetDatabase(),
            typeSignature.GetGenericTypeReference())));

        // A GenericInst should refer to a TypeDef or a TypeRef, never another TypeSpec.  We resolve
        // the TypeRef above, so at this point we should always have a TypeDef:
        Verify([&]{ return reResolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });

        return TypeDefAndSpec(reResolvedType, resolvedType);
    }





    // Given 'type', returns a signature instantiator that will instantiate generic classes by
    // replacing the class variables with the generic arguments from 'type'.
    Metadata::ClassVariableSignatureInstantiator CreateInstantiator(Metadata::FullReference const& type)
    {
        // If 'type' isn't a TypeSpec, there is nothing to instantiate:
        if (!type.IsInitialized() || type.AsRowReference().GetTable() != Metadata::TableId::TypeSpec)
            return Metadata::ClassVariableSignatureInstantiator();

        Metadata::TypeSignature const typeSignature(GetTypeSpecSignature(type));

        // We are only expecting to get base classes here, so it should be a GenericInst TypeSpec:
        Verify([&]{ return typeSignature.GetKind() == Metadata::TypeSignature::Kind::GenericInst; });

        return Metadata::ClassVariableSignatureInstantiator(
            typeSignature.BeginGenericArguments(),
            typeSignature.EndGenericArguments());
    }





    // 'InsertElementIntoBuffer' takes a new element and inserts it into the buffer.  TODO
    void InsertElementIntoBuffer(Metadata::ITypeResolver const&,
                                 std::vector<OwnedEvent>      &,
                                 OwnedEvent              const&,
                                 SizeType                const)
    {
        // TODO
    }

    void InsertElementIntoBuffer(Metadata::ITypeResolver const&,
                                 std::vector<OwnedField>      & buffer,
                                 OwnedField              const& newField,
                                 SizeType                const)
    {
        // In theory, fields are hide by name and sig, but the .NET Reflection API will return all
        // of them, regardless of whether they are technically hidden, so we do the same.
        buffer.push_back(newField);
    }

    void InsertElementIntoBuffer(Metadata::ITypeResolver     const&,
                                 std::vector<OwnedInterface>      &,
                                 OwnedInterface              const&,
                                 SizeType                    const)
    {
        // TODO
    }

    void InsertElementIntoBuffer(Metadata::ITypeResolver  const& typeResolver,
                                 std::vector<OwnedMethod>      & buffer,
                                 OwnedMethod              const& newMethod,
                                 SizeType                 const  inheritedMethodCount)
    {
        Metadata::MethodDefRow    const newMethodDef(newMethod.GetElementRow());
        Metadata::MethodSignature const newMethodSig(newMethod.GetElementSignature(typeResolver));

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
        auto const bufferIt(std::find_if(bufferBegin, buffer.rend(), [&](OwnedMethod const& oldMethod)
            -> bool
        {
            Metadata::MethodDefRow    const oldMethodDef(oldMethod.GetElementRow());
            Metadata::MethodSignature const oldMethodSig(oldMethod.GetElementSignature(typeResolver));

            // TODO Does this correctly handle hiding of base-class methods?
            if (!oldMethodDef.GetFlags().IsSet(MethodAttribute::Virtual))
                return false;

            if (oldMethodDef.GetName() != newMethodDef.GetName())
                return false;

            Metadata::SignatureComparer const compareSignatures(
                &typeResolver,
                &oldMethod.GetElement().GetDatabase(),
                &newMethod.GetElement().GetDatabase());

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

    void InsertElementIntoBuffer(Metadata::ITypeResolver    const&,
                                 std::vector<OwnedProperty>      &,
                                 OwnedProperty              const&,
                                 SizeType                   const)
    {
        // TODO
    }




    // BeginOwnedElements and EndOwnedElements encapsulate the creation of iterators to owned
    // elements. This allows us to handle varying levels of indirection and internal representation,
    // required to handle Methods and Fields (which have one layer of indirection from a Type),
    // Events and Properties which have two layers of indirection, and Interfaces, which have one
    // layer of indirection but are represented differently from the other element types.

    Metadata::RowIterator<Metadata::TableId::Event> BeginOwnedElements(Metadata::Database   const&,
                                                                       Metadata::TypeDefRow const&,
                                                                       Metadata::EventRow   const&)
    {
        throw LogicError(L"NYI"); // TODO
    }

    Metadata::RowIterator<Metadata::TableId::Event> EndOwnedElements(Metadata::Database   const&,
                                                                     Metadata::TypeDefRow const&,
                                                                     Metadata::EventRow   const&)
    {
        throw LogicError(L"NYI"); // TODO
    }

    Metadata::RowIterator<Metadata::TableId::Field> BeginOwnedElements(Metadata::Database   const& database,
                                                                       Metadata::TypeDefRow const& type,
                                                                       Metadata::FieldRow   const&)
    {
        Assert([&]{ return database.IsInitialized() && type.IsInitialized(); });

        return database.Begin<Metadata::TableId::Field>() + type.GetFirstField().GetIndex();
    }

    Metadata::RowIterator<Metadata::TableId::Field> EndOwnedElements(Metadata::Database   const& database,
                                                                     Metadata::TypeDefRow const& type,
                                                                     Metadata::FieldRow   const&)
    {
        Assert([&]{ return database.IsInitialized() && type.IsInitialized(); });

        return database.Begin<Metadata::TableId::Field>() + type.GetLastField().GetIndex();
    }

    Metadata::RowIterator<Metadata::TableId::InterfaceImpl> BeginOwnedElements(Metadata::Database         const& database,
                                                                               Metadata::TypeDefRow       const& type,
                                                                               Metadata::InterfaceImplRow const&)
    {
        Assert([&]{ return database.IsInitialized() && type.IsInitialized(); });

        /*
        auto const first(database.Begin<Metadata::TableId::InterfaceImpl>());
        auto const last (database.End  <Metadata::TableId::InterfaceImpl>());

        auto const range(EqualRange(first, last, type.GetColumnOffset
        */

        throw LogicError(L"NYI"); // TODO
    }

    Metadata::RowIterator<Metadata::TableId::InterfaceImpl> EndOwnedElements(Metadata::Database         const&,
                                                                             Metadata::TypeDefRow       const&,
                                                                             Metadata::InterfaceImplRow const&)
    {
        throw LogicError(L"NYI"); // TODO
    }

    Metadata::RowIterator<Metadata::TableId::MethodDef> BeginOwnedElements(Metadata::Database     const& database,
                                                                           Metadata::TypeDefRow   const& type,
                                                                           Metadata::MethodDefRow const&)
    {
        Assert([&]{ return database.IsInitialized() && type.IsInitialized(); });

        return database.Begin<Metadata::TableId::MethodDef>() + type.GetFirstMethod().GetIndex();
    }

    Metadata::RowIterator<Metadata::TableId::MethodDef> EndOwnedElements(Metadata::Database     const& database,
                                                                         Metadata::TypeDefRow   const& type,
                                                                         Metadata::MethodDefRow const&)
    {
        Assert([&]{ return database.IsInitialized() && type.IsInitialized(); });

        return database.Begin<Metadata::TableId::MethodDef>() + type.GetLastMethod().GetIndex();
    }

    Metadata::RowIterator<Metadata::TableId::Property> BeginOwnedElements(Metadata::Database    const&,
                                                                          Metadata::TypeDefRow  const&,
                                                                          Metadata::PropertyRow const&)
    {
        throw LogicError(L"NYI"); // TODO
    }

    Metadata::RowIterator<Metadata::TableId::Property> EndOwnedElements(Metadata::Database    const&,
                                                                        Metadata::TypeDefRow  const&,
                                                                        Metadata::PropertyRow const&)
    {
        throw LogicError(L"NYI"); // TODO
    }





    // GetSignatureForElement encapsulates the location of signatures for elements.
    // TODO More documentation

    template <typename TElementRow>
    auto GetSignatureForElement(Metadata::ITypeResolver const&,
                                Metadata::Database      const&,
                                TElementRow             const& row) -> decltype(row.GetSignature())
    {
        Assert([&]{ return row.IsInitialized(); });

        return row.GetSignature();
    }

    Metadata::BlobReference GetSignatureForElement(Metadata::ITypeResolver const&,
                                                   Metadata::Database      const&,
                                                   Metadata::EventRow      const&)
    {
        // Events do not have signatures for hiding purposes (their methods have signatures, though)
        return Metadata::BlobReference();
    }

    Metadata::BlobReference GetSignatureForElement(Metadata::ITypeResolver    const& typeResolver,
                                                   Metadata::Database         const& database,
                                                   Metadata::InterfaceImplRow const& interfaceImplRow)
    {
        Assert([&]{ return database.IsInitialized() && interfaceImplRow.IsInitialized(); });

        Metadata::FullReference const interfaceReference(&database, interfaceImplRow.GetInterface());
        Metadata::FullReference const resolvedInterface(typeResolver.ResolveType(interfaceReference));

        // If we resolved the interface to a TypeDef, it has no signature:
        if (resolvedInterface.AsRowReference().GetTable() == Metadata::TableId::TypeDef)
            return Metadata::BlobReference();

        // Otherwise, it is a TypeSpec, so we return the TypeSpec's signature:
        Metadata::TypeSpecRow const interfaceTypeSpec(resolvedInterface
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeSpec>(resolvedInterface));

        return interfaceTypeSpec.GetSignature();
    }


} } } }

namespace CxxReflect { namespace Detail {

    #define CXXREFLECT_GENERATE(r, m)                                                  \
        template <typename TElement, typename TElementRow, typename TElementSignature> \
        r OwnedElement<TElement, TElementRow, TElementSignature>::m

    CXXREFLECT_GENERATE(, OwnedElement)()
    {
    }

    CXXREFLECT_GENERATE(, OwnedElement)(Metadata::FullReference const& owningType,
                                        Metadata::FullReference const& element)
        : _owningType(owningType),
          _element(element)
    {
        Assert([&]{ return owningType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });
        Assert([&]{ return element.IsInitialized() && element.IsRowReference();                  });
    }

    CXXREFLECT_GENERATE(, OwnedElement)(Metadata::FullReference const& owningType,
                                        Metadata::FullReference const& element,
                                        Metadata::FullReference const& instantiatingType,
                                        ConstByteRange          const& instantiatedSignature)
        : _owningType(owningType),
          _element(element),
          _instantiatingType(instantiatingType),
          _instantiatedSignature(instantiatedSignature)
    {
        Assert([&]{ return owningType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });
        Assert([&]{ return element.IsInitialized() && element.IsRowReference();                  });
    }

    CXXREFLECT_GENERATE(TElement, Resolve)(Type const& reflectedType) const
    {
        AssertInitialized();
        return TElement(reflectedType, this, InternalKey());
    }

    CXXREFLECT_GENERATE(Metadata::FullReference, GetOwningType)() const
    {
        AssertInitialized();
        return _owningType;
    }

    CXXREFLECT_GENERATE(Metadata::FullReference, GetElement)() const
    {
        AssertInitialized();
        return _element;
    }

    CXXREFLECT_GENERATE(TElementRow, GetElementRow)() const
    {
        AssertInitialized();

        Metadata::TableId const TId(static_cast<Metadata::TableId>(Metadata::RowTypeToTableId<TElementRow>::Value));
        return _element.GetDatabase().GetRow<TId>(_element);
    }

    CXXREFLECT_GENERATE(TElementSignature, GetElementSignature)(Metadata::ITypeResolver const& typeResolver) const
    {
        AssertInitialized();
        if (HasInstantiatedSignature())
        {
            return TElementSignature(_instantiatedSignature.Begin(), _instantiatedSignature.End());
        }
        else
        {
            return _element
                .GetDatabase()
                .GetBlob(Private::GetSignatureForElement(typeResolver, _element.GetDatabase(), GetElementRow()))
                .As<TElementSignature>();
        }
    }

    CXXREFLECT_GENERATE(bool, HasInstantiatingType)() const
    {
        AssertInitialized();
        return _instantiatingType.IsInitialized();
    }

    CXXREFLECT_GENERATE(Metadata::FullReference, GetInstantiatingType)() const
    {
        Assert([&]{ return HasInstantiatingType(); });
        return _instantiatingType;
    }

    CXXREFLECT_GENERATE(bool, HasInstantiatedSignature)() const
    {
        AssertInitialized();
        return _instantiatedSignature.IsInitialized();
    }

    CXXREFLECT_GENERATE(ConstByteRange, GetInstantiatedSignature)() const
    {
        Assert([&]{ return HasInstantiatedSignature(); });
        return _instantiatedSignature;
    }

    CXXREFLECT_GENERATE(bool, IsInitialized)() const
    {
        return _owningType.IsInitialized() && _element.IsInitialized();
    }

    CXXREFLECT_GENERATE(void, AssertInitialized)() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    #undef CXXREFLECT_GENERATE

    template class OwnedElement<Event,    Metadata::EventRow,         Metadata::TypeSignature    >;
    template class OwnedElement<Field,    Metadata::FieldRow,         Metadata::FieldSignature   >;
    template class OwnedElement<Type,     Metadata::InterfaceImplRow, Metadata::TypeSignature    >;
    template class OwnedElement<Method,   Metadata::MethodDefRow,     Metadata::MethodSignature  >;
    template class OwnedElement<Property, Metadata::PropertyRow,      Metadata::PropertySignature>;





    template <typename T>
    OwnedElementTableCollection<T>::OwnedElementTableCollection(Metadata::ITypeResolver const* const typeResolver)
        : _typeResolver(typeResolver)
    {
        AssertInitialized();
    }

    template <typename T>
    OwnedElementTableCollection<T>::OwnedElementTableCollection(OwnedElementTableCollection&& other)
        : _typeResolver      (std::move(other._typeResolver      )),
          _signatureAllocator(std::move(other._signatureAllocator)),
          _tableAllocator    (std::move(other._tableAllocator    )),
          _index             (std::move(other._index             )),
          _buffer            (std::move(other._buffer            ))
    {
        AssertInitialized();
        other._typeResolver.Reset();
    }

    template <typename T>
    OwnedElementTableCollection<T>& OwnedElementTableCollection<T>::operator=(OwnedElementTableCollection&& other)
    {
        Swap(other);

        AssertInitialized();
        other._typeResolver.Reset();
        return *this;
    }

    template <typename T>
    void OwnedElementTableCollection<T>::Swap(OwnedElementTableCollection& other)
    {
        using std::swap;
        swap(other._typeResolver,       _typeResolver      );
        swap(other._signatureAllocator, _signatureAllocator);
        swap(other._tableAllocator,     _tableAllocator    );
        swap(other._index,              _index             );
        swap(other._buffer,             _buffer            );
    }

    template <typename T>
    bool OwnedElementTableCollection<T>::IsInitialized() const
    {
        return _typeResolver.Get() != nullptr;
    }

    template <typename T>
    void OwnedElementTableCollection<T>::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    template <typename T>
    typename OwnedElementTableCollection<T>::OwnedElementTableType
    OwnedElementTableCollection<T>::GetOrCreateTable(FullReference const& type) const
    {
        AssertInitialized();

        // First, handle the "Get" of "GetOrCreate".  If we already created a table, return it:
        auto const it(_index.find(type));
        if (it != end(_index))
            return it->second;

        // When this function returns, we want to clear the buffer so it is ready for our next use.
        // Note that we use resize(0) instead of clear() because in the Visual C++ 11 implementation
        // the latter will destroy the underlying storage.  We want to keep the buffer intact so
        // that we can reuse it.
        ScopeGuard const bufferCleanupGuard([&]{ _buffer.resize(0); });

        Private::TypeDefAndSpec const typeDefAndSpec(Private::ResolveTypeDefAndSpec(*_typeResolver.Get(), type));
        FullReference const& typeDefReference(typeDefAndSpec.GetTypeDef());
        FullReference const& typeSpecReference(typeDefAndSpec.GetTypeSpec());

        Metadata::Database const& database(typeDefReference.GetDatabase());

        SizeType const typeDefIndex(typeDefReference.AsRowReference().GetIndex());
        Metadata::TypeDefRow const typeDef(database.GetRow<Metadata::TableId::TypeDef>(typeDefIndex));

        Instantiator const instantiator(Private::CreateInstantiator(typeSpecReference));

        // First, recursively handle the base type hierarchy so that inherited members are emplaced
        // into the table first; this allows us to emulate runtime overriding and hiding behaviors.
        Metadata::RowReference const baseTypeReference(typeDef.GetExtends());
        if (baseTypeReference.IsValid())
        {
            OwnedElementTableType const table(GetOrCreateTable(FullReference(&database, baseTypeReference)));

            std::transform(table.Begin(), table.End(), std::back_inserter(_buffer), [&](OwnedElementType const& e)
                -> OwnedElementType
            {
                if (!instantiator.HasArguments() ||
                    !Instantiator::RequiresInstantiation(e.GetElementSignature(*_typeResolver.Get())))
                    return e;

                return OwnedElementType(
                    e.GetOwningType(),
                    e.GetElement(),
                    e.GetInstantiatingType(),
                    Instantiate(instantiator, e.GetElementSignature(*_typeResolver.Get())));
            });
        }

        SizeType const inheritedMemberCount(static_cast<SizeType>(_buffer.size()));

        // Second, enumerate the elements declared by this type itself (i.e., not inherited) and
        // insert them into the buffer in the correct location.

        auto const firstElement(Private::BeginOwnedElements(database, typeDef, ElementRowType()));
        auto const lastElement (Private::EndOwnedElements  (database, typeDef, ElementRowType()));

        std::for_each(firstElement, lastElement, [&](ElementRowType const& elementDef)
        {
            Metadata::BlobReference const signatureRef(Private::GetSignatureForElement(
                *_typeResolver.Get(),
                database,
                elementDef));

            Metadata::FullReference const elementDefReference(&database, elementDef.GetSelfReference());

            if (!signatureRef.IsInitialized())
                return OwnedElementType(typeDefReference, elementDefReference);

            ElementSignatureType const elementSig(database.GetBlob(signatureRef).As<ElementSignatureType>());

            bool const requiresInstantiation(
                instantiator.HasArguments() &&
                Instantiator::RequiresInstantiation(elementSig));

            ConstByteRange const instantiatedSig(requiresInstantiation
                ? Instantiate(instantiator, elementSig)
                : ConstByteRange(elementSig.BeginBytes(), elementSig.EndBytes()));

            OwnedElementType const ownedElement(instantiatedSig.IsInitialized()
                ? OwnedElementType(typeDefReference, elementDefReference, typeSpecReference, instantiatedSig)
                : OwnedElementType(typeDefReference, elementDefReference));

            InsertIntoBuffer(ownedElement, inheritedMemberCount);
        });

        OwnedElementTableType const range(_tableAllocator.Allocate(_buffer.size()));
        RangeCheckedCopy(_buffer.begin(), _buffer.end(), range.Begin(), range.End());

        _index.insert(std::make_pair(type, range));
        return range;
    }

    template <typename T>
    ConstByteRange OwnedElementTableCollection<T>::Instantiate(Instantiator         const& instantiator,
                                                               ElementSignatureType const& signature) const
    {
        Assert([&]{ return signature.IsInitialized(); });
        Assert([&]{ return Instantiator::RequiresInstantiation(signature); });

        ElementSignatureType const& instantiation(instantiator.Instantiate(signature));
        SizeType const instantiationSize(
            static_cast<SizeType>(std::distance(instantiation.BeginBytes(), instantiation.EndBytes())));

        ByteRange const ownedInstantiation(_signatureAllocator.Allocate(instantiationSize));

        RangeCheckedCopy(
            instantiation.BeginBytes(), instantiation.EndBytes(),
            ownedInstantiation.Begin(), ownedInstantiation.End());

        return ownedInstantiation;
    }

    template <typename T>
    void OwnedElementTableCollection<T>::InsertIntoBuffer(OwnedElementType const& newElement,
                                                          SizeType         const inheritedElementCount) const
    {
        return Private::InsertElementIntoBuffer(*_typeResolver.Get(), _buffer, newElement, inheritedElementCount);
    }

    template class OwnedElementTableCollection<OwnedEvent    >;
    template class OwnedElementTableCollection<OwnedField    >;
    template class OwnedElementTableCollection<OwnedInterface>;
    template class OwnedElementTableCollection<OwnedMethod   >;
    template class OwnedElementTableCollection<OwnedProperty >;

} }
