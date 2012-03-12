//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Event.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Event::Event()
    {
    }

    Event::Event(Type const& reflectedType, Detail::OwnedEvent const* const ownedEvent, InternalKey)
        : _reflectedType(reflectedType),
          _ownedEvent(ownedEvent)
    {
        Detail::AssertNotNull(ownedEvent);
        Detail::Assert([&]{ return reflectedType.IsInitialized(); });
        Detail::Assert([&]{ return ownedEvent->IsInitialized();   });
    }

    bool Event::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _ownedEvent.Get() != nullptr;
    }

    bool Event::operator!() const
    {
        return !IsInitialized();
    }

    void Event::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    Detail::OwnedEvent const& Event::GetOwnedEvent(InternalKey) const
    {
        AssertInitialized();
        return *_ownedEvent.Get();
    }

    Type Event::GetDeclaringType() const
    {
        AssertInitialized();
        Loader                  const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_ownedEvent.Get()->GetOwningType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _ownedEvent.Get()->GetOwningType().AsRowReference(), InternalKey());
    }

    Type Event::GetReflectedType() const
    {
        AssertInitialized();
        return _reflectedType.Realize();
    }

    bool operator==(Event const& lhs, Event const& rhs)
    {
        return lhs._ownedEvent.Get() == rhs._ownedEvent.Get();
    }

    bool operator<(Event const& lhs, Event const& rhs)
    {
        return std::less<Detail::OwnedEvent const*>()(lhs._ownedEvent.Get(), rhs._ownedEvent.Get());
    }

}
