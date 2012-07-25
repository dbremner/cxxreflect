
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/detail/independent_handles.hpp"
#include "cxxreflect/reflection/method.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/parameter.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    assembly_handle::assembly_handle()
    {
    }

    assembly_handle::assembly_handle(assembly_context const* context)
        : _context(context)
    {
        core::assert_not_null(context);
    }

    assembly_handle::assembly_handle(assembly const& element)
        : _context(&element.context(core::internal_key()))
    {
        core::assert_initialized(element);
    }

    auto assembly_handle::realize() const -> assembly
    {
        core::assert_initialized(*this);
        return assembly(_context.get(), core::internal_key());
    }

    auto assembly_handle::is_initialized() const -> bool
    {
        return _context.get() != nullptr;
    }

    auto operator==(assembly_handle const& lhs, assembly_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::equal_to<assembly_context const*>()(lhs._context.get(), rhs._context.get());
    }

    auto operator<(assembly_handle const& lhs, assembly_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::less<assembly_context const*>()(lhs._context.get(), rhs._context.get());
    }





    method_handle::method_handle()
    {
    }

    method_handle::method_handle(module_context                           const* reflected_module,
                                 metadata::type_def_ref_spec_or_signature const& reflected_type,
                                 method_context                           const* context)
        : _reflected_module(reflected_module), _reflected_type(reflected_type), _context(context)
    {
        core::assert_not_null(reflected_module);
        core::assert_initialized(reflected_type);
        core::assert_not_null(context);
    }

    method_handle::method_handle(method const& element)
        : _reflected_module(&element.reflected_type().defining_module().context(core::internal_key())),
          _reflected_type(element.reflected_type().self_reference(core::internal_key())),
          _context(&element.context(core::internal_key()))
    {
        core::assert_initialized(*this);
    }

    auto method_handle::realize() const -> method
    {
        module const reflected_module(_reflected_module.get(), core::internal_key());
        type   const reflected_type(reflected_module, _reflected_type, core::internal_key());

        return method(reflected_type, _context.get(), core::internal_key());
    }

    auto method_handle::is_initialized() const -> bool
    {
        return _reflected_module.get() != nullptr
            && _reflected_type.is_initialized()
            && _context.get() != nullptr;
    }

    auto operator==(method_handle const& lhs, method_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::equal_to<method_context const*>()(lhs._context.get(), rhs._context.get());
    }
    
    auto operator<(method_handle const& lhs, method_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::less<method_context const*>()(lhs._context.get(), rhs._context.get());
    }





    module_handle::module_handle()
    {
    }

    module_handle::module_handle(module_context const* context)
        : _context(context)
    {
        core::assert_not_null(context);
    }

    module_handle::module_handle(module const& element)
        : _context(&element.context(core::internal_key()))
    {
        core::assert_initialized(*this);
    }

    auto module_handle::realize() const -> module
    {
        core::assert_initialized(*this);

        return module(_context.get(), core::internal_key());
    }

    auto module_handle::context() const -> module_context const&
    {
        core::assert_initialized(*this);

        return *_context.get();
    }

    auto module_handle::is_initialized() const -> bool
    {
        return _context.get() != nullptr;
    }

    auto operator==(module_handle const& lhs, module_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::equal_to<module_context const*>()(lhs._context.get(), rhs._context.get());

    }

    auto operator<(module_handle const& lhs, module_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return std::less<module_context const*>()(lhs._context.get(), rhs._context.get());
    }





    parameter_handle::parameter_handle()
    {
    }

    parameter_handle::parameter_handle(module_context                           const* reflected_module,
                                       metadata::type_def_ref_spec_or_signature const& reflected_type,
                                       method_context                           const* context,
                                       metadata::param_token                    const& parameter_token,
                                       metadata::type_signature                 const& parameter_signature)
    {
    }

    parameter_handle::parameter_handle(parameter const& element)
    {
    }

    // auto parameter_handle::realize() const -> parameter
    // {
    // }

    auto parameter_handle::is_initialized() const -> bool
    {
        // TODO
        return false;
    }

    auto operator==(parameter_handle const& lhs, parameter_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        // TODO
        return false;
    }

    auto operator<(parameter_handle const& lhs, parameter_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        // TODO
        return false;
    }





    type_handle::type_handle()
    {
    }

    type_handle::type_handle(module_context                           const* module,
                             metadata::type_def_ref_spec_or_signature const& token_or_sig)
        : _module(module), _type(token_or_sig)
    {
        core::assert_not_null(module);
        core::assert_initialized(token_or_sig);
    }

    type_handle::type_handle(type const& element)
        : _module(&element.defining_module().context(core::internal_key())),
          _type(element.self_reference(core::internal_key()))
    {
        core::assert_initialized(element);
    }

    auto type_handle::realize() const -> type
    {
        core::assert_initialized(*this);

        return type(module(_module.get(), core::internal_key()), _type, core::internal_key());
    }

    auto type_handle::is_initialized() const -> bool
    {
        return _module.get() != nullptr && _type.is_initialized();
    }

    auto operator==(type_handle const& lhs, type_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._type == rhs._type;
    }

    auto operator<(type_handle const& lhs, type_handle const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._type < rhs._type;
    }





    parameter_data::parameter_data()
    {
    }

    parameter_data::parameter_data(metadata::param_token                          const& token,
                                   metadata::method_signature::parameter_iterator const& signature,
                                   core::internal_key)
        : _token(token), _signature(signature)
    {
        core::assert_initialized(token);
    }

    auto parameter_data::token() const -> metadata::param_token const&
    {
        core::assert_initialized(*this);

        return _token;
    }

    auto parameter_data::signature() const -> metadata::type_signature const&
    {
        core::assert_initialized(*this);

        return *_signature;
    }

    auto parameter_data::is_initialized() const -> bool
    {
        return _token.is_initialized();
    }

    auto parameter_data::operator++() -> parameter_data&
    {
        core::assert_initialized(*this);

        _token = metadata::param_token(&_token.scope(), _token.value() + 1);
        ++_signature;
        return *this;
    }

    auto parameter_data::operator++(int) -> parameter_data
    {
        parameter_data const x(*this);
        ++*this;
        return x;
    }

    auto operator==(parameter_data const& lhs, parameter_data const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._token == rhs._token;
    }

    auto operator<(parameter_data const& lhs, parameter_data const& rhs) -> bool
    {
        core::assert_initialized(lhs);
        core::assert_initialized(rhs);

        return lhs._token < rhs._token;
    }

} } }
