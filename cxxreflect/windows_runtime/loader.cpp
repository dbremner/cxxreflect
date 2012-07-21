
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/loader.hpp"
#include "cxxreflect/windows_runtime/detail/runtime_utility.hpp"

namespace cxxreflect { namespace windows_runtime { namespace generated { 

    // These are defined in generated\platform_types_embedded.cpp; that file is generated at build;
    // we need only to declare its two functions here.
    auto begin_platform_types_embedded() -> core::const_byte_iterator;
    auto end_platform_types_embedded()   -> core::const_byte_iterator;

} } } 

namespace cxxreflect { namespace windows_runtime { namespace {

    auto platform_types_location() -> reflection::module_location
    {
        return reflection::module_location(core::const_byte_range(
            generated::begin_platform_types_embedded(),
            generated::end_platform_types_embedded()));
    }

} } }

namespace cxxreflect { namespace windows_runtime {

    package_module_locator::package_module_locator(core::string const& package_root)
        : _package_root(package_root)
    {
        std::vector<core::string> const metadata_files(detail::enumerate_package_metadata_files(package_root.c_str()));
    
        std::transform(begin(metadata_files), end(metadata_files),
                       std::inserter(_metadata_files, end(_metadata_files)),
                       [](core::string const& file_name) -> path_map::value_type
        {
            // TODO We definitely need error checking here.
            auto const first(std::find(file_name.rbegin(), file_name.rend(), L'\\').base());
            auto const last (std::find(file_name.rbegin(), file_name.rend(), L'.' ).base());

            core::string const simple_name(first, std::prev(last));

            return std::make_pair(core::to_lowercase(simple_name), core::to_lowercase(file_name));
        });
    }

    package_module_locator::package_module_locator(package_module_locator const& other)
    {
        auto const lock(other._sync.lock());

        _package_root   = other._package_root;
        _metadata_files = other._metadata_files;
    }

    auto package_module_locator::operator=(package_module_locator other) -> package_module_locator&
    {
        // Since we've copied the argument into this function, there's no need to synchronize here;
        // we have the sole reference to the object.
        std::swap(other._package_root,   _package_root);
        std::swap(other._metadata_files, _metadata_files);

        return *this;
    }

    auto package_module_locator::locate_assembly(reflection::assembly_name const& target_assembly) const
        -> reflection::module_location
    {
        core::string const simple_name(core::to_lowercase(target_assembly.simple_name()));

        // Redirect platform and mscorlib references to our in-module replacement assembly.  Neither
        // of these are resolvable at runtime.
        if (simple_name == L"platform" || simple_name == L"mscorlib")
        {
            return platform_types_location();
        }

        // TODO We are not expecting to have to locate an assembly without a type name, so we expect
        // the other overload of locate_assembly to be called all of the time.  If we find that this
        // function actually is called, we should see whether we need to implement futher logic here
        // or if we need to change the calling code to call the other overload.
        throw core::logic_error(L"unexpected call to package_module_locator::locate_assembly");
    }

    auto package_module_locator::locate_assembly(reflection::assembly_name const& target_assembly,
                                                 core::string              const& full_type_name) const
        -> reflection::module_location
    {
        core::string const simple_name(core::to_lowercase(target_assembly.simple_name()));

        // Redirect platform and mscorlib references to our in-module replacement assembly.  Neither
        // of these are resolvable at runtime.
        if (simple_name == L"platform" || simple_name == L"mscorlib")
        {
            return platform_types_location();
        }

        // The name of the assembly must be a prefix of the name of the type.
        core::string const lowercase_full_type_name(core::to_lowercase(full_type_name));
        if (!core::starts_with(lowercase_full_type_name.c_str(), simple_name.c_str()))
            throw core::runtime_error(L"provided assembly/type pair does not match Windows Runtime naming rules");

        core::string namespace_name(full_type_name);
        detail::remove_rightmost_type_name_component(namespace_name);
        if (namespace_name.empty())
            throw core::runtime_error(L"provided type has no namespace to resolve");

        return reflection::module_location(find_metadata_for_namespace(namespace_name));
        
    }

