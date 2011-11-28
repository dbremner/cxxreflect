//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/MetadataSignature.hpp"

// TODO:  There are parts of the signature reading that are embarrasingly inefficient, requiring
// multiple scans of the signature just to read each part from it.  We should come back and clean
// this up once we have everything working neatly.

namespace { namespace Private {

    using namespace CxxReflect;
    using namespace CxxReflect::Metadata;

    char const* const IteratorReadUnexpectedEnd("Unexpectedly reached end of range");

    Byte ReadByte(ByteIterator& it, ByteIterator const last)
    {
        if (it == last)
            throw ReadError(Private::IteratorReadUnexpectedEnd);

        return *it++;
    }

    Byte PeekByte(ByteIterator it, ByteIterator const last)
    {
        return ReadByte(it, last);
    }

    struct CompressedIntBytes
    {
        std::array<Byte, 4> Bytes;
        SizeType            Count;

        CompressedIntBytes()
            : Bytes(), Count()
        {
        }
    };

    CompressedIntBytes ReadCompressedIntBytes(ByteIterator& it, ByteIterator const last)
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
            result.Bytes[i] = ReadByte(it, last);

        // TODO Integrate this reversal into the above logic
        std::reverse(result.Bytes.begin(), result.Bytes.begin() + result.Count);

        return result;
    }

    std::int32_t ReadCompressedInt32(ByteIterator& it, ByteIterator const last)
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
            std::uint32_t p(*reinterpret_cast<std::uint16_t*>(&bytes.Bytes[0]));
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

    std::int32_t PeekCompressedInt32(ByteIterator it, ByteIterator const last)
    {
        return ReadCompressedInt32(it, last);
    }

    std::uint32_t ReadCompressedUInt32(ByteIterator& it, ByteIterator const last)
    {
        CompressedIntBytes const bytes(ReadCompressedIntBytes(it, last));

        switch (bytes.Count)
        {
        case 1:  return *reinterpret_cast<std::uint8_t  const*>(&bytes.Bytes[0]);
        case 2:  return *reinterpret_cast<std::uint16_t const*>(&bytes.Bytes[0]);
        case 4:  return *reinterpret_cast<std::uint32_t const*>(&bytes.Bytes[0]);
        default: Detail::VerifyFail("It is impossible to get here"); return 0;
        }
    }

    std::uint32_t PeekCompressedUInt32(ByteIterator it, ByteIterator const last)
    {
        return ReadCompressedUInt32(it, last);
    }

