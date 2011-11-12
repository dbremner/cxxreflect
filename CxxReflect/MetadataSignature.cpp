//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/MetadataSignature.hpp"

namespace { namespace Private {

    using namespace CxxReflect;
    using namespace CxxReflect::Metadata;

    char const* const IteratorReadUnexpectedEnd("Unexpectedly reached end of range");

    Byte ReadByte(ByteIterator& it, ByteIterator last)
    {
        if (it == last)
            throw ReadError(Private::IteratorReadUnexpectedEnd);

        return *it++;
    }

    Byte PeekByte(ByteIterator it, ByteIterator last)
    {
        return ReadByte(it, last);
    }

    struct CompressedIntBytes
    {
        typedef std::array<Byte, 4> BytesType;
        typedef std::uint32_t       CountType;

        BytesType Bytes;
        CountType Count;

        CompressedIntBytes()
            : Bytes(), Count()
        {
        }

        CompressedIntBytes(BytesType bytes, CountType count)
            : Bytes(bytes), Count(count)
        {
        }
    };

    CompressedIntBytes ReadCompressedIntBytes(ByteIterator& it, ByteIterator last)
    {
        CompressedIntBytes result;

        result.Bytes[0] = ReadByte(it, last);
        if      ((result.Bytes[0] & 0x80) == 0) { result.Count = 1;                          }
        else if ((result.Bytes[0] & 0x40) == 0) { result.Count = 2; result.Bytes[0] ^= 0x80; }
        else if ((result.Bytes[0] & 0x20) == 0) { result.Count = 4; result.Bytes[0] ^= 0xC0; }
        else
        {
            throw ReadError("Ill-formed length value");
        }

        for (unsigned i(1); i < result.Count; ++i)
        {
            result.Bytes[i] = ReadByte(it, last);
        }

        return result;
    }

    std::int32_t ReadCompressedInt32(ByteIterator& it, ByteIterator last)
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
            Detail::VerifyFail("It is impossible to get here");
            return 0;
        }
        }
    }

    std::int32_t PeekCompressedInt32(ByteIterator it, ByteIterator last)
    {
        return ReadCompressedInt32(it, last);
    }

    std::uint32_t ReadCompressedUInt32(ByteIterator& it, ByteIterator last)
    {
        CompressedIntBytes bytes(ReadCompressedIntBytes(it, last));

        switch (bytes.Count)
        {
        case 1:  return *reinterpret_cast<std::uint8_t*> (&bytes.Bytes[0]);
        case 2:  return *reinterpret_cast<std::uint16_t*>(&bytes.Bytes[0]);
        case 4:  return *reinterpret_cast<std::uint32_t*>(&bytes.Bytes[0]);
        default: Detail::VerifyFail("It is impossible to get here"); return 0;
        }
    }

    std::uint32_t PeekCompressedUInt32(ByteIterator it, ByteIterator last)
    {
        return ReadCompressedUInt32(it, last);
    }

    std::uint32_t ReadTypeDefOrRefOrSpecEncoded(ByteIterator& it, ByteIterator last)
    {
        std::array<Byte, 4> bytes = { 0 };

        bytes[0] = ReadByte(it, last);
        IndexType const encodedTokenLength(bytes[0] >> 6);
        Detail::Verify([&]{ return encodedTokenLength < 4; });

        for (unsigned i(1); i < encodedTokenLength; ++i)
        {
            bytes[i] = ReadByte(it, last);
        }

        IndexType const tokenValue(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
        IndexType const tokenType(tokenValue & 0x03);

        switch (tokenType)
        {
        case 0x00: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeDef)  << 24);
        case 0x01: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeRef)  << 24);
        case 0x02: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeSpec) << 24);
        default:   throw ReadError("Unexpected table id in TypeDefOrRefOrSpecEncoded");
        }
    }

    std::uint32_t PeekTypeDefOrRefOrSpecEncoded(ByteIterator it, ByteIterator last)
    {
        return ReadTypeDefOrRefOrSpecEncoded(it, last);
    }

    ElementType ReadElementType(ByteIterator& it, ByteIterator last)
    {
        Byte const value(ReadByte(it, last));
        if (!IsValidElementType(value))
            throw ReadError("Unexpected element type");

        return static_cast<ElementType>(value);
    }

    ElementType PeekElementType(ByteIterator it, ByteIterator last)
    {
        return ReadElementType(it, last);
    }

    bool IsCustomModifierElementType(Byte const value)
    {
        return value == ElementType::CustomModifierOptional
            || value == ElementType::CustomModifierRequired;
    }

} }

namespace CxxReflect { namespace Metadata {

    ArrayShape::ArrayShape()
    {
    }

    ArrayShape::ArrayShape(ByteIterator const first, ByteIterator const last)
        : _first(first), _last(last)
    {
        Detail::VerifyNotNull(first);
        Detail::VerifyNotNull(last);
    }

