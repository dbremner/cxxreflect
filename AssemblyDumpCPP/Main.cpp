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
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

#include <algorithm>
#include <cstdio>

using namespace CxxReflect;
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

    MetadataLoader loader(std::move(resolver));

    Assembly a(loader.LoadAssembly(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll"));

    //Type const object(a.GetType(L"System.Object"));
    //Method const firstMethod(*object.BeginMethods());

    Detail::FileHandle fs(L"d:\\jm\\mscorlib.cpp.txt", Detail::FileMode::Write);
    Dump(fs, a);
}
