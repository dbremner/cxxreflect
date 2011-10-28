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

#include <algorithm>
#include <cstdio>

using namespace CxxReflect;
using namespace CxxReflect::Metadata;

namespace
{
    // An ostream-like wrapper around the <cstdio> file API; the <iostream> implementation is
    // RIDICULOUSLY slow and hampers performance verification. On Visual C++ 11 Developer Preview,
    // this is over 10x faster than std::ofstream.
    class fauxstream
    {
    public:

        fauxstream(std::string path)
        {
            #pragma warning(push)
            #pragma warning(disable: 4996)
            _handle = ::fopen(path.c_str(), "w");
            #pragma warning(pop)

            if (_handle == nullptr)
                throw std::runtime_error("Failed to open file");
        }

        ~fauxstream()
        {
            ::fclose(_handle);
        }

        fauxstream const& operator<<(wchar_t const* s) const
        {
            ::fprintf(_handle, "%ls", s);
            return *this;
        }

        fauxstream const& operator<<(StringReference const& sr) const
        {
            ::fprintf(_handle, "%ls", sr.c_str());
            return *this;
        }

        fauxstream const& operator<<(String const& s) const
        {
            ::fprintf(_handle, "%ls", s.c_str());
            return *this;
        }

        fauxstream const& operator<<(SizeType const& x) const
        {
            ::fprintf(_handle, "%u", x);
            return *this;
        }

    private:

        fauxstream(fauxstream const&);
        fauxstream& operator=(fauxstream const&);

        FILE* _handle;
    };

    void Dump(fauxstream const& os, Assembly const& a);
    void Dump(fauxstream const& os, Type const& t);

    void Dump(fauxstream const& os, Assembly const& a)
    {
        os << L"Assembly [" << a.GetName().GetFullName() << L"]\n";
        os << L"!!BeginAssemblyReferences\n";
        std::for_each(a.BeginReferencedAssemblyNames(), a.EndReferencedAssemblyNames(), [&](AssemblyName const& x)
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

    void Dump(fauxstream const& os, Type const& t)
    {
        os << L" -- Type [" << t.GetFullName() << L"] [$" << t.GetMetadataToken() << L"]\n";
        os << L"     -- AssemblyQualifiedName [" << t.GetAssemblyQualifiedName() << L"]\n";
        os << L"     -- BaseType [" << (t.GetBaseType() ? t.GetBaseType().GetFullName() : L"NO BASE TYPE") << L"]\n";
        os << L"         -- AssemblyQualifiedName [" << (t.GetBaseType() ? t.GetBaseType().GetAssemblyQualifiedName() : L"NO BASE TYPE") << L"]\n"; // TODO DO WE NEED TO DUMP FULL TYPE INFO?

        #define F(n) (t.n() ? 1 : 0)
        os << L"     -- IsTraits [" 
           << F(IsAbstract) << F(IsAnsiClass) << F(IsArray) << F(IsAutoClass) << F(IsAutoLayout) << F(IsByRef) << F(IsClass) << F(IsComObject) << L"] ["
           << F(IsContextful) << F(IsEnum) << F(IsExplicitLayout) << F(IsGenericParameter) << F(IsGenericType) << F(IsGenericTypeDefinition) << F(IsImport) << F(IsInterface) << L"] ["
           << F(IsLayoutSequential) << F(IsMarshalByRef) << F(IsNested) << F(IsNestedAssembly) << F(IsNestedFamilyAndAssembly) << F(IsNestedFamily) << F(IsNestedFamilyOrAssembly) << F(IsNestedPrivate) << L"] ["
           << F(IsNestedPublic) << F(IsNotPublic) << F(IsPointer) << F(IsPrimitive) << F(IsPublic) << F(IsSealed) << F(IsSerializable) << F(IsSpecialName) << L"] ["
           << F(IsUnicodeClass) << F(IsValueType) << F(IsVisible) << L"     ]\n";
        #undef F

        os << L"     -- Name [" << t.GetName() << L"]\n";
        os << L"     -- Namespace [" << t.GetNamespace() << L"]\n";
    }
}

int main()
{
    DirectoryBasedMetadataResolver::DirectorySet directories;
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319");

    std::unique_ptr<IMetadataResolver> resolver(new DirectoryBasedMetadataResolver(directories));

    MetadataLoader loader(std::move(resolver));

    Assembly a(loader.LoadAssembly(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll"));

    fauxstream oh("d:\\jm\\mscorlib.cpp.txt");
    Dump(oh, a);
}


/*
// My light-on-dark color scheme test file
#include <algorithm>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv)
{
    std::vector<int> data;
    data.push_back(0x54545454);
    data.push_back(0x00000000);
    data.push_back(0xffffffff);

    std::sort(begin(data), end(data));

    std::copy(data.begin(), data.end(), std::ostream_iterator<int>(std::cout, ", "));

    return 0;
}
*

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
    std::transform(types.begin(), types.end(), std::back_inserter(typeNames), [&](Type const& t) -> String
    {
        Type tx = t.GetDeclaringType();
        return t.GetName().c_str();
    });

    std::vector<AssemblyName> refs(ass.BeginReferencedAssemblyNames(), ass.EndReferencedAssemblyNames());

    std::vector<String> refNames;
    std::transform(refs.begin(), refs.end(), std::back_inserter(refNames), [&](AssemblyName const& an)
    {
        return an.GetFullName();
    });

    std::vector<String> lattice;
    Type t = ass.GetType(L"System.NullReferenceException");
    
    while (t)
    {
        lattice.push_back(t.GetFullName());

        t = t.GetBaseType();
    }

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
*/

/*
template <typename T> struct remove_reference      { typedef T type; };
template <typename T> struct remove_reference<T&>  { typedef T type; };
template <typename T> struct remove_reference<T&&> { typedef T type; };

template <typename T>
struct add_rvalue_reference
{
    typedef typename remove_reference<T>::type&& type;
};

template <typename T>
struct add_rvalue_reference<T&> { typedef T& type; };

template <typename T>
typename add_rvalue_reference<T>::type declval(); // not implemented

struct S { };

int    f(S) { return 42;   }
double g(S) { return 42.0; }

template <typename Callable>
auto call_if_true(
    bool test,
    Callable callable, 
    decltype(declval<Callable>()(declval<S>())) default_value = 
        declval<decltype(declval<Callable>()(declval<S>()))>
        */
