#ifndef CXXREFLECT_EVENT_HPP_
#define CXXREFLECT_EVENT_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect {

    class Event
    {
    public:

        Event();
        Event(Type const& reflectedType, Detail::OwnedEvent const* ownedEvent, InternalKey);

        Type GetDeclaringType() const;
        Type GetReflectedType() const;

        bool IsInitialized() const;
        bool operator!()     const;

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

        friend bool operator==(Event const& lhs, Event const& rhs);
        friend bool operator< (Event const& lhs, Event const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Event)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Event)

    public: // Internal members

        Detail::OwnedEvent const& GetOwnedEvent(InternalKey) const;

    private:

        void AssertInitialized() const;

        Metadata::EventRow GetEventRow() const;

        Detail::TypeHandle                                  _reflectedType;
        Detail::ValueInitialized<Detail::OwnedEvent const*> _ownedEvent;

    };

}

#endif
