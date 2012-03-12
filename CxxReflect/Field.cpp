//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Field::Field()
    {
    }

    Field::Field(Type const& reflectedType, Detail::OwnedField const* const ownedField, InternalKey)
        : _reflectedType(reflectedType),
          _ownedField(ownedField)
    {
        Detail::AssertNotNull(ownedField);
        Detail::Assert([&]{ return reflectedType.IsInitialized(); });
        Detail::Assert([&]{ return ownedField->IsInitialized();   });
    }

    bool Field::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _ownedField.Get() != nullptr;
    }

    bool Field::operator!() const
    {
        return !IsInitialized();
    }

    void Field::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    Detail::OwnedField const& Field::GetOwnedField(InternalKey) const
    {
        AssertInitialized();
        return *_ownedField.Get();
    }

    Type Field::GetDeclaringType() const
    {
        AssertInitialized();
        Loader                  const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_ownedField.Get()->GetOwningType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _ownedField.Get()->GetOwningType().AsRowReference(), InternalKey());
    }

    Type Field::GetReflectedType() const
    {
        AssertInitialized();
        return _reflectedType.Realize();
    }

    FieldFlags Field::GetAttributes() const
    {
        return GetOwnedField(InternalKey()).GetElementRow().GetFlags();
    }

    Type Field::GetFieldType() const
    {
        return Type(
            GetDeclaringType().GetAssembly(),
            GetOwnedField(InternalKey()).GetElementRow().GetSignature(),
            InternalKey());
    }

    SizeType Field::GetMetadataToken() const
    {
        return GetOwnedField(InternalKey()).GetElementRow().GetSelfReference().GetToken();
    }

    StringReference Field::GetName() const
    {
        return GetOwnedField(InternalKey()).GetElementRow().GetName();
    }

    bool Field::IsAssembly() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::Assembly;
    }

    bool Field::IsFamily() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::Family;
    }

    bool Field::IsFamilyAndAssembly() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::FamilyAndAssembly;
    }

    bool Field::IsFamilyOrAssembly() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::FamilyOrAssembly;
    }

    bool Field::IsInitOnly() const
    {
        return GetAttributes().IsSet(FieldAttribute::InitOnly);
    }

    bool Field::IsLiteral() const
    {
        return GetAttributes().IsSet(FieldAttribute::Literal);
    }

    bool Field::IsNotSerialized() const
    {
        return GetAttributes().IsSet(FieldAttribute::NotSerialized);
    }

    bool Field::IsPinvokeImpl() const
    {
        return GetAttributes().IsSet(FieldAttribute::PInvokeImpl);
    }

    bool Field::IsPrivate() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::Private;
    }

    bool Field::IsPublic() const
    {
        return GetAttributes().WithMask(FieldAttribute::FieldAccessMask) == FieldAttribute::Public;
    }

    bool Field::IsSpecialName() const
    {
        // TODO Do we need to check for RuntimeSpecialName too?
        return GetAttributes().IsSet(FieldAttribute::SpecialName);
    }

    bool Field::IsStatic() const
    {
        return GetAttributes().IsSet(FieldAttribute::Static);
    }

    bool operator==(Field const& lhs, Field const& rhs)
    {
        return lhs._ownedField.Get() == rhs._ownedField.Get();
    }

    bool operator<(Field const& lhs, Field const& rhs)
    {
        return std::less<Detail::OwnedField const*>()(lhs._ownedField.Get(), rhs._ownedField.Get());
    }

}
