
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_ASSEMBLY_HPP_
#define CXXREFLECT_REFLECTION_ASSEMBLY_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"

namespace cxxreflect { namespace reflection {

    class assembly
    {
    private:

        typedef core::instantiating_iterator<
            metadata::token_with_arithmetic<metadata::type_def_token>::type,
            type,
            module
        > inner_type_iterator;

        static auto begin_module_types(module const& m) -> inner_type_iterator;
        static auto end_module_types  (module const& m) -> inner_type_iterator;

    public:

        typedef core::instantiating_iterator<
            core::size_type,
            module,
            assembly
        > module_iterator;

        typedef core::instantiating_iterator<
            metadata::token_with_arithmetic<metadata::file_token>::type,
            file,
            assembly
        > file_iterator;

        typedef core::instantiating_iterator<
            metadata::token_with_arithmetic<metadata::assembly_ref_token>::type,
            assembly_name,
            assembly
        > assembly_name_iterator;

        typedef core::concatenating_iterator<
            module_iterator,
            inner_type_iterator,
            module,
            type,
            &assembly::begin_module_types,
            &assembly::end_module_types
        > type_iterator;

        assembly();

        auto name()     const -> assembly_name const&;
        auto location() const -> core::string_reference;

        auto referenced_assembly_count()       const -> core::size_type;
        auto begin_referenced_assembly_names() const -> assembly_name_iterator;
        auto end_referenced_assembly_names()   const -> assembly_name_iterator;

        auto begin_files() const -> file_iterator;
        auto end_files()   const -> file_iterator;

        auto find_file(core::string_reference name) const -> file;

        auto begin_modules() const -> module_iterator;
        auto end_modules()   const -> module_iterator;

        auto find_module(core::string_reference name) const -> module;

        auto begin_types() const -> type_iterator;
        auto end_types()   const -> type_iterator;

        auto find_type(core::string_reference full_name) const -> type;
        auto find_type(core::string_reference namespace_name, core::string_reference simple_name) const -> type;

        // EntryPoint
        // ImageRuntimeVersion
        // ManifestModule

        // [static] CreateQualifiedName
        // GetCustomAttributes
        // GetExportedTypes
        // GetManifestResourceInfo
        // GetManifestResourceNames
        // GetManifestResourceStream
        // GetReferencedAssemblies
        // IsDefined

        // -- These members are not implemented --
        //
        // CodeBase               Use GetName().GetPath()
        // EscapedCodeBase        Use GetName().GetPath()
        // Evidence               Not applicable outside of runtime
        // FullName               Use GetName().GetFullName()
        // GlobalAssemblyCache    TODO Can we implement this?
        // HostContext            Not applicable outside of runtime
        // IsDynamic              Not applicable outside of runtime
        // IsFullyTrusted         Not applicable outside of runtime
        // Location               Use GetName().GetPath()
        // PermissionSet          Not applicable outside of runtime
        // ReflectionOnly         Would always be true
        // SecurityRuleSet        Not applicable outside of runtime
        //
        // CreateInstance         Not applicable outside of runtime
        // GetAssembly            Not applicable outside of runtime
        // GetCallingAssembly     Not applicable outside of runtime
        // GetEntryAssembly       Not applicable outside of runtime
        // GetExecutingAssembly   Not applicable outside of runtime
        // GetLoadedModules       Not applicable outside of runtime
        // GetSatelliteAssembly   TODO Can we implement this?
        // Load                   Not applicable outside of runtime
        // LoadFile               Not applicable outside of runtime
        // LoadModule             Not applicable outside of runtime
        // LoadWithPartialName    Not applicable outside of runtime
        // ReflectionOnlyLoad     Use Loader::LoadAssembly()
        // ReflectionOnlyLoadFrom Use Loader::LoadAssembly()
        // UnsafeLoadFrom         Not applicable outside of runtime

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        friend auto operator==(assembly const&, assembly const&) -> bool;
        friend auto operator< (assembly const&, assembly const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(assembly)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(assembly)

    public: // internal members

        assembly(detail::assembly_context const* context, core::internal_key);

        auto context(core::internal_key) const -> detail::assembly_context const&;

    private:

        core::value_initialized<detail::assembly_context const*> _context;
    };

} }

#endif 
