
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_ARRAY_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_ARRAY_HPP_

#include "cxxreflect/reflection/detail/type_policy_specialization.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// \ingroup cxxreflect_reflection_type_policies
    ///
    /// @{





    /// A type policy for array type specializations (TypeSpec simple and general types)
    class array_type_policy : public specialization_type_policy
    {
    public:

        virtual auto base_type(type_def_or_signature_with_module const&) const -> type_def_or_signature_with_module;

        virtual auto is_abstract      (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_array         (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_interface     (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_marshal_by_ref(type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_nested        (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_sealed        (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_serializable  (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_value_type    (type_def_or_signature_with_module const&) const -> bool;

        virtual auto layout    (type_def_or_signature_with_module const&) const -> type_attribute_layout;
        virtual auto visibility(type_def_or_signature_with_module const&) const -> type_attribute_visibility;
    };





    /// @}

} } }

#endif
