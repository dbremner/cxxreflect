
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_LOADER_CONTEXTS_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_LOADER_CONTEXTS_HPP_

#include "cxxreflect/metadata/metadata.hpp"

#include "cxxreflect/reflection/assembly_name.hpp"
#include "cxxreflect/reflection/detail/element_contexts.hpp"
#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/loader_configuration.hpp"
#include "cxxreflect/reflection/module_locator.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// \defgroup cxxreflect_reflection_detail_corecontexts Reflection -> Details -> Core Contexts
    ///
    /// These types make up the core of the CxxReflect metadata system.  They own all of the
    /// persistent state and most other types in the library are simply iterator-like references
    /// into an instance of one of these types.
    ///
    ///                         +------------+      +-----------------+
    ///                     +-->| Assembly 0 |----->| Manifest Module |
    ///                     |   +------------+      +-----------------+
    ///      +----------+   |
    ///      | Loader   |---|
    ///      +----------+   |
    ///                     |   +------------+      +-----------------+
    ///                     +-->| Assembly 1 |--+-->| Manifest Module |
    ///                         +------------+  |   +-----------------+
    ///                                         |
    ///                                         |   +-----------------+
    ///                                         +-->| Other Module    |
    ///                                             +-----------------+
    ///
    /// * **loader_context**:  There is exactly one loader context for a type universe.  It owns all
    ///   of the assemblies that are loaded through it, and their lifetimes are tied to it.
    ///
    /// * **assembly_context**:  An assembly context is created for each assembly that is loaded
    ///   through a loader.  The assembly context is simply a collection of module contexts.  When
    ///   an assembly is loaded, a single module context is created for its manifest module, which
    ///   is the module that contains the assembly manifest (and a database with an assembly row).
    ///   The assembly context will load any other modules for the assembly when they are required.
    ///
    /// * **module_context**:  A module context represents a single module.  It creates and owns the
    ///   metadata database for the module.
    ///
    /// There is a 1:N mapping of loader context to assembly context, and a 1:N mapping of assembly
    /// context to module context.  Most assemblies have exactly one module.
    ///
    /// @{





    class module_context : public metadata::database_owner
    {
    public:

        module_context(assembly_context const* assembly, module_location const& location);

        auto assembly() const -> assembly_context const&;
        auto location() const -> module_location const&;
        auto database() const -> metadata::database const&;

        auto find_type_def(core::string_reference namespace_name,
                           core::string_reference simple_name) const -> metadata::type_def_token;

    private:

        module_context(module_context const&);
        auto operator=(module_context const&) -> module_context&;

        auto create_database(module_location const& location) -> metadata::database;

        core::value_initialized<assembly_context const*> _assembly;
        module_location                                  _location;
        metadata::database                               _database;
    };

    typedef std::unique_ptr<module_context> unique_module_context;





    class assembly_context
    {
    public:

        typedef std::vector<unique_module_context> module_context_sequence;

        assembly_context(loader_context const* loader, module_location const& location);

        auto loader()          const -> loader_context const&;
        auto manifest_module() const -> module_context const&;
        auto modules()         const -> module_context_sequence const&;
        auto name()            const -> assembly_name const&;

        auto find_type_def(core::string_reference namespace_name,
                           core::string_reference simple_name) const -> metadata::type_def_token;

    private:

        assembly_context(assembly_context const&);
        auto operator=(assembly_context const&) -> assembly_context&;

        enum class realization_state
        {
            name    = 0x01,
            modules = 0x02
        };

        void realize_name()    const;
        void realize_modules() const;

        core::value_initialized<loader_context const*> _loader;
        module_context_sequence                mutable _modules;

        core::flags<realization_state>         mutable _state;
        std::unique_ptr<assembly_name>         mutable _name;
    };

    typedef std::unique_ptr<assembly_context> unique_assembly_context;





    class loader_context : public metadata::type_resolver
    {
    public:

        // Disable "'this' : used in base member initializer list"  We know what we're doing :-D
        #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP
        #    pragma warning(push)
        #    pragma warning(disable: 4355)
        #endif

        template <typename Locator>
        loader_context(Locator locator)
            : _locator      (std::forward<Locator>(locator)),
              _configuration(detail::default_loader_configuration()),
              _events    (this, &_context_storage),
              _fields    (this, &_context_storage),
              _interfaces(this, &_context_storage),
              _methods   (this, &_context_storage),
              _properties(this, &_context_storage)
        {
        }

        template <typename Locator, typename Configuration>
        loader_context(Locator locator, Configuration configuration)
            : _locator      (std::forward<Locator>(locator)            ),
              _configuration(std::forward<Configuration>(configuration)),
              _events    (this, &_context_storage),
              _fields    (this, &_context_storage),
              _interfaces(this, &_context_storage),
              _methods   (this, &_context_storage),
              _properties(this, &_context_storage)
        {
        }

        #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP
        #    pragma warning(pop)
        #endif

        auto get_or_load_assembly(module_location const& location) const -> assembly_context const&;
        auto get_or_load_assembly(assembly_name   const& name)     const -> assembly_context const&;

        // metadata::type_resolver implementation
        virtual auto resolve_type(metadata::type_def_ref_spec_token type) const -> metadata::type_def_spec_token;
        virtual auto resolve_fundamental_type(metadata::element_type type) const -> metadata::type_def_token;

        auto locator() const -> module_locator const&;

        auto module_from_scope(metadata::database const& scope) const -> module_context const&;
        auto register_module(module_context const* module) const -> void;
        auto system_module() const -> module_context const&;

        auto system_namespace() const -> core::string_reference;

        auto compute_event_table    (metadata::type_def_or_signature const& type) const -> event_context_table;
        auto compute_field_table    (metadata::type_def_or_signature const& type) const -> field_context_table;
        auto compute_interface_table(metadata::type_def_or_signature const& type) const -> interface_context_table;
        auto compute_method_table   (metadata::type_def_or_signature const& type) const -> method_context_table;
        auto compute_property_table (metadata::type_def_or_signature const& type) const -> property_context_table;

        static auto from(assembly_context   const&) -> loader_context const&;
        static auto from(module_context     const&) -> loader_context const&;
        static auto from(assembly           const&) -> loader_context const&;
        static auto from(module             const&) -> loader_context const&;
        static auto from(type               const&) -> loader_context const&;
        static auto from(metadata::database const&) -> loader_context const&;

    private:

        enum : core::size_type
        {
            fundamental_type_count = static_cast<core::size_type>(metadata::element_type::concrete_element_type_max)
        };

        typedef std::map<core::string, unique_assembly_context>            assembly_map;
        typedef assembly_map::value_type                                   assembly_map_entry;
        typedef std::map<metadata::database const*, module_context const*> module_map;
        typedef module_map::value_type                                     module_map_entry;

        loader_context(loader_context const&);
        auto operator=(loader_context const&) -> loader_context&;

        module_locator       _locator;
        loader_configuration _configuration;

        /// The set of loaded assemblies, mapped by Absolute URI
        assembly_map mutable _assemblies;

        /// A map of each database to the module that owns it, used for rapid reverse lookup
        module_map mutable _module_map;

        std::array<metadata::type_def_token, fundamental_type_count> mutable _fundamental_types;

        core::value_initialized<module_context const*> mutable _system_module;

        element_context_table_storage          mutable _context_storage;
        event_context_table_collection         mutable _events;
        field_context_table_collection         mutable _fields;
        interface_context_table_collection     mutable _interfaces;
        method_context_table_collection        mutable _methods;
        property_context_table_collection      mutable _properties;

        core::recursive_mutex                  mutable _sync;
    };

    typedef std::unique_ptr<loader_context> unique_loader_context;

    /// @}

} } }

#endif
