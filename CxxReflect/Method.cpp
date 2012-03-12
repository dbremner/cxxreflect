//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CoreComponents.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Method::Method()
    {
    }

    Method::Method(Type const& reflectedType, Detail::OwnedMethod const* const ownedMethod, InternalKey)
        : _reflectedType(reflectedType),
          _ownedMethod(ownedMethod)
    {
        Detail::AssertNotNull(ownedMethod);
        Detail::Assert([&]{ return reflectedType.IsInitialized(); });
        Detail::Assert([&]{ return ownedMethod->IsInitialized();  });
    }

    bool Method::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _ownedMethod.Get() != nullptr;
    }

    bool Method::operator!() const
    {
        return !IsInitialized();
    }

    void Method::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    Detail::OwnedMethod const& Method::GetOwnedMethod(InternalKey) const
    {
        AssertInitialized();
        return *_ownedMethod.Get();
    }

    Type Method::GetDeclaringType() const
    {
        AssertInitialized();
        Loader                  const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_ownedMethod.Get()->GetOwningType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _ownedMethod.Get()->GetOwningType().AsRowReference(), InternalKey());
    }

    Type Method::GetReflectedType() const
    {
        AssertInitialized();
        return _reflectedType.Realize();
    }

    bool operator==(Method const& lhs, Method const& rhs)
    {
        return lhs._ownedMethod.Get() == rhs._ownedMethod.Get();
    }

    bool operator<(Method const& lhs, Method const& rhs)
    {
        return std::less<Detail::OwnedMethod const*>()(lhs._ownedMethod.Get(), rhs._ownedMethod.Get());
    }

    bool Method::ContainsGenericParameters() const
    {
        return false; // TODO
    }

    MethodFlags Method::GetAttributes() const
    {
        return GetMethodDefRow().GetFlags();
    }

    CallingConvention Method::GetCallingConvention() const
    {
        Metadata::SignatureAttribute const convention(_ownedMethod.Get()
            ->GetElementSignature(GetReflectedType().GetAssembly().GetContext(InternalKey()).GetLoader())
            .GetCallingConvention());
        return static_cast<CallingConvention>(static_cast<unsigned>(convention));
    }

    SizeType Method::GetMetadataToken() const
    {
        return _ownedMethod.Get()->GetElement().AsRowReference().GetToken();
    }

    Metadata::MethodDefRow Method::GetMethodDefRow() const
    {
        AssertInitialized();
        return _ownedMethod.Get()->GetElementRow();
    }

    StringReference Method::GetName() const
    {
        return GetMethodDefRow().GetName();
    }

    bool Method::IsAbstract() const
    {
        return GetAttributes().IsSet(MethodAttribute::Abstract);
    }

    bool Method::IsAssembly() const
    {
        return GetAttributes().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::Assembly;
    }

    bool Method::IsConstructor() const
    {
        if (!GetAttributes().IsSet(MethodAttribute::SpecialName))
            return false;

        StringReference const name(GetName());
        return name == L".ctor" || name == L".cctor";
    }

    bool Method::IsFamily() const
    {
        return GetAttributes().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::Family;
    }

    bool Method::IsFamilyAndAssembly() const
    {
        return GetAttributes().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::FamilyAndAssembly;
    }

    bool Method::IsFamilyOrAssembly() const
    {
        return GetAttributes().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::FamilyOrAssembly;
    }

    bool Method::IsFinal() const
    {
        return GetAttributes().IsSet(MethodAttribute::Final);
    }

    bool Method::IsGenericMethod() const
    {
        return false; // TODO
    }

    bool Method::IsGenericMethodDefinition() const
    {
        return false; // TODO
    }

    bool Method::IsHideBySig() const
    {
        return GetAttributes().IsSet(MethodAttribute::HideBySig);
    }

    bool Method::IsPrivate() const
    {
        return GetAttributes().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::Private;
    }

    bool Method::IsPublic() const
    {
        return GetAttributes().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::Public;
    }

    bool Method::IsSpecialName() const
    {
        return GetAttributes().IsSet(MethodAttribute::SpecialName);
    }

    bool Method::IsStatic() const
    {
        return GetAttributes().IsSet(MethodAttribute::Static);
    }

    bool Method::IsVirtual() const
    {
        return GetAttributes().IsSet(MethodAttribute::Virtual);
    }

    CustomAttributeIterator Method::BeginCustomAttributes() const
    {
        AssertInitialized();

        // TODO Is this usage of AsRowReference() safe?
        return CustomAttribute::BeginFor(
            _ownedMethod.Get()->Resolve(_reflectedType.Realize()).GetDeclaringType().GetAssembly(),
            _ownedMethod.Get()->GetElement().AsRowReference(),
            InternalKey());
    }

    CustomAttributeIterator Method::EndCustomAttributes() const
    {
        AssertInitialized();

        // TODO Is this usage of AsRowReference() safe?
        return CustomAttribute::EndFor(
            _ownedMethod.Get()->Resolve(_reflectedType.Realize()).GetDeclaringType().GetAssembly(),
            _ownedMethod.Get()->GetElement().AsRowReference(),
            InternalKey());
    }

    Method::ParameterIterator Method::BeginParameters() const
    {
        AssertInitialized();

        Detail::OwnedMethod const& ownedMethod(*_ownedMethod.Get());
        Metadata::MethodDefRow const& methodRow(ownedMethod.GetElementRow());

        Metadata::RowReference firstParameterReference(methodRow.GetFirstParameter());
        Metadata::RowReference const lastParameterReference(methodRow.GetLastParameter());

        // If this method owns at least one param row, test the first param row's sequence number.
        // A param row with a sequence number of '0' is not a real param row, it is used to attach
        // metadata to the return type.
        if (firstParameterReference != lastParameterReference)
        {
            Metadata::ParamRow const& firstParameter(ownedMethod.GetElement()
                .GetDatabase()
                .GetRow<Metadata::TableId::Param>(firstParameterReference));

            if (firstParameter.GetSequence() == 0)
                ++firstParameterReference;
        }

        Metadata::ITypeResolver const& typeResolver(_reflectedType
            .Realize()
            .GetAssembly()
            .GetContext(InternalKey())
            .GetLoader());

        return ParameterIterator(*this, Detail::ParameterData(
            firstParameterReference,
            ownedMethod.GetElementSignature(typeResolver).BeginParameters(),
            InternalKey()));
    }

    Method::ParameterIterator Method::EndParameters() const
    {
        AssertInitialized();

        Metadata::ITypeResolver const& typeResolver(_reflectedType
            .Realize()
            .GetAssembly()
            .GetContext(InternalKey())
            .GetLoader());

        return ParameterIterator(*this, Detail::ParameterData(
            _ownedMethod.Get()->GetElementRow().GetLastParameter(),
            _ownedMethod.Get()->GetElementSignature(typeResolver).EndParameters(),
            InternalKey()));
    }

}
