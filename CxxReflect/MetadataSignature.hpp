//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATASIGNATURE_HPP_
#define CXXREFLECT_METADATASIGNATURE_HPP_

#include "CxxReflect/Core.hpp"

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




    // An iterator that counts elements as it materializes them.  This is used for metadata sequences
    // where the number of elements is known in advance and some function is required to materialize
    // each element in the sequence from the raw bytes.
    template <typename TValue, TValue(*FMaterialize)(ByteIterator&, ByteIterator)>
    class CountingIterator
        : Detail::EqualityComparable<CountingIterator<TValue, FMaterialize>>
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

        CountingIterator(ByteIterator const current, ByteIterator const last,
                         SizeType const index, SizeType const count)
            : _current(current), _last(last), _index(index), _count(count)
        {
            if (index != count)
                Materialize();
        }

        reference operator*()  const { return _value.Get();  }
        pointer   operator->() const { return &_value.Get(); }

        CountingIterator& operator++()
        {
            ++_index.Get();
            if (_index.Get() != _count.Get())
                Materialize();
        }

        CountingIterator operator++(int)
        {
            CountingIterator const it(*this);
            ++*this;
            return it;
        }

        friend bool operator==(CountingIterator const& lhs, CountingIterator const& rhs)
        {
            return lhs._current == rhs._current;
        }

    private:

        void Materialize()
        {
            _value.Get() = FMaterialize(_current.Get(), _last.Get());
        }

        // For a non-singular iterator, _current always points one-past the location in the sequence
        // whence _value was materialized.  This is because materialization requires advacement.
        Detail::ValueInitialized<ByteIterator> _current;
        Detail::ValueInitialized<ByteIterator> _last;

        Detail::ValueInitialized<SizeType>     _index;
        Detail::ValueInitialized<SizeType>     _count;

        Detail::ValueInitialized<value_type>   _value;
    };




    // A generic iterator that reads elements from a sequence via FMaterialize until FSentinelCheck
    // returns false.  This is used for sequences of elements where the sequence is terminated by
    // failing to read another element (e.g. CustomMod sequences).
    template <typename TValue,
              bool(*FSentinelCheck)(ByteIterator,  ByteIterator),
              TValue(*FMaterialize)(ByteIterator&, ByteIterator)>
    class SentinelIterator
        : Detail::EqualityComparable<SentinelIterator<TValue, FSentinelCheck, FMaterialize>>
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

        SentinelIterator(ByteIterator const current, ByteIterator const last)
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

        Detail::ValueInitialized<ByteIterator> _current;
        Detail::ValueInitialized<ByteIterator> _last;

        Detail::ValueInitialized<value_type>   _value;
    };




    // A generic iterator that reads elements from a sequence via FMaterialize until FSentinelCheck
    // returns false.  This is used for sequences of elements where the sequence is terminated by
    // failing to read another element (e.g. CustomMod sequences).
    template <typename TValue,
              bool(*FSentinelCheck)(ByteIterator,  ByteIterator),
              TValue(*FMaterialize)(ByteIterator&, ByteIterator)>
    class CountingSentinelIterator
        : Detail::EqualityComparable<CountingSentinelIterator<TValue, FSentinelCheck, FMaterialize>>
    {
    public:

        typedef TValue                    value_type;
        typedef TValue const&             reference;
        typedef TValue const*             pointer;
        typedef std::ptrdiff_t            difference_type;
        typedef std::forward_iterator_tag iterator_category;

        CountingSentinelIterator()
        {
        }

        CountingSentinelIterator(ByteIterator const current, ByteIterator const last,
                                 SizeType const index, SizeType const count)
            : _current(current), _last(last), _index(index), _count(count)
        {
            if (current != last && index != count)
                Materialize();
        }

        reference operator*()  const { return _value.Get();  }
        pointer   operator->() const { return &_value.Get(); }

        CountingSentinelIterator& operator++()
        {
            ++_index.Get();
            if (_index.Get() != _count.Get())
                Materialize();

            return *this;
        }

        CountingSentinelIterator operator++(int)
        {
            SentinelIterator const it(*this);
            ++*this;
            return it;
        }

        friend bool operator==(CountingSentinelIterator const& lhs, CountingSentinelIterator const& rhs)
        {
            return lhs._current.Get() == rhs._current.Get();
        }

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

        Detail::ValueInitialized<ByteIterator> _current;
        Detail::ValueInitialized<ByteIterator> _last;

        Detail::ValueInitialized<SizeType>     _index;
        Detail::ValueInitialized<SizeType>     _count;

        Detail::ValueInitialized<value_type>   _value;
    };




    class ArrayShape;
    class CustomModifier;
    class FieldSignature;
    class MethodSignature;
    class PropertySignature;
    class TypeSignature;




    // Represents an ArrayShape signature item
    class ArrayShape
    {
    private:

        static SizeType ReadSize(ByteIterator& current, ByteIterator last);
        static SizeType ReadLowBound(ByteIterator& current, ByteIterator last);

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
        ArrayShape(ByteIterator first, ByteIterator last);

        SizeType         GetRank()           const;

        SizeType         GetSizesCount()     const;
        SizeIterator     BeginSizes()        const;
        SizeIterator     EndSizes()          const;

        SizeType         GetLowBoundsCount() const;
        LowBoundIterator BeginLowBounds()    const;
        LowBoundIterator EndLowBounds()      const;

        SizeType         ComputeSize()       const;

        bool             IsInitialized()     const;

    private:

        ByteIterator SeekTo(Part part) const;

        void VerifyInitialized() const;

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(ArrayShape::Part)




    // Represents a CustomMod signature item
    class CustomModifier
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
        CustomModifier(ByteIterator first, ByteIterator last);

        bool           IsOptional()       const;
        bool           IsRequired()       const;
        TableReference GetTypeReference() const;

        SizeType       ComputeSize()      const;

        bool           IsInitialized()    const;

    private:

        ByteIterator SeekTo(Part part) const;

        void VerifyInitialized() const;

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(CustomModifier::Part)




    // Represents a FieldSig signature item; note that the optional CustomMod sequence is contained
    // in TypeSignature itself and is thus not exposed directly here.
    class FieldSignature
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
        FieldSignature(ByteIterator first, ByteIterator last);

        TypeSignature GetTypeSignature() const;
        SizeType      ComputeSize()      const;
        bool          IsInitialized()    const;

    private:

        ByteIterator SeekTo(Part part) const;

        void VerifyInitialized() const;

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(FieldSignature::Part);




    // Represents a PropertySig signature item; note that the optional CustomMod sequence is
    // contained in TypeSignature itself and is thus not exposed directly here.
    class PropertySignature
    {
    private:

        static TypeSignature ReadParameter(ByteIterator& current, ByteIterator last);

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
        PropertySignature(ByteIterator first, ByteIterator last);

        bool              HasThis()           const;
        SizeType          GetParameterCount() const;
        ParameterIterator BeginParameters()   const;
        ParameterIterator EndParameters()     const;
        TypeSignature     GetTypeSignature()  const;

        SizeType          ComputeSize()       const;
        bool              IsInitialized()     const;

    private:

        ByteIterator SeekTo(Part part) const;

        void VerifyInitialized() const;

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(PropertySignature::Part);




    // Represents a MethodDefSig, MethodRefSig, or StandAloneMethodSig.
    class MethodSignature
    {
    private:

        static bool ParameterEndCheck(ByteIterator current, ByteIterator last);
        static TypeSignature ReadParameter(ByteIterator& current, ByteIterator last);

    public:

        typedef CountingSentinelIterator<
            TypeSignature,
            &MethodSignature::ParameterEndCheck,
            &MethodSignature::ReadParameter
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
        MethodSignature(ByteIterator first, ByteIterator last);

        bool HasThis()         const;
        bool HasExplicitThis() const;

        // Calling conventions; exactly one of these will be true.
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
        bool              IsInitialized()         const;

    private:

        ByteIterator SeekTo(Part part) const;

        void VerifyInitialized() const;

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(MethodSignature::Part)




    // A 'Type' signature item, with support for an optional prefix sequence of CustomModifier items,
    // an optional BYREF tag before the type proper, and either a TYPEDBYREF or VOID type in place of
    // a real type.  This supports Param, RetType, Type, and TypeSpec signatures in entirety, as well
    // as the core parts of FieldSig and PropertySig.
    class TypeSignature
    {
    private:

        static bool CustomModifierEndCheck(ByteIterator current, ByteIterator last);
        static CustomModifier ReadCustomModifier(ByteIterator& current, ByteIterator last);

        static TypeSignature ReadType(ByteIterator& current, ByteIterator last);

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

            ArrayType                = static_cast<SizeType>(Kind::Array)       + 0x0a,
            ArrayShape               = static_cast<SizeType>(Kind::Array)       + 0x0b,

            SzArrayType              = static_cast<SizeType>(Kind::SzArray)     + 0x0a,

            ClassTypeReference       = static_cast<SizeType>(Kind::ClassType)   + 0x0a,

            MethodSignature          = static_cast<SizeType>(Kind::FnPtr)       + 0x0a,

            GenericInstTypeCode      = static_cast<SizeType>(Kind::GenericInst) + 0x0a,
            GenericInstTypeReference = static_cast<SizeType>(Kind::GenericInst) + 0x0b,
            GenericInstArgumentCount = static_cast<SizeType>(Kind::GenericInst) + 0x0c,
            FirstGenericInstArgument = static_cast<SizeType>(Kind::GenericInst) + 0x0d,

            PointerTypeSignature     = static_cast<SizeType>(Kind::Ptr)         + 0x0a,

            VariableNumber           = static_cast<SizeType>(Kind::Var)         + 0x0a,

            End            = 0xff
        };

        typedef SentinelIterator<
            CustomModifier,
            &TypeSignature::CustomModifierEndCheck,
            &TypeSignature::ReadCustomModifier
        > CustomModifierIterator;

        typedef CountingIterator<TypeSignature, &TypeSignature::ReadType> GenericArgumentIterator;

        TypeSignature();
        TypeSignature(ByteIterator first, ByteIterator last);

        SizeType ComputeSize()     const;
        bool     IsInitialized()   const;
        bool     IsKind(Kind kind) const;

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
        bool           IsClassType()      const;
        bool           IsValueType()      const;
        TableReference GetTypeReference() const;

        // FNPTR:
        bool            IsFunctionPointer()  const;
        MethodSignature GetMethodSignature() const;

        // GENERICINST:
        bool                    IsGenericInstance()          const;
        bool                    IsGenericClassTypeInstance() const;
        bool                    IsGenericValueTypeInstance() const;
        TableReference          GetGenericTypeReference()    const;
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

        ByteIterator SeekTo(Part part) const;

        ElementType GetElementType() const;

        void VerifyInitialized() const;
        void VerifyKind(Kind kind) const;

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(TypeSignature::Kind)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(TypeSignature::Part)

    

    // Represents a Blob in the metadata database TODO MOVE THIS BACK INTO METADATADATABASE.CPP
    class Blob
    {
    public:

        Blob()
        {
        }

        // Note that this 'last' is not the end of the blob, it is the end of the whole blob stream.
        Blob(ByteIterator const first, ByteIterator const last)
        {
            Detail::VerifyNotNull(first);
            Detail::VerifyNotNull(last);

            std::tie(_first.Get(), _last.Get()) = ComputeBounds(first, last);
        }

        ByteIterator Begin()   const { VerifyInitialized(); return _first.Get(); }
        ByteIterator End()     const { VerifyInitialized(); return _last.Get();  }

        SizeType GetSize() const
        {
            VerifyInitialized();
            return static_cast<SizeType>(_last.Get() - _first.Get());
        }

        bool IsInitialized() const
        {
            return _first.Get() != nullptr && _last.Get() != nullptr;
        }

    private:

        static std::pair<ByteIterator, ByteIterator> ComputeBounds(ByteIterator const first,
                                                                   ByteIterator const last)
        {
            if (first == last)
                throw ReadError("Invalid blob");

            Byte initialByte(*first);
            SizeType blobSizeBytes(0);
            switch (initialByte >> 5)
            {
            case 0:
            case 1:
            case 2:
            case 3:
                blobSizeBytes = 1;
                initialByte &= 0x7f;
                break;

            case 4:
            case 5:
                blobSizeBytes = 2;
                initialByte &= 0x3f;
                break;

            case 6:
                blobSizeBytes = 4;
                initialByte &= 0x1f;
                break;

            case 7:
            default:
                throw ReadError("Invalid blob");
            }

            if (static_cast<SizeType>(last - first) < blobSizeBytes)
                throw ReadError("Invalid blob");

            SizeType blobSize(initialByte);
            for (unsigned i(1); i < blobSizeBytes; ++ i)
                blobSize = (blobSize << 8) + *(first + i);

            if (static_cast<SizeType>(last - first) < blobSizeBytes + blobSize)
                throw ReadError("Invalid blob");

            return std::make_pair(first + blobSizeBytes, first + blobSizeBytes + blobSize);
        }

        void VerifyInitialized() const
        {
            Detail::Verify([&]{ return IsInitialized(); });
        }

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };

} }

#endif
