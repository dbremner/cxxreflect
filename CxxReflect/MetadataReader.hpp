//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATAREADER_HPP_
#define CXXREFLECT_METADATAREADER_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

#include <functional>
#include <memory>

namespace CxxReflect {

    namespace Detail { class MetadataReaderImpl; }

    typedef std::function<String(AssemblyName)> AssemblyResolutionCallback;

    class MetadataReader
    {
    public:

        MetadataReader();
        MetadataReader(AssemblyResolutionCallback const& resolver);

        ~MetadataReader();

        AssemblyHandle GetAssemblyByPath(String const& path) const;
        AssemblyHandle GetAssemblyByName(AssemblyName const& name) const;

        IMetaDataDispenserEx* UnsafeGetDispenser() const;

    private:

        CXXREFLECT_NONCOPYABLE(MetadataReader);

        std::unique_ptr<Detail::MetadataReaderImpl> _impl;
    };

}

#endif