    IndexType ArrayShape::GetRank() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::Rank), _last.Get());
    }

    IndexType ArrayShape::GetSizesCount() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::NumSizes), _last.Get());
    }

    ArrayShape::SizeIterator ArrayShape::BeginSizes() const
    {
        VerifyInitialized();

        return SizeIterator(SeekTo(Part::FirstSize), _last.Get(), 0, GetSizesCount());
    }

    ArrayShape::SizeIterator ArrayShape::EndSizes() const
    {
        VerifyInitialized();

        IndexType const sizesCount(GetSizesCount());
        return SizeIterator(nullptr, nullptr, sizesCount, sizesCount);
    }

    IndexType ArrayShape::GetLowBoundsCount() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::NumLowBounds), _last.Get());
    }

    ArrayShape::LowBoundIterator ArrayShape::BeginLowBounds() const
    {
        VerifyInitialized();

        return LowBoundIterator(SeekTo(Part::FirstLowBound), _last.Get(), 0, GetLowBoundsCount());
    }

    ArrayShape::LowBoundIterator ArrayShape::EndLowBounds() const
    {
        VerifyInitialized();

        IndexType const lowBoundsCount(GetLowBoundsCount());
        return LowBoundIterator(nullptr, nullptr, lowBoundsCount, lowBoundsCount);
    }

    IndexType ArrayShape::ComputeSize() const
    {
        VerifyInitialized();

        return SeekTo(Part::End) - _first.Get();
    }

    bool ArrayShape::IsInitialized() const
    {
        return _first.Get() != nullptr && _last.Get() != nullptr;
    }

    void ArrayShape::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }

    ByteIterator ArrayShape::SeekTo(Part const part) const
    {
        VerifyInitialized();

        ByteIterator current(_first.Get());

        if (part > Part::Rank)
        {
            Private::ReadCompressedUInt32(current, _last.Get());
        }

        IndexType numSizes(0);
        if (part > Part::NumSizes)
        {
            numSizes = Private::ReadCompressedUInt32(current, _last.Get());
        }

        if (part > Part::FirstSize)
        {
            for (unsigned i(0); i < numSizes; ++i)
                Private::ReadCompressedUInt32(current, _last.Get());
        }

        IndexType numLowBounds(0);
        if (part > Part::NumLowBounds)
        {
            numLowBounds = Private::ReadCompressedUInt32(current, _last.Get());
        }

        if (part > Part::FirstLowBound)
        {
            for (unsigned i(0); i < numSizes; ++i)
                Private::ReadCompressedUInt32(current, _last.Get());
        }

        if (part > Part::End)
        {
            Detail::VerifyFail("Invalid signature part requested");
        }

        return current;
    }




    CustomModifier::CustomModifier()
    {
    }

    CustomModifier::CustomModifier(ByteIterator const first, ByteIterator const last)
        : _first(first), _last(last)
    {
        Detail::VerifyNotNull(first);
        Detail::VerifyNotNull(last);
        Detail::Verify([&]{ return IsOptional() || IsRequired(); });
    }

    bool CustomModifier::IsInitialized() const
    {
        return _first.Get() != nullptr && _last.Get() != nullptr;
    }

    bool CustomModifier::IsOptional() const
    {
        VerifyInitialized();

        return Private::PeekByte(_first.Get(), _last.Get()) == ElementType::CustomModifierOptional;
    }

    bool CustomModifier::IsRequired() const
    {
        VerifyInitialized();

        return Private::PeekByte(_first.Get(), _last.Get()) == ElementType::CustomModifierRequired;
    }

    TableReference CustomModifier::GetTypeReference() const
    {
        VerifyInitialized();

        ByteIterator current(_first.Get());
        Private::ReadByte(current, _last.Get());
        return TableReference::FromToken(Private::ReadTypeDefOrRefOrSpecEncoded(current, _last.Get()));
    }

    IndexType CustomModifier::ComputeSize() const
    {
        VerifyInitialized();

        ByteIterator current(_first.Get());
        Private::ReadByte(current, _last.Get());
        Private::ReadTypeDefOrRefOrSpecEncoded(current, _last.Get());
        return current - _first.Get();
    }

    void CustomModifier::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }




    bool MethodSignature::HasThis() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .IsSet(SignatureAttribute::HasThis);
    }

    bool MethodSignature::HasExplicitThis() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .IsSet(SignatureAttribute::ExplicitThis);
    }

    bool MethodSignature::HasDefaultConvention() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::Default;
    }

    bool MethodSignature::HasVarArgConvention() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::VarArg;
    }

    bool MethodSignature::HasCConvention() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::C;
    }

    bool MethodSignature::HasStdCallConvention() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::StdCall;
    }

    bool MethodSignature::HasThisCallConvention() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::ThisCall;
    }

    bool MethodSignature::HasFastCallConvention() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::FastCall;
    }

    bool MethodSignature::IsGeneric() const
    {
        return SignatureFlags(Private::PeekByte(_first, _last))
            .IsSet(SignatureAttribute::Generic);
    }

    IndexType MethodSignature::GetGenericParameterCount() const
    {
        if (!IsGeneric())
            return 0;

        ByteIterator current(_first);
        Private::ReadByte(current, _last);
        return Private::PeekCompressedUInt32(current, _last);
    }

    IndexType TypeSignature::ComputeSize() const
    {
        return 0; // TODO
    }

    ByteIterator TypeSignature::GetFirstCustomModifier() const
    {
        if (_first == _last)
            return _last;

        ByteIterator current(_first);

        Byte const initialByte(Private::ReadByte(current, _last));
        if (current == _last)
            return _last;

        if (initialByte == SignatureAttribute::Field)
        {
            return Private::IsCustomModifierElementType(Private::PeekByte(current, _last)) ? current : _last;
        }

        if (initialByte == SignatureAttribute::Property ||
            initialByte == (SignatureAttribute::Property | SignatureAttribute::HasThis))
        {
            // Skip the ParamCount:
            Private::ReadCompressedUInt32(current, _last);

            return Private::IsCustomModifierElementType(Private::PeekByte(current, _last)) ? current : _last;
        }

        // TODO LocalVarSig support

        // Any other kind of signature starts immediately with custom modifiers (e.g. Param, RetType)
        // Note that CustomModifiers also appear in the PTR and SZARRAY
        // and the two PTR kinds of Type and the SZARRAY kind of type).
        return Private::IsCustomModifierElementType(initialByte) ? current : _last;
    }

    ByteIterator TypeSignature::GetFirstType() const
    {
        if (IsTypeElementType(Private::PeekByte(_first, _last)))
            return _first;
        return nullptr; // TODO
    }

} }
