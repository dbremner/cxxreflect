
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_LOADER_CONTEXT_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_LOADER_CONTEXT_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/membership.hpp"
#include "cxxreflect/reflection/loader_configuration.hpp"
#include "cxxreflect/reflection/module_locator.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    class loader_context final : public metadata::type_resolver
    {
    public:

        loader_context(module_locator locator, loader_configuration configuration);

        auto get_or_load_assembly(module_location const& location) const -> assembly_context const&;
        auto get_or_load_assembly(assembly_name   const& name)     const -> assembly_context const&;

        // metadata::type_resolver implementation
        virtual auto resolve_member          (metadata::member_ref_token       ) const -> metadata::field_or_method_def_token override;
        virtual auto resolve_type            (metadata::type_def_ref_spec_token) const -> metadata::type_def_spec_token       override;
        virtual auto resolve_fundamental_type(metadata::element_type           ) const -> metadata::type_def_token            override;

        // Routines for reference resolution:
        //
        // These routines centralize the resolution of the various types of cross-module references
        // that are present in metadata databases.  They utilize the per-module caches to reduce the
        // amount of time spent performing resolution.
        //
        // Note that the resolve_assembly_ref may only be called for an assembly reference to an
        // ordinary metadata module (i.e., a non-Windows Runtime metadata module).  Windows Runtime
        // modules can only be resolved by namespace.
        auto resolve_assembly_ref(metadata::assembly_ref_token) const -> metadata::database const&;
        auto resolve_module_ref  (metadata::module_ref_token  ) const -> metadata::database const&;
        auto resolve_member_ref  (metadata::member_ref_token  ) const -> metadata::field_or_method_def_token;
        auto resolve_type_ref    (metadata::type_ref_token    ) const -> metadata::type_def_token;
        auto resolve_namespace   (core::string_reference      ) const -> metadata::database const&;

        auto locator() const -> module_locator const&;

        auto system_module()    const -> module_context const&;
        auto system_namespace() const -> core::string_reference;

        auto is_filtered_type(metadata::type_def_token const& type) const -> bool;

        auto get_membership(metadata::type_def_or_signature const& type) const -> membership_handle;

        static auto from(metadata::database const& scope) -> loader_context const&;

    private:

        enum : core::size_type
        {
            fundamental_type_count = static_cast<core::size_type>(metadata::element_type::concrete_element_type_max)
        };

        typedef std::map<core::string, unique_assembly_context>              assembly_map;
        typedef assembly_map::value_type                                     assembly_map_entry;
        typedef std::map<core::string, metadata::database const*>            namespace_map;
        typedef namespace_map::value_type                                    namespace_map_entry;
        typedef std::array<metadata::type_def_token, fundamental_type_count> fundamental_type_cache;

        loader_context(loader_context const&);
        auto operator=(loader_context const&) -> loader_context&;

        module_locator       _locator;
        loader_configuration _configuration;

        assembly_map                                mutable _assemblies;
        namespace_map                               mutable _namespaces;
        membership_storage                          mutable _membership;
        fundamental_type_cache                      mutable _fundamental_types;
        core::checked_pointer<module_context const> mutable _system_module;
        core::recursive_mutex                       mutable _sync;
    };

} } }

#endif
