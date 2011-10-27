//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Type.hpp"

#include <sstream>

namespace {

    bool IsSystemAssembly(CxxReflect::Assembly const& assembly)
    {
        // The system assembly has no assembly references; it is usually mscorlib.dll, but it could
        // be named something else (e.g., Platform.winmd in WinRT?)
        return std::distance(assembly.BeginReferencedAssemblyNames(), assembly.EndReferencedAssemblyNames()) == 0;
    }

    bool IsSystemType(CxxReflect::Type const& type, CxxReflect::StringReference const& name)
    {
        return IsSystemAssembly(type.GetAssembly()) && type.GetFullName() == name;
    }

}

namespace CxxReflect {

    bool const TodoNotYetImplementedFlag = false;

    void Type::AccumulateFullNameInto(std::wostream& os) const
    {
        // TODO ENSURE WE ESCAPE THE TYPE NAME CORRECTLY

        if (IsTypeDef())
        {
            if (IsNested())
            {
                GetDeclaringType().AccumulateFullNameInto(os);
                os << L'+' << GetName();
            }
            else
            {
                os << GetNamespace() << L'.' << GetName();
            }
        }

        // TODO TYPESPEC SUPPORT
    }

    void Type::AccumulateAssemblyQualifiedNameInto(std::wostream& os) const
    {
        AccumulateFullNameInto(os);
        os << L", " << _assembly.GetName().GetFullName();
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

        });
    }

    Type Type::GetDeclaringType() const
    {
        if (IsNested())
        {
            Metadata::Database const& database(_assembly.GetDatabase());
            auto const it(std::lower_bound(
                database.Begin<Metadata::TableId::NestedClass>(),
                database.End<Metadata::TableId::NestedClass>(),
                _type,
                [](Metadata::NestedClassRow const& r, Metadata::TableReference const index)
            {
                return r.GetNestedClass() < index;
            }));

            if (it == database.End<Metadata::TableId::NestedClass>() || it->GetNestedClass() != _type)
                throw std::logic_error("wtf");

            // TODO IS THE TYPE DEF CHECK DONE AT THE PHYSICAL LAYER?
            Metadata::TableReference const enclosingType(it->GetEnclosingClass());
            if (enclosingType.GetTable() != Metadata::TableId::TypeDef)
                throw std::logic_error("wtf");

            return Type(_assembly, enclosingType);
        }

        // TODO OTHER KINDS OF DECLARING TYPES
        return Type();
    }

    String Type::GetAssemblyQualifiedName() const
    {
        std::wostringstream oss;
        AccumulateAssemblyQualifiedNameInto(oss);
        return oss.str();
    }

    String Type::GetFullName() const
    {
        std::wostringstream oss;
        AccumulateFullNameInto(oss);
        return oss.str();
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
        if (IsNested())
        {
            return GetDeclaringType().GetNamespace();
        }
        
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
        });
    }

    bool Type::IsAnsiClass() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::StringFormatMask) == TypeAttribute::AnsiClass;
        });
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
        });
    }

    bool Type::IsAutoLayout() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::LayoutMask) == TypeAttribute::AutoLayout;
        });
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
        });
    }

    bool Type::IsGenericParameter() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsGenericType() const
    {
        // TODO THIS IS WRONG
        StringReference const name(GetName());
        return std::find(name.begin(), name.end(), L'`') != name.end();
    }

    bool Type::IsGenericTypeDefinition() const
    {
        // TODO THIS IS WRONG
        return IsGenericType();
    }

    bool Type::IsImport() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::Import);
        });
    }

    bool Type::IsInterface() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::ClassSemanticsMask) == TypeAttribute::Interface;
        });
    }

    bool Type::IsLayoutSequential() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::LayoutMask) == TypeAttribute::SequentialLayout;
        });
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
        });
    }

    bool Type::IsNestedAssembly() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedAssembly;
        });
    }

    bool Type::IsNestedFamilyAndAssembly() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedFamilyAndAssembly;
        });

    }
    bool Type::IsNestedFamily() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedFamily;
        });
    }

    bool Type::IsNestedFamilyOrAssembly() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedFamilyOrAssembly;
        });
    }

    bool Type::IsNestedPrivate() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedPrivate;
        });
    }

    bool Type::IsNestedPublic() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NestedPublic;
        });
    }

    bool Type::IsNotPublic() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NotPublic;
        });
    }

    bool Type::IsPointer() const
    {
        if (IsTypeDef()) { return false; }

        return TodoNotYetImplementedFlag;
    }

    bool Type::IsPrimitive() const
    {
        if (!IsTypeDef()) { return false; }

        if (!IsSystemAssembly(_assembly)) { return false; }

        if (GetTypeDefRow().GetNamespace() != L"System") { return false; }

        StringReference const& name(GetTypeDefRow().GetName());
        switch (name[0])
        {
        case L'B': return name == L"Boolean" || name == L"Byte";
        case L'C': return name == L"Char";
        case L'D': return name == L"Double";
        case L'I': return name == L"Int16" || name == L"Int32" || name == L"Int64" || name == L"IntPtr";
        case L'S': return name == L"SByte" || name == L"Single";
        case L'U': return name == L"UInt16" || name == L"UInt32" || name == L"UInt64" || name == L"UIntPtr";
        }

        return false;
    }

    bool Type::IsPublic() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::Public;
        });
    }

    bool Type::IsSealed() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::Sealed);
        });
    }

    bool Type::IsSerializable() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::Serializable);
        });
    }

    bool Type::IsSpecialName() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().IsSet(TypeAttribute::SpecialName);
        });
    }

    bool Type::IsUnicodeClass() const
    {
        return ResolveTypeDefAndCall([](Metadata::TypeDefRow const& r)
        {
            return r.GetFlags().WithMask(TypeAttribute::StringFormatMask) == TypeAttribute::UnicodeClass;
        });
    }

    bool Type::IsValueType() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsVisible() const
    {
        if (IsTypeDef())
        {
            if (IsNested() && !GetDeclaringType().IsVisible())
                return false;

            return GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask) > TypeAttribute::NotPublic;
        }
        // TODO CHECK BEHAVIOR FOR TYPESPECS

        return TodoNotYetImplementedFlag;
    }

}
