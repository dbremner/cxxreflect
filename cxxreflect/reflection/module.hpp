
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_MODULE_HPP_
#define CXXREFLECT_REFLECTION_MODULE_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"

namespace cxxreflect { namespace reflection {

    class module
    {
    public:
        
        typedef void /* TODO */ field_iterator;
        typedef void /* TODO */ method_iterator;

        typedef core::instantiating_iterator<
            metadata::token_with_arithmetic<metadata::type_def_token>::type,
            type,
            module
        > type_iterator;

        module();

        auto defining_assembly() const -> assembly;

        auto metadata_token() const -> core::size_type;

        auto name() const -> core::string_reference;
        auto path() const -> core::string_reference;

        auto begin_custom_attributes() const -> custom_attribute_iterator;
        auto end_custom_attributes()   const -> custom_attribute_iterator;

        // TODO auto begin_fields() const -> field_iterator;
        // TODO auto end_fields()   const -> field_iterator;

        // TODO auto begin_methods() const -> method_iterator;
        // TODO auto end_methods()   const -> method_iterator;

        auto begin_types() const -> type_iterator;
        auto end_types()   const -> type_iterator;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        friend auto operator==(module const&, module const&) -> bool;
        friend auto operator< (module const&, module const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(module)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(module)


    public: // internal members

        module(detail::module_context const* context, core::internal_key);
        module(assembly const& defining_assembly, core::size_type module_index, core::internal_key);

        auto context(core::internal_key) const -> detail::module_context const&;

    private:

        core::value_initialized<detail::module_context const*> _context;
    };

} }

#endif 
