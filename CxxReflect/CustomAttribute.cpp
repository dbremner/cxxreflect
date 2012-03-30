//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace { namespace Private {

    typedef std::pair<Metadata::RowReference, Metadata::RowReference> RowReferencePair;

    SizeType TableIdToCustomAttributeFlag(Metadata::TableId tableId)
    {
        switch (tableId)
        {
        case Metadata::TableId::MethodDef:                return  0;
        case Metadata::TableId::Field:                    return  1;
        case Metadata::TableId::TypeRef:                  return  2;
        case Metadata::TableId::TypeDef:                  return  3;
        case Metadata::TableId::Param:                    return  4;
        case Metadata::TableId::InterfaceImpl:            return  5;
        case Metadata::TableId::MemberRef:                return  6;
        case Metadata::TableId::Module:                   return  7;
        case Metadata::TableId::DeclSecurity:             return  8;
        case Metadata::TableId::Property:                 return  9;
        case Metadata::TableId::Event:                    return 10;
        case Metadata::TableId::StandaloneSig:            return 11;
        case Metadata::TableId::ModuleRef:                return 12;
        case Metadata::TableId::TypeSpec:                 return 13;
        case Metadata::TableId::Assembly:                 return 14;
        case Metadata::TableId::AssemblyRef:              return 15;
        case Metadata::TableId::File:                     return 16;
        case Metadata::TableId::ExportedType:             return 17;
        case Metadata::TableId::ManifestResource:         return 18;
        case Metadata::TableId::GenericParam:             return 19;
        case Metadata::TableId::GenericParamConstraint:   return 20;
        case Metadata::TableId::MethodSpec:               return 21;
        default: Detail::AssertFail(L"Invalid table id"); return  0;
        }
    }

    struct CustomAttributeStrictWeakOrdering
    {
        typedef Metadata::CustomAttributeRow CustomAttributeRow;
        typedef Metadata::RowReference       RowReference;

        bool operator()(CustomAttributeRow const& lhs, CustomAttributeRow const& rhs) const volatile
        {
            return (*this)(lhs.GetParent(), rhs.GetParent());
        }

        bool operator()(CustomAttributeRow const& lhs, RowReference const& rhs) const volatile
        {
            return (*this)(lhs.GetParent(), rhs);
        }

        bool operator()(RowReference const& lhs, CustomAttributeRow const& rhs) const volatile
        {
            return (*this)(lhs, rhs.GetParent());
        }

        bool operator()(RowReference const& lhs, RowReference const& rhs) const volatile
        {
            if (lhs.GetIndex() < rhs.GetIndex())
                return true;

            if (lhs.GetIndex() > rhs.GetIndex())
                return false;

            if (TableIdToCustomAttributeFlag(lhs.GetTable()) < TableIdToCustomAttributeFlag(rhs.GetTable()))
                return true;

            return false;
        }
    };

    RowReferencePair GetCustomAttributesRange(Metadata::Database const& database, Metadata::RowReference const& parent)
    {
        auto const range(Detail::EqualRange(database.Begin<Metadata::TableId::CustomAttribute>(),
                                            database.End<Metadata::TableId::CustomAttribute>(),
                                            parent,
                                            CustomAttributeStrictWeakOrdering()));;

        return std::make_pair(range.first.GetReference(), range.second.GetReference());
    }

} } }

namespace CxxReflect {

    CustomAttribute::CustomAttribute()
    {
    }

    CustomAttribute::CustomAttribute(Assembly               const& assembly,
                                     Metadata::RowReference const& customAttribute,
                                     InternalKey)
    {
        Detail::Assert([&]{ return assembly.IsInitialized();        });
        Detail::Assert([&]{ return customAttribute.IsInitialized(); });

        Metadata::Database const& parentDatabase(assembly.GetContext(InternalKey()).GetDatabase());
        Metadata::CustomAttributeRow const customAttributeRow(parentDatabase
            .GetRow<Metadata::TableId::CustomAttribute>(customAttribute));

        _parent = Metadata::FullReference(&parentDatabase, customAttributeRow.GetParent());
        _attribute = customAttribute;

        if (customAttributeRow.GetType().GetTable() == Metadata::TableId::MethodDef)
        {
            Metadata::RowReference const methodDefReference(customAttributeRow.GetType());
            Metadata::MethodDefRow const methodDefRow(
                parentDatabase.GetRow<Metadata::TableId::MethodDef>(methodDefReference));

            Metadata::TypeDefRow const& typeDefRow(Metadata::GetOwnerOfMethodDef(parentDatabase, methodDefRow));

            Type const type(assembly, typeDefRow.GetSelfReference(), InternalKey());

            BindingFlags const flags(
                BindingAttribute::Public    |
                BindingAttribute::NonPublic |
                BindingAttribute::Instance);

            auto const constructorIt(std::find_if(type.BeginConstructors(flags),
                                                  type.EndConstructors(),
                                                  [&](Method const& constructor) -> bool
            {
                return constructor.GetMetadataToken() == methodDefReference.GetToken();
            }));

            Detail::Assert([&]{ return constructorIt != type.EndConstructors(); });

            _constructor = Detail::MethodHandle(*constructorIt);
        }
        else if (customAttributeRow.GetType().GetTable() == Metadata::TableId::MemberRef)
        {
            Metadata::RowReference const memberRefReference(customAttributeRow.GetType());
            Metadata::MemberRefRow const memberRefRow(
                parentDatabase.GetRow<Metadata::TableId::MemberRef>(memberRefReference));

            Metadata::RowReference const memberRefClassRef(memberRefRow.GetClass());
            if (memberRefClassRef.GetTable() == Metadata::TableId::TypeRef)
            {
                Type const type(assembly, memberRefClassRef, InternalKey());
                if (memberRefRow.GetName() == L".ctor")
                {
                    BindingFlags const flags(
                        BindingAttribute::Public    |
                        BindingAttribute::NonPublic |
                        BindingAttribute::Instance);

                    if (type.BeginConstructors(flags) == type.EndConstructors())
                        Detail::AssertFail(L"NYI");

                    _constructor = Detail::MethodHandle(*type.BeginConstructors(flags));
                }
            }
            else
            {
                Detail::AssertFail(L"NYI");
            }
        }
        else
        {
            // We should never get here; the Database layer should throw if the type is invalid.
            Detail::AssertFail(L"Invalid custom attribute type");
        }
    }