    auto package_module_locator::locate_module(reflection::assembly_name const& requesting_assembly,
                                               core::string              const& module_name) const
        -> reflection::module_location
    {
        // Windows Runtime does not have multi-module metadata files
        return reflection::module_location();
    }

    auto package_module_locator::metadata_files() const -> path_map
    {
        auto const lock(_sync.lock());

        return _metadata_files;
    }

    auto package_module_locator::find_metadata_for_namespace(core::string const& namespace_name) const
        -> reflection::module_location
    {
        core::string const lowercase_namespace_name(core::to_lowercase(namespace_name));

        // First, search the metadata files we got from RoResolveNamespace:
        {
            auto const lock(_sync.lock());

            core::string enclosing_namespace_name(lowercase_namespace_name);
            while (!enclosing_namespace_name.empty())
            {
                auto const it(_metadata_files.find(enclosing_namespace_name));
                if (it != _metadata_files.end())
                    return reflection::module_location(it->second.c_str());

                detail::remove_rightmost_type_name_component(enclosing_namespace_name);
            }
        }

        // WORKAROUND:  If the above failed, we can try to search in the package root.  However,
        // this should not be necessary.  RoResolveNamespace should return all of the resolvable
        // metadata files.
        // core::string enclosing_namespace_name(lowercase_namespace_name);
        // while (!enclosing_namespace_name.empty())
        // {
        //     core::string const winmd_path(core::to_lowercase(_package_root + enclosing_namespace_name + L".winmd"));
        //     if (core::externals::file_exists(winmd_path.c_str()))
        //     {
        //         auto const lock(_sync.lock());
        //         
        //         _metadata_files.insert(std::make_pair(enclosing_namespace_name, winmd_path));
        //         return reflection::module_location(winmd_path.c_str());
        //     }
        // 
        //     detail::remove_rightmost_type_name_component(enclosing_namespace_name);
        // }

        // If the type is in the 'Platform' or 'System' namespace, we special case it and use our
        // Platform metadata.  This heuristic isn't perfect, but it should be sufficient for non-
        // pathological type names.
        if (core::starts_with(lowercase_namespace_name.c_str(), L"platform") ||
            core::starts_with(lowercase_namespace_name.c_str(), L"system"))
        {
            return platform_types_location();
        }

        // Otherwise, we failed to locate the metadata file.  Rats.
        throw core::runtime_error(L"failed to locate metadata file for provided namespace");
    }





    auto package_loader_configuration::system_namespace() const -> core::string_reference
    {
        return L"Platform";
    }






    package_loader::package_loader(package_module_locator const& locator, std::unique_ptr<reflection::loader> loader)
        : _locator(locator), _loader(std::move(loader))
    {
        core::assert_not_null(_loader.get());
    }

    auto package_loader::loader() const -> reflection::loader const&
    {
        return *_loader;
    }

    auto package_loader::locator() const -> package_module_locator const&
    {
        return _locator;
    }

    auto package_loader::get_type(core::string_reference const full_name) const -> reflection::type
    {
        core::assert_true([&]{ return !full_name.empty(); });

        // TODO To support generics we'll need more advanced type name parsing
        auto const end_of_namespace(std::find(full_name.rbegin(), full_name.rend(), L'.').base());
        if (end_of_namespace == begin(full_name))
            throw core::logic_error(L"provided type name has no namespace");

        core::string const namespace_name(begin(full_name), std::prev(end_of_namespace));
        core::string const simple_name(end_of_namespace, end(full_name));

        return get_type(namespace_name.c_str(), simple_name.c_str());
    }

    auto package_loader::get_type(core::string_reference const namespace_name,
                                  core::string_reference const simple_name) const -> reflection::type
    {
        reflection::module_location const& location(locator().find_metadata_for_namespace(namespace_name.c_str()));
        if (location.get_kind() == reflection::module_location::kind::uninitialized)
            return reflection::type();

        // TODO We need a non-throwing load call
        reflection::assembly const& assembly(loader().load_assembly(location));
        if (!assembly)
            return reflection::type();

        return assembly.find_type(namespace_name, simple_name);
    }

