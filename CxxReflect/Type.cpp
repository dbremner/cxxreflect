//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace { namespace Private {

    template <typename T>
    bool CoreFilterMember(BindingFlags const filter, bool const isDeclaringType, T const& current)
    {
        typedef typename Detail::Identity<decltype(current.GetElementRow().GetFlags())>::Type::EnumerationType AttributeType;
        auto currentFlags(current.GetElementRow().GetFlags());

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

            StringReference const memberName(current.GetElementRow().GetName());

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
            Detail::Assert([&]{ return lhs.GetClass().GetTable() == Metadata::TableId::TypeDef; });
            Detail::Assert([&]{ return rhs.GetClass().GetTable() == Metadata::TableId::TypeDef; });

            return lhs.GetClass().GetIndex() < rhs.GetClass().GetIndex();
        }

        bool operator()(InterfaceImplRow const& lhs, RowReference const& rhs) const volatile
        {
            Detail::Assert([&]{ return lhs.GetClass().GetTable() == Metadata::TableId::TypeDef; });
            Detail::Assert([&]{ return rhs.GetTable()            == Metadata::TableId::TypeDef; });

            return lhs.GetClass().GetIndex() < rhs.GetIndex();
        }

        bool operator()(RowReference const& lhs, InterfaceImplRow const& rhs) const volatile
        {
            Detail::Assert([&]{ return lhs.GetTable()            == Metadata::TableId::TypeDef; });
            Detail::Assert([&]{ return rhs.GetClass().GetTable() == Metadata::TableId::TypeDef; });

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
        Detail::Assert([&] { return assembly.IsInitialized(); });

        // If we were initialized with an empty type, do not attempt to do any type resolution.
        if (!type.IsInitialized())
            return;

        switch (type.GetTable())
        {
        case Metadata::TableId::TypeDef:
        {
            // Good news, everyone!  We have a TypeDef and we don't need to do any further work.
            break;
        }

        case Metadata::TableId::TypeRef:
        {
            // Resolve the TypeRef into a TypeDef, throwing on failure:
            Loader             const& loader(assembly.GetContext(InternalKey()).GetLoader());
            Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());

            Metadata::FullReference const resolvedType(
                loader.ResolveType(Metadata::FullReference(&database, type)));

            Detail::Assert([&]{ return resolvedType.IsInitialized(); });

            _assembly = Assembly(
                &loader.GetContextForDatabase(resolvedType.GetDatabase(), InternalKey()),
                InternalKey());

            _type = Metadata::ElementReference(resolvedType.AsRowReference());
            Detail::Assert([&]{ return _type.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });

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
            Detail::AssertFail(L"Unexpected argument");
            break;
        }
        }
    }

    Type::Type(Assembly const& assembly, Metadata::BlobReference const& type, InternalKey)
        : _assembly(assembly), _type(Metadata::ElementReference(type))
    {
        Detail::Assert([&]{ return assembly.IsInitialized(); });
        Detail::Assert([&]{ return type.IsInitialized();     });

        Metadata::TypeSignature const signature(assembly
            .GetContext(InternalKey())
            .GetDatabase()
            .GetBlob(type)
            .As<Metadata::TypeSignature>());

        if (signature.GetKind() == Metadata::TypeSignature::Kind::Primitive)
        {
            Type const primitiveType(assembly
                .GetContext(InternalKey())
                .GetLoader()
                .GetFundamentalType(signature.GetPrimitiveElementType(), InternalKey()));
            Detail::Assert([&]{ return primitiveType.IsInitialized(); });

            _assembly = primitiveType.GetAssembly();
            _type = Metadata::RowReference::FromToken(primitiveType.GetMetadataToken());
        }
    }

    Type::Type(Type const& reflectedType, Detail::InterfaceContext const* const context, InternalKey)
        : _assembly(&reflectedType
            .GetAssembly()
            .GetContext(InternalKey())
            .GetLoader()
            .GetContextForDatabase(context->GetElement().GetDatabase(), InternalKey()))
    {
        Loader const& loader(reflectedType.GetAssembly().GetContext(InternalKey()).GetLoader());

        if (context->GetElementSignature(loader).IsInitialized())
        {
            Metadata::TypeSignature const typeSignature(context->GetElementSignature(loader));
            _type = Metadata::BlobReference(typeSignature.BeginBytes(), typeSignature.EndBytes());
        }
        else
        {
            Assembly assembly(_assembly.Realize());
            Metadata::RowReference const type(context->GetElementRow().GetInterface());
            _type = type;
            switch (type.GetTable())
            {
            case Metadata::TableId::TypeDef:
            {
                // Good news, everyone!  We have a TypeDef and we don't need to do any further work.
                break;
            }

            case Metadata::TableId::TypeRef:
            {
                // Resolve the TypeRef into a TypeDef, throwing on failure:
                Loader             const& loader(assembly.GetContext(InternalKey()).GetLoader());
                Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());

                Metadata::FullReference const resolvedType(
                    loader.ResolveType(Metadata::FullReference(&database, type)));

                Detail::Assert([&]{ return resolvedType.IsInitialized(); });

                _assembly = Assembly(
                    &loader.GetContextForDatabase(resolvedType.GetDatabase(), InternalKey()),
                    InternalKey());

                _type = Metadata::ElementReference(resolvedType.AsRowReference());
                Detail::Assert([&]{ return _type.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });

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
                Detail::AssertFail(L"Unexpected argument");
                break;
            }
            }
        }

        AssertInitialized();


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
                bool isFirst(true);
                std::for_each(
                    signature.BeginGenericArguments(),
                    signature.EndGenericArguments(),
                    [&](Metadata::TypeSignature const& argumentSignature)
                {
                    if (!isFirst)
                    {
                       os << L",";
                    }

                    isFirst = false;

                    os << L'[';
                    Type const argumentType(
                        _assembly.Realize(),
                        Metadata::BlobReference(argumentSignature),
                        InternalKey());
                    argumentType.AccumulateAssemblyQualifiedNameInto(os);
                    os << L']';
                });

                os << L']';

                if (signature.IsByRef())
                    os << L"&";

                break;
            }
            case Metadata::TypeSignature::Kind::ClassType:
            {
                Type const classType(_assembly.Realize(), signature.GetTypeReference(), InternalKey());
                classType.AccumulateFullNameInto(os);

                if ((int)os.tellp() != 0 && signature.IsByRef())
                    os << L"&";

                break;
            }
            case Metadata::TypeSignature::Kind::SzArray:
            {
                Type const classType(
                    _assembly.Realize(),
                    Metadata::BlobReference(signature.GetArrayType()),
                    InternalKey());

                classType.AccumulateFullNameInto(os);
                if ((int)os.tellp() != 0)
                {
                    os << L"[]";

                    if (signature.IsByRef())
                        os << L"&";
                }

                break;
            }
            case Metadata::TypeSignature::Kind::Ptr:
            {
                Type const pointerType(
                    _assembly.Realize(),
                    Metadata::BlobReference(signature.GetPointerTypeSignature()),
                    InternalKey());

                pointerType.AccumulateFullNameInto(os);
                os << L"*";

                if (signature.IsByRef())
                    os << L"&";

                break;
            }
            case Metadata::TypeSignature::Kind::Var:
            {
                // TODO MVAR?
                //os << L"Var!" << signature.GetVariableNumber();
                break;
            }
            default:
            {
                // Detail::AssertFail(L"NYI");
                os << L"FAIL NYI";
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
        Detail::Assert([&]{ return IsTypeDef(); });

        return _assembly
            .Realize()
            .GetContext(InternalKey())
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeDef>(_type.AsRowReference().GetIndex());
    }

    Metadata::TypeSignature Type::GetTypeSpecSignature() const
    {
        Detail::Assert([&]{ return IsTypeSpec(); });

        return _assembly
            .Realize()
            .GetContext(InternalKey())
            .GetDatabase()
            .GetBlob(_type.AsBlobReference())
            .As<Metadata::TypeSignature>();
    }

    Type::MethodIterator Type::BeginConstructors(BindingFlags flags) const
    {
        AssertInitialized();
        Detail::Assert([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        flags.Set(BindingAttribute::InternalUseOnlyConstructor);
        flags.Set(BindingAttribute::DeclaredOnly);
        flags.Unset(BindingAttribute::FlattenHierarchy);

        // TODO:  For performance, it would be beneficial to maintain two tables per type--one for
        // the full member table like we compute now, and one for declared members only.  This would
        // allow for much faster constructor resolution (and this is one of our core use cases).  We
        // still need to build a table, though, because we need to instantiate generic members.
        Detail::MethodContextTable const& table(_assembly
            .Realize()
            .GetContext(InternalKey())
            .GetLoader()
            .GetOrCreateMethodTable(Metadata::FullReference(&_assembly.Realize().GetContext(InternalKey()).GetDatabase(), _type)));

        return MethodIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::MethodIterator Type::EndConstructors() const
    {
        return MethodIterator();
    }

    Type::EventIterator Type::BeginEvents(BindingFlags const flags) const
    {
        AssertInitialized();
        Detail::Assert([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        Detail::EventContextTable const& table(_assembly
            .Realize()
            .GetContext(InternalKey())
            .GetLoader()
            .GetOrCreateEventTable(Metadata::FullReference(&_assembly.Realize().GetContext(InternalKey()).GetDatabase(), _type)));

        return EventIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::EventIterator Type::EndEvents() const
    {
        return EventIterator();
    }

    Type::FieldIterator Type::BeginFields(BindingFlags const flags) const
    {
        AssertInitialized();
        Detail::Assert([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        Detail::FieldContextTable const& table(_assembly
            .Realize()
            .GetContext(InternalKey())
            .GetLoader()
            .GetOrCreateFieldTable(Metadata::FullReference(&_assembly.Realize().GetContext(InternalKey()).GetDatabase(), _type)));

        return FieldIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::FieldIterator Type::EndFields() const
    {
        return FieldIterator();
    }

    Type::MethodIterator Type::BeginMethods(BindingFlags const flags) const
    {
        AssertInitialized();
        Detail::Assert([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        Detail::MethodContextTable const& table(_assembly
            .Realize()
            .GetContext(InternalKey())
            .GetLoader()
            .GetOrCreateMethodTable(Metadata::FullReference(&_assembly.Realize().GetContext(InternalKey()).GetDatabase(), _type)));

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
            throw RuntimeError(L"Non-unique method requested");

        return *it;
    }

    Type::PropertyIterator Type::BeginProperties(BindingFlags const flags) const
    {
        AssertInitialized();
        Detail::Assert([&]{ return !flags.IsSet(0x10000000); });

        Detail::PropertyContextTable const& table(_assembly
            .Realize()
            .GetContext(InternalKey())
            .GetLoader()
            .GetOrCreatePropertyTable(Metadata::FullReference(&_assembly.Realize().GetContext(InternalKey()).GetDatabase(), _type)));

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

    Type::InterfaceIterator Type::BeginInterfaces() const
    {
        AssertInitialized();

        Detail::InterfaceContextTable const& table(_assembly
            .Realize()
            .GetContext(InternalKey())
            .GetLoader()
            .GetOrCreateInterfaceTable(Metadata::FullReference(&_assembly.Realize().GetContext(InternalKey()).GetDatabase(), _type)));

        return InterfaceIterator(*this, table.Begin(), table.End(), BindingFlags());
    }

    Type::InterfaceIterator Type::EndInterfaces() const
    {
        return InterfaceIterator();
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
                Metadata::RowReference::FromToken(GetMetadataToken()),
                [](Metadata::NestedClassRow const& r, Metadata::ElementReference const& index)
            {
                return r.GetNestedClass() < index;
            }));

            if (it == database.End<Metadata::TableId::NestedClass>())
                throw MetadataReadError(L"Type was identified as nested but had no row in the NestedClass table.");

            // An assertion is sufficient here; if the assertion fails, BinarySearch is broken.
            Detail::Assert([&]() -> bool
            {
                Metadata::RowReference const nestedClass(it->GetNestedClass());
                Metadata::RowReference const typeToken(Metadata::RowReference::FromToken(GetMetadataToken()));

                return nestedClass == typeToken;
            },  L"Binary search returned an unexpected result.");

            Metadata::RowReference const enclosingType(it->GetEnclosingClass());
            if (enclosingType.GetTable() != Metadata::TableId::TypeDef)
                throw MetadataReadError(L"Enclosing type was expected to be a TypeDef; it was not.");

            return Type(_assembly.Realize(), enclosingType, InternalKey());
        }

        // TODO Handle other kinds of declaring types
        return Type();
    }

    String Type::GetAssemblyQualifiedName() const
    {
        // TODO Remove use of <iostream>
        std::wostringstream oss;
        AccumulateAssemblyQualifiedNameInto(oss);
        return oss.str();
    }

    String Type::GetFullName() const
    {
        // TODO Remove use of <iostream>
        std::wostringstream oss;
        AccumulateFullNameInto(oss);
        return oss.str();
    }

    SizeType Type::GetMetadataToken() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
           return t._type.IsRowReference() ? t._type.AsRowReference().GetToken() : 0; 
        });
    }

    StringReference Type::GetName() const
    {
        AssertInitialized();

        if (IsTypeDef())
            return GetTypeDefRow().GetName();

        return L""; // TODO Implement GetName() for TypeSpecs
    }

    StringReference Type::GetNamespace() const
    {
        // A nested type has an empty namespace string in the database; we instead return the 
        // namespace of its declaring type, for consistency.
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
        AssertInitialized();
        if (IsTypeDef())
            return false;

        Metadata::TypeSignature const signature(GetTypeSpecSignature());
        return signature.IsSimpleArray() || signature.IsGeneralArray();
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
        AssertInitialized();

        if (IsTypeDef())
            return false;

        return GetTypeSpecSignature().IsByRef();
    }

    bool Type::IsClass() const
    {
        AssertInitialized();

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
        if (!IsTypeDef())
            return false;

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
        if (IsTypeDef())
            return false;

        Metadata::TypeSignature const signature(GetTypeSpecSignature());
        return signature.IsClassVariableType() || signature.IsMethodVariableType();
    }

    bool Type::IsGenericType() const
    {
        // TODO This is incorrect, but is a close approximation that works much of the time.
        if (IsNested() && GetDeclaringType().IsGenericType())
        {
            return true;
        }

        StringReference const name(GetName());
        return std::find(name.begin(), name.end(), L'`') != name.end();
    }

    bool Type::IsGenericTypeDefinition() const
    {
        // TODO This is incorrect, but is a close approximation that works much of the time.
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
        if (IsTypeDef())
            return false;

        return GetTypeSpecSignature().IsPointer();
    }

    bool Type::IsPrimitive() const
    {
        if (!IsTypeDef())
            return false;

        if (!Utility::IsSystemAssembly(_assembly.Realize()))
            return false;

        if (GetTypeDefRow().GetNamespace() != L"System")
            return false;

        StringReference const& name(GetTypeDefRow().GetName());
        if (name.size() < 4)
            return false;

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
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Utility::IsDerivedFromSystemType(t, L"System", L"ValueType", false)
                && !Utility::IsSystemType(t, L"System", L"Enum");
        });
    }

    bool Type::IsVisible() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            if (t.IsNested() && !t.GetDeclaringType().IsVisible())
                return false;

            switch (t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask).GetEnum())
            {
            case TypeAttribute::Public:
            case TypeAttribute::NestedPublic:
                return true;

            default:
                return false;
            }
        });
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
        Metadata::RowReference const currentType(current.GetOwningType().AsRowReference());
        bool const currentTypeIsDeclaringType(reflectedType.GetMetadataToken() == currentType.GetToken());

        if (Private::CoreFilterMember(filter, currentTypeIsDeclaringType, current))
            return true;

        return false;
    }

    bool Type::FilterInterface(BindingFlags, Type const&, Detail::InterfaceContext const&)
    {
        return false;
    }

    bool Type::FilterMethod(BindingFlags const filter, Type const& reflectedType, Detail::MethodContext const& current)
    {
        Metadata::RowReference const currentType(current.GetOwningType().AsRowReference());
        bool const currentTypeIsDeclaringType(reflectedType.GetMetadataToken() == currentType.GetToken());

        if (Private::CoreFilterMember(filter, currentTypeIsDeclaringType, current))
            return true;

        StringReference const name(current.GetElementRow().GetName());
        bool const isConstructor(
            current.GetElementRow().GetFlags().IsSet(MethodAttribute::SpecialName) && 
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
