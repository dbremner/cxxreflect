
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_UNKNOWN_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_UNKNOWN_HPP_

#include "cxxreflect/reflection/detail/type_policy.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    // TODO REMOVE THIS CLASS
    //
    // This is a temporary type policy being used while we refactor the logic into separate classes
    class unknown_type_policy : public type_policy
    {
    public:

        virtual auto base_type     (type_def_or_signature_with_module const&) const -> type_def_or_signature_with_module;
        virtual auto declaring_type(type_def_or_signature_with_module const&) const -> type_def_or_signature_with_module;

        virtual auto layout       (type_def_or_signature_with_module const&) const -> type_attribute_layout;
        virtual auto string_format(type_def_or_signature_with_module const&) const -> type_attribute_string_format;
        virtual auto visibility   (type_def_or_signature_with_module const&) const -> type_attribute_visibility;
        
        virtual auto is_abstract(type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_sealed  (type_def_or_signature_with_module const&) const -> bool;
        
        virtual auto is_com_object    (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_contextful    (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_enum          (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_interface     (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_marshal_by_ref(type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_primitive     (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_value_type    (type_def_or_signature_with_module const&) const -> bool;

        virtual auto is_generic_parameter      (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_generic_type           (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_generic_type_definition(type_def_or_signature_with_module const&) const -> bool;

        virtual auto is_array  (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_by_ref (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_pointer(type_def_or_signature_with_module const&) const -> bool;

        virtual auto is_import      (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_nested      (type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_serializable(type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_special_name(type_def_or_signature_with_module const&) const -> bool;
        virtual auto is_visible     (type_def_or_signature_with_module const&) const -> bool;
    };

} } }

#endif

// AMDG //
