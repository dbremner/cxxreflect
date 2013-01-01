
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_LOADER_HPP_
#define CXXREFLECT_REFLECTION_LOADER_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection {

    class loader
    {
    public:

        loader();
        loader(detail::loader_context const* context, core::internal_key);

        auto load_assembly(core::string_reference const& path_or_uri) const -> assembly;
        auto load_assembly(module_location        const& location)    const -> assembly;
        auto load_assembly(assembly_name          const& name)        const -> assembly;

        auto locator() const -> module_locator const&;

        auto context(core::internal_key) const -> detail::loader_context const&;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        friend auto operator==(loader const&, loader const&) -> bool;
        friend auto operator< (loader const&, loader const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(loader)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(loader)

    private:

        core::checked_pointer<detail::loader_context const> _context;
    };





    class loader_root
    {
    public:

        loader_root(detail::unique_loader_context context, core::internal_key);

        loader_root(loader_root&&);
        auto operator=(loader_root&&) -> loader_root&;

        auto get() const -> loader;

        auto is_initialized() const -> bool;

    private:

        loader_root(loader_root const&);
        auto operator=(loader_root const&) -> loader_root&;

        detail::unique_loader_context _context;
    };

    auto create_loader_root(module_locator locator, loader_configuration configuration) -> loader_root;

} }

#endif
