
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_LOADER_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_LOADER_HPP_

#include "cxxreflect/reflection/reflection.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/enumerator.hpp"

#include <atomic>
#include <future>

namespace cxxreflect { namespace windows_runtime {

    class package_module_locator
    {
    public:

        typedef std::map<core::string, core::string> path_map;

        package_module_locator(core::string const& package_root);

        package_module_locator(package_module_locator const&);
        auto operator=(package_module_locator) -> package_module_locator&;

        auto locate_assembly(reflection::assembly_name const& target_assembly) const -> reflection::module_location;

        auto locate_assembly(reflection::assembly_name const& target_assembly,
                             core::string              const& full_type_name) const -> reflection::module_location;

        auto locate_module(reflection::assembly_name const& requesting_assembly,
                           core::string              const& module_name) const -> reflection::module_location;

        // TODO We should replace this with something less expensive.  Since we must synchronize
        // access to _metadata_files, direct iterator access is impossible.  This will suffice
        // for the moment.
        auto metadata_files() const -> path_map;

        auto find_metadata_for_namespace(core::string const& namespace_name) const -> reflection::module_location;

    private:

        core::string                  _package_root;
        path_map              mutable _metadata_files;
        core::recursive_mutex mutable _sync;
    };





    class package_loader_configuration
    {
    public:

        // Returns 'Platform', since all of our system types are in the 'Platform' namespace.
        auto system_namespace() const -> core::string_reference;
    };





    class package_loader
    {
    public:

        package_loader(package_module_locator const& locator, std::unique_ptr<reflection::loader> loader);

        auto loader()  const -> reflection::loader const&;
        auto locator() const -> package_module_locator const&;

        auto get_type(core::string_reference const full_name) const -> reflection::type;

        auto get_type(core::string_reference const namespace_name,
                      core::string_reference const simple_name) const -> reflection::type;

        auto get_implementers(reflection::type const& interface_type) const -> std::vector<reflection::type>;
        auto get_enumerators(reflection::type const& enumeration_type) const -> std::vector<enumerator>;

        auto get_activation_factory_type(reflection::type const& activatable_type) const -> reflection::type;

        auto get_guid(reflection::type const& runtime_type) const -> reflection::guid;

        #define CXXREFLECT_WINDOWS_RUNTIME_LOADER_DECLARE_PROPERTY(XTYPE, XNAME)             \
            private:                                                                         \
                mutable core::value_initialized<bool> _delay_init_ ## XNAME ## _initialized; \
                mutable XTYPE                         _delay_init_ ## XNAME;                 \
                                                                                             \
            public:                                                                          \
                XTYPE get_ ## XNAME() const;

        CXXREFLECT_WINDOWS_RUNTIME_LOADER_DECLARE_PROPERTY(reflection::type,   activatable_attribute_type);
        CXXREFLECT_WINDOWS_RUNTIME_LOADER_DECLARE_PROPERTY(reflection::type,   guid_attribute_type);

        CXXREFLECT_WINDOWS_RUNTIME_LOADER_DECLARE_PROPERTY(reflection::method, activatable_attribute_factory_constructor);

        #undef CXXREFLECT_WINDOWS_RUNTIME_LOADER_DECLARE_PROPERTY

    private:

        package_loader(package_loader const&);
        auto operator=(package_loader const&) -> package_loader&;

        package_module_locator                      _locator;
        std::unique_ptr<reflection::loader>         _loader;
        core::recursive_mutex               mutable _sync;
    };





    class global_package_loader
    {
    public:

        typedef std::atomic<bool>                 initialized_flag;
        typedef std::unique_ptr<package_loader>   unique_loader;
        typedef std::shared_future<unique_loader> unique_loader_future;

        static auto initialize(unique_loader_future&& loader) -> void;

        static auto get() -> package_loader&;

        static auto has_initialization_begun() -> bool;

        static auto is_initialized() -> bool;

    private:

        static auto initialized() -> initialized_flag&;
        static auto context()     -> unique_loader_future&;

        global_package_loader();
        global_package_loader(global_package_loader const&);
        auto operator=(global_package_loader const&) -> global_package_loader;
    };

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 
