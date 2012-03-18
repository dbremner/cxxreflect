//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/ElementContexts.hpp"
#include "CxxReflect/Event.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

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





    // Provides a strict-weak ordering for the keys in the EventMap, InterfaceImpl, and PropertyMap
    // tables.  Rows from these tables can be compared against a RowReference.
    class KeyStrictWeakOrdering
    {
    public:

        bool operator()(Metadata::RowReference const& lhsClassRow, Metadata::RowReference const& rhsClassRow) const
        {
            return lhsClassRow < rhsClassRow;
        }

        template <typename TRow>
        bool operator()(Metadata::RowReference const& classRow, TRow const& owningRow) const
        {
            return classRow < GetOrderingKey(owningRow);
        }

        template <typename TRow>
        bool operator()(TRow const& owningRow, Metadata::RowReference const& classRow) const
        {
            return GetOrderingKey(owningRow) < classRow;
        }

        template <typename TRow>
        bool operator()(TRow const& lhsOwningRow, TRow const& rhsOwningRow) const
        {
            return GetOrderingKey(lhsOwningRow) < GetOrderingKey(rhsOwningRow);
        }

    private:

        Metadata::RowReference GetOrderingKey(Metadata::EventMapRow const& eventMap) const
        {
            return eventMap.GetParent();
        }

        Metadata::RowReference GetOrderingKey(Metadata::InterfaceImplRow const& interfaceImpl) const
        {
            return interfaceImpl.GetClass();
        }

        Metadata::RowReference GetOrderingKey(Metadata::PropertyMapRow const& propertyMap) const
        {
            return propertyMap.GetParent();
        }
    };





    typedef Metadata::RowIterator<Metadata::TableId::Event> EventIterator;
    typedef std::pair<EventIterator, EventIterator>         EventIteratorPair;

    EventIteratorPair GetEventsEqualRange(Metadata::TypeDefRow const& typeDef)
    {
        auto const first(typeDef.GetDatabase().Begin<Metadata::TableId::EventMap>());
        auto const last (typeDef.GetDatabase().End  <Metadata::TableId::EventMap>());

        auto const it(Detail::BinarySearch(first, last, typeDef.GetSelfReference(), KeyStrictWeakOrdering()));
        if (it == last)
            return EventIteratorPair(
                EventIterator(&typeDef.GetDatabase(), 0),
                EventIterator(&typeDef.GetDatabase(), 0));

        return std::make_pair(
            EventIterator(&typeDef.GetDatabase(), it->GetFirstEvent().GetIndex()),
            EventIterator(&typeDef.GetDatabase(), it->GetLastEvent().GetIndex()));
    }





    typedef Metadata::RowIterator<Metadata::TableId::InterfaceImpl> InterfaceImplIterator;
    typedef std::pair<InterfaceImplIterator, InterfaceImplIterator> InterfaceImplIteratorPair;
    
    InterfaceImplIteratorPair GetInterfacesEqualRange(Metadata::TypeDefRow const& typeDef)
    {
        auto const first(typeDef.GetDatabase().Begin<Metadata::TableId::InterfaceImpl>());
        auto const last (typeDef.GetDatabase().End  <Metadata::TableId::InterfaceImpl>());
        return std::equal_range(first, last, typeDef.GetSelfReference(), KeyStrictWeakOrdering());
    }





    typedef Metadata::RowIterator<Metadata::TableId::Property> PropertyIterator;
    typedef std::pair<PropertyIterator, PropertyIterator>      PropertyIteratorPair;

    PropertyIteratorPair GetPropertiesEqualRange(Metadata::TypeDefRow const& typeDef)
    {
        auto const first(typeDef.GetDatabase().Begin<Metadata::TableId::PropertyMap>());
        auto const last (typeDef.GetDatabase().End  <Metadata::TableId::PropertyMap>());

        auto const it(Detail::BinarySearch(first, last, typeDef.GetSelfReference(), KeyStrictWeakOrdering()));
        if (it == last)
            return PropertyIteratorPair(
                PropertyIterator(&typeDef.GetDatabase(), 0),
                PropertyIterator(&typeDef.GetDatabase(), 0));

        return std::make_pair(
            PropertyIterator(&typeDef.GetDatabase(), it->GetFirstProperty().GetIndex()),
            PropertyIterator(&typeDef.GetDatabase(), it->GetLastProperty().GetIndex()));
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

} } } }

