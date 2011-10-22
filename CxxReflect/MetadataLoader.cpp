//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/MetadataLoader.hpp"

namespace CxxReflect {

    IMetadataResolver::~IMetadataResolver()
    {
    }

    MetadataLoader::MetadataLoader(std::unique_ptr<IMetadataResolver> resolver)
        : _resolver(std::move(resolver))
    {
    }

    Assembly MetadataLoader::LoadAssembly(String const& path) const
    {
        // TODO PATH NORMALIZATION?
        auto it(_databases.find(path));
        if (it == _databases.end())
            it = _databases.insert(std::make_pair(path, Metadata::Database(path.c_str()))).first;

        return Assembly(it->first.c_str(), this, &it->second);
    }

}
