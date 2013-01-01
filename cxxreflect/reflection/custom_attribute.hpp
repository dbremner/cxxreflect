
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_CUSTOM_ATTRIBUTE_HPP_
#define CXXREFLECT_REFLECTION_CUSTOM_ATTRIBUTE_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"

namespace cxxreflect { namespace reflection {

    class custom_attribute
    {
    public:

        // TODO typedef void positional_argument_iterator;
        // TODO typedef void named_argument_iterator;

        // TODO typedef core::iterator_range<positional_argument_iterator> positional_argument_range;
        // TODO typedef core::iterator_range<named_argument_iterator     > named_argument_range;

        custom_attribute();
        custom_attribute(nullptr_t, metadata::custom_attribute_token const& attribute, core::internal_key);

        auto metadata_token() const -> core::size_type;

        // TODO Implement some sort of parent() functionality.

        auto constructor() const -> method;

        // TODO auto positional_arguments() const -> positional_argument_range;
        // TODO auto named_arguments()      const -> named_argument_range;

        // TODO These must be removed. These return the first fixed argument of the custom attribute,
        // interpreted either as a string or a GUID.  They do no type checking (and are therefore
        // really bad and unsafe).  They are here only to support handling of GuidAttribute and
        // ActivatableAttribute.  Once we implement the positional and named argument support, we
        // will remove these.
        auto single_string_argument() const -> core::string;
        auto single_guid_argument()   const -> guid;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        friend auto operator==(custom_attribute const&, custom_attribute const&) -> bool;
        friend auto operator< (custom_attribute const&, custom_attribute const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(custom_attribute)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(custom_attribute)

        static auto get_for(metadata::has_custom_attribute_token const& parent, core::internal_key) -> detail::custom_attribute_range;

    private:
        
        metadata::custom_attribute_token                        _attribute;
        metadata::type_def_or_signature                         _reflected_type;
        core::checked_pointer<detail::method_table_entry const> _constructor;
    };

} }

#endif 
