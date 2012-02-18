//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
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
        Metadata::Database      const& database(_context.Get()->GetDeclaringType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _context.Get()->GetDeclaringType().AsRowReference(), InternalKey());
    }

    Type Method::GetReflectedType() const
    {
        AssertInitialized();
        return _reflectedType.Realize();
    }

    bool operator==(Method const& lhs, Method const& rhs)
    {
        return lhs._context.Get() == rhs._context.Get();
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
        Metadata::SignatureAttribute const convention(_context.Get()->GetMemberSignature().GetCallingConvention());
        return static_cast<CallingConvention>(static_cast<unsigned>(convention));
    }

    SizeType Method::GetMetadataToken() const
    {
        return _context.Get()->GetMember().AsRowReference().GetToken();
    }

    Metadata::MethodDefRow Method::GetMethodDefRow() const
    {
        AssertInitialized();
        return _context.Get()->GetMemberRow();
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
            _context.Get()->GetMember().AsRowReference(),
            InternalKey());
    }

    CustomAttributeIterator Method::EndCustomAttributes() const
    {
        AssertInitialized();

        // TODO Is this usage of AsRowReference() safe?
        return CustomAttribute::EndFor(
            _context.Get()->Resolve(_reflectedType.Realize()).GetDeclaringType().GetAssembly(),
            _context.Get()->GetMember().AsRowReference(),
            InternalKey());
    }

    Method::ParameterIterator Method::BeginParameters() const
    {
        AssertInitialized();

        return ParameterIterator(*this, Detail::ParameterData(
            _context.Get()->GetMemberRow().GetFirstParameter(),
            _context.Get()->GetMemberSignature().BeginParameters(),
            InternalKey()));
    }

    Method::ParameterIterator Method::EndParameters() const
    {
        AssertInitialized();

        return ParameterIterator(*this, Detail::ParameterData(
            _context.Get()->GetMemberRow().GetLastParameter(),
            _context.Get()->GetMemberSignature().EndParameters(),
            InternalKey()));
    }

}
