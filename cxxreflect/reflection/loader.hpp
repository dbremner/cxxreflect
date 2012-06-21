
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_LOADER_HPP_
#define CXXREFLECT_REFLECTION_LOADER_HPP_

#include "cxxreflect/metadata/metadata.hpp"
#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"

namespace cxxreflect { namespace reflection {

    /// A `loader` is responsible for loading assemblies
    class loader
    {
    public:

        template <typename Locator>
        explicit loader(Locator&& locator)
            : _context(core::make_unique<detail::loader_context>(std::forward<Locator>(locator)))
        {
        }

        template <typename Locator, typename Configuration>
        loader(Locator&& locator, Configuration&& configuration)
            : _context(core::make_unique<detail::loader_context>(
                std::forward<Locator>(locator),
                std::forward<Configuration>(configuration)))
        {
        }

        loader(loader&& other);
        auto operator=(loader&& other) -> loader&;

        auto load_assembly(core::string_reference const& path_or_uri) const -> assembly;
        auto load_assembly(module_location        const& location)    const -> assembly;
        auto load_assembly(assembly_name          const& name)        const -> assembly;

        auto locator() const -> module_locator const&;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(loader)

    public: // internal members

        auto context(core::internal_key) const -> detail::loader_context const&;

    private:

        loader(loader const&);
        auto operator=(loader const&) -> loader&;

        detail::unique_loader_context _context;
    };

} }

#endif 

// AMDG //
