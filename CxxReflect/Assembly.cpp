//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CorEnumIterator.hpp"
#include "CxxReflect/Type.hpp"
#include "CxxReflect/Utility.hpp"

#include <atlbase.h>
#include <cor.h>

namespace CxxReflect {

    Type const* Assembly::GetType(String const& name, bool ignoreCase) const
    {
        int (*compare)(wchar_t const*, wchar_t const*) = ignoreCase ? _wcsicmp : wcscmp;

        auto it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& x)
        {
            return compare(x.GetFullName().c_str(), name.c_str()) == 0;
        }));

        return it != EndTypes() ? &*it : nullptr;
    }

    Type const* Assembly::GetType(MetadataToken token) const
    {
        if (token.GetType() != MetadataTokenType::TypeDef) { return nullptr; } // TODO throw?


        auto it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& x)
        {
            return x.GetMetadataToken() == token;
        }));

        return it != EndTypes() ? &*it : nullptr;
    }

    void Assembly::RealizeName() const
    {
        if (_state.IsSet(RealizedName)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(_import);
        Detail::VerifyNotNull(assemblyImport);

        mdAssembly assemblyToken;
        Detail::ThrowOnFailure(assemblyImport->GetAssemblyFromScope(&assemblyToken));
        _name = Detail::GetAssemblyNameFromToken(assemblyImport, assemblyToken);

        _state.Set(RealizedName);
    }

    void Assembly::RealizeReferencedAssemblies() const
    {
        if (_state.IsSet(RealizedReferencedAssemblies)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(_import);
        Detail::VerifyNotNull(assemblyImport);

        typedef Detail::AssemblyRefIterator Iterator;
        std::transform(Iterator(assemblyImport), Iterator(), std::back_inserter(_referencedAssemblies), [&](mdToken x)
        {
            return Detail::GetAssemblyNameFromToken(assemblyImport, x);
        });

        _state.Set(RealizedReferencedAssemblies);
    }

    void Assembly::RealizeTypes() const
    {
        if (_state.IsSet(RealizedTypes)) { return; }

        using Detail::TypeDefIterator;
        std::vector<mdTypeDef> tokens(TypeDefIterator(_import), (TypeDefIterator()));

        std::sort(tokens.begin(), tokens.end());
        tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());

        _types.Allocate(tokens.size());
        std::for_each(tokens.begin(), tokens.end(), [&](mdToken x)
        {
            _types.EmplaceBack(this, x);
        });

        _state.Set(RealizedTypes);
    }

}
