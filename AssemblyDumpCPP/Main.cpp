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

namespace
{
    typedef std::vector<Detail::MethodReference> MethodTable;
    typedef Detail::LinearArrayAllocator<Byte, (2 << 16)> ByteAllocator;

    bool IsExplicitInterfaceImplementation(StringReference const methodName)
    {
        // TODO THIS IS A HACK
        return std::any_of(methodName.begin(), methodName.end(), [](Character const c) { return c == L'.'; });
    }

    SizeType ComputeIndexForMethod(MetadataLoader          const& loader,
                                   Detail::MethodReference const& newMethodReference,
                                   MethodTable             const& currentTable,
                                   MethodTable                  & newTable)
    {
        Metadata::MethodDefRow    const newMethodDef(newMethodReference.GetMethodDefinition());
        Metadata::MethodSignature const newMethodSig(newMethodReference.GetMethodSignature());

        // If the method occupies a new slot, it does not override any other method:
        if (newMethodDef.GetFlags().WithMask(MethodAttribute::VTableLayoutMask) == MethodAttribute::NewSlot)
        {
            newTable.push_back(newMethodReference);
            return static_cast<SizeType>(-1);
        }

        for (SizeType i(currentTable.size() - 1); i != static_cast<SizeType>(-1); --i)
        {
            Detail::MethodReference const& methodReference(currentTable[i]);

            Metadata::MethodDefRow    const methodDef(methodReference.GetMethodDefinition());
            Metadata::MethodSignature const methodSig(methodReference.GetMethodSignature());

            if (methodDef.GetName() != newMethodDef.GetName())
                continue;

            Metadata::SignatureComparer const compareSignatures(
                &loader,
                &methodReference.GetDeclaringType().GetDatabase(),
                &newMethodReference.GetDeclaringType().GetDatabase());

            // If the signature of the method in the derived class is different from the signature
            // of the method in the base class, it does not replace it, because it is HideBySig:
            if (!compareSignatures(methodSig, newMethodSig))
                continue;

            // If the base class method is final, the derived class method is a new method:
            if (methodDef.GetFlags().IsSet(MethodAttribute::Final))
                break;

            // Otherwise, we have a match:  return the index:
            return i;
        }

        newTable.push_back(newMethodReference);
        return static_cast<SizeType>(-1);
    }

    std::pair<Metadata::DatabaseReference, Metadata::DatabaseReference>
    ResolveTypeDefAndTypeSpec(MetadataLoader const& loader, Metadata::DatabaseReference const& typeReference)
    {
        // Resolve a TypeRef into the TypeDef or TypeSpec it references:
        Metadata::DatabaseReference const resolvedTypeReference(loader.ResolveType(typeReference, InternalKey()));

        if (resolvedTypeReference.GetTableReference().GetTable() == Metadata::TableId::TypeDef)
        {
            return std::make_pair(resolvedTypeReference, Metadata::DatabaseReference());
        }

        Detail::Verify([&]
        {
            return resolvedTypeReference.GetTableReference().GetTable() == Metadata::TableId::TypeSpec;
        });

        Metadata::TypeSpecRow const resolvedTypeSpec(resolvedTypeReference
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeSpec>(resolvedTypeReference.GetTableReference().GetIndex()));

        Metadata::TypeSignature const typeSignature(resolvedTypeReference
            .GetDatabase()
            .GetBlob(resolvedTypeSpec.GetSignature())
            .As<Metadata::TypeSignature>());

        Detail::Verify([&]
        {
            return typeSignature.GetKind() == Metadata::TypeSignature::Kind::GenericInst;
        }, "Not yet implemented");

        return std::make_pair(
            loader.ResolveType(Metadata::DatabaseReference(
                &typeReference.GetDatabase(),
                typeSignature.GetGenericTypeReference()), InternalKey()),
            resolvedTypeReference);
    }

