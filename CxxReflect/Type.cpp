
//                            Copyright James P. McNellis 2011 - 2012.                            //
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

    Metadata::FullReference Resolve(Assembly               const& assembly,
                                    Metadata::RowReference const& type,
                                    InternalKey            const  key)
    {
        switch (type.GetTable())
        {
        case Metadata::TableId::TypeDef:
        {
            // Good news, everyone!  We have a TypeDef and we don't need to do any further work.
            return Metadata::FullReference(&assembly.GetContext(key).GetDatabase(), type);
        }

        case Metadata::TableId::TypeRef:
        {
            // Resolve the TypeRef into a TypeDef, throwing on failure:
            Loader             const& loader(assembly.GetContext(key).GetLoader());
            Metadata::Database const& database(assembly.GetContext(key).GetDatabase());

            Metadata::FullReference const resolvedType(
                loader.ResolveType(Metadata::FullReference(&database, type)));

            Detail::Assert([&]{ return resolvedType.AsRowReference().GetTable() == Metadata::TableId::TypeDef; });

            return resolvedType;
        }

        case Metadata::TableId::TypeSpec:
        {
            // Get the signature for the TypeSpec token and use that instead:
            Metadata::Database const& database(assembly.GetContext(key).GetDatabase());
            Metadata::TypeSpecRow const typeSpec(database.GetRow<Metadata::TableId::TypeSpec>(type.GetIndex()));
            return Metadata::FullReference(&database, typeSpec.GetSignature());
        }

        default:
        {
            Detail::AssertFail(L"Unreachable code");
            return Metadata::FullReference();
        }
        }
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

    template <typename TTable>
    TTable GetOrCreateTable(TTable (CxxReflect::Loader::*getOrCreate)(Metadata::FullReference const&, InternalKey) const,
                            Detail::AssemblyHandle     const& assembly,
                            Metadata::ElementReference const& type,
                            InternalKey key)
    {
        Metadata::FullReference const typeReference(&assembly.Realize().GetContext(key).GetDatabase(), type);

        TTable const& table((assembly
            .Realize()
            .GetContext(key)
            .GetLoader()
            .*getOrCreate)(typeReference, key));

        return table; 
    }

} } }

namespace CxxReflect { namespace Detail {

    String TypeNameBuilder::BuildTypeName(Type const& type, Mode const mode)
    {
        return TypeNameBuilder(type, mode);
    }

    TypeNameBuilder::TypeNameBuilder(Type const& type, Mode const mode)
    {
        _buffer.reserve(1024);

        if (!AccumulateTypeName(type, mode))
            _buffer.resize(0);
    }

    TypeNameBuilder::operator String()
    {
        using std::swap;

        String result;
        swap(result, _buffer);
        return result;
    }

    bool TypeNameBuilder::AccumulateTypeName(Type const& type, Mode const mode)
    {
        Assert([&]{ return type.IsInitialized(); });

        return type.IsTypeDef()
            ? AccumulateTypeDefName(type, mode)
            : AccumulateTypeSpecName(type, mode);
    }

    bool TypeNameBuilder::AccumulateTypeDefName(Type const& type, Mode const mode)
    {
        Assert([&]{ return type.IsTypeDef(); });

        if (mode == Mode::SimpleName)
        {
            _buffer += type.GetTypeDefRow().GetName().c_str();
            return true;
        }

        // Otherwise, we have either a SimpleName or an AssemblyQualifiedName:
        if (type.IsNested())
        {
            AccumulateTypeDefName(type.GetDeclaringType(), Mode::FullName);
            _buffer.push_back(L'+');
        }
        else if (type.GetNamespace().size() > 0)
        {
            _buffer += type.GetNamespace().c_str();
            _buffer.push_back(L'.');
        }

        _buffer += type.GetTypeDefRow().GetName().c_str();

        AccumulateAssemblyQualificationIfRequired(type, mode);
        return true;
    }

