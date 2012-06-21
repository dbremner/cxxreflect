
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_GENERIC_INSTANCE_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_GENERIC_INSTANCE_HPP_

#include "cxxreflect/reflection/detail/type_policy_specialization.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    class generic_instance_type_policy : public specialization_type_policy
    {
    public:

        virtual auto is_generic_type(type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_visible     (type_def_or_signature_with_module const&) const -> bool;
    };

} } }

#endif

// AMDG //
