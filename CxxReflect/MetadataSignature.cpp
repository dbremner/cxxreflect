
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/CoreComponents.hpp"
#include "CxxReflect/Loader.hpp"

// TODO:  There are parts of the signature reading that are embarrasingly inefficient, requiring
// multiple scans of the signature just to read each part from it.  We should come back and clean
// this up once we have everything working neatly.

namespace CxxReflect { namespace Metadata { namespace { namespace Private {

    CharacterIterator const IteratorReadUnexpectedEnd(L"Unexpectedly reached end of range");

    struct CompressedIntBytes
    {
        std::array<Byte, 4> Bytes;
        SizeType            Count;

        CompressedIntBytes()
            : Bytes(), Count()
        {
        }
    };

    CompressedIntBytes ReadCompressedIntBytes(ConstByteIterator& it, ConstByteIterator const last)
    {
        CompressedIntBytes result;

        if (it == last)
            throw MetadataReadError(Private::IteratorReadUnexpectedEnd);

        // Note:  we've manually unrolled this for performance.  Thank you, Mr. Profiler.
        result.Bytes[0] = *it++;
        if ((result.Bytes[0] & 0x80) == 0)
        {
            result.Count = 1;
            return result;
        }
        else if ((result.Bytes[0] & 0x40) == 0)
        {
            result.Count = 2;
            result.Bytes[1] = result.Bytes[0];
            result.Bytes[1] ^= 0x80;

            if (last - it < 1)
                throw MetadataReadError(Private::IteratorReadUnexpectedEnd);

            result.Bytes[0] = *it++;
            return result;
        }
        else if ((result.Bytes[0] & 0x20) == 0)
        {
            result.Count = 4;
            result.Bytes[3] = result.Bytes[0];
            result.Bytes[3] ^= 0xC0;

            if (last - it < 3)
                throw MetadataReadError(Private::IteratorReadUnexpectedEnd);

            result.Bytes[2] = *it++;
            result.Bytes[1] = *it++;
            result.Bytes[0] = *it++;
            return result;
        }
        else
        {
            throw MetadataReadError(Private::IteratorReadUnexpectedEnd);
        }
    }

  

    bool IsCustomModifierElementType(Byte const value)
    {
        return value == ElementType::CustomModifierOptional
            || value == ElementType::CustomModifierRequired;
    }

} } } }

namespace CxxReflect { namespace Metadata {

    bool IsValidElementType(Byte const id)
    {
        static std::array<Byte, 0x60> const mask =
        {
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
        };

        return id < mask.size() && mask[id] == 1;
    }

    // Tests whether a given element type marks the beginning of a Type signature.
    bool IsTypeElementType(Byte const id)
    {
        static std::array<Byte, 0x20> const mask =
        {
            0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0
        };

        return id < mask.size() && mask[id] == 1;
    }

    bool IsIntegralElementType(ElementType const elementType)
    {
        switch (elementType)
        {
        case ElementType::I1:
        case ElementType::U1:
        case ElementType::I2:
        case ElementType::U2:
        case ElementType::I4:
        case ElementType::U4:
        case ElementType::I8:
        case ElementType::U8:
            return true;

        default:
            return false;
        }
    }

    bool IsSignedIntegralElementType(ElementType const elementType)
    {
        switch (elementType)
        {
        case ElementType::I1:
        case ElementType::I2:
        case ElementType::I4:
        case ElementType::I8:
            return true;

        default:
            return false;
        }
    }

    bool IsUnsignedIntegralElementType(ElementType const elementType)
    {
        switch (elementType)
        {
        case ElementType::U1:
        case ElementType::U2:
        case ElementType::U4:
        case ElementType::U8:
            return true;

        default:
            return false;
        }
    }

    bool IsRealElementType(ElementType const elementType)
    {
        return elementType == ElementType::R4 || elementType == ElementType::R8;
    }

    bool IsNumericElementType(ElementType const elementType)
    {
        return IsIntegralElementType(elementType) || IsRealElementType(elementType);
    }





    Byte ReadByte(ConstByteIterator& it, ConstByteIterator const last)
    {
        if (it == last)
            throw MetadataReadError(Private::IteratorReadUnexpectedEnd);

        return *it++;
    }

    Byte PeekByte(ConstByteIterator it, ConstByteIterator const last)
    {
        return ReadByte(it, last);
    }

