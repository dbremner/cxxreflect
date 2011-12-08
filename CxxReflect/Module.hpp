//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_MODULE_HPP_
#define CXXREFLECT_MODULE_HPP_

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataDatabase.hpp"

namespace CxxReflect {

    class Module
    {
    public:

        Module()
            : _assembly(), _type()
        {
        }

        Module(Assembly const assembly, Metadata::RowReference const module)
            : _assembly(assembly), _module(module)
        {
            Detail::Verify([&] { return assembly.IsInitialized();               });
            Detail::Verify([&] { return module.GetIndex() != std::uint32_t(-1); });
        }

        Assembly GetAssembly() const { return _assembly; }

        // TODO This interface is very incomplete

        bool IsInitialized() const { return _assembly.IsInitialized(); }

    private:

        void VerifyInitialized() const
        {
            Detail::Verify([&] { return IsInitialized(); }, "Type is not initialized");
        }

        Assembly               _assembly;
        Metadata::RowReference _module;
    };

}

#endif
