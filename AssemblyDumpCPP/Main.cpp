//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is one of a pair of test programs for CxxReflect:  AssemblyDumpCS and AssemblyDumpCPP.  The
// former uses the .NET Reflection APIs to write metadata for an assembly to a file; the latter uses
// the CxxReflect library to write metadata to a file in the same format.  The files can be diffed
// to validate that the CxxReflect library is functionally equivalent (where appropriate) to the
// .NET Reflection API. 

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/MetadataSignature.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

#include <algorithm>
#include <cstdio>

using namespace CxxReflect;
using namespace CxxReflect::Metadata;

namespace
{
    typedef std::vector<Detail::MethodReference> MethodTable;

    bool IsExplicitInterfaceImplementation(StringReference const methodName)
    {
        return std::any_of(methodName.begin(), methodName.end(), [](Character c) { return c == L'.'; });
    }

    DatabaseReference ResolveTypeDef(MetadataLoader const& loader, DatabaseReference const typeReference)
    {
        // Resolve a TypeRef into the TypeDef or TypeSpec it references:
        DatabaseReference const resolvedTypeReference(loader.ResolveType(typeReference, InternalKey()));

        if (resolvedTypeReference.GetTableReference().GetTable() == TableId::TypeDef)
        {
            return typeReference;
        }

        Detail::Verify([&]{ return resolvedTypeReference.GetTableReference().GetTable() == TableId::TypeSpec; });

        TypeSpecRow const resolvedTypeSpec(resolvedTypeReference
            .GetDatabase()
            .GetRow<TableId::TypeSpec>(resolvedTypeReference.GetTableReference().GetIndex()));

        TypeSignature const typeSignature(resolvedTypeReference
            .GetDatabase()
            .GetBlob(resolvedTypeSpec.GetSignature().GetIndex())
            .As<TypeSignature>());

        Detail::Verify([&]{ return typeSignature.GetKind() == TypeSignature::Kind::GenericInst; }, "Not yet implemented");

        return loader.ResolveType(DatabaseReference(
            &typeReference.GetDatabase(),
            typeSignature.GetGenericTypeReference()), InternalKey());
    }

    bool BuildMethodTableFilter(MetadataLoader          const& loader,
                                Detail::MethodReference const& newMethod,
                                MethodTable             const& currentTable)
    {
        Detail::MethodReference const& r(newMethod);
        MethodDefRow const methodDefR(r.GetDatabase().GetRow<TableId::MethodDef>(r.GetMethod().GetIndex()));
        MethodSignature const methodSigR(r
            .GetDatabase()
            .GetBlob(methodDefR.GetSignature().GetIndex())
            .As<MethodSignature>());

        return !std::any_of(begin(currentTable), end(currentTable), [&](Detail::MethodReference const& s) -> bool
        {
            MethodDefRow const methodDefS(s.GetDatabase().GetRow<TableId::MethodDef>(s.GetMethod().GetIndex()));

            if (methodDefS.GetFlags().IsSet(MethodAttribute::NewSlot))
                return false;

            if (methodDefR.GetName() != methodDefS.GetName())
                return false;

            // If the method isn't HideBySig, it's HideByName, so all inherited methods of that name
            // are not visible from the derived class, regardless of whether the signatures match.
            if (!methodDefS.GetFlags().IsSet(MethodAttribute::HideBySig))
                return true;

            if (methodDefR.GetFlags().IsSet(MethodAttribute::Final))
                return false;

            MethodSignature const methodSigS(s
                .GetDatabase()
                .GetBlob(methodDefS.GetSignature().GetIndex())
                .As<MethodSignature>());

            if (!SignatureComparer(&loader, &r.GetDatabase(), &s.GetDatabase())(methodSigR, methodSigS))
                return false;

            return true;
        });
    }


