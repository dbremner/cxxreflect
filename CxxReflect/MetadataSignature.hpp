//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Functionality for reading and parsing element signatures from metadata blobs.
#ifndef CXXREFLECT_METADATASIGNATURE_HPP_
#define CXXREFLECT_METADATASIGNATURE_HPP_

#include "CxxReflect/MetadataSignature.hpp"

namespace CxxReflect { namespace Metadata {

    enum class SignatureAttribute : std::uint8_t
    {
        HasThis               = 0x20,
        ExplicitThis          = 0x40,

        // Calling Conventions
        CallingConventionMask = 0x0f,
        Default               = 0x00,
        C                     = 0x01,
        StdCall               = 0x02,
        ThisCall              = 0x03,
        FastCall              = 0x04,
        VarArg                = 0x05,

        Field                 = 0x06,
        Local                 = 0x07,
        Property              = 0x08,

        Generic               = 0x10,

        Sentinel              = 0x41
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(SignatureAttribute)

    typedef Detail::FlagSet<SignatureAttribute> SignatureFlags;

    enum class ElementType : std::uint8_t
    {
        End                        = 0x00,
        Void                       = 0x01,
        Boolean                    = 0x02,
        Char                       = 0x03,
        I1                         = 0x04,
        U1                         = 0x05,
        I2                         = 0x06,
        U2                         = 0x07,
        I4                         = 0x08,
        U4                         = 0x09,
        I8                         = 0x0a,
        U8                         = 0x0b,
        R4                         = 0x0c,
        R8                         = 0x0d,
        String                     = 0x0e,
        Ptr                        = 0x0f,
        ByRef                      = 0x10,
        ValueType                  = 0x11,
        Class                      = 0x12,
        Var                        = 0x13,
        Array                      = 0x14,
        GenericInst                = 0x15,
        TypedByRef                 = 0x16,

        I                          = 0x18,
        U                          = 0x19,
        FnPtr                      = 0x1b,
        Object                     = 0x1c,

        ConcreteElementTypeMax     = 0x1d,

        SzArray                    = 0x1d,
        MVar                       = 0x1e,

        CustomModifierRequired     = 0x1f,
        CustomModifierOptional     = 0x20,

        Internal                   = 0x21,
        Modifier                   = 0x40,
        Sentinel                   = 0x41,
        Pinned                     = 0x45,

        Type                       = 0x50,
        CustomAttributeBoxedObject = 0x51,
        CustomAttributeField       = 0x53,
        CustomAttributeProperty    = 0x54,
        CustomAttributeEnum        = 0x55
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(ElementType);

    inline bool IsValidElementType(Byte const id)
    {
        static std::array<Byte, 0x60> const mask =
        {
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        };

        return id < mask.size() && mask[id] == 1;
    }

    // Tests whether a given element type marks the beginning of a Type signature.
    inline bool IsTypeElementType(Byte const id)
    {
        static std::array<Byte, 0x20> const mask =
        {
            0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0
        };

        return id < mask.size() && mask[id] == 1;
    }





    // A generic iterator that reads elements from a sequence via FMaterialize until FSentinelCheck
    // returns false.  This is used for sequences of elements where the sequence is terminated by
    // failing to read another element (e.g. CustomMod sequences).
    template <typename TValue,
              TValue(*FMaterialize)(ConstByteIterator&, ConstByteIterator),
              bool(*FSentinelCheck)(ConstByteIterator,  ConstByteIterator)>
    class SentinelIterator
    {
    public:

        typedef TValue                    value_type;
        typedef TValue const&             reference;
        typedef TValue const*             pointer;
        typedef std::ptrdiff_t            difference_type;
        typedef std::forward_iterator_tag iterator_category;

        SentinelIterator()
        {
        }

        SentinelIterator(ConstByteIterator const current, ConstByteIterator const last)
            : _current(current), _last(last)
        {
            if (current != last)
                Materialize();
        }

        reference operator*()  const { return _value.Get();  }
        pointer   operator->() const { return &_value.Get(); }

        SentinelIterator& operator++()
        {
            Materialize();
        }

        SentinelIterator operator++(int)
        {
            SentinelIterator const it(*this);
            ++*this;
            return it;
        }

