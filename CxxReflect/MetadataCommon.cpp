//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/MetadataCommon.hpp"

namespace CxxReflect { namespace Metadata {

    ITypeResolver::~ITypeResolver()
    {
        // Interface class requires virtual destructor.  No cleanup is required.
    }

} }