    // TODO WE ONLY CURRENTLY SUPPORT GENERICINST AND TYPEDEF HERE
    void BuildMethodTableRecursive(MetadataLoader    const& loader,
                                   DatabaseReference const  typeReference,
                                   MethodTable&             currentTable)
    {
        // The algorithm for building the method table is found in ECMA-335 I/8.10.4.
        DatabaseReference const typeDefReference(ResolveTypeDef(loader, typeReference));

        TypeDefRow const typeDefRow(typeDefReference
            .GetDatabase()
            .GetRow<TableId::TypeDef>(typeDefReference.GetTableReference().GetIndex()));

        MethodTable newEntries;

        SizeType const firstMethod(typeDefRow.GetFirstMethod().GetIndex());
        SizeType const lastMethod(typeDefRow.GetLastMethod().GetIndex());

        for (SizeType index(firstMethod); index != lastMethod; ++index)
        {
            Detail::MethodReference newMethod(Detail::MethodReference(
                &typeDefReference.GetDatabase(),
                typeDefReference.GetTableReference(),
                TableReference(TableId::MethodDef, index)));

            if (BuildMethodTableFilter(loader, newMethod, currentTable))
                newEntries.push_back(newMethod);
        }

        currentTable.insert(end(currentTable), begin(newEntries), end(newEntries));

        if (typeDefRow.GetExtends().IsValid())
        {
            DatabaseReference const baseTypeReference(
                &typeDefReference.GetDatabase(),
                typeDefRow.GetExtends());

            BuildMethodTableRecursive(loader, baseTypeReference, currentTable);
        }
    }

    MethodTable BuildMethodTable(MetadataLoader const& loader, DatabaseReference const typeReference)
    {
        MethodTable result;
        BuildMethodTableRecursive(loader, typeReference, result);
        return result;
    }
}

namespace
{
    void Dump(Detail::FileHandle& os, Assembly const& a);
    void Dump(Detail::FileHandle& os, Type     const& t);
    void Dump(Detail::FileHandle& os, Method   const& m);

    void Dump(Detail::FileHandle& os, Assembly const& a)
    {
        os << L"Assembly [" << a.GetName().GetFullName().c_str() << L"]\n";
        os << L"!!BeginAssemblyReferences\n";
        std::for_each(a.BeginReferencedAssemblyNames(), a.EndReferencedAssemblyNames(), [&](AssemblyName const& x)
        {
            os << L" -- AssemblyName [" << x.GetFullName().c_str() << L"]\n";
        });
        os << L"!!EndAssemblyReferences\n";
        os << L"!!BeginTypes\n";
        std::for_each(a.BeginTypes(), a.EndTypes(), [&](Type const& x)
        {
            Dump(os, x);
        });
        os << L"!!EndTypes\n";
    }

