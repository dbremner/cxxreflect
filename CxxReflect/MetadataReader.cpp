//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/MetadataReader.hpp"
#include "CxxReflect/Utility.hpp"

#include <atlbase.h>
#include <cor.h>

#include <map>
#include <vector>

using CxxReflect::Utility::ThrowOnFailure;
using CxxReflect::Utility::DebugVerifyNotNull;

namespace CxxReflect { namespace Detail {

    // Private implementation details of MetadataReader
    class MetadataReaderImpl
    {
    public:

        MetadataReaderImpl(MetadataReader const* owner, AssemblyResolutionCallback const& resolver)
            : _owner(owner), _resolver(resolver)
        {
            DebugVerifyNotNull(resolver);

            ThrowOnFailure(CoCreateInstance(
                CLSID_CorMetaDataDispenser,
                0,
                CLSCTX_INPROC_SERVER,
                IID_IMetaDataDispenserEx,
                reinterpret_cast<void**>(&_dispenser)));
        }

        Assembly const* GetAssemblyByPath(String const& path) const
        {
            auto const it(_assemblies.find(path));
            if (it != _assemblies.end())
            {
                return it->second.get();
            }

            CComPtr<IMetaDataImport2> import;
            ThrowOnFailure(_dispenser->OpenScope(
                path.c_str(),
                ofReadOnly,
                IID_IMetaDataImport2,
                reinterpret_cast<IUnknown**>(&import)));

            std::unique_ptr<Assembly> newAssembly(new Assembly(_owner, path, import));
            auto const result(_assemblies.insert(std::make_pair(path, std::move(newAssembly))));
            return result.first->second.get();
        }

        Assembly const* GetAssemblyByName(AssemblyName const& name) const
        {
            return GetAssemblyByPath(_resolver(name));
        }

        IMetaDataDispenserEx* UnsafeGetDispenser() const
        {
            return _dispenser;
        }

    private:
    
        MetadataReader const*                               _owner;
        AssemblyResolutionCallback                          _resolver;
        CComPtr<IMetaDataDispenserEx>                       _dispenser;
        mutable std::map<String, std::unique_ptr<Assembly>> _assemblies;
    };

} }

namespace CxxReflect {

    MetadataReader::MetadataReader(AssemblyResolutionCallback const& resolver)
        : _impl(new Detail::MetadataReaderImpl(this, resolver))
    {
    }

    MetadataReader::~MetadataReader()
    {
        // Explicitly defined so that our std::unique_ptr's destructor is instantiated here where we
        // have the definition of Detail::MetadataReaderImpl.
    }

    Assembly const* MetadataReader::GetAssemblyByPath(String const& path) const
    {
        return _impl->GetAssemblyByPath(path);
    }

    Assembly const* MetadataReader::GetAssemblyByName(AssemblyName const& name) const
    {
        return _impl->GetAssemblyByName(name);
    }

    IMetaDataDispenserEx* MetadataReader::UnsafeGetDispenser() const
    {
        return _impl->UnsafeGetDispenser();
    }

}
