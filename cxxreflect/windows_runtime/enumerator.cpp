
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/enumerator.hpp"

namespace cxxreflect { namespace windows_runtime {

    enumerator::enumerator()
    {
    }

    enumerator::enumerator(core::string_reference const name, unsigned_type const value)
        : _name(name), _value(value)
    {
    }

    auto enumerator::name() const -> core::string_reference
    {
        return _name;
    }

    auto enumerator::signed_value() const -> signed_type
    {
        return *reinterpret_cast<signed_type const*>(&_value.get());
    }

    auto enumerator::unsigned_value() const -> unsigned_type
    {
        return _value.get();
    }





    auto enumerator_name_less_than::operator()(enumerator const& lhs, enumerator const& rhs) const -> bool
    {
        return lhs.name() < rhs.name();
    }

    auto enumerator_signed_value_less_than::operator()(enumerator const& lhs, enumerator const& rhs) const -> bool
    {
        return lhs.signed_value() < rhs.signed_value();
    }

    auto enumerator_unsigned_value_less_than::operator()(enumerator const& lhs, enumerator const& rhs) const -> bool
    {
        return lhs.unsigned_value() < rhs.unsigned_value();
    }

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION

// AMDG //
