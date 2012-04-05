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

#include <mutex>

namespace CxxReflect { namespace Detail { namespace { namespace Private {

    // A helper function which, given a TypeSpec, returns its TypeSignature.
    Metadata::TypeSignature GetTypeSpecSignature(Metadata::FullReference const& type)
    {
        Assert([&]{ return type.IsInitialized() && type.IsRowReference();                   });
        Assert([&]{ return type.AsRowReference().GetTable() == Metadata::TableId::TypeSpec; });

        Metadata::TypeSpecRow const typeSpec(type
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeSpec>(type));

        Metadata::TypeSignature const typeSignature(typeSpec.GetSignature().As<Metadata::TypeSignature>());

        return typeSignature;
    }





    // A pair that contains a TypeDef and a TypeSpec.  The TypeDef is guaranteed to be present, but
    // the TypeSpec may or may not be present.  This pair type is returned by ResolveTypeDefAndSpec.
    class TypeDefAndSignature
    {
    public:

        TypeDefAndSignature(Metadata::FullReference const& typeDef)
            : _typeDef(typeDef)
        {
            Assert([&]{ return typeDef.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });
        }

        TypeDefAndSignature(Metadata::FullReference const& typeDef, Metadata::FullReference const& typeSignature)
            : _typeDef      (typeDef      ),
              _typeSignature(typeSignature)
        {
            Assert([&]{ return typeDef.AsRowReference().GetTable() == Metadata::TableId::TypeDef;  });
            Assert([&]{ return typeSignature.IsBlobReference();                                    });
        }

        Metadata::FullReference const& GetTypeDef()       const { return _typeDef;                       }
        Metadata::FullReference const& GetTypeSignature() const { return _typeSignature;                 }
        bool                           HasTypeSignature() const { return _typeSignature.IsInitialized(); }

    private:

        Metadata::FullReference _typeDef;
        Metadata::FullReference _typeSignature;
    };





