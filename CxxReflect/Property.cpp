//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Property::Property()
    {
    }

    Property::Property(Type const& reflectedType, Detail::OwnedProperty const* const ownedProperty, InternalKey)
        : _reflectedType(reflectedType),
          _ownedProperty(ownedProperty)
    {
        Detail::AssertNotNull(ownedProperty);
        Detail::Assert([&]{ return reflectedType.IsInitialized();  });
        Detail::Assert([&]{ return ownedProperty->IsInitialized(); });
    }

    bool Property::IsInitialized() const
    {
        return _reflectedType.IsInitialized() && _ownedProperty.Get() != nullptr;
    }

    bool Property::operator!() const
    {
        return !IsInitialized();
    }

    void Property::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    Detail::OwnedProperty const& Property::GetOwnedProperty(InternalKey) const
    {
        AssertInitialized();
        return *_ownedProperty.Get();
    }

    Type Property::GetDeclaringType() const
    {
        AssertInitialized();
        Loader                  const& loader  (_reflectedType.Realize().GetAssembly().GetContext(InternalKey()).GetLoader());
        Metadata::Database      const& database(_ownedProperty.Get()->GetOwningType().GetDatabase());
        Detail::AssemblyContext const& context (loader.GetContextForDatabase(database, InternalKey()));
        Assembly                const  assembly(&context, InternalKey());

        return Type(assembly, _ownedProperty.Get()->GetOwningType().AsRowReference(), InternalKey());
    }

    Type Property::GetReflectedType() const
    {
        AssertInitialized();
        return _reflectedType.Realize();
    }

    bool operator==(Property const& lhs, Property const& rhs)
    {
        return lhs._ownedProperty.Get() == rhs._ownedProperty.Get();
    }

    bool operator<(Property const& lhs, Property const& rhs)
    {
        return std::less<Detail::OwnedProperty const*>()(lhs._ownedProperty.Get(), rhs._ownedProperty.Get());
    }

}