    bool TypeNameBuilder::AccumulateTypeSpecName(Type const& type, Mode const mode)
    {
        Assert([&]{ return type.IsTypeSpec(); });

        Metadata::TypeSignature const signature(type.GetTypeSpecSignature());

        typedef Metadata::ClassVariableSignatureInstantiator Instantiator;

        // A TypeSpec for an uninstantiated generic type has no name:
        if (mode != Mode::SimpleName && Instantiator::RequiresInstantiation(signature))
            return false;

        switch (signature.GetKind())
        {
        case Metadata::TypeSignature::Kind::Array:       return AccumulateArrayTypeSpecName      (type, mode);
        case Metadata::TypeSignature::Kind::ClassType:   return AccumulateClassTypeSpecName      (type, mode);
        case Metadata::TypeSignature::Kind::FnPtr:       return AccumulateFnPtrTypeSpecName      (type, mode);
        case Metadata::TypeSignature::Kind::GenericInst: return AccumulateGenericInstTypeSpecName(type, mode);
        case Metadata::TypeSignature::Kind::Primitive:   return AccumulatePrimitiveTypeSpecName  (type, mode);
        case Metadata::TypeSignature::Kind::Ptr:         return AccumulatePtrTypeSpecName        (type, mode);
        case Metadata::TypeSignature::Kind::SzArray:     return AccumulateSzArrayTypeSpecName    (type, mode);
        case Metadata::TypeSignature::Kind::Var:         return AccumulateVarTypeSpecName        (type, mode);
        default: AssertFail(L"Unreachable Code");        return false;
        }
    }

    bool TypeNameBuilder::AccumulateArrayTypeSpecName(Type const& type, Mode)
    {
        Assert([&]{ return type.GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::Array); });

        // TODO We need to figure out how to write the general array form:
        AssertFail(L"General array not yet implemented");
        return false;
    }

    bool TypeNameBuilder::AccumulateClassTypeSpecName(Type const& type, Mode const mode)
    {
        Assert([&]{ return type.GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::ClassType); });

        Metadata::Database const* const database(type.GetTypeSpecSignature().GetTypeReferenceScope());

        Assembly const assembly(database != nullptr
            ? Assembly(&type
                .GetAssembly()
                .GetContext(InternalKey())
                .GetLoader()
                .GetContextForDatabase(*database, InternalKey()), InternalKey())
            : type.GetAssembly());

        Type const classType(
            assembly,
            type.GetTypeSpecSignature().GetTypeReference(),
            InternalKey());

        if (!AccumulateTypeName(classType, WithoutAssemblyQualification(mode)))
            return false;

        if (type.GetTypeSpecSignature().IsByRef())
            _buffer.push_back(L'&');

