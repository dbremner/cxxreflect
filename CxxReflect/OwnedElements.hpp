#ifndef CXXREFLECT_OWNEDELEMENTS_HPP_
#define CXXREFLECT_OWNEDELEMENTS_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/MetadataSignature.hpp"

namespace CxxReflect {

    class InternalKey;

    class Event;
    class Field;
    class Loader;
    class Method;
    class Property;
    class Type;

}

namespace CxxReflect { namespace Detail {

    template <typename TElement, typename TElementRow, typename TElementSignature>
    class OwnedElement
    {
    public:

        typedef TElement          ElementType;
        typedef TElementRow       ElementRowType;
        typedef TElementSignature ElementSignatureType;

        OwnedElement();

        OwnedElement(Metadata::FullReference const& owningType,
                     Metadata::FullReference const& element);

        OwnedElement(Metadata::FullReference const& owningType,
                     Metadata::FullReference const& element,
                     Metadata::FullReference const& instantiatingType,
                     ConstByteRange          const& instantiatedSignature);


        // Resolves the owned element to its public interface type (e.g. a MethodDef -> Method).
        ElementType             Resolve(Type const& reflectedType) const;

        // Returns the TypeDef that owns this element.  For members, this is the declaring type. For
        // interfaces, this is the type that implements the interface.
        Metadata::FullReference GetOwningType()            const;

        // Returns the owned element.  For members, this is the primary row that defines the member.
        // For interfaces, this is the TypeDef or TypeSpec row that defines the interface.
        Metadata::FullReference GetElement()               const;
        ElementRowType          GetElementRow()            const;
        ElementSignatureType    GetElementSignature(Metadata::ITypeResolver const& typeResolver) const;

        // If this element is generic in its owning type but is instantiated in this derived type,
        // these will provide the type that instantiated the element and the instantiated signature.
        bool                    HasInstantiatingType()     const;
        Metadata::FullReference GetInstantiatingType()     const;

        bool                    HasInstantiatedSignature() const;
        ConstByteRange          GetInstantiatedSignature() const;

        bool                    IsInitialized()            const;

    private:

        void                    AssertInitialized()        const;

        Metadata::FullReference _owningType;
        Metadata::FullReference _element;

        Metadata::FullReference _instantiatingType;
        ConstByteRange          _instantiatedSignature;
    };

    typedef OwnedElement<Event,    Metadata::EventRow,         Metadata::TypeSignature    > OwnedEvent;
    typedef OwnedElement<Field,    Metadata::FieldRow,         Metadata::FieldSignature   > OwnedField;
    typedef OwnedElement<Type,     Metadata::InterfaceImplRow, Metadata::TypeSignature    > OwnedInterface;
    typedef OwnedElement<Method,   Metadata::MethodDefRow,     Metadata::MethodSignature  > OwnedMethod;
    typedef OwnedElement<Property, Metadata::PropertyRow,      Metadata::PropertySignature> OwnedProperty;

    typedef Range<OwnedEvent>     OwnedEventTable;
    typedef Range<OwnedField>     OwnedFieldTable;
    typedef Range<OwnedInterface> OwnedInterfaceTable;
    typedef Range<OwnedMethod>    OwnedMethodTable;
    typedef Range<OwnedProperty>  OwnedPropertyTable;

    template <typename TOwnedElement>
    class OwnedElementTableCollection
    {
    public:

        typedef typename TOwnedElement::ElementType               ElementType;
        typedef typename TOwnedElement::ElementRowType            ElementRowType;
        typedef typename TOwnedElement::ElementSignatureType      ElementSignatureType;

        typedef TOwnedElement                                     OwnedElementType;
        typedef Range<TOwnedElement>                              OwnedElementTableType;
        
        typedef LinearArrayAllocator<Byte,             (1 << 16)> SignatureAllocator;
        typedef LinearArrayAllocator<OwnedElementType, (1 << 11)> TableAllocator;

        typedef Metadata::ClassVariableSignatureInstantiator      Instantiator;
        typedef Metadata::FullReference                           FullReference;

        OwnedElementTableCollection(Metadata::ITypeResolver const* typeResolver);
        OwnedElementTableCollection(OwnedElementTableCollection&& other);
        OwnedElementTableCollection& operator=(OwnedElementTableCollection&& other);

        void Swap(OwnedElementTableCollection& other);

        OwnedElementTableType GetOrCreateTable(FullReference const& type) const;

        bool IsInitialized() const;

    private:

        void AssertInitialized() const;

        typedef std::map<FullReference, OwnedElementTableType> IndexType;
        typedef std::vector<OwnedElementType>                  BufferType;

        OwnedElementTableCollection(OwnedElementTableCollection const&);
        OwnedElementTableCollection& operator=(OwnedElementTableCollection const&);

        ConstByteRange Instantiate(Instantiator const& instantiator, ElementSignatureType const& signature) const;
        void InsertIntoBuffer(OwnedElementType const& newElement, SizeType const inheritedElementCount) const;

        ValueInitialized<Metadata::ITypeResolver const*>         _typeResolver;
        SignatureAllocator                               mutable _signatureAllocator;
        TableAllocator                                   mutable _tableAllocator;
        IndexType                                        mutable _index;
        BufferType                                       mutable _buffer;
    };

    typedef OwnedElementTableCollection<OwnedEvent    > OwnedEventTableCollection;
    typedef OwnedElementTableCollection<OwnedField    > OwnedFieldTableCollection;
    typedef OwnedElementTableCollection<OwnedInterface> OwnedInterfaceTableCollection;
    typedef OwnedElementTableCollection<OwnedMethod   > OwnedMethodTableCollection;
    typedef OwnedElementTableCollection<OwnedProperty > OwnedPropertyTableCollection;

} }

#endif
