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
#include "CxxReflect/Type.hpp"


#include <combaseapi.h>

#include <algorithm>
#include <fstream>
#include <iostream>

using namespace CxxReflect::Metadata;
/*
namespace
{
    void Dump(std::wostream& os, Assembly const& a);
    void Dump(std::wostream& os, Type const& t);

    void Dump(std::wostream& os, Assembly const& a)
    {
        os << L"Assembly [" << a.GetName().GetFullName() << L"]\n";
        os << L"!!BeginAssemblyReferences\n";
        std::for_each(a.BeginReferencedAssemblies(), a.EndReferencedAssemblies(), [&](AssemblyName const& x)
        {
            os << L" -- AssemblyName [" << x.GetFullName() << L"]\n";
        });
        os << L"!!EndAssemblyReferences\n";
        os << L"!!BeginTypes\n";
        std::for_each(a.BeginTypes(), a.EndTypes(), [&](Type const& x)
        {
            Dump(os, x);
        });
        os << L"!!EndTypes\n";
    }

    void Dump(std::wostream& os, Type const& t)
    {
        os << L" -- Type [" << t.GetFullName() << L"] [$" << t.GetMetadataToken().Get() << L"]\n";
        os << L"     -- AssemblyQualifiedName [" << t.GetAssemblyQualifiedName() << L"]\n";
        os << L"     -- BaseType [" << (t.GetBaseType() ? t.GetBaseType()->GetFullName() : L"NO BASE TYPE") << L"]\n";
        os << L"         -- AssemblyQualifiedName [" << (t.GetBaseType() ? t.GetBaseType()->GetAssemblyQualifiedName() : L"NO BASE TYPE") << L"]\n"; // TODO DO WE NEED TO DUMP FULL TYPE INFO?

        #define F(n) (t.n() ? 1 : 0)
        os << L"     -- IsTraits [" 
           << F(IsAbstract) << F(IsAnsiClass) << F(IsArray) << F(IsAutoClass) << F(IsAutoLayout) << F(IsByRef) << F(IsClass) << F(IsCOMObject) << L"] ["
           << F(IsContextful) << F(IsEnum) << F(IsExplicitLayout) << F(IsGenericParameter) << F(IsGenericType) << F(IsGenericTypeDefinition) << F(IsImport) << F(IsInterface) << L"] ["
           << F(IsLayoutSequential) << F(IsMarshalByRef) << F(IsNested) << F(IsNestedAssembly) << F(IsNestedFamilyAndAssembly) << F(IsNestedFamily) << F(IsNestedFamilyOrAssembly) << F(IsNestedPrivate) << L"] ["
           << F(IsNestedPublic) << F(IsNotPublic) << F(IsPointer) << F(IsPrimitive) << F(IsPublic) << F(IsSealed) << F(IsSerializable) << F(IsSpecialName) << L"] ["
           << F(IsUnicodeClass) << F(IsValueType) << F(IsVisible) << L"     ]\n";
        #undef F

        os << L"     -- Name [" << t.GetName() << L"]\n";
        os << L"     -- Namespace [" << t.GetNamespace() << L"]\n";
    }
}
*//*
int main()
{
    CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    auto frameworkResolver([](AssemblyName name)
    {
        return String(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\") + name.GetName() + L".dll";
    });

    MetadataReader reader(frameworkResolver);

    Assembly const* a(reader.GetAssemblyByName(AssemblyName(L"mscorlib")));

    std::wofstream os("d:\\jm\\mscorlib.cpp.txt");
    Dump(os, *a);
}
*/


int main()
{
    using namespace CxxReflect;
    using namespace CxxReflect::Metadata;

    DirectoryBasedMetadataResolver::DirectorySet directories;
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319");

    std::unique_ptr<IMetadataResolver> resolver(new DirectoryBasedMetadataResolver(directories));

    MetadataLoader loader(std::move(resolver));

    Assembly ass = loader.LoadAssembly(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");
    AssemblyName an = ass.GetName();

    std::vector<Type> types(ass.BeginTypes(), ass.EndTypes());

    std::vector<String> typeNames;
    std::transform(types.begin(), types.end(), std::back_inserter(typeNames), [&](Type const& t)
    {
        return t.GetName().c_str();
    });

    //return 0;

    Database db(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");

    StringReference moduleName(db.GetRow<TableId::Module>(0).GetName());

    std::vector<std::wstring> names;
    std::transform(db.Begin<TableId::TypeDef>(),
                   db.End<TableId::TypeDef>(),
                   std::back_inserter(names),
                   [&](TypeDefRow const& r)
    {
        return r.GetName().c_str();
    });

    TypeDefRow object(db.GetRow<TableId::TypeDef>(1));
    std::vector<std::wstring> functions;
    std::transform(db.Begin<TableId::MethodDef>() + object.GetFirstMethod().GetIndex(),
                   db.Begin<TableId::MethodDef>() + object.GetLastMethod().GetIndex(),
                   std::back_inserter(functions),
                   [&](MethodDefRow const& r)
    {
        return r.GetName().c_str();
    });

}
