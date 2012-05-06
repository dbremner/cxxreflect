
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





    Enumerator::Enumerator()
    {
    }

    Enumerator::Enumerator(StringReference const name, std::uint64_t const value)
        : _name(name), _value(value)
    {
    }

    StringReference Enumerator::GetName() const
    {
        return _name;
    }

    std::int64_t Enumerator::GetValueAsInt64() const
    {
        return static_cast<std::int64_t>(_value.Get());
    }

    std::uint64_t Enumerator::GetValueAsUInt64() const
    {
        return _value.Get();
    }

    bool EnumeratorSignedValueOrdering::operator()(Enumerator const& lhs, Enumerator const& rhs) const
    {
        return lhs.GetValueAsInt64() < rhs.GetValueAsInt64();
    }

    bool EnumeratorUnsignedValueOrdering::operator()(Enumerator const& lhs, Enumerator const& rhs) const
    {
        return lhs.GetValueAsUInt64() < rhs.GetValueAsUInt64();
    }

    bool EnumeratorNameOrdering::operator()(Enumerator const& lhs, Enumerator const& rhs) const
    {
        return lhs.GetName() < rhs.GetName();
    }

} }
