//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header contains private implementation details only.  It defines types and functions used
// for parsing raw blob data from the metadata.
#ifndef CXXREFLECT_INTERNALBLOBMETADATA_HPP_
#define CXXREFLECT_INTERNALBLOBMETADATA_HPP_

#include "CxxReflect/InternalUtility.hpp"

#include <array>
#include <cstdint>
#include <deque>
#include <stdexcept>
#include <vector>

#include <cor.h>

namespace CxxReflect { namespace Detail { namespace BlobMetadata {

    class BlobAllocator; // TODO

    typedef std::uint8_t const* ByteIterator;

    class IteratorReadException : public std::runtime_error
    {
    public:

        IteratorReadException(ByteIterator it, char const* message)
            : std::runtime_error(message), it_(it)
        {
        }

        ByteIterator GetIterator() const { return it_; }

    private:

        ByteIterator it_;
    };

    char const* const IteratorReadUnexpectedEnd("Unexpectedly reached end of range");

    inline std::int8_t ReadInt8(ByteIterator& it, ByteIterator last)
    {
        if (it == last)
            throw IteratorReadException(it, IteratorReadUnexpectedEnd);

        return *reinterpret_cast<std::int8_t const*>(it++);
    }

    inline std::uint8_t ReadUInt8(ByteIterator& it, ByteIterator last)
    {
        if (it == last)
            throw IteratorReadException(it, IteratorReadUnexpectedEnd);

        return *it++;
    }

    struct CompressedIntBytes
    {
        typedef std::array<std::uint8_t, 4>    BytesType;
        typedef std::uint32_t                  CountType;

        BytesType    Bytes;
        CountType    Count;

        CompressedIntBytes()
            : Bytes(), Count()
        {
        }

        CompressedIntBytes(BytesType bytes, CountType count)
            : Bytes(bytes), Count(count)
        {
        }
    };

    inline CompressedIntBytes ReadCompressedIntBytes(ByteIterator& it, ByteIterator last)
    {
        CompressedIntBytes result;

        result.Bytes[0] = ReadUInt8(it, last);
        if      ((result.Bytes[0] & 0x80) == 0) { result.Count = 1;                          }
        else if ((result.Bytes[0] & 0x40) == 0) { result.Count = 2; result.Bytes[0] ^= 0x80; }
        else if ((result.Bytes[0] & 0x20) == 0) { result.Count = 4; result.Bytes[0] ^= 0xC0; }
        else
        {
            throw IteratorReadException(it, "Ill-formed length value");
        }

        for (unsigned i(1); i < result.Count; ++i)
        {
            result.Bytes[i] = ReadUInt8(it, last);
        }

        return result;
    }

    inline std::int32_t ReadCompressedInt32(ByteIterator& it, ByteIterator last)
    {
        CompressedIntBytes bytes(ReadCompressedIntBytes(it, last));

        bool const lsbSet((bytes.Bytes[bytes.Count - 1] & 0x01) != 0);

        switch (bytes.Count)
        {
        case 1:
        {
            std::uint8_t p(bytes.Bytes[0]);
            p >>= 1;
            lsbSet ? (p |= 0xFFFFFF80) : (p &= 0x0000007F);
            return *reinterpret_cast<std::int8_t*>(&p);
        }
        case 2:
        {
            std::uint16_t p(*reinterpret_cast<std::uint16_t*>(&bytes.Bytes[0]));
            p >>= 1;
            lsbSet ? (p |= 0xFFFFE000) : (p &= 0x00001FFF);
            return *reinterpret_cast<std::int16_t*>(&p);
        }
        case 4:
        {
            std::uint32_t p(*reinterpret_cast<std::uint32_t*>(&bytes.Bytes[0]));
            p >>= 1;
            lsbSet ? (p |= 0xF0000000) : (p &= 0x0FFFFFFF);
            return *reinterpret_cast<std::int32_t*>(&p);
        }
        default:
        {
            throw std::logic_error("It is impossible to get here.");
        }
        }
    }

    inline std::uint32_t ReadCompressedUInt32(ByteIterator& it, ByteIterator last)
    {
        CompressedIntBytes bytes(ReadCompressedIntBytes(it, last));

        switch (bytes.Count)
        {
        case 1:   return *reinterpret_cast<std::uint8_t*> (&bytes.Bytes[0]);
        case 2:   return *reinterpret_cast<std::uint16_t*>(&bytes.Bytes[0]);
        case 4:   return *reinterpret_cast<std::uint32_t*>(&bytes.Bytes[0]);
        default:  throw std::logic_error("It is impossible to get here.");
        }
    }

