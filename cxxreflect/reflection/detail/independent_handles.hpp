
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_INDEPENDENT_HANDLES_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_INDEPENDENT_HANDLES_HPP_

#include "cxxreflect/metadata/metadata.hpp"
#include "cxxreflect/reflection/detail/element_contexts.hpp"
#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// \defgroup cxxreflect_reflection_detail_handles Reflection -> Details -> Independent Handles
    ///
    /// These handle types encapsulate all of the information required to instantiate the
    /// corresponding public interface types, but without being size- or layout-dependent on the 
    /// public interface types.
    ///
    /// This allows us to represent the public interface types without including the actual public
    /// interface headers.  This is important to avoid recursive dependencies between the headers,
    /// and effectively allows us to avoid having to include most of the public interface headers
    /// in other interface headers.
    ///
    /// @{

    class assembly_handle
    {
    public:

        assembly_handle();
        assembly_handle(assembly_context const* context);
        assembly_handle(assembly const& element);

        auto realize() const -> assembly;

        auto is_initialized() const -> bool;

        friend auto operator==(assembly_handle const&, assembly_handle const&) -> bool;
        friend auto operator< (assembly_handle const&, assembly_handle const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(assembly_handle)

    private:

        core::value_initialized<assembly_context const*> _context;
    };

    class method_handle
    {
    public:

        method_handle();
        method_handle(module_context                           const* reflected_module,
                      metadata::type_def_ref_spec_or_signature const& reflected_type,
                      method_context                           const* context);
        method_handle(method const& element);

        auto realize() const -> method;

        auto is_initialized() const -> bool;

        friend auto operator==(method_handle const&, method_handle const&) -> bool;
        friend auto operator< (method_handle const&, method_handle const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(method_handle)

    private:

        core::value_initialized<module_context const*> _reflected_module;
        metadata::type_def_ref_spec_or_signature       _reflected_type;
        core::value_initialized<method_context const*> _context;
    };

    class module_handle
    {
    public:

        module_handle();
        module_handle(module_context const* context);
        module_handle(module const& element);

        auto realize() const -> module;
        auto context() const -> module_context const&;

        auto is_initialized() const -> bool;

        friend auto operator==(module_handle const&, module_handle const&) -> bool;
        friend auto operator< (module_handle const&, module_handle const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(module_handle)

    private:

        core::value_initialized<module_context const*> _context;
    };

    class parameter_handle
    {
    public:

        parameter_handle();
        parameter_handle(module_context                           const* reflected_module,
                         metadata::type_def_ref_spec_or_signature const& reflected_type,
                         method_context                           const* context,
                         metadata::param_token                    const& parameter_token,
                         metadata::type_signature                 const& parameter_signature);
        parameter_handle(parameter const& element);

        auto realize() const -> parameter;

        auto is_initialized() const -> bool;

        friend auto operator==(parameter_handle const&, parameter_handle const&) -> bool;
        friend auto operator< (parameter_handle const&, parameter_handle const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(parameter_handle)

    private:

        core::value_initialized<module_context const*>   _reflected_module;
        metadata::type_def_ref_spec_or_signature         _reflected_type;
        core::value_initialized<method_context const*>   _context;

        metadata::param_token                            _parameter_token;
        metadata::type_signature                         _parameter_signature;
    };

    class type_handle
    {
    public:

        type_handle();
        type_handle(module_context const* module, metadata::type_def_ref_spec_or_signature const& token_or_sig);
        type_handle(type const& element);

        auto realize() const -> type;

        auto is_initialized() const -> bool;

        friend auto operator==(type_handle const&, type_handle const&) -> bool;
        friend auto operator< (type_handle const&, type_handle const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(type_handle)

    private:

        core::value_initialized<module_context const*> _module;
        metadata::type_def_ref_spec_or_signature       _type;
    };





    /// Parameter data wrapper for parameter iteration and instantiation
    ///
    /// This adapter allows us to reuse the `instantiating_iterator` for instantiating parameter
    /// objects.  It joins parameter tokens with their corresponding parts in a method signature.
    class parameter_data
    {
    public:

        parameter_data();

        // Note:  This constructor takes an InternalKey only so that it matches other constructors
        // of types with which the parameter iterator is instantiated.
        parameter_data(metadata::param_token                          const& token,
                       metadata::method_signature::parameter_iterator const& signature,
                       core::internal_key);

        auto token()     const -> metadata::param_token    const&;
        auto signature() const -> metadata::type_signature const&;

        auto is_initialized() const -> bool;

        auto operator++()    -> parameter_data&;
        auto operator++(int) -> parameter_data;

        friend auto operator==(parameter_data const&, parameter_data const&) -> bool;
        friend auto operator< (parameter_data const&, parameter_data const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(parameter_data)

    private:

        metadata::param_token                          _token;
        metadata::method_signature::parameter_iterator _signature;
    };

    /// @}

} } }

#endif
