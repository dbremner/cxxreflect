//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"

using namespace CxxReflect;

namespace {

    PublicKeyToken ComputePublicKeyToken(Metadata::BlobReference const blob, bool const isFullPublicKey)
    {
        PublicKeyToken result;

        if (isFullPublicKey)
        {
            Detail::Sha1Hash const hash(Detail::ComputeSha1Hash(blob.Begin(), blob.End()));
            std::copy(hash.rbegin(), hash.rbegin() + 8, result.begin());
        }
        else
        {
            if (blob.GetSize() != 8)
                throw std::runtime_error("wtf");

            std::copy(blob.Begin(), blob.End(), result.begin());
        }

        return result;
    }

}

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

    AssemblyName Assembly::GetName() const
    {
        Metadata::AssemblyRow const assemblyRow(GetAssemblyRow());

        AssemblyFlags const flags(assemblyRow.GetFlags());

        PublicKeyToken const publicKeyToken(ComputePublicKeyToken(
            assemblyRow.GetPublicKey(),
            flags.IsSet(AssemblyAttribute::PublicKey)));

        return AssemblyName(
            assemblyRow.GetName().c_str(),
            assemblyRow.GetVersion(),
            assemblyRow.GetCulture().c_str(),
            publicKeyToken,
            flags,
            _path);
    }

    Metadata::AssemblyRow Assembly::GetAssemblyRow() const
    {
        VerifyInitialized();

        if (_database->GetTables().GetTable(Metadata::TableId::Assembly).GetRowCount() == 0)
            throw std::runtime_error("wtf");

        return _database->GetRow<Metadata::TableId::Assembly>(0);
    }

}
