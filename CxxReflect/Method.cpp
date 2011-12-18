//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/MetadataLoader.hpp"
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
        Detail::VerifyNotNull(context);
        Detail::Verify([&]{ return reflectedType.IsInitialized(); });
        Detail::Verify([&]{ return context->IsInitialized();      });
    }

    bool Method::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _context.Get() != nullptr;
    }

    void Method::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }

    Type Method::GetDeclaringType() const
    {
        VerifyInitialized();
        MetadataLoader          const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_context.Get()->GetDeclaringType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _context.Get()->GetDeclaringType().AsRowReference(), InternalKey());
    }

    Type Method::GetReflectedType() const
    {
        VerifyInitialized();
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
        VerifyInitialized();
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

}
