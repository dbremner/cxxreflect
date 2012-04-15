//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CoreComponents.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Method::Method()
    {
    }

    Method::Method(Type const& reflectedType, Detail::MethodContext const* const context, InternalKey)
        : _reflectedType(reflectedType),
          _context(context)
    {
        Detail::AssertNotNull(context);
        Detail::Assert([&]{ return reflectedType.IsInitialized(); });
        Detail::Assert([&]{ return context->IsInitialized();      });
    }

    bool Method::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _context.Get() != nullptr;
    }

    bool Method::operator!() const
    {
        return !IsInitialized();
    }

    void Method::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    Detail::MethodContext const& Method::GetContext(InternalKey) const
    {
        AssertInitialized();
        return *_context.Get();
    }

    Type Method::GetDeclaringType() const
    {
        AssertInitialized();
        Loader                  const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_context.Get()->GetOwningType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _context.Get()->GetOwningType().AsRowReference(), InternalKey());
    }

    Type Method::GetReflectedType() const
    {
        AssertInitialized();
        return _reflectedType.Realize();
    }

    bool operator==(Method const& lhs, Method const& rhs)
    {
        // TODO This does not correctly handle generics or other instantiated types.  Or does it?
        return lhs._context.Get()->GetElement() == rhs._context.Get()->GetElement();
    }

    bool operator<(Method const& lhs, Method const& rhs)
    {
        return std::less<Detail::MethodContext const*>()(lhs._context.Get(), rhs._context.Get());
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
        Metadata::SignatureAttribute const convention(_context.Get()
            ->GetElementSignature(GetReflectedType().GetAssembly().GetContext(InternalKey()).GetLoader())
            .GetCallingConvention());
        return static_cast<CallingConvention>(static_cast<unsigned>(convention));
    }

    SizeType Method::GetMetadataToken() const
    {
        return _context.Get()->GetElement().AsRowReference().GetToken();
    }

    Metadata::MethodDefRow Method::GetMethodDefRow() const
    {
        AssertInitialized();
        return _context.Get()->GetElementRow();
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
            _context.Get()->Resolve(_reflectedType.Realize()).GetDeclaringType().GetAssembly(),
            _context.Get()->GetElement().AsRowReference(),
            InternalKey());
    }

    CustomAttributeIterator Method::EndCustomAttributes() const
    {
        AssertInitialized();

        // TODO Is this usage of AsRowReference() safe?
        return CustomAttribute::EndFor(
            _context.Get()->Resolve(_reflectedType.Realize()).GetDeclaringType().GetAssembly(),
            _context.Get()->GetElement().AsRowReference(),
            InternalKey());
    }

    Method::ParameterIterator Method::BeginParameters() const
    {
        AssertInitialized();

        Detail::MethodContext const& context(*_context.Get());
        Metadata::MethodDefRow const& methodRow(context.GetElementRow());

        Metadata::RowReference firstParameterReference(methodRow.GetFirstParameter());
        Metadata::RowReference const lastParameterReference(methodRow.GetLastParameter());

        // If this method owns at least one param row, test the first param row's sequence number.
        // A param row with a sequence number of '0' is not a real param row, it is used to attach
        // metadata to the return type.
        if (firstParameterReference != lastParameterReference)
        {
            Metadata::ParamRow const& firstParameter(context.GetElement()
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
            context.GetElementSignature(typeResolver).BeginParameters(),
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
            _context.Get()->GetElementRow().GetLastParameter(),
            _context.Get()->GetElementSignature(typeResolver).EndParameters(),
            InternalKey()));
    }

    SizeType Method::GetParameterCount() const
    {
        AssertInitialized();

        Metadata::ITypeResolver const& typeResolver(_reflectedType
            .Realize()
            .GetAssembly()
            .GetContext(InternalKey())
            .GetLoader());

        return _context.Get()->GetElementSignature(typeResolver).GetParameterCount();
    }

    Parameter Method::GetReturnParameter() const
    {
        AssertInitialized();

        // Metadata::TypeSignature const returnTypeSignature(_context.Get()->GetElementSignature().GetReturnType());
        // TODO IMPLEMENT
        throw 0;
    }

    Type Method::GetReturnType() const
    {
        AssertInitialized();

        // Metadata::TypeSignature const returnTypeSignature(_context.Get()->GetElementSignature().GetReturnType());
        // TODO IMPLEMENT
        throw 0;
    }

}