    // Resolves 'originalType' to its TypeSpec and primary TypeDef components.  The behavior of this
    // function depends on what 'originalType' is.  If it is a...
    //  * ...TypeDef, it is returned unchanged (no TypeSpec is returned).
    //  * ...TypeSpec, it must be a GenericInst, and the GenericInst's GenericTypeReference is
    //       returned as the TypeDef and the TypeSpec is returned as the TypeSpec.
    //  * ...TypeRef, it is resolved to the TypeDef or TypeSpec to which it refers.  The function
    //       then behaves as if that TypeDef or TypeSpec was passed directly into this function.
    TypeDefAndSignature ResolveTypeDefAndSignature(Metadata::ITypeResolver const& typeResolver,
                                                   Metadata::FullReference const& originalType)
    {
        Assert([&]{ return originalType.IsInitialized(); });

        if (originalType.IsRowReference())
        {
            // Resolve the original type; this will give us either a TypeDef or a TypeSpec:
            Metadata::FullReference const resolvedType(typeResolver.ResolveType(originalType));

            // If we resolve 'type' to a TypeDef, there is no TypeSpec so we can just return the TypeDef:
            if (resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef)
                return TypeDefAndSignature(resolvedType);

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

            return TypeDefAndSignature(reResolvedType, Metadata::FullReference(
                &resolvedType.GetDatabase(),
                Metadata::BlobReference(GetTypeSpecSignature(resolvedType))));
        }
        else if (originalType.IsBlobReference())
        {
            Metadata::BlobReference originalBlob(originalType.AsBlobReference());

            Metadata::TypeSignature const typeSignature(originalBlob.Begin(), originalBlob.End());

            // We are only expecting to resolve to a base class, so we only expect a GenericInst:
            Verify([&]{ return typeSignature.GetKind() == Metadata::TypeSignature::Kind::GenericInst; });

            // Re-resolve the generic type reference to the TypeDef it instantiates:
            Metadata::FullReference const reResolvedType(typeResolver.ResolveType(Metadata::FullReference(
                &originalType.GetDatabase(),
                typeSignature.GetGenericTypeReference())));

            // A GenericInst should refer to a TypeDef or a TypeRef, never another TypeSpec.  We resolve
            // the TypeRef above, so at this point we should always have a TypeDef:
            Verify([&]{ return reResolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });

            return TypeDefAndSignature(reResolvedType, Metadata::FullReference(
                &originalType.GetDatabase(),
                Metadata::BlobReference(originalBlob.Begin(), originalBlob.End())));
        }
        else
        {
            throw LogicError(L"Unreachable code");
        }
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





    // This recursive element inserter creates elements using the TCreateElement functor, inserts
    // them into a buffer using the TInsertElement functor, then finally recurses, getting the next
    // set of elements to be processed from the element that was just processed.  We do not recurse
    // for all element types (for the moment, we only recurse for InterfaceContexts).
    template <typename TTraits, typename TCreateElement, typename TInsertElement>
    class RecursiveElementInserter
    {
    public:

        RecursiveElementInserter(Metadata::ITypeResolver const* const typeResolver,
                                 TCreateElement                 const create,
                                 TInsertElement                 const insert)
            : _typeResolver(typeResolver), _create(create), _insert(insert)
        {
        }

        template <typename T>
        void operator()(T const& x)
        {
            ContextType const newContext(_create(x));
            _insert(newContext);

            Recurse(newContext);
        }

    private:

        typedef typename TTraits::ContextType ContextType;

        // This type is copy constructable but not assignable because the TCreateElement and
        // TInsertElement types are likely to be non-assignable lambda types.  This is kind of ugly.
        RecursiveElementInserter& operator=(RecursiveElementInserter const&);

        // By default, we don't actually recurse.  We provide nontemplate overloads for the context
        // types for which we are going to recurse.
        template <typename T>
        void Recurse(T const&)
        {
        }

        // For an interface element, we get the interface type and enumerate all of the interfaces
        // that it implements.
        void Recurse(InterfaceContext const& context)
        {
            Metadata::FullReference const interfaceRef(
                &context.GetElement().GetDatabase(),
                context.GetElementRow().GetInterface());

            TypeDefAndSignature const typeDefAndSig(ResolveTypeDefAndSignature(*_typeResolver.Get(), interfaceRef));

            Metadata::TypeDefRow const typeDefRow(typeDefAndSig
                .GetTypeDef()
                .GetDatabase()
                .GetRow<Metadata::TableId::TypeDef>(typeDefAndSig.GetTypeDef()));

            auto const firstElement(TTraits::BeginElements(typeDefRow));
            auto const lastElement (TTraits::EndElements  (typeDefRow));

            std::for_each(firstElement, lastElement, *this);
        }

        Detail::ValueInitialized<Metadata::ITypeResolver const*> _typeResolver;
        TCreateElement _create;
        TInsertElement _insert;
    };

    template <typename TTraits, typename TCreateElement, typename TInsertElement>
    RecursiveElementInserter<TTraits, TCreateElement, TInsertElement>
    CreateRecursiveElementInserter(Metadata::ITypeResolver const* typeResolver, TCreateElement create, TInsertElement insert)
    {
        return RecursiveElementInserter<TTraits, TCreateElement, TInsertElement>(typeResolver, create, insert);
    }





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
        if (!type.IsInitialized() || !type.IsBlobReference())
            return Metadata::ClassVariableSignatureInstantiator();

        Metadata::TypeSignature const typeSignature(type.AsBlobReference().Begin(), type.AsBlobReference().End());

        // We are only expecting to get base classes here, so it should be a GenericInst TypeSpec:
        Verify([&]{ return typeSignature.GetKind() == Metadata::TypeSignature::Kind::GenericInst; });

        return Metadata::ClassVariableSignatureInstantiator(
            &type.GetDatabase(),
            typeSignature.BeginGenericArguments(),
            typeSignature.EndGenericArguments());
    }

