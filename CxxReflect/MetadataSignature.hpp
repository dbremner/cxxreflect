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

    typedef Detail::FlagSet<SignatureAttribute> SignatureFlags;

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(SignatureAttribute)

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
                         IndexType const index, IndexType const count)
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

        friend bool operator!=(CountingIterator const& lhs, CountingIterator const& rhs)
        {
            return !(lhs == rhs);
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

        Detail::ValueInitialized<IndexType>    _index;
        Detail::ValueInitialized<IndexType>    _count;

        Detail::ValueInitialized<value_type>   _value;
    };




    // Represents an ArrayShape signature item
    class ArrayShape
    {
    private:

        static IndexType ReadSize(ByteIterator& current, ByteIterator last);
        static IndexType ReadLowBound(ByteIterator& current, ByteIterator last);

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

        typedef CountingIterator<IndexType, &ArrayShape::ReadSize>     SizeIterator;
        typedef CountingIterator<IndexType, &ArrayShape::ReadLowBound> LowBoundIterator;

        ArrayShape();
        ArrayShape(ByteIterator first, ByteIterator last);

        IndexType        GetRank()           const;

        IndexType        GetSizesCount()     const;
        SizeIterator     BeginSizes()        const;
        SizeIterator     EndSizes()          const;

        IndexType        GetLowBoundsCount() const;
        LowBoundIterator BeginLowBounds()    const;
        LowBoundIterator EndLowBounds()      const;

        IndexType        ComputeSize()       const;

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

        CustomModifier();
        CustomModifier(ByteIterator first, ByteIterator last);

        bool           IsOptional()       const;
        bool           IsRequired()       const;
        TableReference GetTypeReference() const;

        IndexType ComputeSize() const;

        bool IsInitialized() const;

    private:

        void VerifyInitialized() const;

        Detail::ValueInitialized<ByteIterator> _first;
        Detail::ValueInitialized<ByteIterator> _last;
    };

    // A blob that represents a type.  We use this to represent the following kinds of blobs:
    // * FieldSig,
    // * PropertySig,
    // * [TODO The repeatable part of LocalVarSig],
    // * Param,
    // * RetType,
    // * Type,
    // * [TODO MethodSpec]
    // TypeSpec is a subset of Type, so we use Type directly to parse it.
    class TypeSignature
    {
    public:

        typedef void /* TODO */ CustomModifierIterator;

        // A FieldSig, PropertySig, 
        CustomModifierIterator BeginCustomModifiers() const;
        CustomModifierIterator EndCustomModifiers()   const;

        IndexType ComputeSize() const;

    private:

        // Computes the pointer to the first CustomModifier that modifies this Type signature
        ByteIterator GetFirstCustomModifier() const;

        // Computes the pointer to the Type signature by skipping past the prologue
        ByteIterator GetFirstType() const;

        ByteIterator _first;
        ByteIterator _last;
    };

    // Represents a MethodDefSig, MethodRefSig, or StandAloneMethodSig.
    class MethodSignature
    {
    public:

        typedef void /* TODO */ ParameterIterator;

        bool HasThis()         const;
        bool HasExplicitThis() const;

        // Calling conventions; exactly one of these will be true.
        bool HasDefaultConvention()  const;
        bool HasVarArgConvention()   const;
        bool HasCConvention()        const;
        bool HasStdCallConvention()  const;
        bool HasThisCallConvention() const;
        bool HasFastCallConvention() const;

        bool      IsGeneric()                const;
        IndexType GetGenericParameterCount() const;

        TypeSignature     GetReturnType()     const;

        // The parameter count is the total of all of the declared parameters and variadic
        // parameters (if there are any).  To compute the count of each of these, use std::distance
        // on the Begin/End pairs.
        IndexType         GetParameterCount()     const;

        ParameterIterator BeginParameters()       const;
        ParameterIterator EndParameters()         const;

        ParameterIterator BeginVarargParameters() const;
        ParameterIterator EndVarargParameters()   const;

    private:

        ByteIterator GetRetTypeIterator() const;
        ByteIterator GetParamIterator()   const;

        ByteIterator _first;
        ByteIterator _last;
    };

    // Represents a Blob in the metadata database
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
