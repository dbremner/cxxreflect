//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is one of a pair of test programs for CxxReflect:  AssemblyDumpCS and AssemblyDumpCPP.  The
// former uses the .NET Reflection APIs to write metadata for an assembly to a file; the latter uses
// the CxxReflect library to write metadata to a file in the same format.  The files can be diffed
// to validate that the CxxReflect library is functionally equivalent (where appropriate) to the
// .NET Reflection API. 

#include "CxxReflect/CxxReflect.hpp"

using namespace CxxReflect;
using namespace CxxReflect::Metadata;

namespace
{
    BindingAttribute const AllBindingFlags = (BindingAttribute::Public | BindingAttribute::NonPublic | BindingAttribute::Static | BindingAttribute::Instance | BindingAttribute::FlattenHierarchy);

    // The CLR hides or modifies some types in the reflection API; we don't get those modifications
    // when we read the metadata directly, so we just ignore those types here in the test program.
    bool IsKnownProblemType(Type const& t)
    {
        return (t.GetNamespace() == L"System"                                        && t.GetName() == L"__ComObject")
            || (t.GetNamespace() == L"System.Runtime.Remoting.Proxies"               && t.GetName() == L"__TransparentProxy")
            || (t.GetNamespace() == L"System.Runtime.InteropServices.WindowsRuntime" && t.GetName() == L"DisposableRuntimeClass");
    }

    void Dump(Detail::FileHandle& os, Assembly        const& a);
    void Dump(Detail::FileHandle& os, CustomAttribute const& c);
    void Dump(Detail::FileHandle& os, Field           const& f);
    void Dump(Detail::FileHandle& os, Method          const& m);
    void Dump(Detail::FileHandle& os, Parameter       const& p);
    void Dump(Detail::FileHandle& os, Type            const& t);

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
            if (IsKnownProblemType(x))
                return;