    auto package_loader::get_implementers(reflection::type const& interface_type) const
        -> std::vector<reflection::type>
    {
        core::assert_initialized(interface_type);

        // HACK:  We only include Windows types if the interface name is from Windows.  This should
        // be correct, but if we improve our filtering below, we should be able to remove this hack
        // and not impact performance.

        // TODO This method is so absurdly slow it is practically unusable.
        bool const include_windows_types(core::starts_with(interface_type.namespace_name().c_str(), L"Windows"));

        std::vector<reflection::type> implementers;

        typedef package_module_locator::path_map             sequence;
        typedef package_module_locator::path_map::value_type element;

        sequence const metadata_files(locator().metadata_files());
        std::for_each(begin(metadata_files), end(metadata_files), [&](element const& f)
        {
            if (!include_windows_types && core::starts_with(f.first.c_str(), L"windows"))
                return;

            // TODO We can do better filtering than this by checking assembly references.
            // TODO Add caching of the obtained data.
            reflection::assembly const a(loader().load_assembly(reflection::module_location(f.second.c_str())));
            std::for_each(a.begin_types(), a.end_types(), [&](reflection::type const& t)
            {
                if (std::find(t.begin_interfaces(), t.end_interfaces(), interface_type) != t.end_interfaces())
                    implementers.push_back(t);
            });
        });

        return implementers;
    }

    auto package_loader::get_enumerators(reflection::type const& enumeration_type) const -> std::vector<enumerator>
    {
        core::assert_initialized(enumeration_type);

        if (!enumeration_type.is_enum())
            return std::vector<enumerator>();

        metadata::binding_flags const flags(
            metadata::binding_attribute::public_ |
            metadata::binding_attribute::static_);

        auto const first(enumeration_type.begin_fields(flags));
        auto const last(enumeration_type.end_fields());

        std::vector<enumerator> result;
        std::transform(first, last, std::back_inserter(result), [&](reflection::field const& field) -> enumerator
        {
            reflection::constant const constant(field.constant_value());

            std::uint32_t value(0);
            switch (constant.get_kind())
            {
            case reflection::constant::kind::int32:  value = core::convert_integer(constant.as_int32());  break;
            case reflection::constant::kind::uint32: value = core::convert_integer(constant.as_uint32()); break;
            default: throw core::runtime_error(L"invalid enumerator type encountered");
            }

            return enumerator(field.name(), value);
        });

        return result;
    }

    auto package_loader::get_activation_factory_type(reflection::type const& activatable_type) const
        -> reflection::type
    {
        core::assert_initialized(activatable_type);

        reflection::method const activatable_constructor(get_activatable_attribute_factory_constructor());

        auto const activatable_attribute_it(std::find_if(activatable_type.begin_custom_attributes(),
                                                         activatable_type.end_custom_attributes(),
                                                         [&](reflection::custom_attribute const& attribute)
        {
            return attribute.constructor() == activatable_constructor;
        }));

        if (activatable_attribute_it == activatable_type.end_custom_attributes())
            throw core::runtime_error(L"type has no activation factory");

        core::string const factory_type_name(activatable_attribute_it->single_string_argument());

        return get_type(factory_type_name.c_str());
    }

    auto package_loader::get_guid(reflection::type const& runtime_type) const -> reflection::guid
    {
        core::assert_initialized(runtime_type);

        reflection::type const guid_attribute_type(get_guid_attribute_type());

        // TODO We can cache the GUID Type and compare using its identity instead, for performance.
        auto const it(std::find_if(runtime_type.begin_custom_attributes(),
                                   runtime_type.end_custom_attributes(),
                                   [&](reflection::custom_attribute const& attribute)
        {
            return attribute.constructor().declaring_type() == guid_attribute_type;
        }));

        // TODO We need to make sure that a type has only one GuidAttribute.
        return it != runtime_type.end_custom_attributes() ? it->single_guid_argument() : reflection::guid();
    }

