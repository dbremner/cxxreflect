
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

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

            Metadata::TypeDefRow const& typeDefRow(Metadata::GetOwnerOfMethodDef(methodDefRow));

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
                        Detail::AssertFail(L"Not yet implemented");

                    Metadata::ITypeResolver const& typeResolver(assembly.GetContext(InternalKey()).GetLoader());

                    Metadata::SignatureComparer const comparer(
                        &typeResolver,
                        &parentDatabase,
                        &type.GetAssembly().GetContext(InternalKey()).GetDatabase());

                    auto const constructorIt(std::find_if(type.BeginConstructors(flags),
                                 type.EndConstructors(),
                                 [&](Method const& constructor)
                    {
                        return comparer(
                            memberRefRow.GetSignature().As<Metadata::MethodSignature>(),
                            constructor.GetContext(InternalKey()).GetElementSignature(typeResolver));
                    }));

                    Detail::Verify([&]{ return constructorIt != type.EndConstructors(); });
                    
                    _constructor = Detail::MethodHandle(*constructorIt);
                }
                else
                {
                    Detail::AssertFail(L"Not yet implemented");
                }
            }
            else
            {
                Detail::AssertFail(L"Not yet implemented");
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
            Metadata::GetCustomAttributesRange(
                Metadata::FullReference(&assembly.GetContext(InternalKey()).GetDatabase(), parent)).first);
    }

    CustomAttributeIterator CustomAttribute::EndFor(Assembly               const& assembly,
                                                    Metadata::RowReference const& parent,
                                                    InternalKey)
    {
        return CustomAttributeIterator(
            assembly,
            Metadata::GetCustomAttributesRange(
                Metadata::FullReference(&assembly.GetContext(InternalKey()).GetDatabase(), parent)).second);
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

    String CustomAttribute::GetSingleStringArgument() const
    {
        Metadata::Database const& database(_parent.GetDatabase());

        Metadata::CustomAttributeRow const& customAttribute(database
            .GetRow<Metadata::TableId::CustomAttribute>(_attribute));

        Metadata::BlobReference const& valueBlob(customAttribute.GetValue());
        ConstByteIterator it(valueBlob.Begin());

        // All custom attribute signatures begin with a two-byte, little-endian integer with the
        // value '1'.  If the signature doesn't have this prefix, we've screwed up somewhere or
        // the metadata is invalid.
        auto const prefix(Metadata::ReadSignatureElement<std::uint16_t>(it, valueBlob.End()));
        if (prefix != 1)
            throw RuntimeError(L"Invalid custom attribute signature");

        // TODO We materialize three separate buffers here; we should look into removing one or two.
        SizeType const utf8Length(Metadata::ReadCompressedUInt32(it, valueBlob.End()));
        Detail::Verify([&]{ return Detail::Distance(it, valueBlob.End()) >= utf8Length; });

        std::vector<char> utf8Buffer(it, it + utf8Length);
        utf8Buffer.push_back(L'\0');

        SizeType const utf16Length(Externals::ComputeUtf16LengthOfUtf8String(reinterpret_cast<char const*>(it)));
        std::vector<wchar_t> utf16buffer(utf16Length);

        Externals::ConvertUtf8ToUtf16(utf8Buffer.data(), utf16buffer.data(), utf16Length);
        return String(utf16buffer.data());
    }

    Guid CustomAttribute::GetSingleGuidArgument() const
    {
        Metadata::Database const& database(_parent.GetDatabase());

        Metadata::CustomAttributeRow const& customAttribute(database
            .GetRow<Metadata::TableId::CustomAttribute>(_attribute));

        Metadata::BlobReference const& valueBlob(customAttribute.GetValue());
        ConstByteIterator it(valueBlob.Begin());

        // All custom attribute signatures begin with a two-byte, little-endian integer with the
        // value '1'.  If the signature doesn't have this prefix, we've screwed up somewhere or
        // the metadata is invalid.
        auto const prefix(Metadata::ReadSignatureElement<std::uint16_t>(it, valueBlob.End()));
        if (prefix != 1)
            throw RuntimeError(L"Invalid custom attribute signature");

        auto const a0(Metadata::ReadSignatureElement<std::uint32_t>(it, valueBlob.End()));

        auto const a1(Metadata::ReadSignatureElement<std::uint16_t>(it, valueBlob.End()));
        auto const a2(Metadata::ReadSignatureElement<std::uint16_t>(it, valueBlob.End()));

        auto const a3(Metadata::ReadSignatureElement<std::uint8_t>(it, valueBlob.End()));
        auto const a4(Metadata::ReadSignatureElement<std::uint8_t>(it, valueBlob.End()));
        auto const a5(Metadata::ReadSignatureElement<std::uint8_t>(it, valueBlob.End()));
        auto const a6(Metadata::ReadSignatureElement<std::uint8_t>(it, valueBlob.End()));
        auto const a7(Metadata::ReadSignatureElement<std::uint8_t>(it, valueBlob.End()));
        auto const a8(Metadata::ReadSignatureElement<std::uint8_t>(it, valueBlob.End()));
        auto const a9(Metadata::ReadSignatureElement<std::uint8_t>(it, valueBlob.End()));
        auto const aa(Metadata::ReadSignatureElement<std::uint8_t>(it, valueBlob.End()));

        return Guid(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, aa);
    }
}
