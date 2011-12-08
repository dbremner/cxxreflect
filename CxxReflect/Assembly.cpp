//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataLoader.hpp"
#include "CxxReflect/Type.hpp"

namespace { namespace Private {

    using namespace CxxReflect;

    typedef int (*StringComparer)(wchar_t const*, wchar_t const*);

    StringComparer GetStringComparer(bool const caseInsensitive)
    {
        // TODO PORTABILITY _wcsicmp is nonstandard
        return caseInsensitive ? _wcsicmp : wcscmp;
    }

} }

namespace CxxReflect {

    Assembly::TypeIterator Assembly::BeginTypes() const
    {
        // We intentionally skip the type at index 0; this isn't a real type, it's the internal
        // <module> "type" containing module-scope thingies.
        return Assembly::TypeIterator(*this, Metadata::RowReference(Metadata::TableId::TypeDef, 1));
    }

    Assembly::TypeIterator Assembly::EndTypes() const
    {
        return TypeIterator(*this, Metadata::RowReference(
            Metadata::TableId::TypeDef,
            _context.Get()->GetDatabase().GetTables().GetTable(Metadata::TableId::TypeDef).GetRowCount()));
    }

    Type Assembly::GetType(StringReference const namespaceQualifiedTypeName, bool const caseInsensitive) const
    {
        Private::StringComparer const compare(Private::GetStringComparer(caseInsensitive));
        auto const it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& t)
        {
            return compare(t.GetFullName().c_str(), namespaceQualifiedTypeName.c_str()) == 0;
        }));

        return it != EndTypes() ? *it : Type();
    }

    Type Assembly::GetType(StringReference const namespaceName,
                           StringReference const unqualifiedTypeName,
                           bool const caseInsensitive) const
    {
        Private::StringComparer const compare(Private::GetStringComparer(caseInsensitive));
        auto const it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& t)
        {
            return compare(t.GetNamespace().c_str(), namespaceName.c_str()) == 0
                && compare(t.GetName().c_str(), unqualifiedTypeName.c_str()) == 0;
        }));

        return it != EndTypes() ? *it : Type();
    }

    SizeType Assembly::GetReferencedAssemblyCount() const
    {
        return _context.Get()->GetDatabase().GetTables().GetTable(Metadata::TableId::AssemblyRef).GetRowCount();
    }

    Assembly::AssemblyNameIterator Assembly::BeginReferencedAssemblyNames() const
    {
        return AssemblyNameIterator(*this, Metadata::RowReference(Metadata::TableId::AssemblyRef, 0));
    }

    Assembly::AssemblyNameIterator Assembly::EndReferencedAssemblyNames() const
    {
        return Assembly::AssemblyNameIterator(*this, Metadata::RowReference(
            Metadata::TableId::AssemblyRef,
            _context.Get()->GetDatabase().GetTables().GetTable(Metadata::TableId::AssemblyRef).GetRowCount()));
    }

    AssemblyName const& Assembly::GetName() const
    {
        VerifyInitialized();

        return _context.Get()->GetAssemblyName();
    }

    Metadata::AssemblyRow Assembly::GetAssemblyRow() const
    {
        VerifyInitialized();

        Metadata::Database const& database(_context.Get()->GetDatabase());

        if (database.GetTables().GetTable(Metadata::TableId::Assembly).GetRowCount() == 0)
            throw std::runtime_error("wtf");

        return database.GetRow<Metadata::TableId::Assembly>(0);
    }

    Detail::AssemblyContext const& Assembly::GetContext(InternalKey) const
    {
        VerifyInitialized();
        return *_context.Get();
    }

}
