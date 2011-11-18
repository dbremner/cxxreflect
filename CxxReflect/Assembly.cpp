//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Type.hpp"

using namespace CxxReflect;

namespace CxxReflect {

    Assembly::TypeIterator Assembly::BeginTypes() const
    {
        // We skip the type at index 0, which is the internal <module> type containing module-scope things:
        return Assembly::TypeIterator(*this, Metadata::TableReference(Metadata::TableId::TypeDef, 1));
    }

    Assembly::TypeIterator Assembly::EndTypes() const
    {
        return Assembly::TypeIterator(*this, Metadata::TableReference(
            Metadata::TableId::TypeDef,
            _database->GetTables().GetTable(Metadata::TableId::TypeDef).GetRowCount()));
    }

    Type Assembly::GetType(StringReference const name, bool const ignoreCase) const
    {
        // TODO PORTABILITY
        typedef int (*Comparer)(wchar_t const*, wchar_t const*);
        Comparer const compare(ignoreCase ? _wcsicmp : wcscmp);

        auto const it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& t)
        {
            return compare(t.GetFullName().c_str(), name.c_str()) == 0;
        }));

        return it != EndTypes() ? *it : Type();
    }

    Type Assembly::GetType(StringReference const namespaceName,
                           StringReference const typeName,
                           bool const ignoreCase) const
    {
        // TODO PORTABILITY
        typedef int (*Comparer)(wchar_t const*, wchar_t const*);
        Comparer const compare(ignoreCase ? _wcsicmp : wcscmp);

        auto const it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& t)
        {
            return compare(t.GetNamespace().c_str(), namespaceName.c_str()) == 0
                && compare(t.GetName().c_str(), namespaceName.c_str()) == 0;
        }));

        return it != EndTypes() ? *it : Type();
    }

    SizeType Assembly::GetReferencedAssemblyCount() const
    {
        return _database->GetTables().GetTable(Metadata::TableId::AssemblyRef).GetRowCount();
    }

    Assembly::AssemblyNameIterator Assembly::BeginReferencedAssemblyNames() const
    {
        return Assembly::AssemblyNameIterator(*this, Metadata::TableReference(Metadata::TableId::AssemblyRef, 0));
    }

    Assembly::AssemblyNameIterator Assembly::EndReferencedAssemblyNames() const
    {
        return Assembly::AssemblyNameIterator(*this, Metadata::TableReference(
            Metadata::TableId::AssemblyRef,
            _database->GetTables().GetTable(Metadata::TableId::AssemblyRef).GetRowCount()));
    }

    AssemblyName const& Assembly::GetName() const
    {
        VerifyInitialized();

        return _loader->GetAssemblyName(*_database, InternalKey());
    }

    /*String const& Assembly::GetPath() const
    {
        return L""; // TODO return _path;
    }*/

    Metadata::AssemblyRow Assembly::GetAssemblyRow() const
    {
        VerifyInitialized();

        if (_database->GetTables().GetTable(Metadata::TableId::Assembly).GetRowCount() == 0)
            throw std::runtime_error("wtf");

        return _database->GetRow<Metadata::TableId::Assembly>(0);
    }

    Metadata::Database const& Assembly::GetDatabase(InternalKey) const
    {
        VerifyInitialized();
        return *_database;
    }

    MetadataLoader const& Assembly::GetLoader(InternalKey) const
    {
        VerifyInitialized();
        return *_loader;
    }

}