namespace CxxReflect { namespace Detail {

    ElementContextTraits<EventContextTag>::RowIteratorType
    ElementContextTraits<EventContextTag>::BeginElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return Private::GetEventsEqualRange(typeDef).first;
    }

    ElementContextTraits<EventContextTag>::RowIteratorType
    ElementContextTraits<EventContextTag>::EndElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return Private::GetEventsEqualRange(typeDef).second;
    }

    Metadata::FullReference
    ElementContextTraits<EventContextTag>::GetSignature(Metadata::ITypeResolver const& typeResolver,
                                                        Metadata::EventRow      const& eventRow)
    {
        Assert([&]{ return eventRow.IsInitialized(); });

        Metadata::FullReference const originalType(&eventRow.GetDatabase(), eventRow.GetType());
        Metadata::FullReference const resolvedType(typeResolver.ResolveType(originalType));

        // If the interface is a TypeDef, it has no distinct signature, so we can simply return an
        // empty signature here:
        if (resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef)
            return Metadata::FullReference();

        // Otherwise, we have a TypeSpec, so we should return its signature:
        Metadata::TypeSpecRow const typeSpec(resolvedType
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeSpec>(resolvedType));

        return Metadata::FullReference(&typeSpec.GetDatabase(), typeSpec.GetSignature());
    }

    void
    ElementContextTraits<EventContextTag>::InsertElement(Metadata::ITypeResolver const&,
                                                         ContextSequenceType          & eventTable,
                                                         ContextType             const& newEvent,
                                                         SizeType                const)
    {
        Assert([&]{ return newEvent.IsInitialized(); });

        eventTable.push_back(newEvent); // TODO Handle hiding and overriding?
    }





    ElementContextTraits<FieldContextTag>::RowIteratorType
    ElementContextTraits<FieldContextTag>::BeginElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return RowIteratorType(&typeDef.GetDatabase(), typeDef.GetFirstField().GetIndex());
    }

    ElementContextTraits<FieldContextTag>::RowIteratorType
    ElementContextTraits<FieldContextTag>::EndElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return RowIteratorType(&typeDef.GetDatabase(), typeDef.GetLastField().GetIndex());
    }

    Metadata::FullReference
    ElementContextTraits<FieldContextTag>::GetSignature(Metadata::ITypeResolver const&,
                                                        Metadata::FieldRow  const& field)
    {
        Assert([&]{ return field.IsInitialized(); });

        return Metadata::FullReference(&field.GetDatabase(), field.GetSignature());
    }

    void
    ElementContextTraits<FieldContextTag>::InsertElement(Metadata::ITypeResolver const&,
                                                         ContextSequenceType          & fieldTable,
                                                         ContextType             const& newField,
                                                         SizeType                const)
    {
        Assert([&]{ return newField.IsInitialized(); });

        fieldTable.push_back(newField); // TODO Verify that this is correct.
    }





    ElementContextTraits<InterfaceContextTag>::RowIteratorType
    ElementContextTraits<InterfaceContextTag>::BeginElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return Private::GetInterfacesEqualRange(typeDef).first;
    }

    ElementContextTraits<InterfaceContextTag>::RowIteratorType
    ElementContextTraits<InterfaceContextTag>::EndElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return Private::GetInterfacesEqualRange(typeDef).second;
    }

    Metadata::FullReference
    ElementContextTraits<InterfaceContextTag>::GetSignature(Metadata::ITypeResolver    const& typeResolver,
                                                            Metadata::InterfaceImplRow const& interfaceImpl)
    {
        Assert([&]{ return interfaceImpl.IsInitialized(); });

        Metadata::FullReference const originalInterface(&interfaceImpl.GetDatabase(), interfaceImpl.GetInterface());
        Metadata::FullReference const resolvedInterface(typeResolver.ResolveType(originalInterface));

        // If the interface is a TypeDef, it has no distinct signature, so we can simply return an
        // empty signature here:
        if (resolvedInterface.AsRowReference().GetTable() == Metadata::TableId::TypeDef)
            return Metadata::FullReference();

        // Otherwise, we have a TypeSpec, so we should return its signature:
        Metadata::TypeSpecRow const interfaceTypeSpec(resolvedInterface
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeSpec>(resolvedInterface));

        return Metadata::FullReference(&resolvedInterface.GetDatabase(), interfaceTypeSpec.GetSignature());
    }

    void
    ElementContextTraits<InterfaceContextTag>::InsertElement(Metadata::ITypeResolver const& typeResolver,
                                                             ContextSequenceType          & interfaceTable,
                                                             ContextType             const& newInterface,
                                                             SizeType                const)
    {
        Assert([&]{ return newInterface.IsInitialized(); });

        auto const newInterfaceRow(newInterface.GetElementRow());
        auto const resolvedNewInterface(typeResolver.ResolveType(Metadata::FullReference(
            &newInterfaceRow.GetDatabase(),
            newInterfaceRow.GetInterface())));

        // Iterate over the interface table and see if it already contains 'newInterface' (this can
        // happen if two classes in the class hierarchy both implement an interface).  If there are
        // two classes that implement an interface, we keep the most derived one.
        auto const it(std::find_if(begin(interfaceTable), end(interfaceTable), [&](ContextType const& context) -> bool
        {
            auto const oldInterfaceRow(context.GetElementRow());
            auto const resolvedOldInterface(typeResolver.ResolveType(Metadata::FullReference(
                &oldInterfaceRow.GetDatabase(),
                oldInterfaceRow.GetInterface())));

            // If the old and new interfaces resolved to different kinds of Types, obviously they
            // are not the same (basically, one is a TypeSpec, the other is a TypeDef).
            if (resolvedOldInterface.AsRowReference().GetTable() != resolvedNewInterface.AsRowReference().GetTable())
                return false;

            // If both interfaces are TypeDefs, they are the same if and only if they point at the
            // same TypeDef row in the same database.  TODO Does this handle type forwarding?
            if (resolvedOldInterface.AsRowReference().GetTable() == Metadata::TableId::TypeDef)
                return resolvedOldInterface == resolvedNewInterface;

            auto const oldSignature(Private::GetTypeSpecSignature(resolvedOldInterface));
            auto const newSignature(Private::GetTypeSpecSignature(resolvedNewInterface));

            Metadata::SignatureComparer const compareSignatures(
                &typeResolver,
                &resolvedOldInterface.GetDatabase(),
                &resolvedNewInterface.GetDatabase());

            // If the signature of the method in the derived class is different from the signature
            // of the method in the base class, it is not an override:
            return compareSignatures(oldSignature, newSignature);
        }));

        return it == interfaceTable.end()
            ? (void)interfaceTable.push_back(newInterface)
            : (void)(*it = newInterface);
    }





    ElementContextTraits<MethodContextTag>::RowIteratorType
    ElementContextTraits<MethodContextTag>::BeginElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return RowIteratorType(&typeDef.GetDatabase(), typeDef.GetFirstMethod().GetIndex());
    }

    ElementContextTraits<MethodContextTag>::RowIteratorType
    ElementContextTraits<MethodContextTag>::EndElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return RowIteratorType(&typeDef.GetDatabase(), typeDef.GetLastMethod().GetIndex());
    }

    Metadata::FullReference
    ElementContextTraits<MethodContextTag>::GetSignature(Metadata::ITypeResolver const&,
                                                         Metadata::MethodDefRow  const& methodDef)
    {
        Assert([&]{ return methodDef.IsInitialized(); });

        return Metadata::FullReference(&methodDef.GetDatabase(), methodDef.GetSignature());
    }

    void
    ElementContextTraits<MethodContextTag>::InsertElement(Metadata::ITypeResolver const& typeResolver,
                                                          ContextSequenceType          & methodTable,
                                                          ContextType             const& newMethod,
                                                          SizeType                const  inheritedMethodCount)
    {
        Assert([&]{ return newMethod.IsInitialized();                  });
        Assert([&]{ return inheritedMethodCount <= methodTable.size(); });

        Metadata::MethodDefRow    const newMethodDef(newMethod.GetElementRow());
        Metadata::MethodSignature const newMethodSig(newMethod.GetElementSignature(typeResolver));

        // If the method occupies a new slot, it does not override any other method.  A static
        // method is always a new method.
        if (newMethodDef.GetFlags().WithMask(MethodAttribute::VTableLayoutMask) == MethodAttribute::NewSlot ||
            newMethodDef.GetFlags().IsSet(MethodAttribute::Static))
        {
            methodTable.push_back(newMethod);
            return;
        }

        bool isNewMethod(true);
        auto const methodTableBegin(methodTable.rbegin() + (methodTable.size() - inheritedMethodCount));
        auto const methodTableIt(std::find_if(methodTableBegin, methodTable.rend(), [&](MethodContext const& oldMethod)
            -> bool
        {
            Metadata::MethodDefRow    const oldMethodDef(oldMethod.GetElementRow());
            Metadata::MethodSignature const oldMethodSig(oldMethod.GetElementSignature(typeResolver));

            // Note that by skipping nonvirtual methods, we also skip the name hiding feature.  We
            // do not hide any names by name or signature; we only hide overridden virtual methods.
            // This matches the runtime reflection behavior of the CLR, not the compiler behavior.
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
            ? (void)methodTable.push_back(newMethod)
            : (void)(*methodTableIt = newMethod);
    }





    ElementContextTraits<PropertyContextTag>::RowIteratorType
    ElementContextTraits<PropertyContextTag>::BeginElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return Private::GetPropertiesEqualRange(typeDef).first;
    }

    ElementContextTraits<PropertyContextTag>::RowIteratorType
    ElementContextTraits<PropertyContextTag>::EndElements(Metadata::TypeDefRow const& typeDef)
    {
        Assert([&]{ return typeDef.IsInitialized(); });

        return Private::GetPropertiesEqualRange(typeDef).second;
    }

    Metadata::FullReference
    ElementContextTraits<PropertyContextTag>::GetSignature(Metadata::ITypeResolver const&,
                                                           Metadata::PropertyRow   const& propertyRow)
    {
        Assert([&]{ return propertyRow.IsInitialized(); });

        return Metadata::FullReference(&propertyRow.GetDatabase(), propertyRow.GetSignature());
    }

    void
    ElementContextTraits<PropertyContextTag>::InsertElement(Metadata::ITypeResolver const&,
                                                            ContextSequenceType          & propertyTable,
                                                            ContextType             const& newProperty,
                                                            SizeType                const)
    {
        Assert([&]{ return newProperty.IsInitialized(); });

        propertyTable.push_back(newProperty); // TODO Handle hiding and overriding?
    }





    template <typename T>
    ElementContext<T>::ElementContext()
    {
    }

    template <typename T>
    ElementContext<T>::ElementContext(Metadata::FullReference const& owningType,
                                      Metadata::FullReference const& element)
        : _owningType(owningType),
          _element(element)
    {
        Assert([&]{ return owningType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });
        Assert([&]{ return element.AsRowReference().GetTable() == TraitsType::RowTableId;        });
    }

    template <typename T>
    ElementContext<T>::ElementContext(Metadata::FullReference const& owningType,
                                      Metadata::FullReference const& element,
                                      Metadata::FullReference const& instantiatingType,
                                      ConstByteRange          const& instantiatedSignature)
        : _owningType(owningType),
          _element(element),
          _instantiatingType(instantiatingType),
          _instantiatedSignature(instantiatedSignature)
    {
        Assert([&]{ return owningType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });
        Assert([&]{ return element.AsRowReference().GetTable() == TraitsType::RowTableId;        });
    }

    template <typename T>
    typename ElementContext<T>::ResolvedType ElementContext<T>::Resolve(Type const& reflectedType) const
    {
        AssertInitialized();
        return ResolvedType(reflectedType, this, InternalKey());
    }

    template <typename T>
    Metadata::FullReference ElementContext<T>::GetOwningType() const
    {
        AssertInitialized();
        return _owningType;
    }

    template <typename T>
    Metadata::FullReference ElementContext<T>::GetElement() const
    {
        AssertInitialized();
        return _element;
    }

    template <typename T>
    typename ElementContext<T>::RowType ElementContext<T>::GetElementRow() const
    {
        AssertInitialized();
        return _element.GetDatabase().GetRow<TraitsType::RowTableId>(_element);
    }

    template <typename T>
    typename ElementContext<T>::SignatureType
    ElementContext<T>::GetElementSignature(Metadata::ITypeResolver const& typeResolver) const
    {
        AssertInitialized();
        if (HasInstantiatedSignature())
        {
            return SignatureType(_instantiatedSignature.Begin(), _instantiatedSignature.End());
        }

        Metadata::FullReference const signatureReference(TraitsType::GetSignature(typeResolver, GetElementRow()));
        if (!signatureReference.IsInitialized())
        {
            return SignatureType();
        }

        return _element
            .GetDatabase()
            .GetBlob(signatureReference.AsBlobReference())
            .As<SignatureType>();
    }

    template <typename T>
    bool ElementContext<T>::HasInstantiatingType() const
    {
        AssertInitialized();
        return _instantiatingType.IsInitialized();
    }

    template <typename T>
    Metadata::FullReference ElementContext<T>::GetInstantiatingType() const
    {
        Assert([&]{ return HasInstantiatingType(); });
        return _instantiatingType;
    }

    template <typename T>
    bool ElementContext<T>::HasInstantiatedSignature() const
    {
        AssertInitialized();
        return _instantiatedSignature.IsInitialized();
    }

    template <typename T>
    ConstByteRange ElementContext<T>::GetInstantiatedSignature() const
    {
        Assert([&]{ return HasInstantiatedSignature(); });
        return _instantiatedSignature;
    }

    template <typename T>
    bool ElementContext<T>::IsInitialized() const
    {
        return _owningType.IsInitialized() && _element.IsInitialized();
    }

    template <typename T>
    void ElementContext<T>::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }





    template class ElementContext<EventContextTag    >;
    template class ElementContext<FieldContextTag    >;
    template class ElementContext<InterfaceContextTag>;
    template class ElementContext<MethodContextTag   >;
    template class ElementContext<PropertyContextTag >;





    template <typename T>
    ElementContextTableCollection<T>::ElementContextTableCollection(Metadata::ITypeResolver const*    const typeResolver,
                                                                    ElementContextSignatureAllocator* const signatureAllocator)
        : _typeResolver(typeResolver),
          _signatureAllocator(signatureAllocator)
    {
        AssertNotNull(typeResolver);
        AssertNotNull(signatureAllocator);
    }

    template <typename T>
    ElementContextTableCollection<T>::ElementContextTableCollection(ElementContextTableCollection&& other)
        : _typeResolver      (std::move(other._typeResolver      )),
          _signatureAllocator(std::move(other._signatureAllocator)),
          _tableAllocator    (std::move(other._tableAllocator    )),
          _index             (std::move(other._index             )),
          _buffer            (std::move(other._buffer            ))
    {
        AssertInitialized();
        other._typeResolver.Reset();
        other._signatureAllocator.Reset();
    }

    template <typename T>
    ElementContextTableCollection<T>& ElementContextTableCollection<T>::operator=(ElementContextTableCollection&& other)
    {
        Swap(other);

        AssertInitialized();
        other._typeResolver.Reset();
        other._signatureAllocator.Reset();
        return *this;
    }

    template <typename T>
    void ElementContextTableCollection<T>::Swap(ElementContextTableCollection& other)
    {
        using std::swap;
        swap(other._typeResolver,       _typeResolver      );
        swap(other._signatureAllocator, _signatureAllocator);
        swap(other._tableAllocator,     _tableAllocator    );
        swap(other._index,              _index             );
        swap(other._buffer,             _buffer            );
    }

    template <typename T>
    bool ElementContextTableCollection<T>::IsInitialized() const
    {
        return _typeResolver.Get() != nullptr;
    }

    template <typename T>
    void ElementContextTableCollection<T>::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    template <typename T>
    typename ElementContextTableCollection<T>::ContextTableType
    ElementContextTableCollection<T>::GetOrCreateTable(FullReference const& type) const
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
            ContextTableType const table(GetOrCreateTable(FullReference(&database, baseTypeReference)));

            std::transform(table.Begin(), table.End(), std::back_inserter(_buffer), [&](ContextType const& e)
                -> ContextType
            {
                if (!instantiator.HasArguments() ||
                    !e.GetElementSignature(*_typeResolver.Get()).IsInitialized() ||
                    !Instantiator::RequiresInstantiation(e.GetElementSignature(*_typeResolver.Get())))
                    return e;

                return ContextType(
                    e.GetOwningType(),
                    e.GetElement(),
                    e.GetInstantiatingType(),
                    Instantiate(instantiator, e.GetElementSignature(*_typeResolver.Get())));
            });
        }

        SizeType const inheritedMemberCount(static_cast<SizeType>(_buffer.size()));

        // Second, enumerate the elements declared by this type itself (i.e., not inherited) and
        // insert them into the buffer in the correct location.

        auto const firstElement(TraitsType::BeginElements(typeDef));
        auto const lastElement (TraitsType::EndElements  (typeDef));

        std::for_each(firstElement, lastElement, [&](RowType const& elementDef)
        {
            Metadata::FullReference const signatureRef(TraitsType::GetSignature(
                *_typeResolver.Get(), 
                elementDef));

            Metadata::FullReference const elementDefReference(&database, elementDef.GetSelfReference());

            if (!signatureRef.IsInitialized())
                return InsertIntoBuffer(ContextType(typeDefReference, elementDefReference), inheritedMemberCount);

            SignatureType const elementSig(database.GetBlob(signatureRef.AsBlobReference()).As<SignatureType>());

            bool const requiresInstantiation(
                instantiator.HasArguments() &&
                Instantiator::RequiresInstantiation(elementSig));

            ConstByteRange const instantiatedSig(requiresInstantiation
                ? Instantiate(instantiator, elementSig)
                : ConstByteRange(elementSig.BeginBytes(), elementSig.EndBytes()));

            ContextType const ownedElement(instantiatedSig.IsInitialized()
                ? ContextType(typeDefReference, elementDefReference, typeSpecReference, instantiatedSig)
                : ContextType(typeDefReference, elementDefReference));

            return InsertIntoBuffer(ownedElement, inheritedMemberCount);
        });

        ContextTableType const range(_tableAllocator.Allocate(_buffer.size()));
        RangeCheckedCopy(_buffer.begin(), _buffer.end(), range.Begin(), range.End());

        _index.insert(std::make_pair(type, range));
        return range;
    }

    template <typename T>
    ConstByteRange ElementContextTableCollection<T>::Instantiate(Instantiator  const& instantiator,
                                                                 SignatureType const& signature) const
    {
        Assert([&]{ return signature.IsInitialized(); });
        Assert([&]{ return Instantiator::RequiresInstantiation(signature); });

        SignatureType const& instantiation(instantiator.Instantiate(signature));
        SizeType const instantiationSize(
            static_cast<SizeType>(std::distance(instantiation.BeginBytes(), instantiation.EndBytes())));

        ByteRange const ownedInstantiation(_signatureAllocator.Get()->Allocate(instantiationSize));

        RangeCheckedCopy(
            instantiation.BeginBytes(), instantiation.EndBytes(),
            ownedInstantiation.Begin(), ownedInstantiation.End());

        return ownedInstantiation;
    }

    template <typename T>
    void ElementContextTableCollection<T>::InsertIntoBuffer(ContextType const& newElement,
                                                            SizeType    const inheritedElementCount) const
    {
        return TraitsType::InsertElement(*_typeResolver.Get(), _buffer, newElement, inheritedElementCount);
    }





    template class ElementContextTableCollection<EventContextTag    >;
    template class ElementContextTableCollection<FieldContextTag    >;
    template class ElementContextTableCollection<InterfaceContextTag>;
    template class ElementContextTableCollection<MethodContextTag   >;
    template class ElementContextTableCollection<PropertyContextTag >;

} }
