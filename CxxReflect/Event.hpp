//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_EVENT_HPP_
#define CXXREFLECT_EVENT_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

namespace CxxReflect {

    class Event
    {
    public:

        // Attributes
        // DeclaringType
        // EventHandlerType
        // IsMulticast
        // IsSpecialName
        // MemberType
        // MetadataToken
        // Module
        // Name
        // ReflectedType

        // AddEventHandler
        // GetAddMethod
        // GetCustomAttributes
        // GetOtherMethods
        // GetRaiseMethod
        // GetRemoveMethod
        // IsDefined
        // RemoveEventHandler

    private:

        CXXREFLECT_NONCOPYABLE(Event);

    };

    bool operator==(Event const& lhs, Event const& rhs); // TODO
    bool operator< (Event const& lhs, Event const& rhs); // TODO

    inline bool operator!=(Event const& lhs, Event const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Event const& lhs, Event const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Event const& lhs, Event const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Event const& lhs, Event const& rhs) { return !(rhs <  lhs); }

}

#endif
