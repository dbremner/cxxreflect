//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace { namespace Private {

    template <typename T>
    bool CoreFilterMember(BindingFlags const filter, bool const isDeclaringType, T const& current)
    {
        typedef typename Detail::Identity<decltype(current.GetMemberRow().GetFlags())>::Type::EnumerationType AttributeType;
        auto currentFlags(current.GetMemberRow().GetFlags());

        if (currentFlags.IsSet(AttributeType::Static))
        {
            if (!filter.IsSet(BindingAttribute::Static))
                return true;
        }
        else
        {
            if (!filter.IsSet(BindingAttribute::Instance))
                return true;
        }

        if (currentFlags.WithMask(AttributeType::MemberAccessMask) == AttributeType::Public)
        {
            if (!filter.IsSet(BindingAttribute::Public))
                return true;
        }
        else
        {
            if (!filter.IsSet(BindingAttribute::NonPublic))
                return true;
        }

        if (!isDeclaringType)
        {
            if (filter.IsSet(BindingAttribute::DeclaredOnly))
                return true;

            // Static members are not inherited, but they are returned with FlattenHierarchy
            if (currentFlags.IsSet(AttributeType::Static) && !filter.IsSet(BindingAttribute::FlattenHierarchy))
                return true;

            StringReference const memberName(current.GetMemberRow().GetName());

            // Nonpublic methods inherited from base classes are never returned, except for
            // explicit interface implementations, which may be returned:
            if (currentFlags.WithMask(AttributeType::MemberAccessMask) == AttributeType::Private)
            {
                if (currentFlags.IsSet(AttributeType::Static))
                    return true;

                if (!std::any_of(memberName.begin(), memberName.end(), [](Character const c) { return c == L'.'; }))
                    return true;
            }
        }

        return false;
    }

    struct InterfaceStrictWeakOrdering
    {
        typedef Metadata::InterfaceImplRow InterfaceImplRow;
        typedef Metadata::RowReference     RowReference;

        bool operator()(InterfaceImplRow const& lhs, InterfaceImplRow const& rhs) const volatile
        {
            Detail::Verify([&]{ return lhs.GetClass().GetTable() == Metadata::TableId::TypeDef; });
            Detail::Verify([&]{ return rhs.GetClass().GetTable() == Metadata::TableId::TypeDef; });

            return lhs.GetClass().GetIndex() < rhs.GetClass().GetIndex();
        }

        bool operator()(InterfaceImplRow const& lhs, RowReference const& rhs) const volatile
        {
            Detail::Verify([&]{ return lhs.GetClass().GetTable() == Metadata::TableId::TypeDef; });
            Detail::Verify([&]{ return rhs.GetTable()            == Metadata::TableId::TypeDef; });

            return lhs.GetClass().GetIndex() < rhs.GetIndex();
        }

        bool operator()(RowReference const& lhs, InterfaceImplRow const& rhs) const volatile
        {
            Detail::Verify([&]{ return lhs.GetTable()            == Metadata::TableId::TypeDef; });
            Detail::Verify([&]{ return rhs.GetClass().GetTable() == Metadata::TableId::TypeDef; });

            return lhs.GetIndex() < rhs.GetClass().GetIndex();
        }
    };

} } }

namespace CxxReflect {

    bool const TodoNotYetImplementedFlag = false;

    Type::Type()
    {
    }

    Type::Type(Assembly const& assembly, Metadata::RowReference const& type, InternalKey)
        : _assembly(assembly), _type(Metadata::ElementReference(type))
    {
        Detail::Verify([&] { return assembly.IsInitialized(); });

        // If we were initialized with an empty type, do not attempt to do any type resolution.
        if (!type.IsInitialized())
            return;

        switch (type.GetTable())
        {
        case Metadata::TableId::TypeDef:
        {
            // Wonderful news!  We have a TypeDef and we don't need to do any further work.
            break;
        }

        case Metadata::TableId::TypeRef:
        {
            // Resolve the TypeRef into a TypeDef, throwing on failure:
            MetadataLoader const& loader(assembly.GetContext(InternalKey()).GetLoader());
            Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());

            Metadata::FullReference const resolvedType(
                loader.ResolveType(Metadata::FullReference(&database, type), InternalKey()));

            Detail::Verify([&]{ return resolvedType.IsInitialized(); });

            _assembly = Assembly(
                &loader.GetContextForDatabase(resolvedType.GetDatabase(), InternalKey()),
                InternalKey());

            _type = Metadata::ElementReference(resolvedType.AsRowReference());
            Detail::Verify([&]{ return _type.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });

            break;
        }

        case Metadata::TableId::TypeSpec:
        {
            // Get the signature for the TypeSpec token and use that instead:
            Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());
            Metadata::TypeSpecRow const typeSpec(database.GetRow<Metadata::TableId::TypeSpec>(type.GetIndex()));
            _type = Metadata::ElementReference(typeSpec.GetSignature());

            break;
        }