    template <typename TStorage, typename TSignature, typename TInstantiator>
    ConstByteRange Instantiate(TStorage& storage, TSignature const& signature, TInstantiator const& instantiator)
    {
        Assert([&]{ return signature.IsInitialized(); });
        Assert([&]{ return TInstantiator::RequiresInstantiation(signature); });

        TSignature const& instantiation(instantiator.Instantiate(signature));
        return storage.AllocateSignature(instantiation.BeginBytes(), instantiation.EndBytes());
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

        return signatureReference.AsBlobReference().As<SignatureType>();
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





    class ElementContextTableStorage
    {
    public:
        
        typedef std::recursive_mutex        MutexType;
        typedef std::unique_lock<MutexType> LockType;

        typedef LinearArrayAllocator<Byte,             (1 << 16)> SignatureStorage;

        typedef LinearArrayAllocator<EventContext,     (1 << 12)> EventContextStorage;
        typedef LinearArrayAllocator<FieldContext,     (1 << 12)> FieldContextStorage;
        typedef LinearArrayAllocator<InterfaceContext, (1 << 12)> InterfaceContextStorage;
        typedef LinearArrayAllocator<MethodContext,    (1 << 12)> MethodContextStorage;
        typedef LinearArrayAllocator<PropertyContext,  (1 << 12)> PropertyContextStorage;

        typedef std::map<Metadata::FullReference, EventContextTable    > EventContextIndex;
        typedef std::map<Metadata::FullReference, FieldContextTable    > FieldContextIndex;
        typedef std::map<Metadata::FullReference, InterfaceContextTable> InterfaceContextIndex;
        typedef std::map<Metadata::FullReference, MethodContextTable   > MethodContextIndex;
        typedef std::map<Metadata::FullReference, PropertyContextTable > PropertyContextIndex;

        class StorageLock
        {
        public:
            
            StorageLock(StorageLock&& other)
                : _storage(other._storage), _lock(std::move(other._lock))
            {
                other._storage.Get() = nullptr;
            }

            StorageLock& operator=(StorageLock&& other)
            {
                _lock = std::move(other._lock);
                _storage = other._storage;
                other._storage.Get() = nullptr;
            }

            ConstByteRange AllocateSignature(ConstByteIterator const first, ConstByteIterator const last) const
            {
                AssertInitialized();
                
                auto& storage(_storage.Get()->_signatureStorage);
                auto const range(storage.Allocate(std::distance(first, last)));
                Detail::RangeCheckedCopy(first, last, range.Begin(), range.End());
                return range;
            }

            template <typename TContextTag, typename TForwardIterator>
            Range<ElementContext<TContextTag>>
            AllocateTable(Metadata::FullReference const& type, TForwardIterator const first, TForwardIterator const last) const
            {
                AssertInitialized();

                auto& storage(GetStorage(TContextTag()));
                auto const range(storage.Allocate(std::distance(first, last)));
                Detail::RangeCheckedCopy(first, last, range.Begin(), range.End());
                GetIndex(TContextTag()).insert(std::make_pair(type, range));
                return range;
            }

            template <typename TContextTag>
            std::pair<bool, Range<ElementContext<TContextTag>>> FindTable(Metadata::FullReference const& type) const
            {
                AssertInitialized();

                auto const& index(GetIndex(TContextTag()));
                auto const it(index.find(type));
                if (it == index.end())
                    return std::make_pair(false, Default());

                return std::make_pair(true, it->second);
            }

            bool IsInitialized() const
            {
                return _storage.Get() != nullptr;
            }

        private:

            EventContextIndex    & GetIndex(EventContextTag    ) const { return _storage.Get()->_eventIndex;     }
            FieldContextIndex    & GetIndex(FieldContextTag    ) const { return _storage.Get()->_fieldIndex;     }
            InterfaceContextIndex& GetIndex(InterfaceContextTag) const { return _storage.Get()->_interfaceIndex; }
            MethodContextIndex   & GetIndex(MethodContextTag   ) const { return _storage.Get()->_methodIndex;    }
            PropertyContextIndex & GetIndex(PropertyContextTag ) const { return _storage.Get()->_propertyIndex;  }

            EventContextStorage    & GetStorage(EventContextTag    ) const { return _storage.Get()->_eventStorage;     }
            FieldContextStorage    & GetStorage(FieldContextTag    ) const { return _storage.Get()->_fieldStorage;     }
            InterfaceContextStorage& GetStorage(InterfaceContextTag) const { return _storage.Get()->_interfaceStorage; }
            MethodContextStorage   & GetStorage(MethodContextTag   ) const { return _storage.Get()->_methodStorage;    }
            PropertyContextStorage & GetStorage(PropertyContextTag ) const { return _storage.Get()->_propertyStorage;  }

            friend ElementContextTableStorage;

            StorageLock(ElementContextTableStorage const* const storage)
                : _storage(storage), _lock(storage->_sync)
            {
            }

            // This class is noncopyable (it is, however, moveable)
            StorageLock(StorageLock const&);
            StorageLock& operator=(StorageLock const&);

            void AssertInitialized() const
            {
                Detail::Assert([&]{ return IsInitialized(); });
            }

            Detail::ValueInitialized<ElementContextTableStorage const*> _storage;
            LockType                                                    _lock;
        };

        ElementContextTableStorage()
        {
        }

        StorageLock Lock() const
        {
            return StorageLock(this);
        }

    private:

        ElementContextTableStorage(ElementContextTableStorage const&);
        ElementContextTableStorage& operator=(ElementContextTableStorage const&);

        SignatureStorage        mutable _signatureStorage;

        // TODO We could combine all five of these into a single allocator, since each context type
        // has exactly the same data members.  We could also merge all of the indices together and
        // have each type map to a set of five tables, one for each element type.  This would help
        // to cut back on the number of dynamic allocations.
        EventContextStorage     mutable _eventStorage;
        FieldContextStorage     mutable _fieldStorage;
        InterfaceContextStorage mutable _interfaceStorage;
        MethodContextStorage    mutable _methodStorage;
        PropertyContextStorage  mutable _propertyStorage;

        EventContextIndex       mutable _eventIndex;
        FieldContextIndex       mutable _fieldIndex;
        InterfaceContextIndex   mutable _interfaceIndex;
        MethodContextIndex      mutable _methodIndex;
        PropertyContextIndex    mutable _propertyIndex;

        MutexType               mutable _sync;
    };

    ElementContextTableStorageInstance CreateElementContextTableStorage()
    {
        ElementContextTableStorageInstance instance(new ElementContextTableStorage());
        return instance;
    }

    void ElementContextTableStorageDeleter::operator()(ElementContextTableStorage const volatile* p) const volatile
    {
        delete p;
    }





    template <typename T>
    ElementContextTableCollection<T>::ElementContextTableCollection(Metadata::ITypeResolver    const* const typeResolver,
                                                                    ElementContextTableStorage const* const storage)
        : _typeResolver(typeResolver),
          _storage(storage)
    {
        AssertNotNull(typeResolver);
        AssertNotNull(storage);
    }

    template <typename T>
    ElementContextTableCollection<T>::ElementContextTableCollection(ElementContextTableCollection&& other)
        : _typeResolver(std::move(other._typeResolver)),
          _storage     (std::move(other._storage     ))
    {
        AssertInitialized();
        other._typeResolver.Reset();
        other._storage.Reset();
    }

    template <typename T>
    ElementContextTableCollection<T>& ElementContextTableCollection<T>::operator=(ElementContextTableCollection&& other)
    {
        other.AssertInitialized();

        // All of these are pointer operations; none will throw.
        _typeResolver = other._typeResolver;
        _storage = other._storage;

        other._typeResolver.Reset();
        other._storage.Reset();

        return *this;
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

        ElementContextTableStorage::StorageLock storage(_storage.Get()->Lock());

        // First, handle the "Get" of "GetOrCreate".  If we already created a table, return it:
        auto const result(storage.FindTable<TagType>(type));
        if (result.first)
            return result.second;

        std::vector<ContextType> newTable;

        Private::TypeDefAndSignature const typeDefAndSig(Private::ResolveTypeDefAndSignature(*_typeResolver.Get(), type));
        FullReference const& typeDefReference(typeDefAndSig.GetTypeDef());
        FullReference const& typeSigReference(typeDefAndSig.GetTypeSignature());

        Metadata::Database const& database(typeDefReference.GetDatabase());

        SizeType const typeDefIndex(typeDefReference.AsRowReference().GetIndex());
        Metadata::TypeDefRow const typeDef(database.GetRow<Metadata::TableId::TypeDef>(typeDefIndex));

        Instantiator const instantiator(Private::CreateInstantiator(typeSigReference));

        // First, recursively handle the base type hierarchy so that inherited members are emplaced
        // into the table first; this allows us to emulate runtime overriding and hiding behaviors.
        Metadata::RowReference const baseTypeReference(typeDef.GetExtends());
        if (baseTypeReference.IsValid())
        {
            ContextTableType const table(GetOrCreateTable(FullReference(&database, baseTypeReference)));

            std::transform(table.Begin(), table.End(), std::back_inserter(newTable), [&](ContextType const& e)
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
                    Private::Instantiate(storage, e.GetElementSignature(*_typeResolver.Get()), instantiator));
            });
        }