        friend bool operator==(SentinelIterator const& lhs, SentinelIterator const& rhs)
        {
            return lhs._current.Get() == rhs._current.Get();
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(SentinelIterator)

    private:

        void Materialize()
        {
            if (FSentinelCheck(_current.Get(), _last.Get()))
            {
                _current.Get() = nullptr;
                _last.Get()    = nullptr;
            }
            else
            {
                _value.Get() = FMaterialize(_current.Get(), _last.Get());
            }
        }

        Detail::ValueInitialized<ConstByteIterator> _current;
        Detail::ValueInitialized<ConstByteIterator> _last;

        Detail::ValueInitialized<value_type>   _value;
    };





    // A default sentinel check that always returns false.  This is useful for iteration over a
    // sequence where the count is known and exact (e.g., there is no sentinel for which to look.
    template <typename T>
    bool AlwaysFalseSentinelCheck(T, T)
    {
        return false;
    }





    // An iterator that yields elements up to a certain number (the count) or until a sentinel is
    // read from the sequence (verified via the FSentinelCheck).
    template <typename TValue,
              TValue(*FMaterialize)(ConstByteIterator&, ConstByteIterator),
              bool(*FSentinelCheck)(ConstByteIterator,  ConstByteIterator) = &AlwaysFalseSentinelCheck<ConstByteIterator>>
    class CountingIterator
    {
    public:

        typedef TValue                    value_type;
        typedef TValue const&             reference;
        typedef TValue const*             pointer;
        typedef std::ptrdiff_t            difference_type;
        typedef std::forward_iterator_tag iterator_category;

        CountingIterator()
        {
        }

        CountingIterator(ConstByteIterator const current,
                         ConstByteIterator const last,
                         SizeType          const index,
                         SizeType          const count)
            : _current(current), _last(last), _index(index), _count(count)
        {
            if (current != last && index != count)
                Materialize();
        }

        reference operator*()  const { return _value.Get();  }
        pointer   operator->() const { return &_value.Get(); }

        CountingIterator& operator++()
        {
            ++_index.Get();
            if (_index.Get() != _count.Get())
                Materialize();

            return *this;
        }

        CountingIterator operator++(int)
        {
            CountingIterator const it(*this);
            ++*this;
            return it;
        }

        friend bool operator==(CountingIterator const& lhs, CountingIterator const& rhs)
        {
            if (lhs._current.Get() == rhs._current.Get())
                return true;

            if (lhs.IsEnd() && rhs.IsEnd())
                return true;

            return false;
        }

        CXXREFLECT_GENERATE_EQUALITY_OPERATORS(CountingIterator)

    private:

        bool IsEnd() const
        {
            if (_current.Get() == nullptr)
                return true;

            // Note that we do not check whether _current == _last because _current always points
            // one past the current element (we materialize the current element on-the-fly).  Also,
            // the index check is sufficient to identify an end iterator.

            if (_index.Get() == _count.Get())
                return true;

            return false;
        }

        void Materialize()
        {
            if (FSentinelCheck(_current.Get(), _last.Get()))
            {
                _current.Get() = nullptr;
                _last.Get()    = nullptr;
            }
            else
            {
                _value.Get() = FMaterialize(_current.Get(), _last.Get());
            }
        }

        Detail::ValueInitialized<ConstByteIterator> _current;
        Detail::ValueInitialized<ConstByteIterator> _last;

        Detail::ValueInitialized<SizeType>          _index;
        Detail::ValueInitialized<SizeType>          _count;

        Detail::ValueInitialized<value_type>        _value;
    };





    // All of the signature types share a few common members; those that also have a common
    // implementation are in this base class.  Note that we do not use polymorphic classes for
    // signatures, so members that are present in all signature types but do not have a common
    // implementation (e.g. SeekTo or ComputeSize) are not present in this base class.
    class BaseSignature
    {
    public:

        ConstByteIterator BeginBytes() const;
        ConstByteIterator EndBytes()   const;

        bool IsInitialized() const;

    protected:

        BaseSignature();
        BaseSignature(ConstByteIterator first, ConstByteIterator last);
        
        BaseSignature(BaseSignature const& other);
        BaseSignature& operator=(BaseSignature const& other);

        ~BaseSignature();

        void AssertInitialized() const;

    private:

        Detail::ValueInitialized<ConstByteIterator> _first;
        Detail::ValueInitialized<ConstByteIterator> _last;
    };





