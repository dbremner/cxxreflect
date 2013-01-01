
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_REFERENCE_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_REFERENCE_HPP_

#include "cxxreflect/reflection/detail/type_policy.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    /// A type policy for type references (TypeRef tokens)
    class reference_type_policy final : public type_policy
    {
    public:

        virtual auto is_array                     (unresolved_type_context const&) const -> bool override;
        virtual auto is_by_ref                    (unresolved_type_context const&) const -> bool override;
        virtual auto is_generic_type_instantiation(unresolved_type_context const&) const -> bool override;
        virtual auto is_nested                    (unresolved_type_context const&) const -> bool override;
        virtual auto is_pointer                   (unresolved_type_context const&) const -> bool override;
        virtual auto is_primitive                 (unresolved_type_context const&) const -> bool override;

        virtual auto namespace_name(unresolved_type_context const&) const -> core::string_reference override;
        virtual auto primary_name  (unresolved_type_context const&) const -> core::string_reference override;

        virtual auto declaring_type(unresolved_type_context const&) const -> unresolved_type_context override;
    };

} } }

#endif
