//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is one of a pair of test programs for CxxReflect:  AssemblyDumpCS and AssemblyDumpCPP.  The
// former uses the .NET Reflection APIs to write metadata for an assembly to a file; the latter uses
// the CxxReflect library to write metadata to a file in the same format.  The files can be diffed
// to validate that the CxxReflect library is functionally equivalent (where appropriate) to the
// .NET Reflection API. 

#include "CxxReflect/CxxReflect.hpp"

#include <combaseapi.h>

#include <fstream>
#include <iostream>

using namespace CxxReflect;

namespace
{
    void Dump(std::wostream& os, Assembly a);
    void Dump(std::wostream& os, Type t);

    void Dump(std::wostream& os, Assembly a)
    {
        os << L"Assembly [" << a.GetFullName() << L"]\n";
        os << L"!!BeginTypes\n";
        TypeSequence const types(a.GetTypes());
        std::for_each(types.begin(), types.end(), [&](Type t)
        {
            Dump(os, t);
        });
        os << L"!!EndTypes\n";
    }

    void Dump(std::wostream& os, Type t)
    {
        os << L" -- Type [" << t.GetFullName() << L"] [$" << t.GetMetadataToken() << L"]\n";
        os << L"     -- AssemblyQualifiedName [" << t.GetAssemblyQualifiedName() << L"]\n";
        os << L"     -- BaseType [" << (t.HasBaseType() ? t.GetBaseType().GetFullName() : L"NO BASE TYPE") << L"]\n";
        os << L"         -- AssemblyQualifiedName [" << (t.HasBaseType() ? t.GetBaseType().GetAssemblyQualifiedName() : L"NO BASE TYPE") << L"]\n"; // TODO DO WE NEED TO DUMP FULL TYPE INFO?

        #define F(n) (t.n() ? 1 : 0)
        os << L"     -- IsTraits [" 
           << F(IsAbstract) << F(IsArray) << F(IsAutoClass) << F(IsAutoLayout) << F(IsByRef) << F(IsClass) << F(IsCOMObject) << F(IsContextful) << L"] ["
           << F(IsEnum) << F(IsExplicitLayout) << F(IsGenericParameter) << F(IsGenericType) << F(IsGenericTypeDefinition) << F(IsImport) << F(IsInterface) << F(IsLayoutSequential) << L"] ["
           << F(IsMarshalByRef) << F(IsNested) << F(IsNestedAssembly) << F(IsNestedFamANDAssem) << F(IsNestedFamily) << F(IsNestedPrivate) << F(IsNestedPublic) << F(IsNotPublic) << L"] ["
           << F(IsPointer) << F(IsPrimitive) << F(IsPublic) << F(IsSealed) << /* F(IsSecurityCritical) << F(IsSecuritySafeCritical) << F(IsSecurityTransparent) << */ F(IsSerializable) << L"] ["
           << F(IsSpecialName) << F(IsUnicodeClass) << F(IsValueType) << F(IsVisible) << L"    ]\n";
        #undef F

        os << L"     -- Name [" << t.GetName() << L"]\n";
        os << L"     -- Namespace [" << t.GetNamespace() << L"]\n";
    }
}

int main()
{
    CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    std::unique_ptr<DirectoryBasedReferenceResolver> referenceResolver(new DirectoryBasedReferenceResolver());
    referenceResolver->AddDirectory(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\");

    MetadataReader reader(std::move(referenceResolver));

    Assembly a(reader.GetAssemblyByName(AssemblyName(L"mscorlib")));
    
    std::wofstream os("d:\\jm\\mscorlib.cpp.txt");
    Dump(os, a);
}
