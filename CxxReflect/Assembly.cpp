//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CorEnumIterator.hpp"
#include "CxxReflect/Type.hpp"
#include "CxxReflect/Utility.hpp"

#include <atlbase.h>
#include <cor.h>

using CxxReflect::Utility::DebugVerifyNotNull;
using CxxReflect::Utility::ThrowOnFailure;

namespace CxxReflect {

    Assembly::Assembly(MetadataReaderHandle reader, String const& path, IMetaDataImport2* import)
        : _reader(reader), _path(path), _import(import)
    {
        DebugVerifyNotNull(_reader);
        DebugVerifyNotNull(_import);

        // We avoid using CComPtr so that we don't need to have IMetaDataImport defined in the header.
        _import->AddRef();
    }

    Assembly::~Assembly()
    {
        _import->Release();
    }

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
        switch (token.GetKind())
        {
        case MetadataTokenKind::TypeDef:
        {
            auto it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& x)
            {
                return x.GetMetadataToken() == token;
            }));

            return it != EndTypes() ? &*it : nullptr;
        }
        
        case MetadataTokenKind::TypeSpec:
        {
            auto const& typeSpecs(PrivateGetTypeSpecs());
            auto it(std::find_if(typeSpecs.Begin(), typeSpecs.End(), [&](Type const& x)
            {
                return x.GetMetadataToken() == token;
            }));

            return it != typeSpecs.End() ? &*it : nullptr;
        }
        
        default:
            throw std::logic_error("wtf");
        }
    }

    void Assembly::RealizeName() const
    {
        if (_state.IsSet(RealizedName)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(_import);
        DebugVerifyNotNull(assemblyImport);

        mdAssembly assemblyToken;
        ThrowOnFailure(assemblyImport->GetAssemblyFromScope(&assemblyToken));
        _name = Utility::GetAssemblyNameFromToken(assemblyImport, assemblyToken);

        _state.Set(RealizedName);
    }

    void Assembly::RealizeReferencedAssemblies() const
    {
        if (_state.IsSet(RealizedReferencedAssemblies)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(_import);
        DebugVerifyNotNull(assemblyImport);

        typedef Detail::AssemblyRefIterator Iterator;
        std::transform(Iterator(assemblyImport), Iterator(), std::back_inserter(_referencedAssemblies), [&](mdToken x)
        {
            return Utility::GetAssemblyNameFromToken(assemblyImport, x);
        });

        _state.Set(RealizedReferencedAssemblies);
    }

    void Assembly::RealizeTypeDefs() const
    {
        if (_state.IsSet(RealizedTypeDefs)) { return; }

        using Detail::TypeDefIterator;
        std::vector<mdTypeDef> tokens(TypeDefIterator(_import), (TypeDefIterator()));

        std::sort(tokens.begin(), tokens.end());
        tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());

        _typeDefs.Allocate(tokens.size());
        std::for_each(tokens.begin(), tokens.end(), [&](mdTypeDef x)
        {
            _typeDefs.EmplaceBack(this, x);
        });

        _state.Set(RealizedTypeDefs);
    }

    void Assembly::RealizeTypeSpecs() const
    {
        if (_state.IsSet(RealizedTypeSpecs)) { return; }

        using Detail::TypeSpecIterator;
        std::vector<mdTypeSpec> tokens(TypeSpecIterator(_import), (TypeSpecIterator()));

        std::sort(tokens.begin(), tokens.end());
        tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());

        _typeSpecs.Allocate(tokens.size());
        std::for_each(tokens.begin(), tokens.end(), [&](mdTypeSpec x)
        {
            _typeSpecs.EmplaceBack(this, x);
        });

        _state.Set(RealizedTypeSpecs);
    }

    void Assembly::RealizeCustomAttributes() const
    {
        if (_state.IsSet(RealizedCustomAttributes)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(_import);
        DebugVerifyNotNull(assemblyImport);

        mdAssembly assemblyToken(0);
        ThrowOnFailure(assemblyImport->GetAssemblyFromScope(&assemblyToken));

        typedef  Detail::CustomAttributeIterator Iterator;
        std::vector<mdCustomAttribute> tokens(Iterator(_import, assemblyToken), (Iterator()));

        std::sort(tokens.begin(), tokens.end());
        tokens.erase(std::unique(tokens.begin(), tokens.end()), tokens.end());

        _customAttributes.Allocate(tokens.size());
        std::for_each(tokens.begin(), tokens.end(), [&](mdCustomAttribute x)
        {
            _customAttributes.EmplaceBack(this, x);
        });

        _state.Set(RealizedCustomAttributes);
    }

}