        SizeType const inheritedElementCount(static_cast<SizeType>(newTable.size()));

        auto const createElement([&](RowType const& elementDef) -> ContextType
        {
            Metadata::FullReference const signatureRef(TraitsType::GetSignature(
                *_typeResolver.Get(), 
                elementDef));

            Metadata::FullReference const elementDefReference(&database, elementDef.GetSelfReference());

            if (!signatureRef.IsInitialized())
                return ContextType(typeDefReference, elementDefReference);

            SignatureType const elementSig(signatureRef.AsBlobReference().As<SignatureType>());

            bool const requiresInstantiation(
                instantiator.HasArguments() &&
                Instantiator::RequiresInstantiation(elementSig));

            ConstByteRange const instantiatedSig(requiresInstantiation
                ? Private::Instantiate(storage, elementSig, instantiator)
                : ConstByteRange(elementSig.BeginBytes(), elementSig.EndBytes()));

            return instantiatedSig.IsInitialized()
                ? ContextType(typeDefReference, elementDefReference, typeSigReference, instantiatedSig)
                : ContextType(typeDefReference, elementDefReference);
        });

        auto const insertIntoBuffer([&](ContextType const& element) -> void
        {
            TraitsType::InsertElement(*_typeResolver.Get(), newTable, element, inheritedElementCount);
        });

        // Second, enumerate the elements declared by this type itself (i.e., not inherited) and
        // insert them into the buffer in the correct location.
        RowIteratorType const firstElement(TraitsType::BeginElements(typeDef));
        RowIteratorType const lastElement (TraitsType::EndElements  (typeDef));

        // TODO This function has become a disaster. :'(
        auto const recursiveInserter(Private::CreateRecursiveElementInserter<TraitsType>(
            _typeResolver.Get(),
            createElement,
            insertIntoBuffer));

        std::for_each(firstElement, lastElement, recursiveInserter);

        return storage.AllocateTable<TagType>(type, newTable.begin(), newTable.end());
    }





    template class ElementContextTableCollection<EventContextTag    >;
    template class ElementContextTableCollection<FieldContextTag    >;
    template class ElementContextTableCollection<InterfaceContextTag>;
    template class ElementContextTableCollection<MethodContextTag   >;
    template class ElementContextTableCollection<PropertyContextTag >;

} }