        AccumulateAssemblyQualificationIfRequired(classType, mode);
        return true;
    }

    bool TypeNameBuilder::AccumulateFnPtrTypeSpecName(Type const& type, Mode)
    {
        Assert([&]{ return type.GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::FnPtr); });

        // TODO We need to figure out how to write the function pointer form:
        AssertFail(L"Function pointer not yet implemented");
        return false;
    }

    bool TypeNameBuilder::AccumulateGenericInstTypeSpecName(Type const& type, Mode const mode)
    {
        Assert([&]{ return type.GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::GenericInst); });

        Type const genericType(
            type.GetAssembly(),
            type.GetTypeSpecSignature().GetGenericTypeReference(),
            InternalKey());

        if (!AccumulateTypeName(genericType, WithoutAssemblyQualification(mode)))
            return false;

        Metadata::TypeSignature const signature(type.GetTypeSpecSignature());

        if (mode == Mode::SimpleName)
        {
            if (signature.IsByRef())
                _buffer.push_back(L'&');
            return true;
        }

        _buffer.push_back(L'[');

        auto const first(signature.BeginGenericArguments());
        auto const last(signature.EndGenericArguments());
        bool isFirst(true);
        bool const success(std::all_of(first, last, [&](Metadata::TypeSignature const& argumentSignature) -> bool
        {
            if (!isFirst)
                _buffer.push_back(L',');

            isFirst = false;

            _buffer.push_back(L'[');

            Type const argumentType(type.GetAssembly(), Metadata::BlobReference(argumentSignature), InternalKey());
            if (!AccumulateTypeName(argumentType, Mode::AssemblyQualifiedName))
                return false;

            _buffer.push_back(L']');
            return true;
        }));

        if (!success)
            return false;

        _buffer.push_back(L']');

        if (signature.IsByRef())
            _buffer.push_back(L'&');

        AccumulateAssemblyQualificationIfRequired(genericType, mode);
        return true;
    }

    bool TypeNameBuilder::AccumulatePrimitiveTypeSpecName(Type const& type, Mode const mode)
    {
        Assert([&]{ return type.GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::Primitive); });

        Type const primitiveType(type
            .GetAssembly()
            .GetContext(InternalKey())
            .GetLoader()
            .GetFundamentalType(type.GetTypeSpecSignature().GetPrimitiveElementType(), InternalKey()));

        if (!AccumulateTypeName(primitiveType, WithoutAssemblyQualification(mode)))
            return false;

        if (type.GetTypeSpecSignature().IsByRef())
            _buffer.push_back(L'&');

        AccumulateAssemblyQualificationIfRequired(primitiveType, mode);
        return true;
    }

    bool TypeNameBuilder::AccumulatePtrTypeSpecName(Type const& type, Mode const mode)
    {
        Assert([&]{ return type.GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::Ptr); });

        Type const pointerType(
            type.GetAssembly(),
            Metadata::BlobReference(type.GetTypeSpecSignature().GetPointerTypeSignature()),
            InternalKey());

        if (!AccumulateTypeName(pointerType, WithoutAssemblyQualification(mode)))
            return false;

        _buffer.push_back(L'*');

        if (type.GetTypeSpecSignature().IsByRef())
            _buffer.push_back(L'&');

        AccumulateAssemblyQualificationIfRequired(pointerType, mode);
        return true;
    }

    bool TypeNameBuilder::AccumulateSzArrayTypeSpecName(Type const& type, Mode const mode)
    {
        Assert([&]{ return type.GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::SzArray); });

        Type const arrayType(
            type.GetAssembly(),
            Metadata::BlobReference(type.GetTypeSpecSignature().GetArrayType()),
            InternalKey());

        if (!AccumulateTypeName(arrayType, WithoutAssemblyQualification(mode)))
            return false;

        _buffer.push_back(L'[');
        _buffer.push_back(L']');

        if (type.GetTypeSpecSignature().IsByRef())
            _buffer.push_back(L'&');

        AccumulateAssemblyQualificationIfRequired(arrayType, mode);
        return true;
    }

    bool TypeNameBuilder::AccumulateVarTypeSpecName(Type const& type, Mode)
    {
        Assert([&]{ return type.GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::Var); });

        // TODO Do we need to support class and method variables?  If so, how do we decide when to
        // write them to the type name (i.e., sometimes they definitely do not belong).
        return false;
    }

    void TypeNameBuilder::AccumulateAssemblyQualificationIfRequired(Type const& type, Mode const mode)
    {
        if (mode != Mode::AssemblyQualifiedName)
            return;

        _buffer.push_back(L',');
        _buffer.push_back(L' ');
        _buffer += type.GetAssembly().GetName().GetFullName().c_str();
    }

    TypeNameBuilder::Mode TypeNameBuilder::WithoutAssemblyQualification(Mode const mode)
    {
        return mode == Mode::SimpleName ? Mode::SimpleName : Mode::FullName;
    }

} }

namespace CxxReflect {

    bool const TodoNotYetImplementedFlag = false;

    Type::Type()
    {
    }

    Type::Type(Assembly const& assembly, Metadata::RowReference const& type, InternalKey)
    {
        Detail::Assert([&] { return assembly.IsInitialized(); });

        // If we were initialized with an empty type, do not attempt to do any type resolution.
        if (!type.IsInitialized())
            return;

        Metadata::FullReference const resolvedType(Private::Resolve(assembly, type, InternalKey()));
        _assembly = Assembly(&assembly
            .GetContext(InternalKey())
            .GetLoader()
            .GetContextForDatabase(resolvedType.GetDatabase(), InternalKey()), InternalKey());

        _type = resolvedType.IsRowReference()
            ? Metadata::ElementReference(resolvedType.AsRowReference())
            : Metadata::ElementReference(resolvedType.AsBlobReference());
    }

