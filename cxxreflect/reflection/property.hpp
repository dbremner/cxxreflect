
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_PROPERTY_HPP_
#define CXXREFLECT_REFLECTION_PROPERTY_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"

namespace cxxreflect { namespace reflection {

    class property
    {
    public:

        // TODO typedef void accessor_iterator;
        // TODO typedef void parameter_iterator;

        // TODO typedef core::iterator_range<accessor_iterator > accessor_range;
        // TODO typedef core::iterator_range<parameter_iterator> parameter_range;

        property();

        auto declaring_type() const -> type;
        auto reflected_type() const -> type;

        auto attributes() const -> metadata::property_flags;

        auto can_read()        const -> bool;
        auto can_write()       const -> bool;
        auto is_special_name() const -> bool;

        auto metadata_token()   const -> core::size_type;
        auto declaring_module() const -> module;

        auto name()          const -> core::string_reference;
        auto property_type() const -> type;

        auto custom_attributes() const -> detail::custom_attribute_range;

        auto default_value() const -> constant;

        // TODO auto accessors() const -> accessor_range;

        auto get_method() const -> method;
        auto set_method() const -> method;

        // TODO auto index_parameters() const -> parameter_range;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        // -- The following members of System.Reflection.PropertyInfo are not implemented --
        // GetValue
        // IsDefined
        // SetValue

        friend auto operator==(property const&, property const&) -> bool;
        friend auto operator< (property const&, property const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(property)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(property)

    public: // internal members

        property(type const& reflected_type, detail::property_table_entry const* context, core::internal_key);

        auto context(core::internal_key) const -> detail::property_table_entry const&;

    private:

        auto row() const -> metadata::property_row;

        metadata::type_def_or_signature                           _reflected_type;
        core::checked_pointer<detail::property_table_entry const> _context;
    };

} }

#endif 