            Dump(os, x);
        });
        os << L"!!EndTypes\n";
    }

    void Dump(Detail::FileHandle& os, Type const& t)
    {
        os << L" -- Type [" << t.GetFullName().c_str() << L"] [$" << Detail::HexFormat(t.GetMetadataToken()) << L"]\n";
        os << L"     -- AssemblyQualifiedName [" << t.GetAssemblyQualifiedName().c_str() << L"]\n";
        os << L"     -- BaseType [" << (t.GetBaseType() ? t.GetBaseType().GetFullName().c_str() : L"NO BASE TYPE") << L"]\n";
        os << L"         -- AssemblyQualifiedName [" << (t.GetBaseType().IsInitialized() ? t.GetBaseType().GetAssemblyQualifiedName().c_str() : L"NO BASE TYPE") << L"]\n"; // TODO DO WE NEED TO DUMP FULL TYPE INFO?

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
        os << L"    !!BeginInterfaces\n";
        std::vector<Type> allInterfaces(t.BeginInterfaces(), t.EndInterfaces());
        std::sort(allInterfaces.begin(), allInterfaces.end(), MetadataTokenLessThanComparer());
        std::for_each(allInterfaces.begin(), allInterfaces.end(), [&](Type const& it)
        {
            os << L"     -- Interface [" << it.GetFullName().c_str() << L"] [$" << Detail::HexFormat(it.GetMetadataToken()) << L"]\n";
        });
        os << L"    !!EndInterfaces\n";

        os << L"    !!BeginCustomAttributes\n";
        std::vector<CustomAttribute> allCustomAttributes(t.BeginCustomAttributes(), t.EndCustomAttributes());
        std::sort(allCustomAttributes.begin(), allCustomAttributes.end(), MetadataTokenLessThanComparer());
        std::for_each(allCustomAttributes.begin(), allCustomAttributes.end(), [&](CustomAttribute const& c)
        {
            Dump(os, c);
        });
        os << L"    !!EndCustomAttributes\n";

        os << L"    !!BeginConstructors\n";
        std::vector<Method> allConstructors(t.BeginConstructors(AllBindingFlags), t.EndConstructors());
        std::sort(allConstructors.begin(), allConstructors.end(), MetadataTokenLessThanComparer());
        std::for_each(allConstructors.begin(), allConstructors.end(), [&](Method const& m)
        {
            Dump(os, m);
        });
        os << L"    !!EndConstructors\n";

        os << L"    !!BeginMethods\n";
        std::vector<Method> allMethods(t.BeginMethods(AllBindingFlags), t.EndMethods());
        std::sort(allMethods.begin(), allMethods.end(), MetadataTokenLessThanComparer());
        std::for_each(allMethods.begin(), allMethods.end(), [&](Method const& m)
        {
            Dump(os, m);
        });
        os << L"    !!EndMethods\n";

        os << L"    !!BeginFields\n";
        std::vector<Field> allFields(t.BeginFields(AllBindingFlags), t.EndFields());
        std::sort(allFields.begin(), allFields.end(), MetadataTokenLessThanComparer());
        std::for_each(allFields.begin(), allFields.end(), [&](Field const& m)
        {
            Dump(os, m);
        });
        os << L"    !!EndFields\n";
    }

    void Dump(Detail::FileHandle& os, Method const& m)
    {
        os << L"     -- Method [" << m.GetName().c_str() << L"] [$" << Detail::HexFormat(m.GetMetadataToken()) << L"]\n";
        os << L"        !!BeginParameters\n";
        std::for_each(m.BeginParameters(), m.EndParameters(), [&](Parameter const& p)
        {
            Dump(os, p);
        });
        os << L"        !!EndParameters\n";

        // TODO
    }

    void Dump(Detail::FileHandle& os, Parameter const& p)
    {
        os << L"         -- [" << p.GetName().c_str() << L"] [" << p.GetType().GetFullName().c_str() << L"]\n";

        // TODO
    }

    void Dump(Detail::FileHandle& os, Field const& f)
    {
        os << L"     -- Field [" << f.GetName().c_str() << L"] [$" << Detail::HexFormat(f.GetMetadataToken()) << L"]\n";

        #define F(n) (f.n() ? 1 : 0)
        os << L"         -- Attributes [" << Detail::HexFormat(f.GetAttributes().GetIntegral()) << L"]\n";
        os << L"         -- Declaring Type [" << f.GetDeclaringType().GetFullName().c_str() << L"]\n";
        os << L"         -- IsTraits "
           << L"[" << F(IsAssembly) << F(IsFamily) << F(IsFamilyAndAssembly) << F(IsFamilyOrAssembly) << F(IsInitOnly) << F(IsLiteral) << F(IsNotSerialized) << F(IsPinvokeImpl) << L"] "
           << L"[" << F(IsPrivate) << F(IsPublic) << F(IsSpecialName) << F(IsStatic) << L"    ]\n";
        #undef F

        // TODO
    }

    void Dump(Detail::FileHandle& os, CustomAttribute const& c)
    {
        os << L"     -- CustomAttribute [" << c.GetConstructor().GetDeclaringType().GetFullName().c_str() << L"]\n";
        // TODO
    }
}

int main()
{
    DirectoryBasedAssemblyLocator::DirectorySet directories;
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319");

    std::unique_ptr<IAssemblyLocator> resolver(new DirectoryBasedAssemblyLocator(directories));

    Loader loader(std::move(resolver));

    Assembly a(loader.LoadAssembly(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll"));
    /*
    Database const& db(a.GetContext(InternalKey()).GetDatabase());
    for (auto it(db.Begin<Metadata::TableId::CustomAttribute>());
         it != db.End<Metadata::TableId::CustomAttribute>();
         ++it)
    {
        std::cout << (int)it->GetParent().GetTable() << "/" << it->GetParent().GetIndex() << std::endl;
    }*/

    Detail::FileHandle fs(L"c:\\jm\\mscorlib.cpp.txt", Detail::FileMode::Write);
    Dump(fs, a);
}
