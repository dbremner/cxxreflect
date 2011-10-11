//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Utility.hpp"

namespace CxxReflect {

    CustomAttribute::CustomAttribute(Assembly const* assembly, MetadataToken token)
        : _assembly(assembly), _token(token)
    {
        Detail::VerifyNotNull(_assembly);
    }

    void CustomAttribute::RealizeConstructor() const
    {
        if (_state.IsSet(RealizedConstructor)) { return; }

        // TODO

        _state.Set(RealizedConstructor);
    }

    void CustomAttribute::RealizeArguments() const
    {
        if (_state.IsSet(RealizedArguments)) { return; }

        // TODO

        _state.Set(RealizedArguments);
    }

}
