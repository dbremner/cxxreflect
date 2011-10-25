//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"

using namespace CxxReflect;

namespace CxxReflect {

    Assembly::TypeIterator Assembly::BeginTypes() const
    {
        return Assembly::TypeIterator(*this, Metadata::TableReference(Metadata::TableId::TypeDef, 0));
    }

    Assembly::TypeIterator Assembly::EndTypes() const
    {
        return Assembly::TypeIterator(*this, Metadata::TableReference(
            Metadata::TableId::TypeDef,
            _database->GetTables().GetTable(Metadata::TableId::TypeDef).GetRowCount()));
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

    AssemblyName Assembly::GetName() const
    {
        return AssemblyName(*this, Metadata::TableReference(Metadata::TableId::Assembly, 0));
    }

    String const& Assembly::GetPath() const
    {
        return _path;
    }

    Metadata::AssemblyRow Assembly::GetAssemblyRow() const
    {
        VerifyInitialized();

        if (_database->GetTables().GetTable(Metadata::TableId::Assembly).GetRowCount() == 0)
            throw std::runtime_error("wtf");

        return _database->GetRow<Metadata::TableId::Assembly>(0);
    }

}
