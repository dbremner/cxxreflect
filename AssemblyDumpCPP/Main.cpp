#include "CxxReflect/CxxReflect.hpp"

#include <cor.h>

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