    SizeType CustomAttribute::GetMetadataToken() const
    {
        return _attribute.GetToken();
    }

    CustomAttributeIterator CustomAttribute::BeginFor(Assembly               const& assembly,
                                                      Metadata::RowReference const& parent,
                                                      InternalKey)
    {
        return CustomAttributeIterator(
            assembly,
            Private::GetCustomAttributesRange(assembly.GetContext(InternalKey()).GetDatabase(), parent).first);
    }

    CustomAttributeIterator CustomAttribute::EndFor(Assembly               const& assembly,
                                                    Metadata::RowReference const& parent,
                                                    InternalKey)
    {
        return CustomAttributeIterator(
            assembly,
            Private::GetCustomAttributesRange(assembly.GetContext(InternalKey()).GetDatabase(), parent).second);
    }

    bool CustomAttribute::IsInitialized() const
    {
        return _parent.IsInitialized() && _constructor.IsInitialized();
    }

    void CustomAttribute::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    Method CustomAttribute::GetConstructor() const
    {
        AssertInitialized();

        return _constructor.Realize();
    }

    // TODO REMOVE ALL OF THIS; It's copypasta'ed from MetadataSignature.cpp
    namespace { namespace Private {

        CharacterIterator const IteratorReadUnexpectedEnd(L"Unexpectedly reached end of range");

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

        std::uint32_t ReadCompressedUInt32(ConstByteIterator& it, ConstByteIterator const last)
        {
            CompressedIntBytes const bytes(ReadCompressedIntBytes(it, last));

            switch (bytes.Count)
            {
            case 1:  return *reinterpret_cast<std::uint8_t  const*>(&bytes.Bytes[0]);
            case 2:  return *reinterpret_cast<std::uint16_t const*>(&bytes.Bytes[0]);
            case 4:  return *reinterpret_cast<std::uint32_t const*>(&bytes.Bytes[0]);
            default: Detail::AssertFail(L"It is impossible to get here"); return 0;
            }
        }

        template <typename T>
        T Read(ConstByteIterator& it, ConstByteIterator const last)
        {
            if (std::distance(it, last) < sizeof(T))
                throw std::logic_error("WTF");

            T const value(*reinterpret_cast<T const*>(it));
            it += sizeof(T);
            return value;
        }

    } }

    String CustomAttribute::GetSingleStringArgument() const
    {
        Metadata::Database const& database(_constructor
            .Realize()
            .GetDeclaringType()
            .GetAssembly()
            .GetContext(InternalKey())
            .GetDatabase());

        Metadata::CustomAttributeRow const& customAttribute(database
            .GetRow<Metadata::TableId::CustomAttribute>(_attribute));

        Metadata::Blob const& valueBlob(database.GetBlob(customAttribute.GetValue()));
        ConstByteIterator it(valueBlob.Begin());

        it += 2;
        std::uint32_t length(Private::ReadCompressedUInt32(it, valueBlob.End()));

        SizeType realLength(Externals::ComputeUtf16LengthOfUtf8String(reinterpret_cast<char const*>(it)));
        std::vector<wchar_t> buffer(realLength + 1);

        Externals::ConvertUtf8ToUtf16(reinterpret_cast<char const*>(it), buffer.data(), realLength + 1);
        return String(buffer.data());
    }

    Guid CustomAttribute::GetGuidArgument() const
    {
        Metadata::Database const& database(_parent.GetDatabase());

        Metadata::CustomAttributeRow const& customAttribute(database
            .GetRow<Metadata::TableId::CustomAttribute>(_attribute));

        Metadata::Blob const& valueBlob(database.GetBlob(customAttribute.GetValue()));
        ConstByteIterator it(valueBlob.Begin());

        // SKIP THE PREFIX
        it += 2;

        auto a0(Private::Read<std::uint32_t>(it, valueBlob.End()));

        auto a1(Private::Read<std::uint16_t>(it, valueBlob.End()));
        auto a2(Private::Read<std::uint16_t>(it, valueBlob.End()));

        auto a3(Private::Read<std::uint8_t>(it, valueBlob.End()));
        auto a4(Private::Read<std::uint8_t>(it, valueBlob.End()));
        auto a5(Private::Read<std::uint8_t>(it, valueBlob.End()));
        auto a6(Private::Read<std::uint8_t>(it, valueBlob.End()));
        auto a7(Private::Read<std::uint8_t>(it, valueBlob.End()));
        auto a8(Private::Read<std::uint8_t>(it, valueBlob.End()));
        auto a9(Private::Read<std::uint8_t>(it, valueBlob.End()));
        auto aa(Private::Read<std::uint8_t>(it, valueBlob.End()));

        return Guid(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, aa);
    }

}
