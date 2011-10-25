//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Type.hpp"

namespace {

    bool IsSystemAssembly(CxxReflect::Assembly const& assembly)
    {
        // The system assembly has no assembly references; it is usually mscorlib.dll, but it could
        // be named something else (e.g., Platform.winmd in WinRT?)
        return std::distance(assembly.BeginReferencedAssemblyNames(), assembly.EndReferencedAssemblyNames()) != 0;
    }

    bool IsSystemType(CxxReflect::Type const& type, CxxReflect::StringReference const& name)
    {
        return IsSystemAssembly(type.GetAssembly()) && type.GetFullName() == name;
    }

}

namespace CxxReflect {

    bool const TodoNotYetImplementedFlag = false;

    bool Type::HasBaseType() const
    {
        return !TodoNotYetImplementedFlag;
    }

    Type Type::GetBaseType() const
    {
        return ResolveTypeDefAndCall([&](Metadata::TypeDefRow const& r) -> Type
        {
            Metadata::TableReference extends(r.GetExtends());
            switch (extends.GetTable())
            {
            case Metadata::TableId::TypeDef:
            case Metadata::TableId::TypeRef:
            case Metadata::TableId::TypeSpec:
                // TODO DO WE WANT TO RESOLVE TYPEREFS HERE OR DEFER RESOLUTION?
                return Type(_assembly, extends);

            default:
                throw std::logic_error("wtf");
            }

        }, Type());
    }

    String Type::GetFullName() const
    {
        return String(GetNamespace().c_str()) + L"." + GetName().c_str(); // TODO HANDLE GENERIC AND NESTED TYPES
    }

    StringReference Type::GetName() const
    {
        VerifyInitialized();

        if (IsTypeDef())
            return GetTypeDefRow().GetName();

        return L""; // TODO
    }

    StringReference Type::GetNamespace() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetNamespace();
        }, StringReference());
    }

    bool Type::IsAbstract() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::Abstract);
        }, false);
    }

    bool Type::IsAnsiClass() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::StringFormatMask) == TypeAttribute::AnsiClass;
        }, false);
    }

    bool Type::IsArray() const
    {
        VerifyInitialized();
        if (IsTypeDef()) { return false; }

        return TodoNotYetImplementedFlag;
    }

    bool Type::IsAutoClass() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::StringFormatMask) == TypeAttribute::AutoClass;
        }, false);
    }

    bool Type::IsAutoLayout() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::LayoutMask) == TypeAttribute::AutoLayout;
        }, false);
    }

    bool Type::IsByRef() const
    {
        VerifyInitialized();

        if (IsTypeDef()) { return false; }

        return TodoNotYetImplementedFlag;
    }

    bool Type::IsClass() const
    {
        VerifyInitialized();

        return !IsInterface() && !IsValueType();
    }

    bool Type::IsComObject() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsContextful() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsEnum() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsExplicitLayout() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::LayoutMask) == TypeAttribute::ExplicitLayout;
        }, false);
    }

    bool Type::IsGenericParameter() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsGenericType() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsGenericTypeDefinition() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsImport() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::Import);
        }, false);
    }

    bool Type::IsInterface() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::ClassSemanticsMask) == TypeAttribute::Interface;
        }, false);
    }

    bool Type::IsLayoutSequential() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::LayoutMask) == TypeAttribute::SequentialLayout;
        }, false);
    }

    bool Type::IsMarshalByRef() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsNested() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) > TypeAttribute::Public;
        }, false);
    }

    bool Type::IsNestedAssembly() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedAssembly;
        }, false);
    }

    bool Type::IsNestedFamilyAndAssembly() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedFamilyAndAssembly;
        }, false);

    }
    bool Type::IsNestedFamily() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedFamily;
        }, false);
    }

    bool Type::IsNestedFamilyOrAssembly() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedFamilyOrAssembly;
        }, false);
    }

    bool Type::IsNestedPrivate() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedPrivate;
        }, false);
    }

    bool Type::IsNestedPublic() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedPublic;
        }, false);
    }

    bool Type::IsNotPublic() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NotPublic;
        }, false);
    }

    bool Type::IsPointer() const
    {
        if (IsTypeDef()) { return false; }

        return TodoNotYetImplementedFlag;
    }

    bool Type::IsPrimitive() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsPublic() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::Public;
        }, false);
    }

    bool Type::IsSealed() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::Sealed);
        }, false);
    }

    bool Type::IsSerializable() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::Serializable);
        }, false);
    }

    bool Type::IsSpecialName() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::SpecialName);
        }, false);
    }

    bool Type::IsUnicodeClass() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::StringFormatMask) == TypeAttribute::UnicodeClass;
        }, false);
    }

    bool Type::IsValueType() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsVisible() const
    {
        return TodoNotYetImplementedFlag;
    }

}