        default:
        {
            Detail::VerifyFail("Unexpected argument");
            break;
        }
        }
    }

    Type::Type(Assembly const& assembly, Metadata::BlobReference const& type, InternalKey)
        : _assembly(assembly), _type(Metadata::ElementReference(type))
    {
        Detail::Verify([&]{ return assembly.IsInitialized(); });
        Detail::Verify([&]{ return type.IsInitialized();     });

        Metadata::TypeSignature const signature(assembly
            .GetContext(InternalKey())
            .GetDatabase()
            .GetBlob(type)
            .As<Metadata::TypeSignature>());

        if (signature.GetKind() == Metadata::TypeSignature::Kind::Primitive)
        {
            Type const primitiveType(Utility::GetPrimitiveType(_assembly.Realize(), signature.GetPrimitiveElementType()));
            Detail::Verify([&]{ return primitiveType.IsInitialized(); });

            _assembly = primitiveType.GetAssembly();
            _type = Metadata::RowReference::FromToken(primitiveType.GetMetadataToken());
        }
    }

    Assembly Type::GetAssembly() const
    {
        return _assembly.Realize();
    }

    bool Type::AccumulateFullNameInto(OutputStream& os) const
    {
        // TODO ENSURE WE ESCAPE THE TYPE NAME CORRECTLY

        if (IsTypeDef())
        {
            if (IsNested())
            {
                GetDeclaringType().AccumulateFullNameInto(os);
                os << L'+' << GetName();
            }
            else if (GetNamespace().size() > 1) // TODO REMOVE NULL TERMINATOR FROM COUNT
            {
                os << GetNamespace() << L'.' << GetName();
            }
            else
            {
                os << GetName();
            }
        }
        else
        {
            Metadata::TypeSignature const signature(GetTypeSpecSignature());

            // A TypeSpec for an uninstantiated generic type has no name:
            if (Metadata::ClassVariableSignatureInstantiator::RequiresInstantiation(signature))
                return false;

            switch (signature.GetKind())
            {
            case Metadata::TypeSignature::Kind::GenericInst:
            {
                if (std::any_of(signature.BeginGenericArguments(), signature.EndGenericArguments(), [&](Metadata::TypeSignature const& sig)
                {
                    return sig.GetKind() == Metadata::TypeSignature::Kind::Var;
                }))
                {
                    return false;
                }

                Type const genericType(_assembly.Realize(), signature.GetGenericTypeReference(), InternalKey());
                genericType.AccumulateFullNameInto(os);

                os << L'[';
                std::for_each(
                    signature.BeginGenericArguments(),
                    signature.EndGenericArguments(),
                    [&](Metadata::TypeSignature const& argumentSignature)
                {
                    os << L'[';
                    Type const argumentType(
                        _assembly.Realize(),
                        Metadata::BlobReference(
                            static_cast<SizeType>(argumentSignature.BeginBytes() - _assembly.Realize().GetContext(InternalKey()).GetDatabase().GetBlobs().Begin()),
                            static_cast<SizeType>(argumentSignature.EndBytes() - argumentSignature.BeginBytes())), InternalKey());
                    argumentType.AccumulateAssemblyQualifiedNameInto(os);
                    os << L']';
                });
                os << L']';
                break;
            }
            case Metadata::TypeSignature::Kind::ClassType:
            {
                Type const classType(_assembly.Realize(), signature.GetTypeReference(), InternalKey());
                classType.AccumulateFullNameInto(os);
                break;
            }
            case Metadata::TypeSignature::Kind::SzArray:
            {
                Type const classType(
                    _assembly.Realize(),
                    Metadata::BlobReference(
                        static_cast<SizeType>(signature.GetArrayType().BeginBytes() - _assembly.Realize().GetContext(InternalKey()).GetDatabase().GetBlobs().Begin()),
                        static_cast<SizeType>(signature.GetArrayType().EndBytes() - signature.BeginBytes())),
                    InternalKey());

                classType.AccumulateFullNameInto(os);
                os << L"[]";
                break;
            }
            case Metadata::TypeSignature::Kind::Var:
            {
                // TODO MVAR?
                os << L"Var!" << signature.GetVariableNumber();
                break;
            }
            default:
            {
                Detail::VerifyFail("Not yet implemented");
                break;
            }
            }
        }

        // TODO TYPESPEC SUPPORT
        return true;
    }

    void Type::AccumulateAssemblyQualifiedNameInto(OutputStream& os) const
    {
        if (AccumulateFullNameInto(os))
        {
            os << L", " << _assembly.Realize().GetName().GetFullName();
        }
    }

    Metadata::TypeDefRow Type::GetTypeDefRow() const
    {
        Detail::Verify([&]{ return IsTypeDef(); });

        return _assembly
            .Realize()
            .GetContext(InternalKey())
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeDef>(_type.AsRowReference().GetIndex());
    }

    Metadata::TypeSignature Type::GetTypeSpecSignature() const
    {
        Detail::Verify([&]{ return IsTypeSpec(); });

        return _assembly
            .Realize()
            .GetContext(InternalKey())
            .GetDatabase()
            .GetBlob(_type.AsBlobReference())
            .As<Metadata::TypeSignature>();
    }

    Type::MethodIterator Type::BeginConstructors(BindingFlags flags) const
    {
        VerifyInitialized();
        Detail::Verify([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        flags.Set(BindingAttribute::InternalUseOnlyConstructor);
        flags.Set(BindingAttribute::DeclaredOnly);
        flags.Unset(BindingAttribute::FlattenHierarchy);

        Detail::MethodTable const& table(_assembly.Realize().GetContext(InternalKey()).GetOrCreateMethodTable(_type));
        return MethodIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::MethodIterator Type::EndConstructors() const
    {
        return MethodIterator();
    }

    Type::EventIterator Type::BeginEvents(BindingFlags const flags) const
    {
        VerifyInitialized();
        Detail::Verify([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        Detail::EventTable const& table(_assembly.Realize().GetContext(InternalKey()).GetOrCreateEventTable(_type));
        return EventIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::EventIterator Type::EndEvents() const
    {
        return EventIterator();
    }

    Type::FieldIterator Type::BeginFields(BindingFlags const flags) const
    {
        VerifyInitialized();
        Detail::Verify([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        Detail::FieldTable const& table(_assembly.Realize().GetContext(InternalKey()).GetOrCreateFieldTable(_type));
        return FieldIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::FieldIterator Type::EndFields() const
    {
        return FieldIterator();
    }

    Type::MethodIterator Type::BeginMethods(BindingFlags const flags) const
    {
        VerifyInitialized();
        Detail::Verify([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        Detail::MethodTable const& table(_assembly.Realize().GetContext(InternalKey()).GetOrCreateMethodTable(_type));
        return MethodIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::MethodIterator Type::EndMethods() const
    {
        return MethodIterator();
    }

    Method Type::GetMethod(StringReference const name, BindingFlags const flags) const
    {
        auto const isNamedMethod([&](Method const& method)
        {
            return method.GetName() == name;
        });

        MethodIterator it(std::find_if(BeginMethods(flags), EndMethods(), isNamedMethod));

        if (it == EndMethods() || std::find_if(std::next(it), EndMethods(), isNamedMethod) != EndMethods())
            throw RuntimeError("Non-unique method");

        return *it;
    }

    Type::PropertyIterator Type::BeginProperties(BindingFlags const flags) const
    {
        VerifyInitialized();
        Detail::Verify([&]{ return !flags.IsSet(0x10000000); });

        Detail::PropertyTable const& table(_assembly.Realize().GetContext(InternalKey()).GetOrCreatePropertyTable(_type));
        return PropertyIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::PropertyIterator Type::EndProperties() const
    {
        return PropertyIterator();
    }

    CustomAttributeIterator Type::BeginCustomAttributes() const
    {
        // TODO In theory, a custom attribute can be applied to a TypeRef or TypeSpec too.
        return ResolveTypeDefTypeAndCall([&](Type const& t)
        {
            return CustomAttribute::BeginFor(t.GetAssembly(), t.GetTypeDefRow().GetSelfReference(), InternalKey());
        });
    }

    CustomAttributeIterator Type::EndCustomAttributes() const
    {
        // TODO In theory, a custom attribute can be applied to a TypeRef or TypeSpec too.
        return ResolveTypeDefTypeAndCall([&](Type const& t)
        {
            return CustomAttribute::EndFor(t.GetAssembly(), t.GetTypeDefRow().GetSelfReference(), InternalKey());
        });
    }

    Type::InterfacesRange Type::GetInterfacesRange() const
    {
        VerifyInitialized();

        Assembly const assembly(_assembly.Realize());
        Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());

        auto const first(database.Begin<Metadata::TableId::InterfaceImpl>());
        auto const last(database.End<Metadata::TableId::InterfaceImpl>());

        auto const range(std::equal_range(first, last, _type.AsRowReference(), Private::InterfaceStrictWeakOrdering()));
        return std::make_pair(range.first->GetSelfReference(), range.second->GetSelfReference());
    }

    Type::InterfaceIterator Type::BeginInterfaces() const
    {
        VerifyInitialized();

        Assembly const assembly(_assembly.Realize());
        Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());
        InterfacesRange const& range(GetInterfacesRange());

        return InterfaceIterator(assembly, Metadata::FullReference(&database, range.first));
    }

    Type::InterfaceIterator Type::EndInterfaces() const
    {
        VerifyInitialized();

        Assembly const assembly(_assembly.Realize());
        Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());
        InterfacesRange const& range(GetInterfacesRange());

        return InterfaceIterator(assembly, Metadata::FullReference(&database, range.second));
    }

    Type Type::GetBaseType() const
    {
        return ResolveTypeDefTypeAndCall([&](Type const& t) -> Type
        {
            Metadata::RowReference const extends(t.GetTypeDefRow().GetExtends());
            if (!extends.IsValid())
                return Type();

            switch (extends.GetTable())
            {
            case Metadata::TableId::TypeDef:
            case Metadata::TableId::TypeRef:
            case Metadata::TableId::TypeSpec:
                return Type(_assembly.Realize(), extends, InternalKey());

            default:
                throw std::logic_error("wtf");
            }
        });
    }

    Type Type::GetDeclaringType() const
    {
        if (IsNested())
        {
            Metadata::Database const& database(_assembly.Realize().GetContext(InternalKey()).GetDatabase());
            auto const it(std::lower_bound(
                database.Begin<Metadata::TableId::NestedClass>(),
                database.End<Metadata::TableId::NestedClass>(),
                _type,
                [](Metadata::NestedClassRow const& r, Metadata::ElementReference const& index)
            {
                return r.GetNestedClass() < index;
            }));

            if (it == database.End<Metadata::TableId::NestedClass>() || it->GetNestedClass() != _type)
                throw std::logic_error("wtf");

            // TODO IS THE TYPE DEF CHECK DONE AT THE PHYSICAL LAYER?
            Metadata::RowReference const enclosingType(it->GetEnclosingClass());
            if (enclosingType.GetTable() != Metadata::TableId::TypeDef)
                throw std::logic_error("wtf");

            return Type(_assembly.Realize(), enclosingType, InternalKey());
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
        
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetNamespace();
        });
    }

    bool Type::IsAbstract() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Abstract);
        });
    }

    bool Type::IsAnsiClass() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::StringFormatMask)
                == TypeAttribute::AnsiClass;
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
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::StringFormatMask)
                == TypeAttribute::AutoClass;
        });
    }

    bool Type::IsAutoLayout() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::LayoutMask) == TypeAttribute::AutoLayout;
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
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Utility::IsDerivedFromSystemType(t, L"System", L"__ComObject", true);
        });
    }

    bool Type::IsContextful() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Utility::IsDerivedFromSystemType(t, L"System", L"ContextBoundObject", true);
        });
    }

    bool Type::IsEnum() const
    {
        if (!IsTypeDef()) { return false; }

        return Utility::IsDerivedFromSystemType(*this, L"System", L"Enum", false);
    }

    bool Type::IsExplicitLayout() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::LayoutMask)
                == TypeAttribute::ExplicitLayout;
        });
    }

    bool Type::IsGenericParameter() const
    {
        return TodoNotYetImplementedFlag;
    }

    bool Type::IsGenericType() const
    {
        // TODO THIS IS WRONG
        if (IsNested() && GetDeclaringType().IsGenericType())
        {
            return true;
        }

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
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Import);
        });
    }

    bool Type::IsInterface() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::ClassSemanticsMask)
                == TypeAttribute::Interface;
        });
    }

    bool Type::IsLayoutSequential() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::LayoutMask)
                == TypeAttribute::SequentialLayout;
        });
    }

    bool Type::IsMarshalByRef() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Utility::IsDerivedFromSystemType(t, L"System", L"MarshalByRefObject", true);
        });
    }

    bool Type::IsNested() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                >  TypeAttribute::Public;
        });
    }

    bool Type::IsNestedAssembly() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedAssembly;
        });
    }

    bool Type::IsNestedFamilyAndAssembly() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamilyAndAssembly;
        });

    }
    bool Type::IsNestedFamily() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamily;
        });
    }

    bool Type::IsNestedFamilyOrAssembly() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamilyOrAssembly;
        });
    }

    bool Type::IsNestedPrivate() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedPrivate;
        });
    }

    bool Type::IsNestedPublic() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedPublic;
        });
    }

    bool Type::IsNotPublic() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NotPublic;
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

        if (!Utility::IsSystemAssembly(_assembly.Realize())) { return false; }

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
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::Public;
        });
    }

    bool Type::IsSealed() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Sealed);
        });
    }

    bool Type::IsSerializable() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Serializable)
                || t.IsEnum()
                || Utility::IsDerivedFromSystemType(t, L"System", L"MulticastDelegate", true);
        });
    }

    bool Type::IsSpecialName() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::SpecialName);
        });
    }

    bool Type::IsUnicodeClass() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::StringFormatMask)
                == TypeAttribute::UnicodeClass;
        });
    }

    bool Type::IsValueType() const
    {
        return Utility::IsDerivedFromSystemType(*this, L"System", L"ValueType", false)
            && !Utility::IsSystemType(*this, L"System", L"Enum");
    }

    bool Type::IsVisible() const
    {
        if (IsTypeDef())
        {
            if (IsNested() && !GetDeclaringType().IsVisible())
                return false;

            switch (GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask).GetEnum())
            {
            case TypeAttribute::Public:
            case TypeAttribute::NestedPublic:
                return true;

            default:
                return false;
            }
        }
        // TODO CHECK BEHAVIOR FOR TYPESPECS

        return TodoNotYetImplementedFlag;
    }

    bool operator==(Type const& lhs, Type const& rhs)
    {
        return lhs.GetAssembly() == rhs.GetAssembly()
            && lhs.GetMetadataToken() == rhs.GetMetadataToken();
    }

    // We provide a total ordering of Types across all loaded assemblies.  Types within a given
    // assembly are ordered by metadata token; types in different assemblies have an unspecified
    // total ordering.
    bool operator<(Type const& lhs, Type const& rhs)
    {
        if (lhs.GetAssembly() < rhs.GetAssembly())
            return true;

        return lhs.GetAssembly() == rhs.GetAssembly()
            && lhs.GetMetadataToken() == rhs.GetMetadataToken();
    }

    bool Type::FilterEvent(BindingFlags const /*filter*/, Type const& /*reflectedType*/, Detail::EventContext const& /*current*/)
    {
        // Metadata::RowReference const currentType(current.GetDeclaringType().AsRowReference());
        // bool const currentTypeIsDeclaringType(reflectedType.GetMetadataToken() == currentType.GetToken());

        // TODO To filter events, we need to compute the most accessible related method.

        return false;
    }

    bool Type::FilterField(BindingFlags const filter, Type const& reflectedType, Detail::FieldContext const& current)
    {
        Metadata::RowReference const currentType(current.GetDeclaringType().AsRowReference());
        bool const currentTypeIsDeclaringType(reflectedType.GetMetadataToken() == currentType.GetToken());

        if (Private::CoreFilterMember(filter, currentTypeIsDeclaringType, current))
            return true;

        return false;
    }

    bool Type::FilterMethod(BindingFlags const filter, Type const& reflectedType, Detail::MethodContext const& current)
    {
        Metadata::RowReference const currentType(current.GetDeclaringType().AsRowReference());
        bool const currentTypeIsDeclaringType(reflectedType.GetMetadataToken() == currentType.GetToken());

        if (Private::CoreFilterMember(filter, currentTypeIsDeclaringType, current))
            return true;

        StringReference const name(current.GetMemberRow().GetName());
        bool const isConstructor(
            current.GetMemberRow().GetFlags().IsSet(MethodAttribute::SpecialName) && 
            (name == L".ctor" || name == L".cctor"));

        return isConstructor != filter.IsSet(BindingAttribute::InternalUseOnlyConstructor);
    }

    bool Type::FilterProperty(BindingFlags const /*filter*/, Type const& /*reflectedType*/, Detail::PropertyContext const& /*current*/)
    {
        // Metadata::RowReference const currentType(current.GetDeclaringType().AsRowReference());
        // bool const currentTypeIsDeclaringType(reflectedType.GetMetadataToken() == currentType.GetToken());

        // TODO To filter properties, we need to compute the most accessible related method.

        return false;
    }


}