    std::uint32_t ReadTypeDefOrRefOrSpecEncoded(ByteIterator& it, ByteIterator const last)
    {
        std::uint32_t const tokenValue(ReadCompressedUInt32(it, last));
        std::uint32_t const tokenType(tokenValue & 0x03);

        switch (tokenType)
        {
        case 0x00: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeDef)  << 24);
        case 0x01: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeRef)  << 24);
        case 0x02: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeSpec) << 24);
        default:   throw ReadError("Unexpected table id in TypeDefOrRefOrSpecEncoded");
        }
    }

    std::uint32_t PeekTypeDefOrRefOrSpecEncoded(ByteIterator it, ByteIterator const last)
    {
        return ReadTypeDefOrRefOrSpecEncoded(it, last);
    }

    ElementType ReadElementType(ByteIterator& it, ByteIterator const last)
    {
        Byte const value(ReadByte(it, last));
        if (!IsValidElementType(value))
            throw ReadError("Unexpected element type");

        return static_cast<ElementType>(value);
    }

    ElementType PeekElementType(ByteIterator it, ByteIterator const last)
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

    SignatureComparer::SignatureComparer(MetadataLoader const* loader,
                                         Database const* lhsDatabase,
                                         Database const* rhsDatabase)
        : _loader(loader), _lhsDatabase(lhsDatabase), _rhsDatabase(rhsDatabase)
    {
        Detail::VerifyNotNull(loader);
        Detail::VerifyNotNull(lhsDatabase);
        Detail::VerifyNotNull(rhsDatabase);
    }

    bool SignatureComparer::operator()(ArrayShape const& lhs, ArrayShape const& rhs) const
    {
        if (lhs.GetRank() != rhs.GetRank())
            return false;

        if (!Detail::RangeCheckedEqual(
                lhs.BeginSizes(), lhs.EndSizes(),
                rhs.BeginSizes(), rhs.EndSizes()))
            return false;

        if (!Detail::RangeCheckedEqual(
                lhs.BeginLowBounds(), lhs.EndLowBounds(),
                rhs.BeginLowBounds(), rhs.EndLowBounds()))
            return false;

        return true;
    }

    bool SignatureComparer::operator()(CustomModifier const& lhs, CustomModifier const& rhs) const
    {
        if (lhs.IsOptional() != rhs.IsOptional())
            return false;

        if (!(*this)(lhs.GetTypeReference(), rhs.GetTypeReference()))
            return false;

        return true;
    }

    bool SignatureComparer::operator()(FieldSignature const& lhs, FieldSignature const& rhs) const
    {
        if (!(*this)(lhs.GetTypeSignature(), rhs.GetTypeSignature()))
            return false;

        return true;
    }

    bool SignatureComparer::operator()(MethodSignature const& lhs, MethodSignature const& rhs) const
    {
        if (lhs.GetCallingConvention() != rhs.GetCallingConvention())
            return false;

        if (lhs.HasThis() != rhs.HasThis())
            return false;

        if (lhs.HasExplicitThis() != rhs.HasExplicitThis())
            return false;

        if (lhs.IsGeneric() != rhs.IsGeneric())
            return false;

        if (lhs.GetGenericParameterCount() != rhs.GetGenericParameterCount())
            return false;

        // TODO Check assignable-to?  Shouldn't this always be the case for derived classes?

        if (lhs.GetParameterCount() != rhs.GetParameterCount())
            return false;

        if (!Detail::RangeCheckedEqual(
                lhs.BeginParameters(), lhs.EndParameters(),
                rhs.BeginParameters(), rhs.EndParameters(),
                *this))
            return false;

        if (!(*this)(lhs.GetReturnType(), rhs.GetReturnType()))
            return false;

        return true;
    }

    bool SignatureComparer::operator()(PropertySignature const& lhs, PropertySignature const& rhs) const
    {
        if (lhs.HasThis() != rhs.HasThis())
            return false;

        if (!Detail::RangeCheckedEqual(
                lhs.BeginParameters(), lhs.EndParameters(),
                rhs.BeginParameters(), rhs.EndParameters(),
                *this))
            return false;

        if (!(*this)(lhs.GetTypeSignature(), rhs.GetTypeSignature()))
            return false;

        return true;
    }

    bool SignatureComparer::operator()(TypeSignature const& lhs, TypeSignature const& rhs) const
    {
        // TODO DO WE NEED TO CHECK CUSTOM MODIFIERS?

        if (lhs.GetKind() != rhs.GetKind())
            return false;

        if (lhs.GetKind() == TypeSignature::Kind::Unknown || rhs.GetKind() == TypeSignature::Kind::Unknown)
            return false;

        switch (lhs.GetKind())
        {
        case TypeSignature::Kind::Array:
        {
            if (!(*this)(lhs.GetArrayType(), rhs.GetArrayType()))
                return false;

            if (!(*this)(lhs.GetArrayShape(), rhs.GetArrayShape()))
                return false;

            return true;
        }

        case TypeSignature::Kind::ClassType:
        {
            if (lhs.IsClassType() != rhs.IsClassType())
                return false;

            if (!(*this)(lhs.GetTypeReference(), rhs.GetTypeReference()))
                return false;

            return true;
        }

        case TypeSignature::Kind::FnPtr:
        {
            if (!(*this)(lhs.GetMethodSignature(), rhs.GetMethodSignature()))
                return false;

            return true;
        }

        case TypeSignature::Kind::GenericInst:
        {
            if (lhs.IsGenericClassTypeInstance() != rhs.IsGenericClassTypeInstance())
                return false;

            if (lhs.GetGenericTypeReference() != rhs.GetGenericTypeReference())
                return false;

            if (lhs.GetGenericArgumentCount() != rhs.GetGenericArgumentCount())
                return false;

            if (!Detail::RangeCheckedEqual(
                    lhs.BeginGenericArguments(), lhs.EndGenericArguments(),
                    rhs.BeginGenericArguments(), rhs.EndGenericArguments(),
                    *this))
                return false;

            return true;
        }

        case TypeSignature::Kind::Primitive:
        {
            if (lhs.GetPrimitiveElementType() != rhs.GetPrimitiveElementType())
                return false;

            return true;
        }

        case TypeSignature::Kind::Ptr:
        {
            if (!(*this)(lhs.GetPointerTypeSignature(), rhs.GetPointerTypeSignature()))
                return false;

            return true;
        }

        case TypeSignature::Kind::SzArray:
        {
            if (!(*this)(lhs.GetArrayType(), rhs.GetArrayType()))
                return false;

            return true;
        }

        case TypeSignature::Kind::Var:
        {
            if (lhs.IsClassVariableType() != rhs.IsClassVariableType())
                return false;

            if (lhs.GetVariableNumber() != rhs.GetVariableNumber())
                return false;

            return true;
        }

        default:
        {
            return false;
        }
        }
    }

    bool SignatureComparer::operator()(TableReference const& lhs, TableReference const& rhs) const
    {
        // TODO Do we need to handle generic type argument instantiation here?
        DatabaseReference const lhsResolved(_loader.Get()->ResolveType(DatabaseReference(_lhsDatabase.Get(), lhs), InternalKey()));
        DatabaseReference const rhsResolved(_loader.Get()->ResolveType(DatabaseReference(_rhsDatabase.Get(), rhs), InternalKey()));

        // If the types are from different tables, they cannot be equal:
        if (lhsResolved.GetTableReference().GetTable() != rhsResolved.GetTableReference().GetTable())
            return false;

        // If we have a pair of TypeDefs, they are only equal if they refer to the same type in the
        // same database; in no other case can they be equal:
        if (lhsResolved.GetTableReference().GetTable() == TableId::TypeDef)
        {
            if (lhsResolved.GetDatabase() == rhsResolved.GetDatabase() &&
                lhsResolved.GetTableReference() == rhsResolved.GetTableReference())
                return true;

            return false;
        }

        // Otherwise, we have a pair of TypeSpec tokens and we have to compare them recursively:
        Database const& lhsDatabase(lhsResolved.GetDatabase());
        Database const& rhsDatabase(rhsResolved.GetDatabase());

        SizeType const lhsRowIndex(lhsResolved.GetTableReference().GetIndex());
        SizeType const rhsRowIndex(rhsResolved.GetTableReference().GetIndex());

        TypeSpecRow const lhsTypeSpec(lhsDatabase.GetRow<TableId::TypeSpec>(lhsRowIndex));
        TypeSpecRow const rhsTypeSpec(rhsDatabase.GetRow<TableId::TypeSpec>(rhsRowIndex));

        Blob const lhsSignature(lhsDatabase.GetBlob(lhsTypeSpec.GetSignature()));
        Blob const rhsSignature(rhsDatabase.GetBlob(rhsTypeSpec.GetSignature()));

        // Note that we use a new signature comparer because the LHS and RHS signatures may have come
        // from new and/or different databases.
        if (SignatureComparer(_loader.Get(), &lhsDatabase, &rhsDatabase)(
                TypeSignature(lhsSignature.Begin(), lhsSignature.End()),
                TypeSignature(rhsSignature.Begin(), rhsSignature.End())))
            return true;

        return false;
    }




    template <typename TSignature>
    TSignature ClassVariableSignatureInstantiator::Instantiate(TSignature const& signature) const
    {
        _buffer.clear();
        InstantiateInto(_buffer, signature);
        return TSignature(&*_buffer.begin(), &*_buffer.end());
    }

    template ArrayShape        ClassVariableSignatureInstantiator::Instantiate<ArrayShape>       (ArrayShape        const&) const;
    template FieldSignature    ClassVariableSignatureInstantiator::Instantiate<FieldSignature>   (FieldSignature    const&) const;
    template MethodSignature   ClassVariableSignatureInstantiator::Instantiate<MethodSignature>  (MethodSignature   const&) const;
    template PropertySignature ClassVariableSignatureInstantiator::Instantiate<PropertySignature>(PropertySignature const&) const;
    template TypeSignature     ClassVariableSignatureInstantiator::Instantiate<TypeSignature>    (TypeSignature     const&) const;

    template <typename TSignature>
    bool ClassVariableSignatureInstantiator::RequiresInstantiation(TSignature const& signature) const
    {
        return RequiresInstantiationInternal(signature);
    }

    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<ArrayShape>       (ArrayShape        const&) const;
    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<FieldSignature>   (FieldSignature    const&) const;
    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<MethodSignature>  (MethodSignature   const&) const;
    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<PropertySignature>(PropertySignature const&) const;
    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<TypeSignature>    (TypeSignature     const&) const;

    void ClassVariableSignatureInstantiator::InstantiateInto(InternalBuffer& buffer, ArrayShape const& s) const
    {
        typedef ArrayShape::Part Part;

        CopyBytesInto(buffer, s, Part::Begin, Part::End);
    }

    void ClassVariableSignatureInstantiator::InstantiateInto(InternalBuffer& buffer, FieldSignature const& s) const
    {
        typedef FieldSignature::Part Part;

        CopyBytesInto(buffer, s, Part::Begin, Part::Type);
        InstantiateInto(buffer, s.GetTypeSignature());
    }

    void ClassVariableSignatureInstantiator::InstantiateInto(InternalBuffer& buffer, MethodSignature const& s) const
    {
        typedef MethodSignature::Part Part;

        CopyBytesInto(buffer, s, Part::Begin, Part::RetType);
        InstantiateInto(buffer, s.GetReturnType());
        InstantiateRangeInto(buffer, s.BeginParameters(), s.EndParameters());

        if (s.BeginVarargParameters() == s.EndVarargParameters())
            return;

        CopyBytesInto(buffer, s, Part::Sentinel, Part::FirstVarargParam);
        InstantiateRangeInto(buffer, s.BeginVarargParameters(), s.EndVarargParameters());
    }

    void ClassVariableSignatureInstantiator::InstantiateInto(InternalBuffer& buffer, PropertySignature const& s) const
    {
        typedef PropertySignature::Part Part;

        CopyBytesInto(buffer, s, Part::Begin, Part::Type);
        InstantiateInto(buffer, s.GetTypeSignature());
        InstantiateRangeInto(buffer, s.BeginParameters(), s.EndParameters());
    }

    void ClassVariableSignatureInstantiator::InstantiateInto(InternalBuffer& buffer, TypeSignature const& s) const
    {
        typedef TypeSignature::Part Part;

        switch (s.GetKind())
        {
        case TypeSignature::Kind::Primitive:
        case Metadata::TypeSignature::Kind::ClassType:
        {
            CopyBytesInto(buffer, s, Part::Begin, Part::End);
            break;
        }
        case TypeSignature::Kind::Array:
        {
            CopyBytesInto(buffer, s, Part::Begin, Part::ArrayType);
            InstantiateInto(buffer, s.GetArrayType());
            CopyBytesInto(buffer, s, Part::ArrayShape, Part::End);
            break;
        }
        case TypeSignature::Kind::SzArray:
        {
            CopyBytesInto(buffer, s, Part::Begin, Part::SzArrayType);
            InstantiateInto(buffer, s.GetArrayType());
            break;
        }
        case TypeSignature::Kind::FnPtr:
        {
            CopyBytesInto(buffer, s, Part::Begin, Part::MethodSignature);
            InstantiateInto(buffer, s.GetMethodSignature());
            break;
        }
        case TypeSignature::Kind::GenericInst:
        {
            CopyBytesInto(buffer, s, Part::Begin, Part::FirstGenericInstArgument);
            InstantiateRangeInto(buffer, s.BeginGenericArguments(), s.EndGenericArguments());
            break;
        }
        case TypeSignature::Kind::Ptr:
        {
            CopyBytesInto(buffer, s, Part::Begin, Part::PointerTypeSignature);
            InstantiateInto(buffer, s.GetPointerTypeSignature());
            break;
        }
        case TypeSignature::Kind::Var:
        {
            if (s.IsClassVariableType())
            {
                SizeType const variableNumber(s.GetVariableNumber());
                if (variableNumber > _arguments.size())
                    throw std::logic_error("wtf"); // Invalid metadata?

                CopyBytesInto(buffer, _arguments[variableNumber], Part::Begin, Part::End);
            }
            else if (s.IsMethodVariableType())
            {
                CopyBytesInto(buffer, s, Part::Begin, Part::End);
            }
            else
            {
                throw std::logic_error("wtf");
            }
            break;
        }
        default:
        {
            throw std::logic_error("wtf");
        }
        }
    }

    template <typename TSignature, typename TPart>
    void ClassVariableSignatureInstantiator::CopyBytesInto(InternalBuffer      & buffer,
                                                           TSignature     const& s,
                                                           TPart          const  first,
                                                           TPart          const  last) const
    {
        std::copy(s.SeekTo(first), s.SeekTo(last), std::back_inserter(buffer));
    }

    template <typename TForIt>
    void ClassVariableSignatureInstantiator::InstantiateRangeInto(InternalBuffer      & buffer,
                                                                  TForIt         const  first,
                                                                  TForIt         const  last) const
    {
        std::for_each(first, last, [&](decltype(*first) const& s) { InstantiateInto(buffer, s); });
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(ArrayShape const&) const
    {
        return false;
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(FieldSignature const& s) const
    {
        return RequiresInstantiationInternal(s.GetTypeSignature());
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(MethodSignature const& s) const
    {
        if (RequiresInstantiationInternal(s.GetReturnType()))
            return true;

        if (AnyRequiresInstantiationInternal(s.BeginParameters(), s.EndParameters()))
            return true;

        if (AnyRequiresInstantiationInternal(s.BeginVarargParameters(), s.EndVarargParameters()))
            return true;

        return false;
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(PropertySignature const& s) const
    {
        if (RequiresInstantiationInternal(s.GetTypeSignature()))
            return true;

        if (AnyRequiresInstantiationInternal(s.BeginParameters(), s.EndParameters()))
            return true;

        return false;
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(TypeSignature const& s) const
    {
        switch (s.GetKind())
        {
        case TypeSignature::Kind::ClassType:
        case TypeSignature::Kind::Primitive:
            return false;

        case TypeSignature::Kind::Array:
        case TypeSignature::Kind::SzArray:
            return RequiresInstantiationInternal(s.GetArrayType());

        case TypeSignature::Kind::FnPtr:
            return RequiresInstantiationInternal(s.GetMethodSignature());

        case TypeSignature::Kind::GenericInst:
            return AnyRequiresInstantiationInternal(s.BeginGenericArguments(), s.EndGenericArguments());
            
        case TypeSignature::Kind::Ptr:
            return RequiresInstantiationInternal(s.GetPointerTypeSignature());

        case TypeSignature::Kind::Var:
            return s.IsClassVariableType();

        default:
            throw std::logic_error("wtf");
        }
    }

    template <typename TForIt>
    bool ClassVariableSignatureInstantiator::AnyRequiresInstantiationInternal(TForIt const first,
                                                                              TForIt const last) const
    {
        return std::any_of(first, last, [&](decltype(*first) const& s)
        {
            return RequiresInstantiationInternal(s);
        });
    }




    BaseSignature::BaseSignature()
    {
    }

    BaseSignature::BaseSignature(ByteIterator const first, ByteIterator const last)
        : _first(first), _last(last)
    {
        Detail::VerifyNotNull(first);
        Detail::VerifyNotNull(last);
    }

    BaseSignature::BaseSignature(BaseSignature const& other)
        : _first(other._first), _last(other._last)
    {
    }

    BaseSignature& BaseSignature::operator=(BaseSignature const& other)
    {
        _first = other._first;
        _last = other._last;
        return *this;
    }

    BaseSignature::~BaseSignature()
    {
    }

    ByteIterator BaseSignature::BeginBytes() const
    {
        VerifyInitialized();
        return _first.Get();
    }

    ByteIterator BaseSignature::EndBytes() const
    {
        VerifyInitialized();
        return _last.Get();
    }

    bool BaseSignature::IsInitialized() const
    {
        return BeginBytes() != nullptr && EndBytes() != nullptr;
    }

    void BaseSignature::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }




    ArrayShape::ArrayShape()
    {
    }

    ArrayShape::ArrayShape(ByteIterator const first, ByteIterator const last)
        : BaseSignature(first, last)
    {
    }

    IndexType ArrayShape::GetRank() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::Rank), EndBytes());
    }

    IndexType ArrayShape::GetSizesCount() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::NumSizes), EndBytes());
    }

    ArrayShape::SizeIterator ArrayShape::BeginSizes() const
    {
        VerifyInitialized();

        return SizeIterator(SeekTo(Part::FirstSize), EndBytes(), 0, GetSizesCount());
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

        return Private::PeekCompressedUInt32(SeekTo(Part::NumLowBounds), EndBytes());
    }

    ArrayShape::LowBoundIterator ArrayShape::BeginLowBounds() const
    {
        VerifyInitialized();

        return LowBoundIterator(SeekTo(Part::FirstLowBound), EndBytes(), 0, GetLowBoundsCount());
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

        return SeekTo(Part::End) - BeginBytes();
    }

    ByteIterator ArrayShape::SeekTo(Part const part) const
    {
        VerifyInitialized();

        ByteIterator current(BeginBytes());

        if (part > Part::Rank)
        {
            Private::ReadCompressedUInt32(current, EndBytes());
        }

        IndexType numSizes(0);
        if (part > Part::NumSizes)
        {
            numSizes = Private::ReadCompressedUInt32(current, EndBytes());
        }

        if (part > Part::FirstSize)
        {
            for (unsigned i(0); i < numSizes; ++i)
                Private::ReadCompressedUInt32(current, EndBytes());
        }

        IndexType numLowBounds(0);
        if (part > Part::NumLowBounds)
        {
            numLowBounds = Private::ReadCompressedUInt32(current, EndBytes());
        }

        if (part > Part::FirstLowBound)
        {
            for (unsigned i(0); i < numSizes; ++i)
                Private::ReadCompressedUInt32(current, EndBytes());
        }

        if (part > Part::End)
        {
            Detail::VerifyFail("Invalid signature part requested");
        }

        return current;
    }

    SizeType ArrayShape::ReadSize(ByteIterator& current, ByteIterator const last)
    {
        return Private::ReadCompressedUInt32(current, last);
    }

    SizeType ArrayShape::ReadLowBound(ByteIterator& current, ByteIterator const last)
    {
        return Private::ReadCompressedUInt32(current, last);
    }




    CustomModifier::CustomModifier()
    {
    }

    CustomModifier::CustomModifier(ByteIterator const first, ByteIterator const last)
        : BaseSignature(first, last)
    {
        Detail::Verify([&]{ return IsOptional() || IsRequired(); });
    }

    bool CustomModifier::IsOptional() const
    {
        VerifyInitialized();

        return Private::PeekByte(SeekTo(Part::ReqOptFlag), EndBytes()) == ElementType::CustomModifierOptional;
    }

    bool CustomModifier::IsRequired() const
    {
        VerifyInitialized();

        return Private::PeekByte(SeekTo(Part::ReqOptFlag), EndBytes()) == ElementType::CustomModifierRequired;
    }

    TableReference CustomModifier::GetTypeReference() const
    {
        VerifyInitialized();

        return TableReference::FromToken(
            Private::PeekTypeDefOrRefOrSpecEncoded(SeekTo(Part::Type), EndBytes()));
    }

    IndexType CustomModifier::ComputeSize() const
    {
        VerifyInitialized();

        return SeekTo(Part::End) - BeginBytes();
    }

    ByteIterator CustomModifier::SeekTo(Part const part) const
    {
        VerifyInitialized();

        ByteIterator current(BeginBytes());

        if (part > Part::ReqOptFlag)
        {
            Private::ReadByte(current, EndBytes());
        }

        if (part > Part::Type)
        {
            Private::ReadTypeDefOrRefOrSpecEncoded(current, EndBytes());
        }

        if (part > Part::End)
        {
            Detail::VerifyFail("Invalid signature part requested");
        }

        return current;
    }




    FieldSignature::FieldSignature()
    {
    }

    FieldSignature::FieldSignature(ByteIterator const first, ByteIterator const last)
        : BaseSignature(first, last)
    {
        Detail::Verify([&]
        {
            return Private::PeekByte(SeekTo(Part::FieldTag), EndBytes()) == SignatureAttribute::Field; }
        );
    }

    TypeSignature FieldSignature::GetTypeSignature() const
    {
        VerifyInitialized();

        return TypeSignature(SeekTo(Part::Type), EndBytes());
    }

    SizeType FieldSignature::ComputeSize() const
    {
        VerifyInitialized();

        return SeekTo(Part::End) - BeginBytes();
    }

    ByteIterator FieldSignature::SeekTo(Part const part) const
    {
        VerifyInitialized();

        ByteIterator current(BeginBytes());

        if (part > Part::FieldTag)
        {
            Private::ReadByte(current, EndBytes());
        }

        if (part > Part::Type)
        {
            current += TypeSignature(current, EndBytes()).ComputeSize();
        }

        if (part > Part::End)
        {
            Detail::VerifyFail("Invalid signature part requested");
        }

        return current;
    }




    PropertySignature::PropertySignature()
    {
    }

    PropertySignature::PropertySignature(ByteIterator const first, ByteIterator const last)
        : BaseSignature(first, last)
    {
        Detail::Verify([&]() -> bool
        {
            Byte const initialByte(Private::PeekByte(SeekTo(Part::PropertyTag), EndBytes()));
            return initialByte == SignatureAttribute::Property
                || initialByte == (SignatureAttribute::Property | SignatureAttribute::HasThis);
        });
    }

    bool PropertySignature::HasThis() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::PropertyTag), EndBytes()))
            .IsSet(SignatureAttribute::HasThis);
    }

    SizeType PropertySignature::GetParameterCount() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::ParameterCount), EndBytes());
    }

    PropertySignature::ParameterIterator PropertySignature::BeginParameters() const
    {
        VerifyInitialized();

        return ParameterIterator(BeginBytes(), EndBytes(), 0, GetParameterCount());
    }

    PropertySignature::ParameterIterator PropertySignature::EndParameters() const
    {
        VerifyInitialized();

        SizeType const parameterCount(GetParameterCount());
        return ParameterIterator(nullptr, nullptr, parameterCount, parameterCount);
    }

    TypeSignature PropertySignature::GetTypeSignature() const
    {
        VerifyInitialized();

        return TypeSignature(SeekTo(Part::Type), EndBytes());
    }

    SizeType PropertySignature::ComputeSize() const
    {
        VerifyInitialized();

        return SeekTo(Part::End) - BeginBytes();
    }

    ByteIterator PropertySignature::SeekTo(Part const part) const
    {
        VerifyInitialized();

        ByteIterator current(BeginBytes());

        if (part > Part::PropertyTag)
        {
            Private::ReadByte(current, EndBytes());
        }

        SizeType parameterCount(0);
        if (part > Part::ParameterCount)
        {
            parameterCount = Private::ReadCompressedUInt32(current, EndBytes());
        }

        if (part > Part::Type)
        {
            current += TypeSignature(current, EndBytes()).ComputeSize();
        }

        if (part > Part::FirstParameter)
        {
            for (unsigned i(0); i < parameterCount; ++i)
                current += TypeSignature(current, EndBytes()).ComputeSize();
        }

        if (part > Part::End)
        {
            Detail::VerifyFail("Invalid signature part requested");
        }

        return current;
    }

    TypeSignature PropertySignature::ReadParameter(ByteIterator& current, ByteIterator const last)
    {
        TypeSignature const type(current, last);
        current += type.ComputeSize();
        return type;
    }




    MethodSignature::MethodSignature()
    {
    }

    MethodSignature::MethodSignature(ByteIterator const first, ByteIterator const last)
        : BaseSignature(first, last)
    {
    }

    bool MethodSignature::ParameterEndCheck(ByteIterator current, ByteIterator last)
    {
        return Private::PeekByte(current, last) == ElementType::Sentinel;
    }

    TypeSignature MethodSignature::ReadParameter(ByteIterator& current, ByteIterator last)
    {
        TypeSignature const typeSignature(current, last);
        current += typeSignature.ComputeSize();
        return typeSignature;
    }

    bool MethodSignature::HasThis() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .IsSet(SignatureAttribute::HasThis);
    }

    bool MethodSignature::HasExplicitThis() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .IsSet(SignatureAttribute::ExplicitThis);
    }

    SignatureAttribute MethodSignature::GetCallingConvention() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask)
            .GetEnum();
    }

    bool MethodSignature::HasDefaultConvention() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::Default;
    }

    bool MethodSignature::HasVarArgConvention() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::VarArg;
    }

    bool MethodSignature::HasCConvention() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::C;
    }

    bool MethodSignature::HasStdCallConvention() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::StdCall;
    }

    bool MethodSignature::HasThisCallConvention() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::ThisCall;
    }


    bool MethodSignature::HasFastCallConvention() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::FastCall;
    }

    bool MethodSignature::IsGeneric() const
    {
        VerifyInitialized();

        return SignatureFlags(Private::PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .IsSet(SignatureAttribute::Generic);
    }

    SizeType MethodSignature::GetGenericParameterCount() const
    {
        VerifyInitialized();

        if (!IsGeneric())
            return 0;

        return Private::PeekCompressedUInt32(SeekTo(Part::GenParamCount), EndBytes());
    }

    TypeSignature MethodSignature::GetReturnType() const
    {
        VerifyInitialized();

        return TypeSignature(SeekTo(Part::RetType), EndBytes());
    }

    SizeType MethodSignature::GetParameterCount() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::ParamCount), EndBytes());
    }

    MethodSignature::ParameterIterator MethodSignature::BeginParameters() const
    {
        VerifyInitialized();

        return ParameterIterator(SeekTo(Part::FirstParam), EndBytes(), 0, GetParameterCount());
    }

    MethodSignature::ParameterIterator MethodSignature::EndParameters() const
    {
        VerifyInitialized();

        SizeType const parameterCount(GetParameterCount());
        return ParameterIterator(nullptr, nullptr, parameterCount, parameterCount);
    }

    MethodSignature::ParameterIterator MethodSignature::BeginVarargParameters() const
    {
        VerifyInitialized();

        SizeType const parameterCount(GetParameterCount());
        SizeType const actualParameters(std::distance(BeginParameters(), EndParameters()));
        SizeType const varArgParameters(parameterCount - actualParameters);

        return ParameterIterator(SeekTo(Part::FirstVarargParam), EndBytes(), 0, varArgParameters);
    }

    MethodSignature::ParameterIterator MethodSignature::EndVarargParameters() const
    {
        VerifyInitialized();

        SizeType const parameterCount(GetParameterCount());
        SizeType const actualParameters(std::distance(BeginParameters(), EndParameters()));
        SizeType const varArgParameters(parameterCount - actualParameters);

        return ParameterIterator(nullptr, nullptr, varArgParameters, varArgParameters);
    }

    SizeType MethodSignature::ComputeSize() const
    {
        VerifyInitialized();

        return SeekTo(Part::End) - BeginBytes();
    }

    ByteIterator MethodSignature::SeekTo(Part const part) const
    {
        VerifyInitialized();

        ByteIterator current(BeginBytes());

        SignatureFlags typeFlags;
        if (part > Part::TypeTag)
        {
            typeFlags = Private::ReadByte(current, EndBytes());
        }

        if (part == Part::GenParamCount && !typeFlags.IsSet(SignatureAttribute::Generic))
        {
            return nullptr;
        }

        if (part > Part::GenParamCount && typeFlags.IsSet(SignatureAttribute::Generic))
        {
            Private::ReadCompressedUInt32(current, EndBytes());
        }

        SizeType parameterCount(0);
        if (part > Part::ParamCount)
        {
            parameterCount = Private::ReadCompressedUInt32(current, EndBytes());
        }

        if (part > Part::RetType)
        {
            current += TypeSignature(current, EndBytes()).ComputeSize();
        }

        unsigned parametersRead(0);
        if (part > Part::FirstParam)
        {
            while (Private::PeekByte(current, EndBytes()) != ElementType::Sentinel
                && parametersRead < parameterCount)
            {
                ++parametersRead;
                current += TypeSignature(current, EndBytes()).ComputeSize();
            }

            if (Private::PeekByte(current, EndBytes()) == ElementType::Sentinel)
            {
                current += Private::ReadByte(current, EndBytes());
            }
        }

        if (part > Part::FirstVarargParam && parametersRead < parameterCount)
        {
            for (unsigned i(parametersRead); i != parameterCount; ++i)
            {
                current += TypeSignature(current, EndBytes()).ComputeSize();
            }
        }

        if (part > Part::End)
        {
            Detail::VerifyFail("Invalid signature part requested");
        }

        return current;
    }




    TypeSignature::TypeSignature()
    {
    }

    TypeSignature::TypeSignature(ByteIterator const first, ByteIterator const last)
        : BaseSignature(first, last)
    {
    }

    SizeType TypeSignature::ComputeSize() const
    {
        VerifyInitialized();

        return SeekTo(Part::End) - BeginBytes();
    }

    TypeSignature::Kind TypeSignature::GetKind() const
    {
        VerifyInitialized();

        switch (GetElementType())
        {
        case ElementType::Void:
        case ElementType::Boolean:
        case ElementType::Char:
        case ElementType::I1:
        case ElementType::U1:
        case ElementType::I2:
        case ElementType::U2:
        case ElementType::I4:
        case ElementType::U4:
        case ElementType::I8:
        case ElementType::U8:
        case ElementType::R4:
        case ElementType::R8:
        case ElementType::I:
        case ElementType::U:
        case ElementType::String:
        case ElementType::Object:
        case ElementType::TypedByRef:
            return Kind::Primitive;

        case ElementType::Array:
            return Kind::Array;

        case ElementType::SzArray:
            return Kind::SzArray;

        case ElementType::Class:
        case ElementType::ValueType:
            return Kind::ClassType;

        case ElementType::FnPtr:
            return Kind::FnPtr;

        case ElementType::GenericInst:
            return Kind::GenericInst;

        case ElementType::Ptr:
            return Kind::Ptr;

        case ElementType::MVar:
        case ElementType::Var:
            return Kind::Var;

        default:
            return Kind::Unknown;
        }
    }

    bool TypeSignature::IsKind(Kind const kind) const
    {
        return GetKind() == kind;
    }

    void TypeSignature::VerifyKind(Kind const kind) const
    {
        VerifyInitialized();
        Detail::Verify([&]{ return IsKind(kind); });
    }

    TypeSignature TypeSignature::ReadType(ByteIterator& current, ByteIterator last)
    {
        TypeSignature const typeSignature(current, last);
        current += typeSignature.ComputeSize();
        return typeSignature;
    }

    bool TypeSignature::CustomModifierEndCheck(ByteIterator current, ByteIterator last)
    {
        return current == last || !Private::IsCustomModifierElementType(Private::PeekByte(current, last));
    }

    CustomModifier TypeSignature::ReadCustomModifier(ByteIterator& current, ByteIterator last)
    {
        CustomModifier const customModifier(current, last);
        current += customModifier.ComputeSize();
        return customModifier;
    }

    TypeSignature::CustomModifierIterator TypeSignature::BeginCustomModifiers() const
    {
        VerifyInitialized();

        ByteIterator const firstCustomModifier(SeekTo(Part::FirstCustomMod));
        return CustomModifierIterator(firstCustomModifier, firstCustomModifier == nullptr ? nullptr : EndBytes());
    }

    TypeSignature::CustomModifierIterator TypeSignature::EndCustomModifiers() const
    {
        VerifyInitialized();

        return CustomModifierIterator();
    }

    ElementType TypeSignature::GetElementType() const
    {
        VerifyInitialized();

        Byte const typeTag(Private::PeekByte(SeekTo(Part::TypeCode), EndBytes()));
        return IsValidElementType(typeTag) ? static_cast<ElementType>(typeTag) : ElementType::End;
    }

    bool TypeSignature::IsByRef() const
    {
        VerifyInitialized();

        ByteIterator const byRefTag(SeekTo(Part::ByRefTag));
        return byRefTag != nullptr && Private::PeekByte(byRefTag, EndBytes()) == ElementType::ByRef;
    }

    bool TypeSignature::IsPrimitive() const
    {
        VerifyInitialized();

        return GetPrimitiveElementType() != ElementType::End;
    }

    ElementType TypeSignature::GetPrimitiveElementType() const
    {
        VerifyInitialized();

        ElementType const type(GetElementType());
        switch (type)
        {
        case ElementType::Boolean:
        case ElementType::Char:
        case ElementType::I1:
        case ElementType::U1:
        case ElementType::I2:
        case ElementType::U2:
        case ElementType::I4:
        case ElementType::U4:
        case ElementType::I8:
        case ElementType::U8:
        case ElementType::R4:
        case ElementType::R8:
        case ElementType::I:
        case ElementType::U:
        case ElementType::Object:
        case ElementType::String:
        case ElementType::Void:
        case ElementType::TypedByRef:
            return type;

        default:
            return ElementType::End;
        }
    }

    bool TypeSignature::IsGeneralArray() const
    {
        VerifyInitialized();

        return GetElementType() == ElementType::Array;
    }

    bool TypeSignature::IsSimpleArray() const
    {
        VerifyInitialized();

        return GetElementType() == ElementType::SzArray;
    }

    TypeSignature TypeSignature::GetArrayType() const
    {
        VerifyInitialized();

        return TypeSignature(
            IsKind(Kind::Array) ? SeekTo(Part::ArrayType) : SeekTo(Part::SzArrayType),
            EndBytes());
    }

    ArrayShape TypeSignature::GetArrayShape() const
    {
        VerifyInitialized();

        return ArrayShape(SeekTo(Part::ArrayShape), EndBytes());
    }

    bool TypeSignature::IsClassType() const
    {
        VerifyInitialized();

        return GetElementType() == ElementType::Class;
    }

    bool TypeSignature::IsValueType() const
    {
        VerifyInitialized();

        return GetElementType() == ElementType::ValueType;
    }

    TableReference TypeSignature::GetTypeReference() const
    {
        VerifyInitialized();

        return TableReference::FromToken(
            Private::PeekTypeDefOrRefOrSpecEncoded(SeekTo(Part::ClassTypeReference), EndBytes()));
    }

    bool TypeSignature::IsFunctionPointer() const
    {
        VerifyInitialized();
        
        return GetElementType() == ElementType::FnPtr;
    }

    MethodSignature TypeSignature::GetMethodSignature() const
    {
        VerifyInitialized();

        return MethodSignature(SeekTo(Part::MethodSignature), EndBytes());
    }

    bool TypeSignature::IsGenericInstance() const
    {
        VerifyInitialized();

        return GetElementType() == ElementType::GenericInst;
    }

    bool TypeSignature::IsGenericClassTypeInstance() const
    {
        VerifyInitialized();

        return Private::PeekByte(SeekTo(Part::GenericInstTypeCode), EndBytes()) == ElementType::Class;
    }

    bool TypeSignature::IsGenericValueTypeInstance() const
    {
        VerifyInitialized();

        return Private::PeekByte(SeekTo(Part::GenericInstTypeCode), EndBytes()) == ElementType::ValueType;
    }

    TableReference TypeSignature::GetGenericTypeReference() const
    {
        VerifyInitialized();

        return TableReference::FromToken(
            Private::PeekTypeDefOrRefOrSpecEncoded(SeekTo(Part::GenericInstTypeReference), EndBytes()));
    }

    SizeType TypeSignature::GetGenericArgumentCount() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::GenericInstArgumentCount), EndBytes());
    }

    TypeSignature::GenericArgumentIterator TypeSignature::BeginGenericArguments() const
    {
        VerifyInitialized();

        return GenericArgumentIterator(
            SeekTo(Part::FirstGenericInstArgument),
            EndBytes(),
            0,
            GetGenericArgumentCount());
    }

    TypeSignature::GenericArgumentIterator TypeSignature::EndGenericArguments() const
    {
        VerifyInitialized();

        SizeType const count(Private::PeekCompressedUInt32(SeekTo(Part::GenericInstArgumentCount), EndBytes()));
        return GenericArgumentIterator(nullptr, nullptr, count, count);
    }

    bool TypeSignature::IsPointer() const
    {
        VerifyInitialized();

        return GetElementType() == ElementType::Ptr;
    }

    TypeSignature TypeSignature::GetPointerTypeSignature() const
    {
        VerifyInitialized();

        return TypeSignature(SeekTo(Part::PointerTypeSignature), EndBytes());
    }

    bool TypeSignature::IsClassVariableType() const
    {
        VerifyInitialized();

        return GetElementType() == ElementType::Var;
    }

    bool TypeSignature::IsMethodVariableType() const
    {
        VerifyInitialized();

        return GetElementType() == ElementType::MVar;
    }

    SizeType TypeSignature::GetVariableNumber() const
    {
        VerifyInitialized();

        return Private::PeekCompressedUInt32(SeekTo(Part::VariableNumber), EndBytes());
    }

    ByteIterator TypeSignature::SeekTo(Part const part) const
    {
        VerifyInitialized();

        Kind const partKind(static_cast<Kind>(static_cast<SizeType>(part)) & Kind::Mask);
        Part const partCode(part & static_cast<Part>(~static_cast<SizeType>(Kind::Mask)));

        ByteIterator current(BeginBytes());

        if (partCode > Part::FirstCustomMod)
        {
            while (Private::IsCustomModifierElementType(Private::PeekByte(current, EndBytes())))
                current += CustomModifier(current, EndBytes()).ComputeSize();
        }

        if (partCode > Part::ByRefTag && Private::PeekByte(current, EndBytes()) == ElementType::ByRef)
        {
            Private::ReadByte(current, EndBytes());
        }

        if (partCode > Part::TypeCode)
        {
            Private::ReadByte(current, EndBytes());
            if (partKind != Kind::Unknown && !IsKind(partKind))
            {
                Detail::VerifyFail("Invalid signature part requested");
                // TODO RETURN EARLY?
            }

            auto const ExtractPart([](Part const p)
            {
                return static_cast<Part>(static_cast<SizeType>(p) & ~static_cast<SizeType>(Kind::Mask));
            });

            switch (GetKind())
            {
            case Kind::Primitive:
            {
                break;
            }
            case Kind::Array:
            {
                if (partCode > ExtractPart(Part::ArrayType))
                {
                    current += TypeSignature(current, EndBytes()).ComputeSize();
                }

                if (partCode > ExtractPart(Part::ArrayShape))
                {
                    current += ArrayShape(current, EndBytes()).ComputeSize();
                }

                break;
            }
            case Kind::SzArray:
            {
                if (partCode > ExtractPart(Part::SzArrayType))
                {
                    current += TypeSignature(current, EndBytes()).ComputeSize();
                }

                break;
            }
            case Kind::ClassType:
            {
                if (partCode > ExtractPart(Part::ClassTypeReference))
                {
                    Private::ReadTypeDefOrRefOrSpecEncoded(current, EndBytes());
                }

                break;
            }
            case Kind::FnPtr:
            {
                if (partCode > ExtractPart(Part::MethodSignature))
                {
                    current += MethodSignature(current, EndBytes()).ComputeSize();
                }

                break;
            }
            case Kind::GenericInst:
            {
                if (partCode > ExtractPart(Part::GenericInstTypeCode))
                {
                    Private::ReadByte(current, EndBytes());
                }

                if (partCode > ExtractPart(Part::GenericInstTypeReference))
                {
                    Private::ReadTypeDefOrRefOrSpecEncoded(current, EndBytes());
                }

                SizeType argumentCount(0);
                if (partCode > ExtractPart(Part::GenericInstArgumentCount))
                {
                    argumentCount = Private::ReadCompressedUInt32(current, EndBytes());
                }

                if (partCode > ExtractPart(Part::FirstGenericInstArgument))
                {
                    for (unsigned i(0); i < argumentCount; ++i)
                        current += TypeSignature(current, EndBytes()).ComputeSize();
                }

                break;
            }
            case Kind::Ptr:
            {
                if (partCode > ExtractPart(Part::PointerTypeSignature))
                {
                    current += TypeSignature(current, EndBytes()).ComputeSize();
                }

                break;
            }
            case Kind::Var:
            {
                if (partCode > ExtractPart(Part::VariableNumber))
                {
                    Private::ReadCompressedUInt32(current, EndBytes());
                }

                break;
            }
            default:
            {
                Detail::VerifyFail("It is impossible to get here");
            }
            }
        }

        if (partCode > Part::End)
        {
            Detail::VerifyFail("Invalid signature part requested");
        }

        return current;
    }

} }
