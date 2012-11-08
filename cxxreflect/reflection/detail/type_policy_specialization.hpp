
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_SPECIALIZATION_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_SPECIALIZATION_HPP_

#include "cxxreflect/reflection/detail/type_policy.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// A base type policy for type specializations (signatures)
    class specialization_type_policy : public type_policy
    {
    public:

        virtual auto is_array                     (unresolved_type_context const&) const -> bool override;
        virtual auto is_by_ref                    (unresolved_type_context const&) const -> bool override;
        virtual auto is_generic_type_instantiation(unresolved_type_context const&) const -> bool override;
        virtual auto is_nested                    (unresolved_type_context const&) const -> bool override;
        virtual auto is_pointer                   (unresolved_type_context const&) const -> bool override;
        virtual auto is_primitive                 (unresolved_type_context const&) const -> bool override final;

        virtual auto namespace_name(unresolved_type_context const&) const -> core::string_reference override;
        virtual auto primary_name  (unresolved_type_context const&) const -> core::string_reference override;

        virtual auto declaring_type(unresolved_type_context const&) const -> unresolved_type_context override;



        virtual auto attributes(resolved_type_context const&) const -> metadata::type_flags    override;
        virtual auto base_type (resolved_type_context const&) const -> unresolved_type_context override;

        virtual auto is_abstract               (resolved_type_context const&) const -> bool override;
        virtual auto is_com_object             (resolved_type_context const&) const -> bool override;
        virtual auto is_contextful             (resolved_type_context const&) const -> bool override;
        virtual auto is_enum                   (resolved_type_context const&) const -> bool override;
        virtual auto is_generic_parameter      (resolved_type_context const&) const -> bool override;
        virtual auto is_generic_type           (resolved_type_context const&) const -> bool override;
        virtual auto is_generic_type_definition(resolved_type_context const&) const -> bool override final;
        virtual auto is_import                 (resolved_type_context const&) const -> bool override;
        virtual auto is_interface              (resolved_type_context const&) const -> bool override;
        virtual auto is_marshal_by_ref         (resolved_type_context const&) const -> bool override;
        virtual auto is_sealed                 (resolved_type_context const&) const -> bool override;
        virtual auto is_serializable           (resolved_type_context const&) const -> bool override;
        virtual auto is_special_name           (resolved_type_context const&) const -> bool override;
        virtual auto is_value_type             (resolved_type_context const&) const -> bool override;
        virtual auto is_visible                (resolved_type_context const&) const -> bool override;

        virtual auto layout        (resolved_type_context const&) const -> type_layout        override;
        virtual auto metadata_token(resolved_type_context const&) const -> core::size_type    override;
        virtual auto string_format (resolved_type_context const&) const -> type_string_format override;
        virtual auto visibility    (resolved_type_context const&) const -> type_visibility    override;
    };

} } }

#endif
