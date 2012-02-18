//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
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

    Event::Event(Type const& reflectedType, Detail::EventContext const* const context, InternalKey)
        : _reflectedType(reflectedType),
          _context(context)
    {
        Detail::AssertNotNull(context);
        Detail::Assert([&]{ return reflectedType.IsInitialized(); });
        Detail::Assert([&]{ return context->IsInitialized();      });
    }

    bool Event::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _context.Get() != nullptr;
    }

    bool Event::operator!() const
    {
        return !IsInitialized();
    }

    void Event::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    Detail::EventContext const& Event::GetContext(InternalKey) const
    {
        AssertInitialized();
        return *_context.Get();
    }

    Type Event::GetDeclaringType() const
    {
        AssertInitialized();
        Loader                  const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_context.Get()->GetDeclaringType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _context.Get()->GetDeclaringType().AsRowReference(), InternalKey());
    }

    Type Event::GetReflectedType() const
    {
        AssertInitialized();
        return _reflectedType.Realize();
    }

    bool operator==(Event const& lhs, Event const& rhs)
    {
        return lhs._context.Get() == rhs._context.Get();
    }

    bool operator<(Event const& lhs, Event const& rhs)
    {
        return std::less<Detail::EventContext const*>()(lhs._context.Get(), rhs._context.Get());
    }

}
