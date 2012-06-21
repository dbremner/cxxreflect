
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/initialization.hpp"
#include "cxxreflect/windows_runtime/loader.hpp"
#include "cxxreflect/windows_runtime/detail/runtime_utility.hpp"
#include "cxxreflect/windows_runtime/externals/winrt_externals.hpp"

namespace cxxreflect { namespace windows_runtime { namespace generated { 

    // These are defined in generated\platform_types_embedded.cpp; that file is generated at build;
    // we need only to declare its two functions here.
    auto begin_platform_types_embedded() -> core::const_byte_iterator;
    auto end_platform_types_embedded()   -> core::const_byte_iterator;

} } } 

namespace cxxreflect { namespace windows_runtime {

    auto begin_initialization() -> void
    {
        if (global_package_loader::has_initialization_begun())
            throw core::logic_error(L"initialization has already begun");

        core::externals::initialize(externals::winrt_externals());

        // Start initialization in the background.  Note:  we explicitly want to specify an async
        // launch here.  This cannot run on an STA thread when /ZW is used.
        global_package_loader::initialize(std::async(std::launch::async, []() -> std::unique_ptr<package_loader>
        {
            core::string const current_package_root(detail::current_package_root());

            package_module_locator locator(current_package_root);

            std::unique_ptr<reflection::loader> loader_instance(core::make_unique<reflection::loader>(
                locator,
                package_loader_configuration()));

            // Load the embedded platform assembly
            loader_instance->load_assembly(reflection::module_location(core::const_byte_range(
                generated::begin_platform_types_embedded(),
                generated::end_platform_types_embedded())));

            typedef package_module_locator::path_map             sequence;
            typedef package_module_locator::path_map::value_type element;

            sequence const metadata_files(locator.metadata_files());
            std::for_each(begin(metadata_files), end(metadata_files), [&](element const& e)
            {
                loader_instance->load_assembly(reflection::module_location(e.second.c_str()));
            });

            std::unique_ptr<package_loader> context(core::make_unique<package_loader>(
                locator,
                std::move(loader_instance)));

            return context;
        }));
    }

    auto has_initialization_begun() -> bool
    {
        return global_package_loader::has_initialization_begun();
    }

    auto is_initialized() -> bool
    {
        return global_package_loader::is_initialized();
    }

    auto when_initialized_call(std::function<void()> const callable) -> void
    {
        Concurrency::task<void>([&]{ global_package_loader::get(); }).then(callable);
    }

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION

// AMDG //
