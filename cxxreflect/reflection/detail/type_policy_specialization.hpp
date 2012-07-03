
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_SPECIALIZATION_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_SPECIALIZATION_HPP_

#include "cxxreflect/reflection/detail/type_policy.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// \ingroup cxxreflect_reflection_type_policies
    ///
    /// @{





    /// A base type policy for type specializations (TypeSpec types)
    ///
    /// This implementation is straightforward:  it simply resolves the primary TypeDef from the
    /// TypeSpec and calls the `definition_type_policy` implementation of the correct function to
    /// get the result.  This class should not be instantiated directly; there are several derived
    /// classes (e.g. `array_type_policy` and `pointer_type_policy`) that should be instantiated for
    /// the specific kind of TypeSpec.
    ///
    /// Some of the `is_*` functions simply return `false` because they either will never be `true`
    /// (as is the case for `is_primitive`) or will only be `true` in a derived class that will
    /// override the method declared here (as is the case, e.g., for `is_array` or `is_pointer`).
    ///
    /// This implementation is, however, concrete.  It implements all of the abstract virtual member
    /// functions declared by the base `type_policy`.
    ///
    /// \todo This behavior is probably incorrect; we should walk each level of the nested type
    ///       specialization to check each layer.  E.g., for a ByRef GenericInst, we should first
    ///       check the ByRef, then the GenericInst, then the primary TypeDef!
    class specialization_type_policy : public type_policy
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

// AMDG //