    // TODO WE ONLY CURRENTLY SUPPORT GENERICINST AND TYPEDEF HERE
    void BuildMethodTableRecursive(MetadataLoader                   const& loader,
                                   Metadata::DatabaseReference      const  type,
                                   MethodTable                           & currentTable,
                                   ByteAllocator      & allocator)
    {
        auto const typeDefAndSpec(ResolveTypeDefAndTypeSpec(loader, type));
        Metadata::DatabaseReference const& typeDefReference(typeDefAndSpec.first);
        Metadata::DatabaseReference const& typeSpecReference(typeDefAndSpec.second);

        Metadata::Database const& database(typeDefReference.GetDatabase());

        Metadata::TypeDefRow const typeDef(typeDefReference
            .GetDatabase()
            .GetRow<Metadata::TableId::TypeDef>(typeDefReference.GetTableReference().GetIndex()));

        Metadata::TableReference const baseTypeReference(typeDef.GetExtends());
        if (baseTypeReference.IsValid())
        {
            BuildMethodTableRecursive(
                loader,
                Metadata::DatabaseReference(&typeDefReference.GetDatabase(), baseTypeReference),
                currentTable,
                allocator);
        }

        Metadata::ClassVariableSignatureInstantiator instantiator;
        if (typeSpecReference.IsInitialized())
        {
            Metadata::TypeSignature const typeSpecSignature(typeSpecReference
                .GetDatabase()
                .GetBlob(typeSpecReference
                    .GetDatabase()
                    .GetRow<Metadata::TableId::TypeSpec>(typeSpecReference.GetTableReference().GetIndex())
                    .GetSignature())
                .As<Metadata::TypeSignature>());

            instantiator = Metadata::ClassVariableSignatureInstantiator(
                typeSpecSignature.BeginGenericArguments(),
                typeSpecSignature.EndGenericArguments());
        }

        // We'll accumulate new method table entries into a separate sequence so that we correctly
        // handle hide by name-and-sig and other subtleties.
        MethodTable newTable;

        auto const beginMethod(database.Begin<Metadata::TableId::MethodDef>());
        auto const firstMethod(beginMethod + typeDef.GetFirstMethod().GetIndex());
        auto const lastMethod (beginMethod + typeDef.GetLastMethod().GetIndex());

        std::for_each(firstMethod, lastMethod, [&](Metadata::MethodDefRow const& methodDef)
        {
            Metadata::MethodSignature methodSig(typeDefReference
                .GetDatabase()
                .GetBlob(methodDef.GetSignature())
                .As<Metadata::MethodSignature>());

            Detail::MethodSignatureAllocator::Range replacementSig;
            if (instantiator.RequiresInstantiation(methodSig))
            {
                auto const newSigTemp(instantiator.Instantiate(methodSig));
                replacementSig = allocator.Allocate(std::distance(newSigTemp.BeginBytes(), newSigTemp.EndBytes()));
                Detail::RangeCheckedCopy(
                    newSigTemp.BeginBytes(), newSigTemp.EndBytes(),
                    replacementSig.Begin(), replacementSig.End());
            }

             Detail::MethodReference const methodReference(typeSpecReference.IsInitialized()
                ? Detail::MethodReference(typeDefReference, methodDef.GetSelfReference(), typeSpecReference, replacementSig)
                : Detail::MethodReference(typeDefReference, methodDef.GetSelfReference()));;

            SizeType const insertionIndex(ComputeIndexForMethod(loader, methodReference, currentTable, newTable));
            if (insertionIndex != static_cast<SizeType>(-1))
            {
                currentTable[insertionIndex] = methodReference;
            }

            // TODO This algorithm does not correctly handle nongeneric virtual methods of a generic
            // base class that are overridden in a derived class; we are unable to match the derived
            // class override with the base class method because of the signature differences--we do
            // not know that 'Var0' in the base class is the same as the type with which the derived
            // class instantiated the base class.  This looks to be a rather difficult problem :'(.b
        });

        currentTable.insert(end(currentTable), begin(newTable), end(newTable));
    }

    // The algorithm for building the method table is found in ECMA-335 I/8.10.4.
    MethodTable BuildMethodTable(MetadataLoader const& loader, Metadata::DatabaseReference const type, ByteAllocator& allocator)
    {
        MethodTable result;
        BuildMethodTableRecursive(loader, type, result, allocator);
        // TODO We need to remove hidden methods using the hide by name and hide by name-and-sig rules.
        return result;
    }
}

using namespace CxxReflect::Metadata;

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

        ByteAllocator signatureAllocator;

        MethodTable allMethods(BuildMethodTable(
            t.GetAssembly().GetContext(InternalKey()).GetLoader(),
            DatabaseReference(
                &t.GetAssembly().GetContext(InternalKey()).GetDatabase(),
                TableReference::FromToken(t.GetMetadataToken())),
            signatureAllocator));

        std::sort(begin(allMethods), end(allMethods), [](Detail::MethodReference const& lhs, Detail::MethodReference const& rhs)
        {
            return lhs.GetMethod().GetTableReference().GetToken() < rhs.GetMethod().GetTableReference().GetToken();
        });

        std::for_each(begin(allMethods), end(allMethods), [&](Detail::MethodReference const& m)
        {
            MethodDefRow const mdr(m
                .GetMethod()
                .GetDatabase()
                .GetRow<TableId::MethodDef>(m.GetMethod().GetTableReference().GetIndex()));

            // Inherited type:
            if (m.GetDeclaringType().GetTableReference().GetToken() != t.GetMetadataToken() ||
                m.GetMethod().GetDatabase() != t.GetAssembly().GetContext(InternalKey()).GetDatabase())
            {
                //if (mdr.GetFlags().IsSet(MethodAttribute::Static))
                //    return;

                if (mdr.GetFlags().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::Private &&
                    !IsExplicitInterfaceImplementation(mdr.GetName()))
                    return;

                if (mdr.GetFlags().WithMask(MethodAttribute::MemberAccessMask) == MethodAttribute::Private &&
                    mdr.GetFlags().IsSet(MethodAttribute::Static))
                    return;
            }

            if (mdr.GetFlags().IsSet(MethodAttribute::SpecialName) && mdr.GetName() == L".ctor")
                return;

            if (mdr.GetFlags().IsSet(MethodAttribute::SpecialName) && mdr.GetName() == L".cctor")
                return;

            //if (String(mdr.GetName().c_str()).substr(0, 8) == L"<.cctor>")
            //    return;

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