    void Dump(Detail::FileHandle& os, Type const& t)
    {
        os << L" -- Type [" << t.GetFullName().c_str() << L"] [$" << Detail::HexFormat(t.GetMetadataToken()) << L"]\n";
        os << L"     -- AssemblyQualifiedName [" << t.GetAssemblyQualifiedName().c_str() << L"]\n";
        os << L"     -- BaseType [" << (t.GetBaseType() ? t.GetBaseType().GetFullName().c_str() : L"NO BASE TYPE") << L"]\n";
        os << L"         -- AssemblyQualifiedName [" << (t.GetBaseType() ? t.GetBaseType().GetAssemblyQualifiedName().c_str() : L"NO BASE TYPE") << L"]\n"; // TODO DO WE NEED TO DUMP FULL TYPE INFO?

        #define F(n) (t.n() ? 1 : 0)
        os << L"     -- IsTraits [" 
           << F(IsAbstract) << F(IsAnsiClass) << F(IsArray) << F(IsAutoClass) << F(IsAutoLayout) << F(IsByRef) << F(IsClass) << F(IsComObject) << L"] ["
           << F(IsContextful) << F(IsEnum) << F(IsExplicitLayout) << F(IsGenericParameter) << F(IsGenericType) << F(IsGenericTypeDefinition) << F(IsImport) << F(IsInterface) << L"] ["
           << F(IsLayoutSequential) << F(IsMarshalByRef) << F(IsNested) << F(IsNestedAssembly) << F(IsNestedFamilyAndAssembly) << F(IsNestedFamily) << F(IsNestedFamilyOrAssembly) << F(IsNestedPrivate) << L"] ["
           << F(IsNestedPublic) << F(IsNotPublic) << F(IsPointer) << F(IsPrimitive) << F(IsPublic) << F(IsSealed) << F(IsSerializable) << F(IsSpecialName) << L"] ["
           << F(IsUnicodeClass) << F(IsValueType) << F(IsVisible) << L"     ]\n";
        #undef F

        os << L"     -- Name [" << t.GetName().c_str() << L"]\n";
        os << L"     -- Namespace [" << t.GetNamespace().c_str() << L"]\n";
        os << L"    !!BeginMethods\n";
        Type methodsType = t.IsEnum() ? t.GetBaseType() : t;

        MethodTable allMethods(BuildMethodTable(
            t.GetAssembly().GetContext(InternalKey()).GetLoader(),
            DatabaseReference(
                &t.GetAssembly().GetContext(InternalKey()).GetDatabase(),
                TableReference::FromToken(t.GetMetadataToken()))));

        std::sort(begin(allMethods), end(allMethods), [](Detail::MethodReference const& lhs, Detail::MethodReference const& rhs)
        {
            return lhs.GetMethod().GetToken() < rhs.GetMethod().GetToken();
        });

        std::for_each(begin(allMethods), end(allMethods), [&](Detail::MethodReference const& m)
        {
            MethodDefRow const mdr(m.GetDatabase().GetRow<TableId::MethodDef>(m.GetMethod().GetIndex()));

            // Inherited type:
            if (m.GetDeclaringType().GetToken() != t.GetMetadataToken() ||
                m.GetDatabase() != t.GetAssembly().GetContext(InternalKey()).GetDatabase())
            {
                //if (mdr.GetFlags().IsSet(MethodAttribute::Static))
                //    return;

                if (mdr.GetFlags().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::Private &&
                    !IsExplicitInterfaceImplementation(mdr.GetName()))
                    return;
            }

            if (mdr.GetFlags().IsSet(MethodAttribute::SpecialName) && mdr.GetName() == L".ctor")
                return;

            if (mdr.GetFlags().IsSet(MethodAttribute::SpecialName) && mdr.GetName() == L".cctor")
                return;

            if (String(mdr.GetName().c_str()).substr(0, 8) == L"<.cctor>")
                return;

            os << L"     -- Method [" << mdr.GetName().c_str() << L"] [$" << Detail::HexFormat(mdr.GetSelfReference().GetToken()) << L"]\n";
        });

        /*
        BindingFlags const bindings(
            BindingAttribute::FlattenHierarchy |
            BindingAttribute::Instance         |
            BindingAttribute::NonPublic        |
            BindingAttribute::Public           |
            BindingAttribute::Static);
        std::for_each(methodsType.BeginMethods(bindings), methodsType.EndMethods(), [&](Method const& m)
        {
            Dump(os, m);
        });
        */
        os << L"    !!EndMethods\n";
    }

    void Dump(Detail::FileHandle& os, Method const& m)
    {
        os << L"     -- Method [" << m.GetName().c_str() << L"] [$" << Detail::HexFormat(m.GetMetadataToken()) << L"]\n"; // TODO
    }
}

int main()
{
    DirectoryBasedMetadataResolver::DirectorySet directories;
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319");

    std::unique_ptr<IMetadataResolver> resolver(new DirectoryBasedMetadataResolver(directories));

    Metadata::Database db(L"D:\\jm\\dev\\TestData\\ConsoleApplication1\\ConsoleApplication1\\bin\\Debug\\ConsoleApplication1.exe");

    std::vector<std::pair<std::wstring, unsigned>> vx;

    std::transform(
        db.Begin<TableId::MethodDef>(),
        db.End<TableId::MethodDef>(),
        std::back_inserter(vx),
        [&](MethodDefRow const& r)
    {
        return std::make_pair(r.GetName().c_str(), r.GetSignature().GetIndex());
    });

    std::vector<std::pair<std::wstring, unsigned>> vy;

    std::transform(
        db.Begin<TableId::MemberRef>(),
        db.End<TableId::MemberRef>(),
        std::back_inserter(vy),
        [&](MemberRefRow const& r)
    {
        return std::make_pair(r.GetName().c_str(), r.GetSignature().GetIndex());
    });


    MetadataLoader loader(std::move(resolver));

    Assembly a(loader.LoadAssembly(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll"));

    Type const object(a.GetType(L"System.Object"));
    Method const toString(*object.BeginMethods());

    Detail::FileHandle fs(L"d:\\jm\\mscorlib.cpp.txt", Detail::FileMode::Write);
    Dump(fs, a);
}