    // Represents an ArrayShape signature item
    class ArrayShape
        : public BaseSignature
    {
    private:

        static SizeType ReadSize(ConstByteIterator& current, ConstByteIterator last);
        static SizeType ReadLowBound(ConstByteIterator& current, ConstByteIterator last);

    public:

        enum class Part
        {
            Begin,
            Rank,
            NumSizes,
            FirstSize,
            NumLowBounds,
            FirstLowBound,
            End
        };

        typedef CountingIterator<SizeType, &ArrayShape::ReadSize>     SizeIterator;
        typedef CountingIterator<SizeType, &ArrayShape::ReadLowBound> LowBoundIterator;

        ArrayShape();
        ArrayShape(ConstByteIterator first, ConstByteIterator last);

        SizeType          GetRank()           const;

        SizeType          GetSizesCount()     const;
        SizeIterator      BeginSizes()        const;
        SizeIterator      EndSizes()          const;

        SizeType          GetLowBoundsCount() const;
        LowBoundIterator  BeginLowBounds()    const;
        LowBoundIterator  EndLowBounds()      const;

        SizeType          ComputeSize()       const;
        ConstByteIterator SeekTo(Part part)   const;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(ArrayShape::Part)





    // Represents a CustomMod signature item
    class CustomModifier
        : public BaseSignature
    {
    public:

        enum class Part
        {
            Begin,
            ReqOptFlag,
            Type,
            End
        };

        CustomModifier();
        CustomModifier(ConstByteIterator first, ConstByteIterator last);

        bool              IsOptional()       const;
        bool              IsRequired()       const;
        RowReference      GetTypeReference() const;

        SizeType          ComputeSize()      const;
        ConstByteIterator SeekTo(Part part)  const;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(CustomModifier::Part)





    // Represents a FieldSig signature item; note that the optional CustomMod sequence is contained
    // in TypeSignature itself and is thus not exposed directly here.
    class FieldSignature
        : public BaseSignature
    {
    public:

        enum Part
        {
            Begin,
            FieldTag,
            Type,
            End
        };

        FieldSignature();
        FieldSignature(ConstByteIterator first, ConstByteIterator last);

        TypeSignature     GetTypeSignature() const;
        SizeType          ComputeSize()      const;
        ConstByteIterator SeekTo(Part part)  const;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(FieldSignature::Part);





    // Represents a PropertySig signature item; note that the optional CustomMod sequence is
    // contained in TypeSignature itself and is thus not exposed directly here.
    class PropertySignature
        : public BaseSignature
    {
    private:

        static TypeSignature ReadParameter(ConstByteIterator& current, ConstByteIterator last);

    public:

        typedef CountingIterator<TypeSignature, &PropertySignature::ReadParameter> ParameterIterator;

        enum Part
        {
            Begin,
            PropertyTag,
            ParameterCount,
            Type,
            FirstParameter,
            End
        };

        PropertySignature();
        PropertySignature(ConstByteIterator first, ConstByteIterator last);

        bool              HasThis()           const;
        SizeType          GetParameterCount() const;
        ParameterIterator BeginParameters()   const;
        ParameterIterator EndParameters()     const;
        TypeSignature     GetTypeSignature()  const;

        SizeType          ComputeSize()       const;
        ConstByteIterator SeekTo(Part part)   const;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(PropertySignature::Part);





    // Represents a MethodDefSig, MethodRefSig, or StandAloneMethodSig.
    class MethodSignature
        : public BaseSignature
    {
    private:

        static bool ParameterEndCheck(ConstByteIterator current, ConstByteIterator last);
        static TypeSignature ReadParameter(ConstByteIterator& current, ConstByteIterator last);

    public:

        typedef CountingIterator<
            TypeSignature,
            &MethodSignature::ReadParameter,
            &MethodSignature::ParameterEndCheck
        > ParameterIterator;

        enum class Part
        {
            Begin,
            TypeTag,
            GenParamCount,
            ParamCount,
            RetType,
            FirstParam,
            Sentinel,
            FirstVarargParam,
            End
        };

        MethodSignature();
        MethodSignature(ConstByteIterator first, ConstByteIterator last);

        bool HasThis()         const;
        bool HasExplicitThis() const;

        // Calling conventions; exactly one of these will be true.
        SignatureAttribute GetCallingConvention() const;
        bool HasDefaultConvention()  const;
        bool HasVarArgConvention()   const;
        bool HasCConvention()        const;
        bool HasStdCallConvention()  const;
        bool HasThisCallConvention() const;
        bool HasFastCallConvention() const;

        bool     IsGeneric()                const;
        SizeType GetGenericParameterCount() const;

        TypeSignature     GetReturnType()         const;
        SizeType          GetParameterCount()     const;
        ParameterIterator BeginParameters()       const;
        ParameterIterator EndParameters()         const;
        ParameterIterator BeginVarargParameters() const;
        ParameterIterator EndVarargParameters()   const;

        SizeType          ComputeSize()           const;
        ConstByteIterator SeekTo(Part part)       const;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(MethodSignature::Part)





    // A 'Type' signature item, with support for an optional prefix sequence of CustomModifier items,
    // an optional BYREF tag before the type proper, and either a TYPEDBYREF or VOID type in place of
    // a real type.  This supports Param, RetType, Type, and TypeSpec signatures in entirety, as well
    // as the core parts of FieldSig and PropertySig.
    class TypeSignature
        : public BaseSignature
    {
    private:

        static bool CustomModifierEndCheck(ConstByteIterator current, ConstByteIterator last);
        static CustomModifier ReadCustomModifier(ConstByteIterator& current, ConstByteIterator last);
        static TypeSignature ReadType(ConstByteIterator& current, ConstByteIterator last);

    public:

        enum class Kind
        {
            Mask        = 0xff00,

            Unknown     = 0x0000,

            Primitive   = 0x0100,
            Array       = 0x0200,
            SzArray     = 0x0300,
            ClassType   = 0x0400,
            FnPtr       = 0x0500,
            GenericInst = 0x0600,
            Ptr         = 0x0700,
            Var         = 0x0800
        };

        enum class Part
        {
            Begin          = 0x00,
            FirstCustomMod = 0x01,
            ByRefTag       = 0x02,

            // The TypeCode marks the start of the actual 'Type' signature element
            TypeCode       = 0x03,

            ArrayType                = static_cast<SizeType>(Kind::Array)       + 0x04,
            ArrayShape               = static_cast<SizeType>(Kind::Array)       + 0x05,

            SzArrayType              = static_cast<SizeType>(Kind::SzArray)     + 0x04,

            ClassTypeReference       = static_cast<SizeType>(Kind::ClassType)   + 0x04,

            MethodSignature          = static_cast<SizeType>(Kind::FnPtr)       + 0x04,

            GenericInstTypeCode      = static_cast<SizeType>(Kind::GenericInst) + 0x04,
            GenericInstTypeReference = static_cast<SizeType>(Kind::GenericInst) + 0x05,
            GenericInstArgumentCount = static_cast<SizeType>(Kind::GenericInst) + 0x06,
            FirstGenericInstArgument = static_cast<SizeType>(Kind::GenericInst) + 0x07,

            PointerTypeSignature     = static_cast<SizeType>(Kind::Ptr)         + 0x04,

            VariableNumber           = static_cast<SizeType>(Kind::Var)         + 0x04,

            End            = 0x08
        };

        typedef SentinelIterator<
            CustomModifier,
            &TypeSignature::ReadCustomModifier,
            &TypeSignature::CustomModifierEndCheck
        > CustomModifierIterator;

        typedef CountingIterator<TypeSignature, &TypeSignature::ReadType> GenericArgumentIterator;

        TypeSignature();
        TypeSignature(ConstByteIterator first, ConstByteIterator last);

        SizeType          ComputeSize()     const;
        ConstByteIterator SeekTo(Part part) const;
        Kind              GetKind()         const;
        bool              IsKind(Kind kind) const;

        // FieldSig, PropertySig, Param, RetType signatures, and PTR and SZARRAY Type signatures:
        CustomModifierIterator BeginCustomModifiers() const;
        CustomModifierIterator EndCustomModifiers()   const;

        // Param and RetType signatures:
        bool IsByRef() const;

        // BOOLEAN, CHAR, I1, U1, I2, U2, I4, U4, I8, U8, R4, R8, I, U, OBJECT, and STRING (also,
        // VOID for RetType signatures and TYPEDBYREF for Param and RetType signatures).
        bool        IsPrimitive()             const;
        ElementType GetPrimitiveElementType() const;

        // ARRAY, SZARRAY:
        bool          IsGeneralArray() const;
        bool          IsSimpleArray()  const;
        TypeSignature GetArrayType()   const;
        ArrayShape    GetArrayShape()  const; // ARRAY only

        // CLASS and VALUETYPE:
        bool         IsClassType()      const;
        bool         IsValueType()      const;
        RowReference GetTypeReference() const;

        // FNPTR:
        bool            IsFunctionPointer()  const;
        MethodSignature GetMethodSignature() const;

        // GENERICINST:
        bool                    IsGenericInstance()          const;
        bool                    IsGenericClassTypeInstance() const;
        bool                    IsGenericValueTypeInstance() const;
        RowReference            GetGenericTypeReference()    const;
        SizeType                GetGenericArgumentCount()    const;
        GenericArgumentIterator BeginGenericArguments()      const;
        GenericArgumentIterator EndGenericArguments()        const;

        // PTR:
        bool          IsPointer()               const;
        TypeSignature GetPointerTypeSignature() const;

        // MVAR and VAR:
        bool     IsClassVariableType()  const;
        bool     IsMethodVariableType() const;
        SizeType GetVariableNumber()    const;

    private:

        ElementType GetElementType() const;

        void AssertKind(Kind kind) const;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(TypeSignature::Kind)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(TypeSignature::Part)





    // A function object that compares signatures using the compatibility and equivalence rules as
    // specified by ECMA-355 section 8.6.1.6, "Signature Matching."
    class SignatureComparer
    {
    public:

        SignatureComparer(ITypeResolver const* loader, Database const* lhsDatabase, Database const* rhsDatabase);

        bool operator()(ArrayShape        const& lhs, ArrayShape        const& rhs) const;
        bool operator()(CustomModifier    const& lhs, CustomModifier    const& rhs) const;
        bool operator()(FieldSignature    const& lhs, FieldSignature    const& rhs) const;
        bool operator()(MethodSignature   const& lhs, MethodSignature   const& rhs) const;
        bool operator()(PropertySignature const& lhs, PropertySignature const& rhs) const;
        bool operator()(TypeSignature     const& lhs, TypeSignature     const& rhs) const;

    private:

        bool operator()(RowReference      const& lhs, RowReference      const& rhs) const;

        Detail::ValueInitialized<ITypeResolver const*> _loader;
        Detail::ValueInitialized<Database const*>      _lhsDatabase;
        Detail::ValueInitialized<Database const*>      _rhsDatabase;
    };





    // A function object that decomposes signatures and replaces generic class variables.  Method
    // variables are left unreplaced.
    class ClassVariableSignatureInstantiator
    {
    public:

        ClassVariableSignatureInstantiator();

        template <typename TForIt>
        ClassVariableSignatureInstantiator(TForIt const firstArgument, TForIt const lastArgument)
            : _arguments(firstArgument, lastArgument)
        {
        }

        bool HasArguments() const { return !_arguments.empty(); }

        // Instantiates 'signature' by replacing each generic class variables in it with the
        // corresponding generic argument provided in the constructor of this functor.  The returned
        // signature is a range in an internal buffer and the caller is respondible for copying the
        // returned signature into a more permanent buffer.
        template <typename TSignature>
        TSignature Instantiate(TSignature const& signature) const;

        // Determines whether 'signature' has any generic class variables in it (and thus whether a
        // call to Instantiate() would yield a different signature).
        template <typename TSignature>
        static bool RequiresInstantiation(TSignature const& signature);

    private:

        typedef std::vector<TypeSignature> TypeSignatureSequence;
        typedef std::vector<Byte>          InternalBuffer;

        void InstantiateInto(InternalBuffer& buffer, ArrayShape        const& s) const;
        void InstantiateInto(InternalBuffer& buffer, FieldSignature    const& s) const;
        void InstantiateInto(InternalBuffer& buffer, MethodSignature   const& s) const;
        void InstantiateInto(InternalBuffer& buffer, PropertySignature const& s) const;
        void InstantiateInto(InternalBuffer& buffer, TypeSignature     const& s) const;

        template <typename TSignature, typename TPart>
        void CopyBytesInto(InternalBuffer& buffer, TSignature const& s, TPart first, TPart last) const;

        template <typename TForIt>
        void InstantiateRangeInto(InternalBuffer& buffer, TForIt first, TForIt last) const;

        static bool RequiresInstantiationInternal(ArrayShape        const& s);
        static bool RequiresInstantiationInternal(FieldSignature    const& s);
        static bool RequiresInstantiationInternal(MethodSignature   const& s);
        static bool RequiresInstantiationInternal(PropertySignature const& s);
        static bool RequiresInstantiationInternal(TypeSignature     const& s);

        template <typename TForIt>
        static bool AnyRequiresInstantiationInternal(TForIt first, TForIt last);

        InternalBuffer        mutable _buffer;
        TypeSignatureSequence         _arguments;
    };

} }

#endif
