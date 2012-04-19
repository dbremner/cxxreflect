
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/WindowsRuntimeCommon.hpp"

#include <inspectable.h>

namespace CxxReflect { namespace WindowsRuntime {

    void InspectableDeleter::operator()(IInspectable* inspectable)
    {
        if (inspectable)
            inspectable->Release();
    }

} }
