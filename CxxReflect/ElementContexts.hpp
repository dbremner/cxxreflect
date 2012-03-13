#ifndef CXXREFLECT_ELEMENTCONTEXTS_HPP_
#define CXXREFLECT_ELEMENTCONTEXTS_HPP_

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

    // Tag types used to identify the different instantiations of ElementContext and related classes
    struct EventContextTag     { };
    struct FieldContextTag     { };
    struct InterfaceContextTag { };
    struct MethodContextTag    { };
    struct PropertyContextTag  { };

    template <typename TContextTag>
    class ElementContext;

    // The ElementContextTraits defines the types and operations for each instantiation of the
    // ElementContext class.  These provide all of the specific implementation details that enable
    // use of the extremely general ElementContext class.
    template <typename TContextTag>
    class ElementContextTraits
    {
    };

    template <>
    class ElementContextTraits<EventContextTag>
    {
    public:

        static const Metadata::TableId RowTableId = Metadata::TableId::Event;

        typedef EventContextTag                      TagType;
        typedef Event                                ResolvedType;
        typedef Metadata::EventRow                   RowType;
        typedef Metadata::RowIterator<RowTableId>    RowIteratorType;
        typedef Metadata::TypeSignature              SignatureType;

        typedef ElementContext<TagType>              ContextType;
        typedef std::vector<ContextType>             ContextSequenceType;

        static RowIteratorType         BeginElements(Metadata::TypeDefRow    const& typeDef);
        static RowIteratorType         EndElements  (Metadata::TypeDefRow    const& typeDef);

        static Metadata::FullReference GetSignature (Metadata::ITypeResolver const& typeResolver,
                                                     Metadata::EventRow      const& eventRow);

        static void                    InsertElement(Metadata::ITypeResolver const& typeResolver,
                                                     ContextSequenceType          & elementTable,
                                                     ContextType             const& newElement,
                                                     SizeType                const  inheritedElementCount);
    };

    template <>
    class ElementContextTraits<FieldContextTag>
    {
    public:

        static const Metadata::TableId RowTableId = Metadata::TableId::Field;

        typedef FieldContextTag                      TagType;
        typedef Field                                ResolvedType;
        typedef Metadata::FieldRow                   RowType;
        typedef Metadata::RowIterator<RowTableId>    RowIteratorType;
        typedef Metadata::FieldSignature             SignatureType;

        typedef ElementContext<TagType>              ContextType;
        typedef std::vector<ContextType>             ContextSequenceType;

        static RowIteratorType         BeginElements(Metadata::TypeDefRow    const& typeDef);
        static RowIteratorType         EndElements  (Metadata::TypeDefRow    const& typeDef);

        static Metadata::FullReference GetSignature (Metadata::ITypeResolver const& typeResolver,
                                                       Metadata::FieldRow    const& fieldRow);

        static void                    InsertElement(Metadata::ITypeResolver const& typeResolver,
                                                     ContextSequenceType          & elementTable,
                                                     ContextType             const& newElement,
                                                     SizeType                const  inheritedElementCount);
    };

    template <>
    class ElementContextTraits<InterfaceContextTag>
    {
    public:

        typedef Type                          ResolvedType;
        typedef Metadata::InterfaceImplRow    RowType;
        typedef Metadata::TypeSignature       SignatureType;

        static const Metadata::TableId RowTableId = Metadata::TableId::InterfaceImpl;

        typedef InterfaceContextTag                  TagType;
        typedef Type                                 ResolvedType;
        typedef Metadata::InterfaceImplRow           RowType;
        typedef Metadata::RowIterator<RowTableId>    RowIteratorType;
        typedef Metadata::TypeSignature              SignatureType;

        typedef ElementContext<TagType>              ContextType;
        typedef std::vector<ContextType>             ContextSequenceType;

        static RowIteratorType         BeginElements(Metadata::TypeDefRow       const& typeDef);
        static RowIteratorType         EndElements  (Metadata::TypeDefRow       const& typeDef);

        static Metadata::FullReference GetSignature (Metadata::ITypeResolver    const& typeResolver,
                                                     Metadata::InterfaceImplRow const& interfaceImpl);

        static void                    InsertElement(Metadata::ITypeResolver    const& typeResolver,
                                                     ContextSequenceType             & elementTable,
                                                     ContextType                const& newElement,
                                                     SizeType                   const  inheritedElementCount);
    };

    template <>
    class ElementContextTraits<MethodContextTag>
    {
    public:

        static const Metadata::TableId RowTableId = Metadata::TableId::MethodDef;

        typedef MethodContextTag                     TagType;
        typedef Method                               ResolvedType;
        typedef Metadata::MethodDefRow               RowType;
        typedef Metadata::RowIterator<RowTableId>    RowIteratorType;
        typedef Metadata::MethodSignature            SignatureType;

        typedef ElementContext<TagType>              ContextType;
        typedef std::vector<ContextType>             ContextSequenceType;

        static RowIteratorType         BeginElements(Metadata::TypeDefRow    const& typeDef);
        static RowIteratorType         EndElements  (Metadata::TypeDefRow    const& typeDef);

        static Metadata::FullReference GetSignature (Metadata::ITypeResolver const& typeResolver,
                                                     Metadata::MethodDefRow  const& methodDef);

        static void                    InsertElement(Metadata::ITypeResolver const& typeResolver,
                                                     ContextSequenceType          & elementTable,
                                                     ContextType             const& newElement,
                                                     SizeType                const  inheritedElementCount);
    };

    template <>
    class ElementContextTraits<PropertyContextTag>
    {
    public:

        static const Metadata::TableId RowTableId = Metadata::TableId::Property;

        typedef PropertyContextTag                   TagType;
        typedef Property                             ResolvedType;
        typedef Metadata::PropertyRow                RowType;
        typedef Metadata::RowIterator<RowTableId>    RowIteratorType;
        typedef Metadata::PropertySignature          SignatureType;

        typedef ElementContext<TagType>              ContextType;
        typedef std::vector<ContextType>             ContextSequenceType;

        static RowIteratorType         BeginElements(Metadata::TypeDefRow    const& typeDef);
        static RowIteratorType         EndElements  (Metadata::TypeDefRow    const& typeDef);

        static Metadata::FullReference GetSignature (Metadata::ITypeResolver const& typeResolver,
                                                     Metadata::PropertyRow   const& propertyRow);

        static void                    InsertElement(Metadata::ITypeResolver const& typeResolver,
                                                     ContextSequenceType          & elementTable,
                                                     ContextType             const& newElement,
                                                     SizeType                const  inheritedElementCount);
    };





    // The ElementContext represents an "owned" element.  That is, something that is owned by a type
    // in the metadata. For example, one instantiation of ElementContext represents a Method because
    // a type owns a collection of methods (the "collection" part is represented by the next classes
    // ElementContextTable).
    template <typename TContextTag>
    class ElementContext
    {
    public:

        typedef TContextTag                           TagType;
        typedef ElementContextTraits<TContextTag>     TraitsType;
        typedef typename TraitsType::ResolvedType     ResolvedType;
        typedef typename TraitsType::RowType          RowType;
        typedef typename TraitsType::SignatureType    SignatureType;

        ElementContext();

        ElementContext(Metadata::FullReference const& owningType,
                       Metadata::FullReference const& element);

        ElementContext(Metadata::FullReference const& owningType,
                       Metadata::FullReference const& element,
                       Metadata::FullReference const& instantiatingType,
                       ConstByteRange          const& instantiatedSignature);

        ResolvedType            Resolve(Type const& reflectedType) const;

        // This returns the type that owns the element.  For members, this is the declaring type. 
        // For interfaces, this is the type that declares that it implements the interface.
        Metadata::FullReference GetOwningType()            const;
        
        // These return the full reference to the owned element's declaration, the row in which the
        // element is declared, and the signature of the element, if it has one.
        Metadata::FullReference GetElement()               const;
        RowType                 GetElementRow()            const;
        SignatureType           GetElementSignature(Metadata::ITypeResolver const& typeResolver) const;

        // If the element is declared as generic but has been instantiated in the reflected type or
        // one of its base classes, these will provide the type that instantiated the element and
        // the instantiated signature.
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





    // The instantiations for which ElementContext is explicitly instantiated:
    typedef ElementContext<EventContextTag    > EventContext;
    typedef ElementContext<FieldContextTag    > FieldContext;
    typedef ElementContext<InterfaceContextTag> InterfaceContext;
    typedef ElementContext<MethodContextTag   > MethodContext;
    typedef ElementContext<PropertyContextTag > PropertyContext;





    // An [Element]ContextTable represents the sequence of [Element]s that are owned by a type.  For
    // example, a MethodContextTable contains all of the methods owned by the type.
    typedef Range<EventContext    > EventContextTable;
    typedef Range<FieldContext    > FieldContextTable;
    typedef Range<InterfaceContext> InterfaceContextTable;
    typedef Range<MethodContext   > MethodContextTable;
    typedef Range<PropertyContext > PropertyContextTable;





    typedef LinearArrayAllocator<Byte, (1 << 16)> ElementContextSignatureAllocator;





    // An [Element]ContextTableCollection is simply a collection of ElementContextTables.  It owns
    // the tables (and thus their lifetime is bound to the lifetime of the collection) and caches
    // results of generating the tables for faster lookup.
    //
    // TODO We should provide a way to rollback to a previous cache state.  This should be fairly
    // simple since we use a linear cache without any internal reallocation of data.
    //
    // TODO We should try to use a single allocator for all of the ElementContext types; this would
    // substantially cut back the overhead for small type universes.
    template <typename TContextTag>
    class ElementContextTableCollection
    {
    public:

        typedef TContextTag                                  TagType;
        typedef ElementContextTraits<TContextTag>            TraitsType;
        typedef ElementContext<TContextTag>                  ContextType;
        typedef Range<ContextType>                           ContextTableType;

        typedef typename TraitsType::RowType                 RowType;
        typedef typename TraitsType::SignatureType           SignatureType;

        typedef ElementContextSignatureAllocator             SignatureAllocator;
        typedef LinearArrayAllocator<ContextType, (1 << 11)> TableAllocator;

        typedef Metadata::ClassVariableSignatureInstantiator Instantiator;
        typedef Metadata::FullReference                      FullReference;

        ElementContextTableCollection(Metadata::ITypeResolver const* typeResolver,
                                      SignatureAllocator           * signatureAllocator);

        ElementContextTableCollection(ElementContextTableCollection&& other);
        ElementContextTableCollection& operator=(ElementContextTableCollection&& other);

        void Swap(ElementContextTableCollection& other);

        ContextTableType GetOrCreateTable(FullReference const& type) const;

        bool IsInitialized() const;

    private:

        typedef std::map<FullReference, ContextTableType>    IndexType;
        typedef std::vector<ContextType>                     BufferType;

        ElementContextTableCollection(ElementContextTableCollection const&);
        ElementContextTableCollection& operator=(ElementContextTableCollection const&);

        ConstByteRange Instantiate(Instantiator const& instantiator, SignatureType const& signature) const;
        void InsertIntoBuffer(ContextType const& newElement, SizeType const inheritedElementCount) const;

        void AssertInitialized() const;

        ValueInitialized<Metadata::ITypeResolver const*>            _typeResolver;
        ValueInitialized<ElementContextSignatureAllocator*>         _signatureAllocator;
        TableAllocator                                      mutable _tableAllocator;
        IndexType                                           mutable _index;
        BufferType                                          mutable _buffer;
    };





    typedef ElementContextTableCollection<EventContextTag    > EventContextTableCollection;
    typedef ElementContextTableCollection<FieldContextTag    > FieldContextTableCollection;
    typedef ElementContextTableCollection<InterfaceContextTag> InterfaceContextTableCollection;
    typedef ElementContextTableCollection<MethodContextTag   > MethodContextTableCollection;
    typedef ElementContextTableCollection<PropertyContextTag > PropertyContextTableCollection;

} }

#endif
