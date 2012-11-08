
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_ASSEMBLY_HPP_
#define CXXREFLECT_REFLECTION_ASSEMBLY_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection {

    class assembly
    {
    private:

        static auto begin_module_types(module const&) -> detail::module_type_iterator;
        static auto end_module_types  (module const&) -> detail::module_type_iterator;

    public:

        typedef core::instantiating_iterator<
            core::size_type,
            module,
            assembly,
            core::internal_constructor_forwarder<module>
        > module_iterator;

        typedef core::instantiating_iterator<
            metadata::token_with_arithmetic<metadata::file_token>::type,
            file,
            assembly,
            core::internal_constructor_forwarder<file>
        > file_iterator;

        typedef core::instantiating_iterator<
            metadata::token_with_arithmetic<metadata::assembly_ref_token>::type,
            assembly_name,
            std::nullptr_t,
            core::internal_constructor_forwarder<assembly_name>
        > assembly_name_iterator;

        typedef core::concatenating_iterator<
            module_iterator,
            detail::module_type_iterator,
            module,
            type,
            &assembly::begin_module_types,
            &assembly::end_module_types
        > type_iterator;

        typedef core::iterator_range<module_iterator       > module_range;
        typedef core::iterator_range<file_iterator         > file_range;
        typedef core::iterator_range<assembly_name_iterator> assembly_name_range;
        typedef core::iterator_range<type_iterator         > type_range;

        assembly();
        assembly(detail::assembly_context const* context, core::internal_key);

        auto owning_loader() const -> loader;

        auto name()     const -> assembly_name const&;
        auto location() const -> core::string_reference;

        auto referenced_assembly_names() const -> assembly_name_range;
        auto files()                     const -> file_range;
        auto modules()                   const -> module_range;
        auto types()                     const -> type_range;

        auto find_file  (core::string_reference const& name) const -> file;
        auto find_module(core::string_reference const& name) const -> module;
        auto find_type  (core::string_reference const& namespace_name, core::string_reference const& simple_name) const -> type;

        auto manifest_module() const -> module;

        auto context(core::internal_key) const -> detail::assembly_context const&;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        friend auto operator==(assembly const&, assembly const&) -> bool;
        friend auto operator< (assembly const&, assembly const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(assembly)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(assembly)

    private:

        core::checked_pointer<detail::assembly_context const> _context;
    };

} }

#endif
