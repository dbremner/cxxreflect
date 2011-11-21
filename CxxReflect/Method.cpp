//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Method.hpp"

namespace CxxReflect {

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
        return CallingConvention(); // TODO
    }

    SizeType Method::GetMetadataToken() const
    {
        return _method.GetToken();
    }

    Metadata::MethodDefRow Method::GetMethodDefRow() const
    {
        Detail::Verify([&]{ return _method.GetTable() == Metadata::TableId::MethodDef; });
        return _declaringType
            .GetAssembly()
            .GetContext(InternalKey())
            .GetDatabase()
            .GetRow<Metadata::TableId::MethodDef>(_method.GetIndex());
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