    std::int32_t ReadCompressedInt32(ConstByteIterator& it, ConstByteIterator const last)
    {
        Private::CompressedIntBytes bytes(Private::ReadCompressedIntBytes(it, last));

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
            Detail::AssertFail(L"It is impossible to get here");
            return 0;
        }
        }
    }

    std::int32_t PeekCompressedInt32(ConstByteIterator it, ConstByteIterator const last)
    {
        return ReadCompressedInt32(it, last);
    }

    std::uint32_t ReadCompressedUInt32(ConstByteIterator& it, ConstByteIterator const last)
    {
        Private::CompressedIntBytes const bytes(Private::ReadCompressedIntBytes(it, last));

        switch (bytes.Count)
        {
        case 1:  return *reinterpret_cast<std::uint8_t  const*>(&bytes.Bytes[0]);
        case 2:  return *reinterpret_cast<std::uint16_t const*>(&bytes.Bytes[0]);
        case 4:  return *reinterpret_cast<std::uint32_t const*>(&bytes.Bytes[0]);
        default: Detail::AssertFail(L"It is impossible to get here"); return 0;
        }
    }

    std::uint32_t PeekCompressedUInt32(ConstByteIterator it, ConstByteIterator const last)
    {
        return ReadCompressedUInt32(it, last);
    }

    std::uint32_t ReadTypeDefOrRefOrSpec(ConstByteIterator& it, ConstByteIterator const last)
    {
        std::uint32_t const tokenValue(ReadCompressedUInt32(it, last));
        std::uint32_t const tokenType(tokenValue & 0x03);

        switch (tokenType)
        {
        case 0x00: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeDef)  << 24);
        case 0x01: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeRef)  << 24);
        case 0x02: return (tokenValue >> 2) | (Detail::AsInteger(TableId::TypeSpec) << 24);
        default:   throw MetadataReadError(L"Unexpected table id in TypeDefOrRefOrSpecEncoded");
        }
    }

    std::uint32_t PeekTypeDefOrRefOrSpec(ConstByteIterator it, ConstByteIterator const last)
    {
        return ReadTypeDefOrRefOrSpec(it, last);
    }

    ElementType ReadElementType(ConstByteIterator& it, ConstByteIterator const last)
    {
        Byte const value(ReadByte(it, last));
        if (!IsValidElementType(value))
            throw MetadataReadError(L"Unexpected element type");

        return static_cast<ElementType>(value);
    }

    ElementType PeekElementType(ConstByteIterator it, ConstByteIterator const last)
    {
        return ReadElementType(it, last);
    }

    std::uintptr_t ReadPointer(ConstByteIterator& it, ConstByteIterator const last)
    {
        if (std::distance(it, last) < sizeof(std::uintptr_t))
            throw MetadataReadError(Private::IteratorReadUnexpectedEnd);

        std::uintptr_t value(0);
        Detail::RangeCheckedCopy(it, it + sizeof(std::uintptr_t), Detail::BeginBytes(value), Detail::EndBytes(value));
        it += sizeof(std::uintptr_t);
        return value;
    }

    std::uintptr_t PeekPointer(ConstByteIterator it, ConstByteIterator const last)
    {
        return ReadPointer(it, last);
    }





    SignatureComparer::SignatureComparer(ITypeResolver const* loader,
                                         Database      const* lhsDatabase,
                                         Database      const* rhsDatabase)
        : _loader(loader), _lhsDatabase(lhsDatabase), _rhsDatabase(rhsDatabase)
    {
        Detail::AssertNotNull(loader);
        Detail::AssertNotNull(lhsDatabase);
        Detail::AssertNotNull(rhsDatabase);
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

        // There is no need to check the parameter count explicitly; RangeCheckedEqual will do that.
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

    bool SignatureComparer::operator()(RowReference const& lhs, RowReference const& rhs) const
    {
        FullReference const lhsFullReference(_lhsDatabase.Get(), lhs);
        FullReference const rhsFullReference(_rhsDatabase.Get(), rhs);

        // TODO Do we need to handle generic type argument instantiation here?
        FullReference const lhsResolved(_loader.Get()->ResolveType(lhsFullReference));
        FullReference const rhsResolved(_loader.Get()->ResolveType(rhsFullReference));

        // If the types are from different tables, they cannot be equal:
        if (lhsResolved.AsRowReference().GetTable() != rhsResolved.AsRowReference().GetTable())
            return false;

        // If we have a pair of TypeDefs, they are only equal if they refer to the same type in the
        // same database; in no other case can they be equal:
        if (lhsResolved.AsRowReference().GetTable() == TableId::TypeDef)
        {
            if (lhsResolved.GetDatabase() == rhsResolved.GetDatabase() &&
                lhsResolved.AsRowReference() == rhsResolved.AsRowReference())
                return true;

            return false;
        }

        // Otherwise, we have a pair of TypeSpec tokens and we have to compare them recursively:
        Database const& lhsDatabase(lhsResolved.GetDatabase());
        Database const& rhsDatabase(rhsResolved.GetDatabase());

        SizeType const lhsRowIndex(lhsResolved.AsRowReference().GetIndex());
        SizeType const rhsRowIndex(rhsResolved.AsRowReference().GetIndex());

        TypeSpecRow const lhsTypeSpec(lhsDatabase.GetRow<TableId::TypeSpec>(lhsRowIndex));
        TypeSpecRow const rhsTypeSpec(rhsDatabase.GetRow<TableId::TypeSpec>(rhsRowIndex));

        BlobReference const lhsSignature(lhsTypeSpec.GetSignature());
        BlobReference const rhsSignature(rhsTypeSpec.GetSignature());

        // Note that we use a new signature comparer because the LHS and RHS signatures may have come
        // from new and/or different databases.
        if (SignatureComparer(_loader.Get(), &lhsDatabase, &rhsDatabase)(
                TypeSignature(lhsSignature.Begin(), lhsSignature.End()),
                TypeSignature(rhsSignature.Begin(), rhsSignature.End())))
            return true;

        return false;
    }





    ClassVariableSignatureInstantiator::ClassVariableSignatureInstantiator()
    {
    }

    ClassVariableSignatureInstantiator::ClassVariableSignatureInstantiator(ClassVariableSignatureInstantiator&& other)
        : _scope             (std::move(other._scope             )),
          _arguments         (std::move(other._arguments         )),
          _argumentSignatures(std::move(other._argumentSignatures)),
          _buffer            (std::move(other._buffer            ))
    {
    }

    ClassVariableSignatureInstantiator& 
    ClassVariableSignatureInstantiator::operator=(ClassVariableSignatureInstantiator&& other)
    {
        _scope              = std::move(other._scope             );
        _arguments          = std::move(other._arguments         );
        _argumentSignatures = std::move(other._argumentSignatures);
        _buffer             = std::move(other._buffer            );
        return *this;
    }
    
    template <typename TSignature>
    TSignature ClassVariableSignatureInstantiator::Instantiate(TSignature const& signature) const
    {
        _buffer.clear();
        InstantiateInto(_buffer, signature);
        return TSignature(&*_buffer.begin(), &*_buffer.begin() + _buffer.size());
    }

    template ArrayShape        ClassVariableSignatureInstantiator::Instantiate<ArrayShape>       (ArrayShape        const&) const;
    template FieldSignature    ClassVariableSignatureInstantiator::Instantiate<FieldSignature>   (FieldSignature    const&) const;
    template MethodSignature   ClassVariableSignatureInstantiator::Instantiate<MethodSignature>  (MethodSignature   const&) const;
    template PropertySignature ClassVariableSignatureInstantiator::Instantiate<PropertySignature>(PropertySignature const&) const;
    template TypeSignature     ClassVariableSignatureInstantiator::Instantiate<TypeSignature>    (TypeSignature     const&) const;

    template <typename TSignature>
    bool ClassVariableSignatureInstantiator::RequiresInstantiation(TSignature const& signature)
    {
        // TODO Does this need to handle scope-conversion?
        return RequiresInstantiationInternal(signature);
    }

    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<ArrayShape>       (ArrayShape        const&);
    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<FieldSignature>   (FieldSignature    const&);
    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<MethodSignature>  (MethodSignature   const&);
    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<PropertySignature>(PropertySignature const&);
    template bool ClassVariableSignatureInstantiator::RequiresInstantiation<TypeSignature>    (TypeSignature     const&);

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
        {
            CopyBytesInto(buffer, s, Part::Begin, Part::End);
            break;
        }
        case TypeSignature::Kind::ClassType:
        {
            switch (s.GetElementType())
            {
            case ElementType::Class:
            case ElementType::ValueType:
                buffer.push_back(static_cast<Byte>(ElementType::CrossModuleTypeReference));
                CopyBytesInto(buffer, s, Part::ClassTypeReference, Part::End);
                std::copy(Detail::BeginBytes(_scope.Get()),
                          Detail::EndBytes(_scope.Get()),
                          std::back_inserter(buffer));
                break;

            case ElementType::CrossModuleTypeReference:
                CopyBytesInto(buffer, s, Part::Begin, Part::End);
                break;

            default:
                throw LogicError(L"Invalid Type Kind");
            }
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
                // TODO THIS IS PROBABLY INVALID METADATA IF WE HAVE A BAD ARG!
                if (variableNumber >= _arguments.size())
                    CopyBytesInto(buffer, s, Part::Begin, Part::End);
                else
                    CopyBytesInto(buffer, _arguments[variableNumber], Part::Begin, Part::End);
                    
                // TODO This is incorrect for cross-module instantiations.  We can have arguments
                // with a different resolution scope than the signature being instantiated. :'(
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

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(ArrayShape const&)
    {
        return false;
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(FieldSignature const& s)
    {
        return RequiresInstantiationInternal(s.GetTypeSignature());
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(MethodSignature const& s)
    {
        if (RequiresInstantiationInternal(s.GetReturnType()))
            return true;

        if (AnyRequiresInstantiationInternal(s.BeginParameters(), s.EndParameters()))
            return true;

        if (AnyRequiresInstantiationInternal(s.BeginVarargParameters(), s.EndVarargParameters()))
            return true;

        return false;
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(PropertySignature const& s)
    {
        if (RequiresInstantiationInternal(s.GetTypeSignature()))
            return true;

        if (AnyRequiresInstantiationInternal(s.BeginParameters(), s.EndParameters()))
            return true;

        return false;
    }

    bool ClassVariableSignatureInstantiator::RequiresInstantiationInternal(TypeSignature const& s)
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
    bool ClassVariableSignatureInstantiator::AnyRequiresInstantiationInternal(TForIt const first, TForIt const last)
    {
        return std::any_of(first, last, [&](decltype(*first) const& s)
        {
            return RequiresInstantiationInternal(s);
        });
    }





    BaseSignature::BaseSignature()
    {
    }

    BaseSignature::BaseSignature(ConstByteIterator const first, ConstByteIterator const last)
        : _first(first), _last(last)
    {
        Detail::AssertNotNull(first);
        Detail::AssertNotNull(last);
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

    ConstByteIterator BaseSignature::BeginBytes() const
    {
        AssertInitialized();
        return _first.Get();
    }

    ConstByteIterator BaseSignature::EndBytes() const
    {
        AssertInitialized();
        return _last.Get();
    }

    bool BaseSignature::IsInitialized() const
    {
        return _first.Get() != nullptr && _last.Get() != nullptr;
    }

    void BaseSignature::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }





    ArrayShape::ArrayShape()
    {
    }

    ArrayShape::ArrayShape(ConstByteIterator const first, ConstByteIterator const last)
        : BaseSignature(first, last)
    {
    }

    SizeType ArrayShape::GetRank() const
    {
        AssertInitialized();

        return PeekCompressedUInt32(SeekTo(Part::Rank), EndBytes());
    }

    SizeType ArrayShape::GetSizesCount() const
    {
        AssertInitialized();

        return PeekCompressedUInt32(SeekTo(Part::NumSizes), EndBytes());
    }

    ArrayShape::SizeIterator ArrayShape::BeginSizes() const
    {
        AssertInitialized();

        return SizeIterator(SeekTo(Part::FirstSize), EndBytes(), 0, GetSizesCount());
    }

    ArrayShape::SizeIterator ArrayShape::EndSizes() const
    {
        AssertInitialized();

        SizeType const sizesCount(GetSizesCount());
        return SizeIterator(nullptr, nullptr, sizesCount, sizesCount);
    }

    SizeType ArrayShape::GetLowBoundsCount() const
    {
        AssertInitialized();

        return PeekCompressedUInt32(SeekTo(Part::NumLowBounds), EndBytes());
    }

    ArrayShape::LowBoundIterator ArrayShape::BeginLowBounds() const
    {
        AssertInitialized();

        return LowBoundIterator(SeekTo(Part::FirstLowBound), EndBytes(), 0, GetLowBoundsCount());
    }

    ArrayShape::LowBoundIterator ArrayShape::EndLowBounds() const
    {
        AssertInitialized();

        SizeType const lowBoundsCount(GetLowBoundsCount());
        return LowBoundIterator(nullptr, nullptr, lowBoundsCount, lowBoundsCount);
    }

    SizeType ArrayShape::ComputeSize() const
    {
        AssertInitialized();

        return static_cast<SizeType>(SeekTo(Part::End) - BeginBytes());
    }

    ConstByteIterator ArrayShape::SeekTo(Part const part) const
    {
        AssertInitialized();

        ConstByteIterator current(BeginBytes());

        if (part > Part::Rank)
        {
            ReadCompressedUInt32(current, EndBytes());
        }

        std::uint32_t numSizes(0);
        if (part > Part::NumSizes)
        {
            numSizes = ReadCompressedUInt32(current, EndBytes());
        }

        if (part > Part::FirstSize)
        {
            for (unsigned i(0); i < numSizes; ++i)
                ReadCompressedUInt32(current, EndBytes());
        }

        std::uint32_t numLowBounds(0);
        if (part > Part::NumLowBounds)
        {
            numLowBounds = ReadCompressedUInt32(current, EndBytes());
        }

        if (part > Part::FirstLowBound)
        {
            for (unsigned i(0); i < numSizes; ++i)
                ReadCompressedUInt32(current, EndBytes());
        }

        Detail::Assert([&]{ return part <= Part::End; }, L"Invalid signature part requested");

        return current;
    }

    SizeType ArrayShape::ReadSize(ConstByteIterator& current, ConstByteIterator const last)
    {
        return ReadCompressedUInt32(current, last);
    }

    SizeType ArrayShape::ReadLowBound(ConstByteIterator& current, ConstByteIterator const last)
    {
        return ReadCompressedUInt32(current, last);
    }





    CustomModifier::CustomModifier()
    {
    }

    CustomModifier::CustomModifier(ConstByteIterator const first, ConstByteIterator const last)
        : BaseSignature(first, last)
    {
        Detail::Assert([&]{ return IsOptional() || IsRequired(); });
    }

    bool CustomModifier::IsOptional() const
    {
        AssertInitialized();

        return PeekByte(SeekTo(Part::ReqOptFlag), EndBytes()) == ElementType::CustomModifierOptional;
    }

    bool CustomModifier::IsRequired() const
    {
        AssertInitialized();

        return PeekByte(SeekTo(Part::ReqOptFlag), EndBytes()) == ElementType::CustomModifierRequired;
    }

    RowReference CustomModifier::GetTypeReference() const
    {
        AssertInitialized();

        return RowReference::FromToken(
            PeekTypeDefOrRefOrSpec(SeekTo(Part::Type), EndBytes()));
    }

    SizeType CustomModifier::ComputeSize() const
    {
        AssertInitialized();

        return static_cast<SizeType>(SeekTo(Part::End) - BeginBytes());
    }

    ConstByteIterator CustomModifier::SeekTo(Part const part) const
    {
        AssertInitialized();

        ConstByteIterator current(BeginBytes());

        if (part > Part::ReqOptFlag)
        {
            ReadByte(current, EndBytes());
        }

        if (part > Part::Type)
        {
            ReadTypeDefOrRefOrSpec(current, EndBytes());
        }

        if (part > Part::End)
        {
            Detail::AssertFail(L"Invalid signature part requested");
        }

        return current;
    }





    FieldSignature::FieldSignature()
    {
    }

    FieldSignature::FieldSignature(ConstByteIterator const first, ConstByteIterator const last)
        : BaseSignature(first, last)
    {
        Detail::Assert([&]
        {
            return PeekByte(SeekTo(Part::FieldTag), EndBytes()) == SignatureAttribute::Field; }
        );
    }

    TypeSignature FieldSignature::GetTypeSignature() const
    {
        AssertInitialized();

        return TypeSignature(SeekTo(Part::Type), EndBytes());
    }

    SizeType FieldSignature::ComputeSize() const
    {
        AssertInitialized();

        return static_cast<SizeType>(SeekTo(Part::End) - BeginBytes());
    }

    ConstByteIterator FieldSignature::SeekTo(Part const part) const
    {
        AssertInitialized();

        ConstByteIterator current(BeginBytes());

        if (part > Part::FieldTag)
        {
            ReadByte(current, EndBytes());
        }

        if (part > Part::Type)
        {
            current += TypeSignature(current, EndBytes()).ComputeSize();
        }

        if (part > Part::End)
        {
            Detail::AssertFail(L"Invalid signature part requested");
        }

        return current;
    }





    PropertySignature::PropertySignature()
    {
    }

    PropertySignature::PropertySignature(ConstByteIterator const first, ConstByteIterator const last)
        : BaseSignature(first, last)
    {
        Detail::Assert([&]() -> bool
        {
            Byte const initialByte(PeekByte(SeekTo(Part::PropertyTag), EndBytes()));
            return initialByte == SignatureAttribute::Property
                || initialByte == (SignatureAttribute::Property | SignatureAttribute::HasThis);
        });
    }

    bool PropertySignature::HasThis() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::PropertyTag), EndBytes()))
            .IsSet(SignatureAttribute::HasThis);
    }

    SizeType PropertySignature::GetParameterCount() const
    {
        AssertInitialized();

        return PeekCompressedUInt32(SeekTo(Part::ParameterCount), EndBytes());
    }

    PropertySignature::ParameterIterator PropertySignature::BeginParameters() const
    {
        AssertInitialized();

        return ParameterIterator(BeginBytes(), EndBytes(), 0, GetParameterCount());
    }

    PropertySignature::ParameterIterator PropertySignature::EndParameters() const
    {
        AssertInitialized();

        SizeType const parameterCount(GetParameterCount());
        return ParameterIterator(nullptr, nullptr, parameterCount, parameterCount);
    }

    TypeSignature PropertySignature::GetTypeSignature() const
    {
        AssertInitialized();

        return TypeSignature(SeekTo(Part::Type), EndBytes());
    }

    SizeType PropertySignature::ComputeSize() const
    {
        AssertInitialized();

        return static_cast<SizeType>(SeekTo(Part::End) - BeginBytes());
    }

    ConstByteIterator PropertySignature::SeekTo(Part const part) const
    {
        AssertInitialized();

        ConstByteIterator current(BeginBytes());

        if (part > Part::PropertyTag)
        {
            ReadByte(current, EndBytes());
        }

        SizeType parameterCount(0);
        if (part > Part::ParameterCount)
        {
            parameterCount = ReadCompressedUInt32(current, EndBytes());
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
            Detail::AssertFail(L"Invalid signature part requested");
        }

        return current;
    }

    TypeSignature PropertySignature::ReadParameter(ConstByteIterator& current, ConstByteIterator const last)
    {
        TypeSignature const type(current, last);
        current += type.ComputeSize();
        return type;
    }





    MethodSignature::MethodSignature()
    {
    }

    MethodSignature::MethodSignature(ConstByteIterator const first, ConstByteIterator const last)
        : BaseSignature(first, last)
    {
    }

    bool MethodSignature::ParameterEndCheck(ConstByteIterator current, ConstByteIterator last)
    {
        return PeekByte(current, last) == ElementType::Sentinel;
    }

    TypeSignature MethodSignature::ReadParameter(ConstByteIterator& current, ConstByteIterator last)
    {
        TypeSignature const typeSignature(current, last);
        current += typeSignature.ComputeSize();
        return typeSignature;
    }

    bool MethodSignature::HasThis() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .IsSet(SignatureAttribute::HasThis);
    }

    bool MethodSignature::HasExplicitThis() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .IsSet(SignatureAttribute::ExplicitThis);
    }

    SignatureAttribute MethodSignature::GetCallingConvention() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask)
            .GetEnum();
    }

    bool MethodSignature::HasDefaultConvention() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::Default;
    }

    bool MethodSignature::HasVarArgConvention() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::VarArg;
    }

    bool MethodSignature::HasCConvention() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::C;
    }

    bool MethodSignature::HasStdCallConvention() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::StdCall;
    }

    bool MethodSignature::HasThisCallConvention() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::ThisCall;
    }


    bool MethodSignature::HasFastCallConvention() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .WithMask(SignatureAttribute::CallingConventionMask) == SignatureAttribute::FastCall;
    }

    bool MethodSignature::IsGeneric() const
    {
        AssertInitialized();

        return SignatureFlags(PeekByte(SeekTo(Part::TypeTag), EndBytes()))
            .IsSet(SignatureAttribute::Generic);
    }

    SizeType MethodSignature::GetGenericParameterCount() const
    {
        AssertInitialized();

        if (!IsGeneric())
            return 0;

        return PeekCompressedUInt32(SeekTo(Part::GenParamCount), EndBytes());
    }

    TypeSignature MethodSignature::GetReturnType() const
    {
        AssertInitialized();

        return TypeSignature(SeekTo(Part::RetType), EndBytes());
    }

    SizeType MethodSignature::GetParameterCount() const
    {
        AssertInitialized();

        return PeekCompressedUInt32(SeekTo(Part::ParamCount), EndBytes());
    }

    MethodSignature::ParameterIterator MethodSignature::BeginParameters() const
    {
        AssertInitialized();

        return ParameterIterator(SeekTo(Part::FirstParam), EndBytes(), 0, GetParameterCount());
    }

    MethodSignature::ParameterIterator MethodSignature::EndParameters() const
    {
        AssertInitialized();

        SizeType const parameterCount(GetParameterCount());
        return ParameterIterator(nullptr, nullptr, parameterCount, parameterCount);
    }

    MethodSignature::ParameterIterator MethodSignature::BeginVarargParameters() const
    {
        AssertInitialized();

        SizeType const parameterCount(GetParameterCount());
        SizeType const actualParameters(static_cast<SizeType>(std::distance(BeginParameters(), EndParameters())));
        SizeType const varArgParameters(parameterCount - actualParameters);

        return ParameterIterator(SeekTo(Part::FirstVarargParam), EndBytes(), 0, varArgParameters);
    }

    MethodSignature::ParameterIterator MethodSignature::EndVarargParameters() const
    {
        AssertInitialized();

        SizeType const parameterCount(GetParameterCount());
        SizeType const actualParameters(static_cast<SizeType>(std::distance(BeginParameters(), EndParameters())));
        SizeType const varArgParameters(parameterCount - actualParameters);

        return ParameterIterator(nullptr, nullptr, varArgParameters, varArgParameters);
    }

    SizeType MethodSignature::ComputeSize() const
    {
        AssertInitialized();

        return static_cast<SizeType>(SeekTo(Part::End) - BeginBytes());
    }

    ConstByteIterator MethodSignature::SeekTo(Part const part) const
    {
        AssertInitialized();

        ConstByteIterator current(BeginBytes());

        SignatureFlags typeFlags;
        if (part > Part::TypeTag)
        {
            typeFlags = ReadByte(current, EndBytes());
        }

        if (part == Part::GenParamCount && !typeFlags.IsSet(SignatureAttribute::Generic))
        {
            return nullptr;
        }

        if (part > Part::GenParamCount && typeFlags.IsSet(SignatureAttribute::Generic))
        {
            ReadCompressedUInt32(current, EndBytes());
        }

        SizeType parameterCount(0);
        if (part > Part::ParamCount)
        {
            parameterCount = ReadCompressedUInt32(current, EndBytes());
        }

        if (part > Part::RetType)
        {
            current += TypeSignature(current, EndBytes()).ComputeSize();
        }

        unsigned parametersRead(0);
        if (part > Part::FirstParam)
        {
            while (parametersRead < parameterCount &&
                   PeekByte(current, EndBytes()) != ElementType::Sentinel)
            {
                ++parametersRead;
                current += TypeSignature(current, EndBytes()).ComputeSize();
            }

            if (current != EndBytes() &&
                PeekByte(current, EndBytes()) == ElementType::Sentinel)
            {
                current += ReadByte(current, EndBytes());
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
            Detail::AssertFail(L"Invalid signature part requested");
        }

        return current;
    }





    TypeSignature::TypeSignature()
    {
    }

    TypeSignature::TypeSignature(ConstByteIterator const first, ConstByteIterator const last)
        : BaseSignature(first, last)
    {
    }

    SizeType TypeSignature::ComputeSize() const
    {
        AssertInitialized();

        return static_cast<SizeType>(SeekTo(Part::End) - BeginBytes());
    }

    TypeSignature::Kind TypeSignature::GetKind() const
    {
        AssertInitialized();

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
        case ElementType::CrossModuleTypeReference:
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

    void TypeSignature::AssertKind(Kind const kind) const
    {
        AssertInitialized();
        Detail::Assert([&]{ return IsKind(kind); });
    }

    TypeSignature TypeSignature::ReadType(ConstByteIterator& current, ConstByteIterator last)
    {
        TypeSignature const typeSignature(current, last);
        current += typeSignature.ComputeSize();
        return typeSignature;
    }

    bool TypeSignature::CustomModifierEndCheck(ConstByteIterator current, ConstByteIterator last)
    {
        return current == last || !Private::IsCustomModifierElementType(PeekByte(current, last));
    }

    CustomModifier TypeSignature::ReadCustomModifier(ConstByteIterator& current, ConstByteIterator last)
    {
        CustomModifier const customModifier(current, last);
        current += customModifier.ComputeSize();
        return customModifier;
    }

    TypeSignature::CustomModifierIterator TypeSignature::BeginCustomModifiers() const
    {
        AssertInitialized();

        ConstByteIterator const firstCustomModifier(SeekTo(Part::FirstCustomMod));
        return CustomModifierIterator(firstCustomModifier, firstCustomModifier == nullptr ? nullptr : EndBytes());
    }

    TypeSignature::CustomModifierIterator TypeSignature::EndCustomModifiers() const
    {
        AssertInitialized();

        return CustomModifierIterator();
    }

    ElementType TypeSignature::GetElementType() const
    {
        AssertInitialized();

        Byte const typeTag(PeekByte(SeekTo(Part::TypeCode), EndBytes()));
        return IsValidElementType(typeTag) ? static_cast<ElementType>(typeTag) : ElementType::End;
    }

    bool TypeSignature::IsByRef() const
    {
        AssertInitialized();

        ConstByteIterator const byRefTag(SeekTo(Part::ByRefTag));
        return byRefTag != nullptr && PeekByte(byRefTag, EndBytes()) == ElementType::ByRef;
    }

    bool TypeSignature::IsPrimitive() const
    {
        AssertInitialized();

        return GetPrimitiveElementType() != ElementType::End;
    }

    ElementType TypeSignature::GetPrimitiveElementType() const
    {
        AssertInitialized();

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
        AssertInitialized();

        return GetElementType() == ElementType::Array;
    }

    bool TypeSignature::IsSimpleArray() const
    {
        AssertInitialized();

        return GetElementType() == ElementType::SzArray;
    }

    TypeSignature TypeSignature::GetArrayType() const
    {
        AssertInitialized();

        return TypeSignature(
            IsKind(Kind::Array) ? SeekTo(Part::ArrayType) : SeekTo(Part::SzArrayType),
            EndBytes());
    }

    ArrayShape TypeSignature::GetArrayShape() const
    {
        AssertInitialized();

        return ArrayShape(SeekTo(Part::ArrayShape), EndBytes());
    }

    bool TypeSignature::IsClassType() const
    {
        AssertInitialized();

        return GetElementType() == ElementType::Class;
    }

    bool TypeSignature::IsValueType() const
    {
        AssertInitialized();

        return GetElementType() == ElementType::ValueType;
    }

    RowReference TypeSignature::GetTypeReference() const
    {
        AssertInitialized();

        return RowReference::FromToken(PeekTypeDefOrRefOrSpec(SeekTo(Part::ClassTypeReference), EndBytes()));
    }

    Database const* TypeSignature::GetTypeReferenceScope() const
    {
        AssertInitialized();

        if (GetElementType() != ElementType::CrossModuleTypeReference)
            return nullptr;

        return reinterpret_cast<Database const*>(PeekPointer(SeekTo(Part::ClassTypeScope), EndBytes()));
    }

    bool TypeSignature::IsFunctionPointer() const
    {
        AssertInitialized();
        
        return GetElementType() == ElementType::FnPtr;
    }

    MethodSignature TypeSignature::GetMethodSignature() const
    {
        AssertInitialized();

        return MethodSignature(SeekTo(Part::MethodSignature), EndBytes());
    }

    bool TypeSignature::IsGenericInstance() const
    {
        AssertInitialized();

        return GetElementType() == ElementType::GenericInst;
    }

    bool TypeSignature::IsGenericClassTypeInstance() const
    {
        AssertInitialized();

        return PeekByte(SeekTo(Part::GenericInstTypeCode), EndBytes()) == ElementType::Class;
    }

    bool TypeSignature::IsGenericValueTypeInstance() const
    {
        AssertInitialized();

        return PeekByte(SeekTo(Part::GenericInstTypeCode), EndBytes()) == ElementType::ValueType;
    }

    RowReference TypeSignature::GetGenericTypeReference() const
    {
        AssertInitialized();

        return RowReference::FromToken(PeekTypeDefOrRefOrSpec(SeekTo(Part::GenericInstTypeReference), EndBytes()));
    }

    SizeType TypeSignature::GetGenericArgumentCount() const
    {
        AssertInitialized();

        return PeekCompressedUInt32(SeekTo(Part::GenericInstArgumentCount), EndBytes());
    }

    TypeSignature::GenericArgumentIterator TypeSignature::BeginGenericArguments() const
    {
        AssertInitialized();

        return GenericArgumentIterator(
            SeekTo(Part::FirstGenericInstArgument),
            EndBytes(),
            0,
            GetGenericArgumentCount());
    }

    TypeSignature::GenericArgumentIterator TypeSignature::EndGenericArguments() const
    {
        AssertInitialized();

        SizeType const count(PeekCompressedUInt32(SeekTo(Part::GenericInstArgumentCount), EndBytes()));
        return GenericArgumentIterator(nullptr, nullptr, count, count);
    }

    bool TypeSignature::IsPointer() const
    {
        AssertInitialized();

        return GetElementType() == ElementType::Ptr;
    }

    TypeSignature TypeSignature::GetPointerTypeSignature() const
    {
        AssertInitialized();

        return TypeSignature(SeekTo(Part::PointerTypeSignature), EndBytes());
    }

    bool TypeSignature::IsClassVariableType() const
    {
        AssertInitialized();

        return GetElementType() == ElementType::Var;
    }

    bool TypeSignature::IsMethodVariableType() const
    {
        AssertInitialized();

        return GetElementType() == ElementType::MVar;
    }

    SizeType TypeSignature::GetVariableNumber() const
    {
        AssertInitialized();

        return PeekCompressedUInt32(SeekTo(Part::VariableNumber), EndBytes());
    }

    ConstByteIterator TypeSignature::SeekTo(Part const part) const
    {
        AssertInitialized();

        Kind const partKind(static_cast<Kind>(static_cast<SizeType>(part)) & Kind::Mask);
        Part const partCode(part & static_cast<Part>(~static_cast<SizeType>(Kind::Mask)));

        ConstByteIterator current(BeginBytes());

        if (partCode > Part::FirstCustomMod)
        {
            while (Private::IsCustomModifierElementType(PeekByte(current, EndBytes())))
                current += CustomModifier(current, EndBytes()).ComputeSize();
        }

        if (partCode > Part::ByRefTag && PeekByte(current, EndBytes()) == ElementType::ByRef)
        {
            ReadByte(current, EndBytes());
        }

        if (partCode > Part::TypeCode)
        {
            Byte const typeCode(ReadByte(current, EndBytes()));
            if (partKind != Kind::Unknown && !IsKind(partKind))
            {
                Detail::AssertFail(L"Invalid signature part requested");
            }

            auto const ExtractPart([](Part const p)
            {
                return static_cast<Part>(static_cast<SizeType>(p) & ~static_cast<SizeType>(Kind::Mask));
            });

            Kind const kind(GetKind());
            switch (kind)
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
                    ReadTypeDefOrRefOrSpec(current, EndBytes());
                }

                if (partCode > ExtractPart(Part::ClassTypeScope) && typeCode == ElementType::CrossModuleTypeReference)
                {
                    ReadPointer(current, EndBytes());
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
                    ReadByte(current, EndBytes());
                }

                if (partCode > ExtractPart(Part::GenericInstTypeReference))
                {
                    ReadTypeDefOrRefOrSpec(current, EndBytes());
                }

                SizeType argumentCount(0);
                if (partCode > ExtractPart(Part::GenericInstArgumentCount))
                {
                    argumentCount = ReadCompressedUInt32(current, EndBytes());
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
                    ReadCompressedUInt32(current, EndBytes());
                }

                break;
            }
            default:
            {
                Detail::AssertFail(L"It is impossible to get here");
            }
            }
        }

        if (partCode > Part::End)
        {
            Detail::AssertFail(L"Invalid signature part requested");
        }

        return current;
    }

} }