    Type::Type(Assembly const& assembly, Metadata::BlobReference const& type, InternalKey)
        : _assembly(assembly), _type(Metadata::ElementReference(type))
    {
        Detail::Assert([&]{ return assembly.IsInitialized(); });
        Detail::Assert([&]{ return type.IsInitialized();     });

        Metadata::TypeSignature const signature(type.As<Metadata::TypeSignature>());

        if (!signature.IsByRef() && signature.GetKind() == Metadata::TypeSignature::Kind::Primitive)
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
    {
        Loader const& loader(reflectedType.GetAssembly().GetContext(InternalKey()).GetLoader());

        _assembly = Assembly(&reflectedType
                .GetAssembly()
                .GetContext(InternalKey())
                .GetLoader()
                .GetContextForDatabase(context->GetElement().GetDatabase(), InternalKey()), InternalKey());

        if (context->GetElementSignature(loader).IsInitialized())
        {
            Metadata::TypeSignature const typeSignature(context->GetElementSignature(loader));
            _type = Metadata::BlobReference(typeSignature.BeginBytes(), typeSignature.EndBytes());
        }
        else
        {
            Metadata::FullReference const resolvedType(Private::Resolve(
                _assembly.Realize(),
                context->GetElementRow().GetInterface(),
                InternalKey()));

            _assembly = Assembly(&_assembly
                .Realize()
                .GetContext(InternalKey())
                .GetLoader()
                .GetContextForDatabase(resolvedType.GetDatabase(), InternalKey()), InternalKey());
            _type = resolvedType.AsRowReference();
        }

        AssertInitialized();
    }

    bool Type::IsInitialized() const
    {
        return _assembly.IsInitialized() && _type.IsInitialized();
    }

    bool Type::operator!() const
    {
        return !IsInitialized();
    }

    void Type::AssertInitialized() const
    {
        Detail::Assert([&] { return IsInitialized(); }, L"Type is not initialized");
    }

    Metadata::ElementReference Type::GetSelfReference(InternalKey) const
    {
        return _type;
    }

    bool Type::IsTypeDef() const
    {
        AssertInitialized();
        
        return _type.IsRowReference();
    }

    bool Type::IsTypeSpec() const
    {
        AssertInitialized();
        
        return _type.IsBlobReference();
    }

    Assembly Type::GetAssembly() const
    {
        return _assembly.Realize();
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

        return _type.AsBlobReference().As<Metadata::TypeSignature>();
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
            .GetOrCreateMethodTable(
                Metadata::FullReference(&_assembly.Realize().GetContext(InternalKey()).GetDatabase(), _type),
                InternalKey()));

        return MethodIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::MethodIterator Type::EndConstructors() const
    {
        return MethodIterator();
    }

    // TODO Type::EventIterator Type::BeginEvents(BindingFlags const flags) const
    // {
    //     AssertInitialized();
    //     Detail::Assert([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });
    // 
    //     Detail::EventContextTable const& table(_assembly
    //         .Realize()
    //         .GetContext(InternalKey())
    //         .GetLoader()
    //         .GetOrCreateEventTable(
    //             Metadata::FullReference(&_assembly.Realize().GetContext(InternalKey()).GetDatabase(), _type),
    //             InternalKey()));
    // 
    //     return EventIterator(*this, table.Begin(), table.End(), flags);
    // }

    // TODO Type::EventIterator Type::EndEvents() const
    // {
    //     return EventIterator();
    // }

    Type::FieldIterator Type::BeginFields(BindingFlags const flags) const
    {
        AssertInitialized();
        Detail::Assert([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        auto const& table(Private::GetOrCreateTable(&Loader::GetOrCreateFieldTable, _assembly, _type, InternalKey()));

        return FieldIterator(*this, table.Begin(), table.End(), flags);
    }

    Type::FieldIterator Type::EndFields() const
    {
        return FieldIterator();
    }

    Type::MethodIterator Type::BeginMethods(BindingFlags const flags) const
    {
        AssertInitialized();

        if (IsTypeSpec())
        {
            if (IsByRef())
                return MethodIterator();
        }
        
        Detail::Assert([&]{ return !flags.IsSet(BindingAttribute::InternalUseOnlyMask); });

        auto const& table(Private::GetOrCreateTable(&Loader::GetOrCreateMethodTable, _assembly, _type, InternalKey()));

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

    // TODO Type::PropertyIterator Type::BeginProperties(BindingFlags const flags) const
    // {
    //     AssertInitialized();
    //     Detail::Assert([&]{ return !flags.IsSet(0x10000000); });
    // 
    //     auto const& table(Private::GetOrCreateTable(&Loader::GetOrCreatePropertyTable, _assembly, _type, InternalKey()));
    // 
    //     return PropertyIterator(*this, table.Begin(), table.End(), flags);
    // }

    // TODO Type::PropertyIterator Type::EndProperties() const
    // {
    //     return PropertyIterator();
    // }

    CustomAttributeIterator Type::BeginCustomAttributes() const
    {
        // TODO In theory, a custom attribute can be applied to a TypeRef or TypeSpec too.
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return CustomAttributeIterator();

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Ptr:
                return CustomAttributeIterator();
            }
        }

        return ResolveTypeDefTypeAndCall([&](Type const& t)
        {
            return CustomAttribute::BeginFor(t.GetAssembly(), t.GetTypeDefRow().GetSelfReference(), InternalKey());
        });
    }

    CustomAttributeIterator Type::EndCustomAttributes() const
    {
        // TODO In theory, a custom attribute can be applied to a TypeRef or TypeSpec too.
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return CustomAttributeIterator();

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Ptr:
                return CustomAttributeIterator();
            }
        }

        return ResolveTypeDefTypeAndCall([&](Type const& t)
        {
            return CustomAttribute::EndFor(t.GetAssembly(), t.GetTypeDefRow().GetSelfReference(), InternalKey());
        });
    }

