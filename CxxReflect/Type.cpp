//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/Type.hpp"
#include "CxxReflect/Utility.hpp"

#include <cor.h>

using CxxReflect::Utility::DebugVerifyNotNull;
using CxxReflect::Utility::ThrowOnFailure;

namespace {

    bool IsDerivedFromSystemType(CxxReflect::Type const&   type,
                                 CxxReflect::String const& systemTypeName,
                                 bool                      includeSelf)
    {
        CxxReflect::TypeHandle current(&type);
        if (!includeSelf)
        {
            current = current->GetBaseType();
        }

        while (current != nullptr)
        {
            if (current->GetAssembly()->IsSystemAssembly() && current->GetFullName() == systemTypeName)
                return true;

            current = current->GetBaseType();
        }

        return false;
    }

}

namespace CxxReflect {

    Type::Type(AssemblyHandle assembly, MetadataToken token)
        : _assembly(assembly),
          _originalToken(token),
          _typeDefToken(0),
          _typeFlags(0),
          _baseTypeToken(0),
          _baseType(nullptr),
          _declaringType(nullptr)
    {
        DebugVerifyNotNull(assembly);

        switch (_originalToken.GetKind())
        {
        // If we have a TypeDef token, we can preemptively set the _typeDefToken value
        case MetadataTokenKind::TypeDef:
            _typeDefToken = _originalToken;
            _state.Set(RealizedTypeDef);
            break;

        // These token types are okay:
        case MetadataTokenKind::TypeRef:
        case MetadataTokenKind::TypeSpec:
            break;

        // We don't know what to do with other token types:
        default:
            throw std::logic_error("wtf");
        }
    }

    String Type::GetAssemblyQualifiedName() const
    {
        return GetFullName() + L", " + GetAssembly()->GetName().GetFullName();
    }

    String Type::GetFullName() const
    {
        String fullName;

        // TODO We may need to handle escape characters in type names here
        Type const* declaringType(PrivateGetDeclaringType());
        if (declaringType != nullptr)
        {
            fullName += declaringType->GetFullName() + L"+";
        }

        fullName += PrivateGetTypeName();
           
        return fullName;
    }

    String Type::GetName() const
    {
        // TODO We need to handle escaped dots here
        String const& typeName(PrivateGetTypeName());
        std::size_t indexOfLastDot(typeName.find_last_of(L'.', std::wstring::npos));
        return indexOfLastDot != std::wstring::npos
            ? typeName.substr(indexOfLastDot + 1, typeName.size() - indexOfLastDot - 1)
            : typeName;
    }

    String Type::GetNamespace() const
    {
        // TODO We need to handle escaped dots here
        String fullName(GetFullName());
        std::size_t indexOfLastDot(fullName.find_last_of(L'.', std::wstring::npos));
        return indexOfLastDot != std::wstring::npos
            ? fullName.substr(0, indexOfLastDot)
            : L"";
    }

    bool Type::HasElementType() const
    {
        return IsArray() || IsPointer() || IsByRef();
    }

    bool Type::IsAbstract() const
    {
        return IsTdAbstract(PrivateGetTypeFlags()) != 0;
    }

    bool Type::IsAnsiClass() const
    {
        return IsTdAnsiClass(PrivateGetTypeFlags());
    }

    bool Type::IsArray() const
    {
        return false; // TODO
    }

    bool Type::IsAutoClass() const
    {
        return IsTdAutoClass(PrivateGetTypeFlags());
    }

    bool Type::IsAutoLayout() const
    {
        return IsTdAutoLayout(PrivateGetTypeFlags());
    }

    bool Type::IsByRef() const
    {
        return false; // TODO
    }

    bool Type::IsClass() const
    {
        return !IsInterface() && !IsValueType();
    }

    bool Type::IsCOMObject() const
    {
        return IsDerivedFromSystemType(*this, L"System.__ComObject", true);
    }

    bool Type::IsContextful() const
    {
        return IsDerivedFromSystemType(*this, L"System.ContextBoundObject", true);
    }

    bool Type::IsEnum() const
    {
        return IsDerivedFromSystemType(*this, L"System.Enum", false);
    }

    bool Type::IsExplicitLayout() const
    {
        return IsTdExplicitLayout(PrivateGetTypeFlags());
    }

    bool Type::IsGenericParameter() const
    {
        return false; // TODO
    }

    bool Type::IsGenericType() const
    {
        return IsGenericTypeDefinition(); // TODO This is incorrect
    }

    bool Type::IsGenericTypeDefinition() const
    {
        return GetFullName().find_last_of(L'`', std::wstring::npos) != std::wstring::npos; // TODO This is incorrect
    }

    bool Type::IsImport() const
    {
        return IsTdImport(PrivateGetTypeFlags()) != 0;
    }

    bool Type::IsInterface() const
    {
        return IsTdInterface(PrivateGetTypeFlags());
    }

    bool Type::IsLayoutSequential() const
    {
        return IsTdSequentialLayout(PrivateGetTypeFlags());
    }

    bool Type::IsMarshalByRef() const
    {
        return IsDerivedFromSystemType(*this, L"System.MarshalByRefObject", true);
    }

    bool Type::IsNested() const
    {
        return IsTdNested(PrivateGetTypeFlags());
    }

