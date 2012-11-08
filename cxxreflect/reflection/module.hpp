
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_MODULE_HPP_
#define CXXREFLECT_REFLECTION_MODULE_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    class module_type_iterator_constructor
    {
    public:

        auto operator()(std::nullptr_t, detail::module_type_def_index_iterator const it) const -> type;
    };

} } }

namespace cxxreflect { namespace reflection {

    class module
    {
    public:

        typedef detail::module_type_iterator type_iterator;

        typedef core::iterator_range<type_iterator> type_range;

        module();
        module(detail::module_context const* context, core::internal_key);
        module(assembly const& defining_assembly, core::size_type index, core::internal_key);

        auto defining_assembly() const -> assembly;
        auto location()          const -> module_location const&;
        auto name()              const -> core::string_reference;

        auto types() const -> type_range;

        auto find_type(core::string_reference const& namespace_name,
                       core::string_reference const& simple_name) const -> type;

        auto context(core::internal_key) const -> detail::module_context const&;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        friend auto operator==(module const&, module const&) -> bool;
        friend auto operator< (module const&, module const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(module)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(module)

    private:

        core::checked_pointer<detail::module_context const> _context;
    };

} }

#endif
