
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_ENUMERATOR_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_ENUMERATOR_HPP_

#include "cxxreflect/windows_runtime/common.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace cxxreflect { namespace windows_runtime {

    class enumerator
    {
    public:

        typedef std::int32_t  signed_type;
        typedef std::uint32_t unsigned_type;

        enumerator();
        enumerator(core::string_reference name, unsigned_type value);

        auto name() const -> core::string_reference;

        auto signed_value()   const -> signed_type;
        auto unsigned_value() const -> unsigned_type;

    private:

        core::string_reference                 _name;
        core::value_initialized<unsigned_type> _value;
    };

    class enumerator_name_less_than
    {
    public:

        auto operator()(enumerator const& lhs, enumerator const& rhs) const -> bool;
    };

    class enumerator_signed_value_less_than
    {
    public:

        auto operator()(enumerator const& lhs, enumerator const& rhs) const -> bool;
    };

    class enumerator_unsigned_value_less_than
    {
    public:

        auto operator()(enumerator const& lhs, enumerator const& rhs) const -> bool;
    };

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 
