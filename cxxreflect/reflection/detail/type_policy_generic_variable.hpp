
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_GENERIC_VARIABLE_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_GENERIC_VARIABLE_HPP_

#include "cxxreflect/reflection/detail/type_policy_specialization.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// \ingroup cxxreflect_reflection_type_policies
    ///
    /// @{





    /// A type policy for generic instance type variables
    class generic_variable_type_policy : public specialization_type_policy
    {
    public:

        virtual auto attributes    (type_def_or_signature_with_module const&) const -> metadata::type_flags;
        virtual auto declaring_type(type_def_or_signature_with_module const&) const -> type_def_or_signature_with_module;

        virtual auto is_generic_parameter(type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_nested           (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_visible          (type_def_or_signature_with_module const&) const -> bool;
        
        virtual auto layout        (type_def_or_signature_with_module const&) const -> type_attribute_layout;
        virtual auto string_format (type_def_or_signature_with_module const&) const -> type_attribute_string_format;
        virtual auto visibility    (type_def_or_signature_with_module const&) const -> type_attribute_visibility;
    };





    /// @}

} } }

#endif

// AMDG //
