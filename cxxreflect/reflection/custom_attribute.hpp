
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_CUSTOM_ATTRIBUTE_HPP_
#define CXXREFLECT_REFLECTION_CUSTOM_ATTRIBUTE_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/independent_handles.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"

namespace cxxreflect { namespace reflection {

    class custom_attribute
    {
    public:

        typedef void /* TODO */ positional_argument_iterator;
        typedef void /* TODO */ named_argument_iterator;

        custom_attribute();

        auto metadata_token() const -> core::size_type;

        // TODO Implement some sort of parent() functionality.

        auto constructor() const -> method;

        auto begin_positional_arguments() const -> positional_argument_iterator;
        auto end_positional_arguments()   const -> positional_argument_iterator;

        auto begin_named_arguments() const -> named_argument_iterator;
        auto end_named_arguments()   const -> named_argument_iterator;

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

    public: // internal members

        custom_attribute(module                           const& declaring_module,
                         metadata::custom_attribute_token const& attribute,
                         core::internal_key);

        static auto begin_for(module                               const& declaring_module,
                              metadata::has_custom_attribute_token const& parent,
                              core::internal_key) -> custom_attribute_iterator;

        static auto end_for  (module                               const& declaring_module,
                              metadata::has_custom_attribute_token const& parent,
                              core::internal_key) -> custom_attribute_iterator;

    private:

        metadata::custom_attribute_token     _attribute;
        detail::method_handle                _constructor;
    };

} }

#endif 

// AMDG //