    inline std::uint32_t ReadTypeDefOrRefOrSpecEncoded(ByteIterator& it, ByteIterator last)
    {
        std::array<std::uint8_t, 4> bytes = { 0 };

        bytes[0] = ReadUInt8(it, last);
        std::uint8_t encodedTokenLength(bytes[0] >> 6);
        //TODO RuntimeCheck::Verify(encodedTokenLength < 4);

        for (unsigned i(1); i < encodedTokenLength; ++i)
        {
            bytes[i] = ReadUInt8(it, last);
        }

        std::uint32_t tokenValue(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
        std::uint32_t tokenType(tokenValue & 0x03);
        tokenValue >>= 2;

        switch (tokenType)
        {
        case 0x00:  tokenValue |= mdtTypeDef;  break;
        case 0x01:  tokenValue |= mdtTypeRef;  break;
        case 0x02:  tokenValue |= mdtTypeSpec; break;
        default: throw std::logic_error("wtf");
        }

        return tokenValue;
    }

    inline CorElementType ReadCorElementType(ByteIterator& it, ByteIterator last)
    {
        std::uint8_t value(ReadUInt8(it, last));
        if (value > ELEMENT_TYPE_MAX && value != 0x41 && value != 0x45 && value != 0x46 && value != 0x47)
            throw std::logic_error("wtf");
        
        return static_cast<CorElementType>(value);
    }

    inline std::int8_t PeekInt8(ByteIterator it, ByteIterator last)
    {
        return ReadInt8(it, last);
    }

    inline std::uint8_t PeekUInt8(ByteIterator it, ByteIterator last)
    {
        return ReadUInt8(it, last);
    }

    inline std::int32_t PeekCompressedInt32(ByteIterator it, ByteIterator last)
    {
        return ReadCompressedInt32(it, last);
    }

    inline std::uint32_t PeekCompressedUInt32(ByteIterator it, ByteIterator last)
    {
        return ReadCompressedUInt32(it, last);
    }

    inline std::uint32_t PeekTypeDefOrRefOrSpecEncoded(ByteIterator it, ByteIterator last)
    {
        return ReadTypeDefOrRefOrSpecEncoded(it, last);
    }

    inline CorElementType PeekCorElementType(ByteIterator it, ByteIterator last)
    {
        return ReadCorElementType(it, last);
    }

    // These are the core signature types
    class ArrayShape;
    class Constraint;
    class CustomMod;
    class FieldSig;
    class LocalVarSig;
    class MethodDefOrRefSig;
    class MethodSpec;
    class Param;
    class PropertySig;
    class RetType;
    class StandAloneMethodSig;
    class Type;
    class TypeSpec;

    // These are subtypes of the RawType signature type
    class TypeArray;
    class TypeClassOrValueType;
    class TypeGenericInst;
    class TypePtr;
    class TypeSzArray;
    class TypeTypeVariable;

    class ArrayShape
    {
    public:

        ArrayShape(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        // TODO ACCESSORS

    private:

        std::uint32_t                 rank_;
        std::vector<std::uint32_t>    sizes_;     // TODO MAY BE ABLE TO AVOID VECTOR
        std::vector<std::int32_t>     lowBounds_; // TODO MAY BE ABLE TO AVOID VECTOR
    };

    class Constraint
    {
        // TODO
    };

    class CustomMod
    {
    public:

        CustomMod(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        bool          IsRequired()   const { return isRequired_;  }
        bool          IsOptional()   const { return !isRequired_; }
        std::uint32_t GetTypeToken() const { return typeToken_;   }

    private:

        bool             isRequired_;
        std::uint32_t    typeToken_;
    };

    class FieldSig
    {
        // TODO
    };

    class LocalVarSig
    {
        // TODO
    };

    class MethodDefOrRefSig
    {
    public:

        MethodDefOrRefSig(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        bool HasThis()      const { return (flags_ & IMAGE_CEE_CS_CALLCONV_HASTHIS)      != 0; }
        bool ExplicitThis() const { return (flags_ & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS) != 0; }

        // TODO ACCESSORS

    private:

        std::uint8_t           flags_;
        std::uint32_t          genericParameterCount_;
        RetType*               returnType_;
        std::vector<Param*>    parameters_;
        std::vector<Param*>    variadicParameters_;
    };

    class MethodSpec
    {
        // TODO
    };

    class Param
    {
    public:

        struct Kind
        {
            enum Type
            {
                Unmodified,
                ByRef,
                TypedByRef
            };
        };

        Param(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        // TODO ACCESSORS

    private:

        std::vector<CustomMod*> customModifiers_;
        Kind::Type              kind_;
        Type*                   type_;
    };

    class PropertySig
    {
        // TODO
    };

    class RetType
    {
    public:

        struct Kind
        {
            enum Type
            {
                Unmodified,
                ByRef,
                TypedByRef,
                Void
            };
        };

        RetType(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        // TODO ACCESSORS

    private:

        std::vector<CustomMod*> customModifiers_;
        Kind::Type              kind_;
        Type*                   type_;
    };

    class StandAloneMethodSig
    {
        // TODO
    };

    class Type
    {
    public:

        Type(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        CorElementType GetKind() const { return kind_; }

        // TODO ACCESSORS

    private:

        CorElementType kind_;
        void*          payload_;

    };

    class TypeSpec
    {
    public:

        struct Kind
        {
            enum Type
            {
                Array,
                FnPtr,
                GenericInst,
                Ptr,
                SzArray         
            };
        };

        TypeSpec(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        Kind::Type GetKind() const { return kind_; }

        // Only one of these is valid; you must call Kind() to know which one.
        TypeArray           const& GetArray()       const { return VerifyAndGet(Kind::Array);       }
        MethodDefOrRefSig   const& GetFnPtr()       const { return VerifyAndGet(Kind::FnPtr);       }
        TypeGenericInst     const& GetGenericInst() const { return VerifyAndGet(Kind::GenericInst); }
        TypePtr             const& GetPtrToType()   const { return VerifyAndGet(Kind::Ptr);         }
        TypeSzArray         const& GetSzArray()     const { return VerifyAndGet(Kind::SzArray);     }     

    private:

        AllowConversionToArbitraryConstReference VerifyAndGet(Kind::Type kind) const
        {
            if (kind != kind_)
                throw std::logic_error("wtf");

            return AllowConversionToArbitraryConstReference(payload_);
        }

        Kind::Type    kind_;
        void*         payload_;
    };

    class TypeArray
    {
    public:

        TypeArray(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        ArrayShape const& GetShape() const { return *shape_; }
        Type       const& GetType()  const { return *type_;  }

    private:

        ArrayShape*    shape_;
        Type*          type_;
    };

    class TypeClassOrValueType
    {
    public:

        TypeClassOrValueType(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        std::uint32_t GetTypeToken() const { return typeToken_; }

    private:

        std::uint32_t typeToken_;
    };

    class TypeGenericInst
    {
    public:

        TypeGenericInst(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        bool                      IsClassType()      const { return isClassType_;   }
        bool                      IsValueType()      const { return !isClassType_;  }
        std::uint32_t             GetTypeToken()     const { return typeToken_;     }
        std::vector<Type*> const& GetTypeArguments() const { return typeArguments_; } // TODO BEGIN/END?

    private:

        bool                  isClassType_;
        std::uint32_t         typeToken_;
        std::vector<Type*>    typeArguments_; // TODO We may be able to eliminate vector here
    };

    class TypePtr
    {
    public:

        TypePtr(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        // TODO ACCESSORS

    private:

        std::vector<CustomMod*>    customModifiers_;
        Type*                      type_;
    };

    class TypeSzArray
    {
    public:

        TypeSzArray(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        // TODO ACCESSORS

    private:

        std::vector<CustomMod*>    customModifiers_;
        Type*                      type_;
    };

    class TypeTypeVariable
    {
    public:

        TypeTypeVariable(BlobAllocator& allocator, ByteIterator& it, ByteIterator last);

        std::uint32_t GetNumber() const { return number_; }

    private:

        std::uint32_t number_;
    };
    
    class BlobAllocator
    {
    public:

        template <typename T>
        T* Allocate(ByteIterator& it, ByteIterator last)
        {
            data_.push_back(Element());
            T* p(reinterpret_cast<T*>(&data_.back()));
            new (p) T(*this, it, last);
            return p;
        }

    private:

        template <std::size_t M, std::size_t N>
        struct Max
        {
            enum { Value = M > N ? M : N };
        };

        enum
        {
            ElementSize = Max<sizeof(ArrayShape),
                          Max<sizeof(Constraint),
                          Max<sizeof(CustomMod),
                          Max<sizeof(FieldSig),
                          Max<sizeof(LocalVarSig),
                          Max<sizeof(MethodDefOrRefSig),
                          Max<sizeof(MethodSpec),
                          Max<sizeof(Param),
                          Max<sizeof(PropertySig),
                          Max<sizeof(RetType),
                          Max<sizeof(StandAloneMethodSig),
                          Max<sizeof(Type),
                          Max<sizeof(TypeSpec),
                          Max<sizeof(TypeArray),
                          Max<sizeof(TypeClassOrValueType),
                          Max<sizeof(TypeGenericInst),
                          Max<sizeof(TypePtr),
                          Max<sizeof(TypeSzArray),
                              sizeof(TypeTypeVariable)
                          >::Value>::Value>::Value>::Value>::Value>::Value>::Value
                          >::Value>::Value>::Value>::Value>::Value>::Value>::Value
                          >::Value>::Value>::Value>::Value
        };

        struct Element
        {
            char data[ElementSize];
        };

        std::deque<Element> data_;
    };

    inline std::vector<CustomMod*> ReadCustomModSequence(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        auto IsCustomMod([](CorElementType x)
        {
            return x == ELEMENT_TYPE_CMOD_OPT || x == ELEMENT_TYPE_CMOD_REQD;
        });

        std::vector<CustomMod*> customModifiers;
        while (IsCustomMod(PeekCorElementType(it, last)))
        {
            customModifiers.push_back(allocator.Allocate<CustomMod>(it, last));
        }

        return customModifiers;
    }

    inline ArrayShape::ArrayShape(BlobAllocator&, ByteIterator& it, ByteIterator last)
    {
        rank_ = ReadCompressedUInt32(it, last);

        std::uint32_t numSizes(ReadCompressedUInt32(it, last));
        for (unsigned i(0); i < numSizes; ++i)
        {
            sizes_.push_back(ReadCompressedUInt32(it, last));
        }

        std::uint32_t numLowBounds(ReadCompressedUInt32(it, last));
        for (unsigned i(0); i < numLowBounds; ++i)
        {
            lowBounds_.push_back(ReadCompressedInt32(it, last));
        }
    }

    inline CustomMod::CustomMod(BlobAllocator&, ByteIterator& it, ByteIterator last)
    {
        CorElementType elementType(ReadCorElementType(it, last));
        switch (elementType)
        {
        case ELEMENT_TYPE_CMOD_OPT:
            isRequired_ = false;
            break;

        case ELEMENT_TYPE_CMOD_REQD:
            isRequired_ = true;
            break;

        default:
            throw std::logic_error("wtf");
        }

        typeToken_ = ReadTypeDefOrRefOrSpecEncoded(it, last);
    }

    inline MethodDefOrRefSig::MethodDefOrRefSig(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        flags_ = ReadUInt8(it, last);

        if ((flags_ & IMAGE_CEE_CS_CALLCONV_GENERIC) != 0)
        {
            genericParameterCount_ = ReadCompressedUInt32(it, last);
        }

        std::uint32_t parameterCount(ReadCompressedUInt32(it, last));

        returnType_ = allocator.Allocate<RetType>(it, last);

        std::vector<Param*>* parametersList = &parameters_;
        for (unsigned i(0); i < parameterCount; ++i)
        {
            if (PeekUInt8(it, last) == ELEMENT_TYPE_SENTINEL)
            {
                ReadUInt8(it, last);
                // TODO ASSERT SENTINEL HASN'T BEEN FOUND YET?
                parametersList = &variadicParameters_;
            }
            else
            {
                parametersList->push_back(allocator.Allocate<Param>(it, last));
            }
        }
    }

    inline Param::Param(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        customModifiers_ = ReadCustomModSequence(allocator, it, last);

        CorElementType elementType(PeekCorElementType(it, last));
        switch (elementType)
        {
        case ELEMENT_TYPE_TYPEDBYREF:
            kind_ = Kind::TypedByRef;
            type_ = nullptr;
            break;

        case ELEMENT_TYPE_BYREF:
            kind_ = Kind::ByRef;
            type_ = allocator.Allocate<Type>(it, last);
            break;

        default:
            kind_ = Kind::Unmodified;
            type_ = allocator.Allocate<Type>(it, last);
            break;
        }
    }

    inline RetType::RetType(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        customModifiers_ = ReadCustomModSequence(allocator, it, last);

        CorElementType elementType(PeekCorElementType(it, last));
        switch (elementType)
        {
        case ELEMENT_TYPE_VOID:
            kind_ = Kind::Void;
            type_ = nullptr;
            break;

        case ELEMENT_TYPE_TYPEDBYREF:
            kind_ = Kind::TypedByRef;
            type_ = nullptr;
            break;

        case ELEMENT_TYPE_BYREF:
            kind_ = Kind::ByRef;
            type_ = allocator.Allocate<Type>(it, last);
            break;

        default:
            kind_ = Kind::Unmodified;
            type_ = allocator.Allocate<Type>(it, last);
            break;
        }
    }

    inline Type::Type(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        kind_ = ReadCorElementType(it, last);
        switch (kind_)
        {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_R8:
        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_U:
        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_STRING:
            payload_ = nullptr;
            break;

        case ELEMENT_TYPE_ARRAY:
            payload_ = allocator.Allocate<TypeArray>(it, last);
            break;

        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_VALUETYPE:
            payload_ = allocator.Allocate<TypeClassOrValueType>(it, last);
            break;

        case ELEMENT_TYPE_FNPTR:
            payload_ = allocator.Allocate<MethodDefOrRefSig>(it, last);
            break;

        case ELEMENT_TYPE_GENERICINST:
            payload_ = allocator.Allocate<TypeGenericInst>(it, last);
            break;

        case ELEMENT_TYPE_MVAR:
        case ELEMENT_TYPE_VAR:
            payload_ = allocator.Allocate<TypeTypeVariable>(it, last);
            break;

        case ELEMENT_TYPE_PTR:
            payload_ = allocator.Allocate<TypePtr>(it, last);
            break;

        case ELEMENT_TYPE_SZARRAY:
            payload_ = allocator.Allocate<TypeSzArray>(it, last);
            break;
        }
    }

    inline TypeSpec::TypeSpec(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        CorElementType elementType(ReadCorElementType(it, last));
        switch (elementType)
        {
        case ELEMENT_TYPE_ARRAY:
            kind_ = Kind::Array;
            payload_ = allocator.Allocate<TypeArray>(it, last);
            break;

        case ELEMENT_TYPE_FNPTR:
            kind_ = Kind::FnPtr;
            payload_ = allocator.Allocate<MethodDefOrRefSig>(it, last);
            break;

        case ELEMENT_TYPE_GENERICINST:
            kind_ = Kind::GenericInst;
            payload_ = allocator.Allocate<TypeGenericInst>(it, last);
            break;

        case ELEMENT_TYPE_PTR:
            kind_ = Kind::Ptr;
            payload_ = allocator.Allocate<TypePtr>(it, last);
            break;

        case ELEMENT_TYPE_SZARRAY:
            kind_ = Kind::SzArray;
            payload_ = allocator.Allocate<TypeSzArray>(it, last);
            break;   

        default:
            throw std::logic_error("wtf");
        }
    }

    inline TypeArray::TypeArray(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        type_ = allocator.Allocate<Type>(it, last);
        shape_ = allocator.Allocate<ArrayShape>(it, last);
    }

    inline TypeClassOrValueType::TypeClassOrValueType(BlobAllocator&, ByteIterator& it, ByteIterator last)
    {
        typeToken_ = ReadTypeDefOrRefOrSpecEncoded(it, last);
    }

    inline TypeGenericInst::TypeGenericInst(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        CorElementType elementType(ReadCorElementType(it, last));
        switch (elementType)
        {
        case ELEMENT_TYPE_CLASS:
            isClassType_ = true;
            break;

        case ELEMENT_TYPE_VALUETYPE:
            isClassType_ = false;
            break;

        default:
            throw std::logic_error("wtf");
        }

        typeToken_ = ReadTypeDefOrRefOrSpecEncoded(it, last);
        
        unsigned argumentCount(ReadCompressedUInt32(it, last));
        for (unsigned i(0); i < argumentCount; ++i)
        {
            typeArguments_.push_back(allocator.Allocate<Type>(it, last));
        }
    }

    inline TypePtr::TypePtr(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        customModifiers_ = ReadCustomModSequence(allocator, it, last);
        type_ = allocator.Allocate<Type>(it, last);
    }

    inline TypeSzArray::TypeSzArray(BlobAllocator& allocator, ByteIterator& it, ByteIterator last)
    {
        customModifiers_ = ReadCustomModSequence(allocator, it, last);
        type_ = allocator.Allocate<Type>(it, last);
    }

    inline TypeTypeVariable::TypeTypeVariable(BlobAllocator&, ByteIterator& it, ByteIterator last)
    {
        number_ = ReadCompressedUInt32(it, last);
    }

} } }

#endif
