
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_ARRAY_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_ARRAY_HPP_

#include "cxxreflect/reflection/detail/type_policy_specialization.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// A base type policy for array type specializations
    class array_type_policy final : public specialization_type_policy
    {
    public:

        virtual auto is_array (unresolved_type_context const&) const -> bool override;
        virtual auto is_nested(unresolved_type_context const&) const -> bool override;



        virtual auto base_type(resolved_type_context const&) const -> unresolved_type_context override;

        virtual auto is_abstract      (resolved_type_context const&) const -> bool override;
        virtual auto is_interface     (resolved_type_context const&) const -> bool override;
        virtual auto is_marshal_by_ref(resolved_type_context const&) const -> bool override;
        virtual auto is_sealed        (resolved_type_context const&) const -> bool override;
        virtual auto is_serializable  (resolved_type_context const&) const -> bool override;
        virtual auto is_value_type    (resolved_type_context const&) const -> bool override;

        virtual auto layout    (resolved_type_context const&) const -> type_layout     override;
        virtual auto visibility(resolved_type_context const&) const -> type_visibility override;
    };

} } }

#endif