    #define CXXREFLECT_WINDOWS_RUNTIME_LOADER_DEFINE_PROPERTY(XTYPE, XNAME, ...)  \
        XTYPE package_loader::get_ ## XNAME() const                               \
        {                                                                         \
            core::recursive_mutex_lock const lock(_sync.lock());                  \
            if (!_delay_init_ ## XNAME ## _initialized.get())                     \
            {                                                                     \
                _delay_init_ ## XNAME = (__VA_ARGS__)();                          \
                _delay_init_ ## XNAME ## _initialized.get() = true;               \
            }                                                                     \
            return _delay_init_ ## XNAME;                                         \
        }

    #define CXXREFLECT_WINDOWS_RUNTIME_LOADER_DEFINE_PROPERTY_TYPE(XNAME, XNAMESPACE, XTYPENAME) \
        CXXREFLECT_WINDOWS_RUNTIME_LOADER_DEFINE_PROPERTY(reflection::type, XNAME, [&]() -> reflection::type \
        {                                                                                                    \
            reflection::type const type(get_type(XNAMESPACE, XTYPENAME));                                    \
            core::assert_initialized(type);                                                                  \
            return type;                                                                                     \
        })

    CXXREFLECT_WINDOWS_RUNTIME_LOADER_DEFINE_PROPERTY_TYPE(
        activatable_attribute_type,
        L"Windows.Foundation.Metadata",
        L"ActivatableAttribute");

    CXXREFLECT_WINDOWS_RUNTIME_LOADER_DEFINE_PROPERTY_TYPE(
        guid_attribute_type,
        L"Windows.Foundation.Metadata",
        L"GuidAttribute");

    CXXREFLECT_WINDOWS_RUNTIME_LOADER_DEFINE_PROPERTY(reflection::method, activatable_attribute_factory_constructor,
    [&]() -> reflection::method
    {
        reflection::type const attribute_type(get_activatable_attribute_type());

        metadata::binding_flags const flags(
            metadata::binding_attribute::public_ |
            metadata::binding_attribute::instance);

        auto const first_constructor(attribute_type.begin_constructors(flags));
        auto const last_constructor(attribute_type.end_constructors());

        auto const constructor_it(std::find_if(first_constructor, last_constructor, [&](reflection::method const& c)
        {
            // TODO We should also check parameter types.
            return core::distance(c.begin_parameters(), c.end_parameters()) == 2;
        }));

        if (constructor_it == last_constructor)
            throw core::runtime_error(L"failed to find activation factory type for type");

        return *constructor_it;

    });

    #undef CXXREFLECT_WINDOWS_RUNTIME_LOADER_DEFINE_PROPERTY
    #undef CXXREFLECT_WINDOWS_RUNTIME_LOADER_DEFINE_PROPERTY_TYPE





    global_package_loader::initialized_flag     global_package_loader::_initialized;
    global_package_loader::unique_loader_future global_package_loader::_context;

    auto global_package_loader::initialize(unique_loader_future&& loader) -> void
    {
        // Ensure that we only initialize the global loader once:
        bool expected(false);
        if (!_initialized.compare_exchange_strong(expected, true))
            throw core::logic_error(L"initialize was already called");

        _context = std::move(loader);
    }

    auto global_package_loader::get() -> package_loader&
    {
        if (!_initialized.load())
            throw core::logic_error(L"initialize has not yet been called");

        package_loader* const context(_context.get().get());
        if (context == nullptr)
            throw core::logic_error(L"initialization completed but the result was invalid (nullptr)");

        return *context;
    }

    auto global_package_loader::has_initialization_begun() -> bool
    {
        return _initialized.load();
    }

    auto global_package_loader::is_initialized() -> bool
    {
        return _context.valid();
    }

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION

// AMDG //
