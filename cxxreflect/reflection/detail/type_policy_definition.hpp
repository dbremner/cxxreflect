
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_DEFINITION_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_DEFINITION_HPP_

#include "cxxreflect/reflection/detail/type_policy.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// \ingroup cxxreflect_reflection_type_policies
    ///
    /// @{





    /// A type policy for type definitions (TypeDef types)
    ///
    /// There is only one type policy for type definitions; there are no policies derived from this
    /// one and it is intended to be instantiated directly.  Because the TypeDef is the fundamental
    /// unit of the type metadata, most of the core functionality is contained in this policy's
    /// implementation, and many of the TypeSpec type policies defer back to this policy eventually.
    class definition_type_policy : public type_policy
    {
    public:

        virtual auto attributes    (type_def_or_signature_with_module const&) const -> metadata::type_flags;
        virtual auto base_type     (type_def_or_signature_with_module const&) const -> type_def_or_signature_with_module;
        virtual auto declaring_type(type_def_or_signature_with_module const&) const -> type_def_or_signature_with_module;

        virtual auto is_abstract               (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_array                  (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_by_ref                 (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_com_object             (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_contextful             (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_enum                   (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_generic_parameter      (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_generic_type           (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_generic_type_definition(type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_import                 (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_interface              (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_marshal_by_ref         (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_nested                 (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_pointer                (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_primitive              (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_sealed                 (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_serializable           (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_special_name           (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_value_type             (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_visible                (type_def_or_signature_with_module const&) const -> bool;

        virtual auto layout        (type_def_or_signature_with_module const&) const -> type_attribute_layout;
        virtual auto metadata_token(type_def_or_signature_with_module const&) const -> core::size_type;
        virtual auto namespace_name(type_def_or_signature_with_module const&) const -> core::string_reference;
        virtual auto string_format (type_def_or_signature_with_module const&) const -> type_attribute_string_format;
        virtual auto visibility    (type_def_or_signature_with_module const&) const -> type_attribute_visibility;
    };





    /// @}

} } }

#endif
