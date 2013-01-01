
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_ASSEMBLY_CONTEXT_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_ASSEMBLY_CONTEXT_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    class assembly_context
    {
    public:

        typedef std::vector<unique_module_context> module_context_storage_type;

        assembly_context(loader_context const* loader, module_location const& manifest_module_location);

        auto loader()          const -> loader_context const&;
        auto manifest_module() const -> module_context const&;
        auto modules()         const -> module_context_storage_type const&;
        auto name()            const -> assembly_name const&;

    private:

        enum class realization_state
        {
            name          = 0x01,
            other_modules = 0x02
        };

        assembly_context(assembly_context const&);
        auto operator=(assembly_context const&) -> assembly_context&;

        auto realize_name()    const -> void;
        auto realize_modules() const -> void;

        core::checked_pointer<loader_context const> _loader;

        core::flags<realization_state> mutable _state;
        core::recursive_mutex          mutable _sync;

        module_context_storage_type    mutable _modules;

        std::unique_ptr<assembly_name> mutable _name;
    };

} } }

#endif