    bool Type::IsNestedAssembly() const
    {
        return IsTdNestedAssembly(PrivateGetTypeFlags());
    }

    bool Type::IsNestedFamilyAndAssembly() const
    {
        return IsTdNestedFamANDAssem(PrivateGetTypeFlags());
    }

    bool Type::IsNestedFamily() const
    {
        return IsTdNestedFamily(PrivateGetTypeFlags());
    }

    bool Type::IsNestedFamilyOrAssembly() const
    {
        return IsTdNestedFamORAssem(PrivateGetTypeFlags());
    }

    bool Type::IsNestedPrivate() const
    {
        return IsTdNestedPrivate(PrivateGetTypeFlags());
    }

    bool Type::IsNestedPublic() const
    {
        return IsTdNestedPublic(PrivateGetTypeFlags());
    }

    bool Type::IsNotPublic() const
    {
        return IsTdNotPublic(PrivateGetTypeFlags());
    }

    bool Type::IsPointer() const
    {
        return false; // TODO
    }

    bool Type::IsPrimitive() const
    {
        if (!_assembly->IsSystemAssembly()) { return false; }

        String fullName(GetFullName());
        return fullName == L"System.Boolean"
            || fullName == L"System.Byte"
            || fullName == L"System.SByte"
            || fullName == L"System.Int16"
            || fullName == L"System.UInt16"
            || fullName == L"System.Int32"
            || fullName == L"System.UInt32"
            || fullName == L"System.Int64"
            || fullName == L"System.UInt64"
            || fullName == L"System.IntPtr"
            || fullName == L"System.UIntPtr"
            || fullName == L"System.Char"
            || fullName == L"System.Double"
            || fullName == L"System.Single";
    }

    bool Type::IsPublic() const
    {
        return IsTdPublic(PrivateGetTypeFlags());
    }

    bool Type::IsSealed() const
    {
        return IsTdSealed(PrivateGetTypeFlags()) != 0;
    }

    bool Type::IsSerializable() const
    {
        return IsTdSerializable(PrivateGetTypeFlags()) != 0
            || IsEnum()
            || IsDerivedFromSystemType(*this, L"System.MulticastDelegate", false);
    }

    bool Type::IsSpecialName() const
    {
        return IsTdSpecialName(PrivateGetTypeFlags()) != 0;
    }

    bool Type::IsUnicodeClass() const
    {
        return IsTdUnicodeClass(PrivateGetTypeFlags());
    }

    bool Type::IsValueType() const
    {
        // System.Enum is derived from System.ValueType but is not itself a value type.  Go figure.
        if (_assembly->IsSystemAssembly() && GetFullName() == L"System.Enum") { return false; }

        return IsDerivedFromSystemType(*this, L"System.ValueType", false);
    }

    bool Type::IsVisible() const
    { 
        if (IsPublic())
            return true;

        if (!IsNestedPublic())
            return false;

        return PrivateGetDeclaringType()->IsVisible();
    }

    void Type::RealizeTypeDef() const
    {
        if (_state.IsSet(RealizedTypeDef)) { return; }

        // TODO

        _state.Set(RealizedTypeDef);
    }

    void Type::RealizeTypeDefProperties() const
    {
        if (_state.IsSet(RealizedTypeDefProperties)) { return; }

        std::array<wchar_t, 512> nameBuffer = { 0 }; // TODO size?
        ULONG count(0);
        DWORD flags(0);
        mdToken extends(0);

        ThrowOnFailure(_assembly->UnsafeGetImport()->GetTypeDefProps(
            PrivateGetTypeDefToken().Get(),
            nameBuffer.data(),
            nameBuffer.size(),
            &count,
            &flags,
            &extends));

        _typeName = String(nameBuffer.begin(), nameBuffer.begin() + count - 1);
        _typeFlags = flags;
        _baseTypeToken = MetadataToken(extends);

        _state.Set(RealizedTypeDefProperties);
    }

    void Type::RealizeBaseType() const
    {
        if (_state.IsSet(RealizedBaseType)) { return; }

        MetadataToken baseTypeToken(PrivateGetBaseTypeToken());
        if (baseTypeToken.Get() == 0)
        {
            _baseType = nullptr;
            _state.Set(RealizedBaseType);
            return;
        }

        switch (baseTypeToken.GetKind())
        {
        case MetadataTokenKind::TypeDef:
        case MetadataTokenKind::TypeSpec:
            _baseType = _assembly->GetType(baseTypeToken);
            break;

        case MetadataTokenKind::TypeRef:
            // TODO
            break;

        default:
            throw std::logic_error("wtf");
        }

        _state.Set(RealizedBaseType);
    }

    void Type::RealizeDeclaringType() const
    {
        if (_state.IsSet(RealizedDeclaringType)) { return; }

        if (IsNested())
        {
            mdTypeDef declaringType(0);

            ThrowOnFailure(_assembly->UnsafeGetImport()->GetNestedClassProps(
                PrivateGetTypeDefToken().Get(),
                &declaringType));

            _declaringType = _assembly->GetType(MetadataToken(declaringType));
        }
        else
        {
            // TODO
        }

        _state.Set(RealizedDeclaringType);
    }

}