
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_METHOD_HPP_
#define CXXREFLECT_REFLECTION_METHOD_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"

namespace cxxreflect { namespace reflection {

    class method
    {
    public:

        typedef core::instantiating_iterator<
            detail::parameter_data,
            parameter,
            method,
            core::internal_constructor_forwarder<parameter>,
            core::identity_transformer,
            std::forward_iterator_tag
        > parameter_iterator;

        typedef core::iterator_range<parameter_iterator> parameter_range;

        method();

        auto declaring_type()   const -> type;
        auto reflected_type()   const -> type;
        auto declaring_module() const -> module;

        auto contains_generic_parameters() const -> bool;
        auto attributes()                  const -> metadata::method_flags;
        auto calling_convention()          const -> metadata::calling_convention;
        auto metadata_token()              const -> core::size_type;
        auto name()                        const -> core::string_reference;

        auto is_abstract()                  const -> bool;
        auto is_assembly()                  const -> bool;
        auto is_constructor()               const -> bool;
        auto is_family()                    const -> bool;
        auto is_family_and_assembly()       const -> bool;
        auto is_family_or_assembly()        const -> bool;
        auto is_final()                     const -> bool;
        auto is_generic_method()            const -> bool;
        auto is_generic_method_definition() const -> bool;
        auto is_hide_by_signature()         const -> bool;
        auto is_private()                   const -> bool;
        auto is_public()                    const -> bool;
        auto is_special_name()              const -> bool;
        auto is_static()                    const -> bool;
        auto is_virtual()                   const -> bool;

        auto is_initialized()               const -> bool;
        auto operator!()                    const -> bool;

        auto custom_attributes() const -> detail::custom_attribute_range;
        auto parameters()        const -> parameter_range;
        auto parameter_count()   const -> core::size_type;

        auto return_parameter() const -> parameter;
        auto return_type()      const -> type;

        // GetBaseDefinition          -- Non-constructor only
        // GetGenericArguments
        // GetGenericMethodDefinition -- Non-constructor only
        // GetMethodBody
        // GetMethodImplementationFlags
        
        // IsDefined
        // MakeGenericMethod          -- Non-constructor only

        // -- The following members of System.Reflection.MethodInfo are not implemented --
        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent
        // MemberType
        // MethodHandle
        //
        // Invoke()

        friend auto operator==(method const&, method const&) -> bool;
        friend auto operator< (method const&, method const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(method)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(method)

    public: // Internal Members

        method(type const& reflected_type, detail::method_table_entry const* context, core::internal_key);

        auto context(core::internal_key) const -> detail::method_table_entry const&;

    private:

        auto row() const -> metadata::method_def_row;

        metadata::type_def_or_signature                         _reflected_type;
        core::checked_pointer<detail::method_table_entry const> _context;
    };

} }

#endif 
