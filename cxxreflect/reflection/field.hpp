
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_FIELD_HPP_
#define CXXREFLECT_REFLECTION_FIELD_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/independent_handles.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"

namespace cxxreflect { namespace reflection {

    class field
    {
    public:

        typedef void /* TODO */ optional_custom_modifier_iterator;
        typedef void /* TODO */ required_custom_modifier_iterator;

        field();

        auto declaring_type()   const -> type;
        auto reflected_type()   const -> type;

        auto attributes()       const -> metadata::field_flags;
        auto field_type()       const -> type;
        auto declaring_module() const -> module;

        auto is_assembly()            const -> bool;
        auto is_family()              const -> bool;
        auto is_family_and_assembly() const -> bool;
        auto is_family_or_assembly()  const -> bool;
        auto is_init_only()           const -> bool;
        auto is_literal()             const -> bool;
        auto is_not_serialized()      const -> bool;
        auto is_pinvoke_impl()        const -> bool;
        auto is_private()             const -> bool;
        auto is_public()              const -> bool;
        auto is_special_name()        const -> bool;
        auto is_static()              const -> bool;

        auto metadata_token() const -> core::size_type;
        auto constant_value() const -> constant;

        auto name() const -> core::string_reference;

        auto begin_custom_attributes() const -> custom_attribute_iterator;
        auto end_custom_attributes()   const -> custom_attribute_iterator;

        auto begin_optional_custom_modifiers() const -> optional_custom_modifier_iterator;
        auto end_optional_custom_modifiers()   const -> optional_custom_modifier_iterator;

        auto begin_required_custom_modifiers() const -> required_custom_modifier_iterator;
        auto end_required_custom_modifiers()   const -> required_custom_modifier_iterator;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;
        
        // -- The following members of System.Reflection.FieldInfo are not implemented --
        // FieldHandle
        // GetValue()             N/A in reflection only
        // GetValueDirect()       N/A in reflection only
        // IsDefined()
        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent
        // MemberType
        // SetValue()             N/A in reflection only
        // SetValueDirect()       N/A in reflection only

        friend auto operator==(field const&, field const&) -> bool;
        friend auto operator< (field const&, field const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(field)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(field)

    public: // internal members

        field(type const& reflected_type, detail::field_context const* context, core::internal_key);

        auto context(core::internal_key) const -> detail::field_context const&;

    private:

        auto row() const -> metadata::field_row;

        detail::type_handle                                   _reflected_type;
        core::value_initialized<detail::field_context const*> _context;
    };

} }

#endif 

// AMDG //