    Type::InterfaceIterator Type::BeginInterfaces() const
    {
        AssertInitialized();

        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return InterfaceIterator();
        }

        auto const& table(Private::GetOrCreateTable(&Loader::GetOrCreateInterfaceTable, _assembly, _type, InternalKey()));

        return InterfaceIterator(*this, table.Begin(), table.End(), BindingFlags());
    }

    Type::InterfaceIterator Type::EndInterfaces() const
    {
        return InterfaceIterator();
    }

    Type Type::GetBaseType() const
    {
        if (IsTypeSpec())
        {
            Metadata::TypeSignature const signature(GetTypeSpecSignature());
            if (signature.IsByRef())
                return Type();

            switch (signature.GetKind())
            {
            // All arrays are derived directly from System.Array:
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return Detail::GetSystemAssembly(GetAssembly()).GetType(L"System", L"Array");

            case Metadata::TypeSignature::Kind::Ptr:
                return Type();

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::FnPtr:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
            case Metadata::TypeSignature::Kind::Var:
            default:
                ;
            //    Detail::AssertFail(L"Not yet implemented");
            }
        }

        return ResolveTypeDefTypeAndCall([&](Type const& t) -> Type
        {
            Metadata::RowReference const extends(t.GetTypeDefRow().GetExtends());
            if (!extends.IsInitialized())
                return Type();

            switch (extends.GetTable())
            {
            case Metadata::TableId::TypeDef:
            case Metadata::TableId::TypeRef:
            case Metadata::TableId::TypeSpec:
                return Type(t.GetAssembly(), extends, InternalKey());

            default:
                Detail::AssertFail(L"Unreachable Code");
                return Type();
            }
        });
    }

    Type Type::GetDeclaringType() const
    {
        // TODO REFACTOR THE ISNESTED LOGIC
        if (ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                >  TypeAttribute::Public;
        }))
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

    Type Type::GetElementType() const
    {
        // Only a TypeSpec has an element type:
        if (IsTypeDef())
            return Type();

        // TODO
        Detail::AssertFail(L"Not yet implemented");
        return Type();
    }

    String Type::GetAssemblyQualifiedName() const
    {
        return Detail::TypeNameBuilder::BuildTypeName(*this, Detail::TypeNameBuilder::Mode::AssemblyQualifiedName);
    }

    String Type::GetFullName() const
    {
        return Detail::TypeNameBuilder::BuildTypeName(*this, Detail::TypeNameBuilder::Mode::FullName);
    }

    SizeType Type::GetMetadataToken() const
    {
        //if (IsTypeSpec())
        //{
        //    if (GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::Array) ||
        //        GetTypeSpecSignature().IsKind(Metadata::TypeSignature::Kind::SzArray))
        //        return 0x02000000;
        //}
        
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
           return t._type.IsRowReference() ? t._type.AsRowReference().GetToken() : 0; 
        });
    }

    TypeFlags Type::GetAttributes() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return TypeFlags();

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Ptr:
                return TypeFlags();
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags();
        });
    }

    String Type::GetName() const
    {
        return Detail::TypeNameBuilder::BuildTypeName(*this, Detail::TypeNameBuilder::Mode::SimpleName);
    }

    StringReference Type::GetBasicName() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetName();
        });
    }

    StringReference Type::GetNamespace() const
    {
        // A nested type has an empty namespace string in the database; we instead return the 
        // namespace of its declaring type, for consistency.
        // TODO Refactor the logic for IsNested().  A nested TypeSpec (e.g. A+B& returns false for
        // IsNested but does inherit the namespace.
        if (ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                >  TypeAttribute::Public;
        }))
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
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            default:
                break;
            }
        }

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
        return !signature.IsByRef() && (signature.IsSimpleArray() || signature.IsGeneralArray());
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
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return true;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::Ptr:
            case Metadata::TypeSignature::Kind::SzArray:
                return true;

            default:
                break;
            }
        }

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
            return Detail::IsDerivedFromSystemType(t, L"System", L"__ComObject", true);
        });
    }

    bool Type::IsContextful() const
    {
        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Detail::IsDerivedFromSystemType(t, L"System", L"ContextBoundObject", true);
        });
    }

    bool Type::IsEnum() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Detail::IsDerivedFromSystemType(t, L"System", L"Enum", false);
        });
    }

    bool Type::IsExplicitLayout() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::Ptr:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            default:
                break;
            }
        }

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

        String const name(GetName());
        return std::find(name.begin(), name.end(), L'`') != name.end() && !IsByRef();
    }

    bool Type::IsGenericTypeDefinition() const
    {
        // TODO This is incorrect, but is a close approximation that works much of the time.
        return IsTypeDef() && IsGenericType();
    }

    bool Type::IsImport() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Import);
        });
    }

    bool Type::IsInterface() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::Ptr:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            default:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::ClassSemanticsMask)
                == TypeAttribute::Interface;
        });
    }

    bool Type::IsLayoutSequential() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::Ptr:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            default:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::LayoutMask)
                == TypeAttribute::SequentialLayout;
        });
    }

    bool Type::IsMarshalByRef() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return false;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Detail::IsDerivedFromSystemType(t, L"System", L"MarshalByRefObject", true);
        });
    }

    bool Type::IsNested() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::Ptr:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            default:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                >  TypeAttribute::Public;
        });
    }

    bool Type::IsNestedAssembly() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return false;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedAssembly;
        });
    }

    bool Type::IsNestedFamilyAndAssembly() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return false;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamilyAndAssembly;
        });

    }
    bool Type::IsNestedFamily() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return false;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamily;
        });
    }

    bool Type::IsNestedFamilyOrAssembly() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return false;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedFamilyOrAssembly;
        });
    }

    bool Type::IsNestedPrivate() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return false;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedPrivate;
        });
    }

    bool Type::IsNestedPublic() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return false;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask)
                == TypeAttribute::NestedPublic;
        });
    }

    bool Type::IsNotPublic() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return true;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return true;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::NotPublic;
        });
    }

    bool Type::IsPointer() const
    {
        if (IsTypeDef())
            return false;

        return !GetTypeSpecSignature().IsByRef() && GetTypeSpecSignature().IsPointer();
    }

    bool Type::IsPrimitive() const
    {
        if (!IsTypeDef())
            return false;

        if (!Detail::IsSystemAssembly(_assembly.Realize()))
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
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            default:
                return false;

            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return true;

            case Metadata::TypeSignature::Kind::ClassType:
            case Metadata::TypeSignature::Kind::GenericInst:
            case Metadata::TypeSignature::Kind::Primitive:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().WithMask(TypeAttribute::VisibilityMask) == TypeAttribute::Public;
        });
    }

    bool Type::IsSealed() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return true;

            case Metadata::TypeSignature::Kind::Ptr:
                return false;

            default:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Sealed);
        });
    }

    bool Type::IsSerializable() const
    {
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::SzArray:
                return true;

            case Metadata::TypeSignature::Kind::Ptr:
                return false;

            default:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return t.GetTypeDefRow().GetFlags().IsSet(TypeAttribute::Serializable)
                || t.IsEnum()
                || Detail::IsDerivedFromSystemType(t, L"System", L"MulticastDelegate", true);
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
        if (IsTypeSpec())
        {
            if (GetTypeSpecSignature().IsByRef())
                return false;

            switch (GetTypeSpecSignature().GetKind())
            {
            case Metadata::TypeSignature::Kind::Array:
            case Metadata::TypeSignature::Kind::Ptr:
            case Metadata::TypeSignature::Kind::SzArray:
                return false;

            default:
                break;
            }
        }

        return ResolveTypeDefTypeAndCall([](Type const& t)
        {
            return Detail::IsDerivedFromSystemType(t, Metadata::ElementType::ValueType, false)
                && !Detail::IsSystemType(t, L"System", L"Enum");
        });
    }

    bool Type::IsVisible() const
    {
        if (IsTypeSpec())
        {
            switch (GetTypeSpecSignature().GetKind())
            {
            // A GenericInst type is visible if and only if the generic type definition is visible
            // and all of the generic type arguments are visible.  We'll check the type definition
            // itself below; we need only to check the arguments here:
            case Metadata::TypeSignature::Kind::GenericInst:
            {
                Metadata::TypeSignature const genericType(GetTypeSpecSignature());
                if (!std::all_of(genericType.BeginGenericArguments(),
                                 genericType.EndGenericArguments(),
                                 [&](Metadata::TypeSignature const argumentSignature) -> bool
                {
                    Type const argumentType(
                        _assembly.Realize(),
                        Metadata::BlobReference(argumentSignature),
                        InternalKey());

                    return argumentType.IsVisible();
                }))
                    return false;
            }

            default:
                break;
            }
        }
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

    Type Type::ResolveTypeDef(Type const type)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        if (!type.IsInitialized() || type.IsTypeDef())
            return type;

        auto const& database(type.GetAssembly().GetContext(InternalKey()).GetDatabase());
        auto const signature(type.GetTypeSpecSignature());

        Metadata::FullReference nextType;
        switch (signature.GetKind())
        {
        case Metadata::TypeSignature::Kind::Array:
            nextType = Metadata::FullReference(&database, Metadata::BlobReference(signature.GetArrayType()));
            break;

        case Metadata::TypeSignature::Kind::ClassType:
            nextType = Metadata::FullReference(&database, signature.GetTypeReference());
            break;

        case Metadata::TypeSignature::Kind::FnPtr:
            // TODO FnPtr Not Yet Implemented (is there anything to implement here?)
            Detail::AssertFail(L"FnPtr TypeDef Resolution Not Yet Implemented");
            return Type();

        case Metadata::TypeSignature::Kind::GenericInst:
            nextType = Metadata::FullReference(&database, signature.GetGenericTypeReference());
            break;

        case Metadata::TypeSignature::Kind::Primitive:
            return ResolveTypeDef(type
                .GetAssembly()
                .GetContext(InternalKey())
                .GetLoader()
                .GetFundamentalType(signature.GetPrimitiveElementType(), InternalKey()));

        case Metadata::TypeSignature::Kind::Ptr:
            nextType = Metadata::FullReference(
                &database,
                Metadata::BlobReference(signature.GetPointerTypeSignature()));
            break;

        case Metadata::TypeSignature::Kind::SzArray:
            nextType = Metadata::FullReference(
                &database,
                Metadata::BlobReference(signature.GetArrayType()));
            break;

        case Metadata::TypeSignature::Kind::Var:
            // A Class or Method Variable is never itself a TypeDef:
            return Type();
        }

        // Recursively resolve the next type.  Note that 'type' and 'nextType' will always be in the
        // same assembly because we haven't yet resolved the 'nextType' into another assembly:
        if (nextType.IsRowReference())
        {
            return ResolveTypeDef(Type(type.GetAssembly(), nextType.AsRowReference(), InternalKey()));
        }
        else
        {
            return ResolveTypeDef(Type(type.GetAssembly(), nextType.AsBlobReference(), InternalKey()));
        }
    }
}
