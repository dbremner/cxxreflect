
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_PARAMETER_HPP_
#define CXXREFLECT_REFLECTION_PARAMETER_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection {

    class parameter
    {
    public:

        parameter();

        parameter(method                 const& declaring_method,
                  detail::parameter_data const& data,
                  core::internal_key);

        parameter(method                   const& declaring_method,
                  metadata::param_token    const& token,
                  metadata::type_signature const& signature,
                  core::internal_key);

        auto self_reference(core::internal_key) const -> metadata::param_token;
        auto self_signature(core::internal_key) const -> metadata::type_signature;

        auto attributes() const -> metadata::parameter_flags;

        auto is_in()       const -> bool;
        auto is_lcid()     const -> bool;
        auto is_optional() const -> bool;
        auto is_out()      const -> bool;
        auto is_ret_val()  const -> bool;

        auto declaring_method() const -> method;

        auto metadata_token() const -> core::size_type;

        auto name() const -> core::string_reference;

        auto parameter_type() const -> type;
        auto position()       const -> core::size_type;
        
        auto default_value() const -> constant;

        auto custom_attributes() const -> detail::custom_attribute_range;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        friend auto operator==(parameter const&, parameter const&) -> bool;
        friend auto operator< (parameter const&, parameter const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(parameter)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(parameter)

    private:

        auto row() const -> metadata::param_row;

        metadata::type_def_or_signature                         _reflected_type;
        core::checked_pointer<detail::method_table_entry const> _method;
        metadata::param_token                                   _parameter;
        metadata::type_signature                                _signature;
    };

} }

#endif 
