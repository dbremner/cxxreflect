
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/type_resolution.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    class type_policy
    {
    public:

        typedef metadata::type_def_or_signature     resolved_type_context;
        typedef metadata::type_def_ref_or_signature unresolved_type_context;

        static auto get_for(unresolved_type_context const&) -> type_policy const&;

        // These functions can test properties of an unresolved type.  A TypeRef always refers to
        // some TypeSpec, so properties that can only appear in a signature are never true for a
        // TypeRef.  A TypeRef includes its namespace name, simple name, and declaring type (if it
        // has one), so we can return these for unresolved types as well.

        virtual auto is_array                     (unresolved_type_context const&) const -> bool;
        virtual auto is_by_ref                    (unresolved_type_context const&) const -> bool;
        virtual auto is_generic_type_instantiation(unresolved_type_context const&) const -> bool;
        virtual auto is_nested                    (unresolved_type_context const&) const -> bool;
        virtual auto is_pointer                   (unresolved_type_context const&) const -> bool;
        virtual auto is_primitive                 (unresolved_type_context const&) const -> bool;

        virtual auto namespace_name(unresolved_type_context const&) const -> core::string_reference;
        virtual auto primary_name  (unresolved_type_context const&) const -> core::string_reference;

        virtual auto declaring_type(unresolved_type_context const&) const -> unresolved_type_context;

        // These functions require a resolved type.  These are either properties defined only for a
        // TypeDef or are properties for which the TypeDef must be resolved in order to compute the
        // value.

        virtual auto attributes(resolved_type_context const&) const -> metadata::type_flags;
        virtual auto base_type (resolved_type_context const&) const -> unresolved_type_context;

        virtual auto is_abstract               (resolved_type_context const&) const -> bool;
        virtual auto is_com_object             (resolved_type_context const&) const -> bool;
        virtual auto is_contextful             (resolved_type_context const&) const -> bool;
        virtual auto is_enum                   (resolved_type_context const&) const -> bool;
        virtual auto is_generic_parameter      (resolved_type_context const&) const -> bool;
        virtual auto is_generic_type           (resolved_type_context const&) const -> bool;
        virtual auto is_generic_type_definition(resolved_type_context const&) const -> bool;
        virtual auto is_import                 (resolved_type_context const&) const -> bool;
        virtual auto is_interface              (resolved_type_context const&) const -> bool;
        virtual auto is_marshal_by_ref         (resolved_type_context const&) const -> bool;
        virtual auto is_sealed                 (resolved_type_context const&) const -> bool;
        virtual auto is_serializable           (resolved_type_context const&) const -> bool;
        virtual auto is_special_name           (resolved_type_context const&) const -> bool;
        virtual auto is_value_type             (resolved_type_context const&) const -> bool;
        virtual auto is_visible                (resolved_type_context const&) const -> bool;

        virtual auto layout        (resolved_type_context const&) const -> type_layout;
        virtual auto metadata_token(resolved_type_context const&) const -> core::size_type;
        virtual auto string_format (resolved_type_context const&) const -> type_string_format;
        virtual auto visibility    (resolved_type_context const&) const -> type_visibility;

        virtual ~type_policy();

    protected:

        /// Computes a type to its primary type definition and calls a function on its type policy
        template <typename MemberFunction>
        static auto compute_primary_type_and_call(unresolved_type_context const& t, MemberFunction const f)
            -> decltype((std::declval<type_policy>().*f)(t))
        {
            unresolved_type_context const type_def(compute_primary_type(t));
            if (!type_def.is_initialized())
                return decltype((std::declval<type_policy>().*f)(t))();

            return (type_policy::get_for(type_def).*f)(type_def);
        }

        /// Computes a type to its next nested element type and calls a function on its type policy
        template <typename MemberFunction>
        static auto compute_element_type_and_call(unresolved_type_context const& t, MemberFunction const f)
            -> decltype((std::declval<type_policy>().*f)(t))
        {
            unresolved_type_context const element_type(compute_element_type(t));
            if (!element_type.is_initialized())
                return decltype((std::declval<type_policy>().*f)(t))();

            return (type_policy::get_for(element_type).*f)(element_type);
        }

        /// Resolves a type to its primary type definition and calls a function on its type policy
        template <typename MemberFunction>
        static auto resolve_primary_type_and_call(resolved_type_context const& t, MemberFunction const f)
            -> decltype((std::declval<type_policy>().*f)(t))
        {
            resolved_type_context const type_def(resolve_primary_type(t));
            if (!type_def.is_initialized())
                return decltype((std::declval<type_policy>().*f)(t))();

            return (type_policy::get_for(type_def).*f)(type_def);
        }

        /// Resolves a type to its next nested element type and calls a function on its type policy
        template <typename MemberFunction>
        static auto resolve_element_type_and_call(resolved_type_context const& t, MemberFunction const f)
            -> decltype((std::declval<type_policy>().*f)(t))
        {
            resolved_type_context const element_type(resolve_element_type(t));
            if (!element_type.is_initialized())
                return decltype((std::declval<type_policy>().*f)(t))();

            return (type_policy::get_for(element_type).*f)(element_type);
        }
    };

} } }

#endif
