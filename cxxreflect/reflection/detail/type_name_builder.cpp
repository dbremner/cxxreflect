
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly.hpp"
#include "cxxreflect/reflection/detail/loader_contexts.hpp"
#include "cxxreflect/reflection/detail/type_name_builder.hpp"
#include "cxxreflect/reflection/module.hpp"
#include "cxxreflect/reflection/type.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    auto type_name_builder::build_type_name(type const& t, mode const m) -> core::string
    {
        return type_name_builder(t, m);
    }

    type_name_builder::type_name_builder(type const& t, mode const m)
    {
        _buffer.reserve(1024);

        if (!accumulate_type_name(t, m))
            _buffer.resize(0);
    }

    type_name_builder::operator core::string()
    {
        core::string result;
        std::swap(result, _buffer);
        return result;
    }

    auto type_name_builder::accumulate_type_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_initialized(); });

        return t.is_type_def()
            ? accumulate_type_def_name(t, m)
            : accumulate_type_spec_name(t, m);
    }

    auto type_name_builder::accumulate_type_def_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_type_def(); });

        if (m == mode::simple_name)
        {
            _buffer += t.get_type_def_row().name().c_str();
            return true;
        }

        // Otherwise, we have either a simple name or an assembly qualified name:
        if (t.is_nested())
        {
            accumulate_type_def_name(t.declaring_type(), mode::full_name);
            _buffer.push_back(L'+');
        }
        else if (t.namespace_name().size() > 0)
        {
            _buffer += t.namespace_name().c_str();
            _buffer.push_back(L'.');
        }

        _buffer += t.get_type_def_row().name().c_str();

        accumulate_assembly_qualification_if_required(t, m);
        return true;
    }

    auto type_name_builder::accumulate_type_spec_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_type_spec(); });

        metadata::type_signature const signature(t.get_type_spec_signature());

        typedef metadata::class_variable_signature_instantiator instantiator;

        // A TypeSpec for an uninstantiated generic type has no name:
        if (m != mode::simple_name && instantiator::requires_instantiation(signature))
            return false;

        switch (signature.get_kind())
        {
        case metadata::type_signature::kind::class_type:       return accumulate_class_type_spec_name           (t, m);
        case metadata::type_signature::kind::function_pointer: return accumulate_method_signature_spec_name(t, m);
        case metadata::type_signature::kind::general_array:    return accumulate_general_array_type_spec_name   (t, m);
        case metadata::type_signature::kind::generic_instance: return accumulate_generic_instance_type_spec_name(t, m);
        case metadata::type_signature::kind::pointer:          return accumulate_pointer_type_spec_name         (t, m);
        case metadata::type_signature::kind::primitive:        return accumulate_primitive_type_spec_name       (t, m);
        case metadata::type_signature::kind::simple_array:     return accumulate_simple_array_type_spec_name    (t, m);
        case metadata::type_signature::kind::variable:         return accumulate_variable_type_spec_name        (t, m);
        default: core::assert_fail(L"unreachable code");       return false;
        }
    }

    auto type_name_builder::accumulate_class_type_spec_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.get_type_spec_signature().is_kind(metadata::type_signature::kind::class_type); });

        metadata::database const* const scope(&t.get_type_spec_signature().class_type().scope());

        module const scope_module(scope != nullptr
            ? module(&loader_context::from(t).module_from_scope(*scope), core::internal_key())
            : t.defining_module());

        type const class_type(
            scope_module,
            t.get_type_spec_signature().class_type(),
            core::internal_key());

        if (!accumulate_type_name(class_type, without_assembly_qualification(m)))
            return false;

        if (t.get_type_spec_signature().is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(class_type, m);
        return true;
    }

    auto type_name_builder::accumulate_method_signature_spec_name(type const& t, mode) -> bool
    {
        core::assert_true([&]{ return t.get_type_spec_signature().is_kind(metadata::type_signature::kind::function_pointer); });

        // TODO We need to figure out how to write the function pointer form:
        throw core::logic_error(L"not yet implemented");
    }

    auto type_name_builder::accumulate_general_array_type_spec_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.get_type_spec_signature().is_kind(metadata::type_signature::kind::general_array); });

        type const array_type(
            t.defining_module(),
            metadata::blob(t.get_type_spec_signature().array_type()),
            core::internal_key());

        if (!accumulate_type_name(array_type, without_assembly_qualification(m)))
            return false;

        _buffer.push_back(L'[');
        for (unsigned i(1); i < t.get_type_spec_signature().array_shape().rank(); ++i)
            _buffer.push_back(L',');
        _buffer.push_back(L']');

        if (t.get_type_spec_signature().is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(array_type, m);
        return true;
    }

    auto type_name_builder::accumulate_generic_instance_type_spec_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.get_type_spec_signature().is_kind(metadata::type_signature::kind::generic_instance); });

        type const generic_type(
            t.defining_module(),
            t.get_type_spec_signature().generic_type(),
            core::internal_key());

        if (!accumulate_type_name(generic_type, without_assembly_qualification(m)))
            return false;

        metadata::type_signature const signature(t.get_type_spec_signature());

        if (m == mode::simple_name)
        {
            if (signature.is_by_ref())
                _buffer.push_back(L'&');
            return true;
        }

        _buffer.push_back(L'[');

        auto const first(signature.begin_generic_arguments());
        auto const last(signature.end_generic_arguments());
        bool is_first(true);
        bool const success(std::all_of(first, last, [&](metadata::type_signature const& arg) -> bool
        {
            if (!is_first)
                _buffer.push_back(L',');

            is_first = false;

            _buffer.push_back(L'[');

            type const argument_type(t.defining_module(), metadata::blob(arg), core::internal_key());
            if (!accumulate_type_name(argument_type, mode::assembly_qualified_name))
                return false;

            _buffer.push_back(L']');
            return true;
        }));

        if (!success)
            return false;

        _buffer.push_back(L']');

        if (signature.is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(generic_type, m);
        return true;
    }

    auto type_name_builder::accumulate_pointer_type_spec_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.get_type_spec_signature().is_kind(metadata::type_signature::kind::pointer); });

        type const pointer_type(
            t.defining_module(),
            metadata::blob(t.get_type_spec_signature().pointer_type()),
            core::internal_key());

        if (!accumulate_type_name(pointer_type, without_assembly_qualification(m)))
            return false;

        _buffer.push_back(L'*');

        if (t.get_type_spec_signature().is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(pointer_type, m);
        return true;
    }

    auto type_name_builder::accumulate_primitive_type_spec_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.get_type_spec_signature().is_kind(metadata::type_signature::kind::primitive); });

        metadata::element_type const e_type(t.get_type_spec_signature().primitive_type());

        detail::loader_context const& loader(detail::loader_context::from(t));

        metadata::type_def_token const type_def(loader.resolve_fundamental_type(e_type));


        type const primitive_type(
            module(&loader.module_from_scope(type_def.scope()), core::internal_key()),
            type_def,
            core::internal_key());

        if (!accumulate_type_name(primitive_type, without_assembly_qualification(m)))
            return false;

        if (t.get_type_spec_signature().is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(primitive_type, m);
        return true;
    }

    auto type_name_builder::accumulate_simple_array_type_spec_name(type const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.get_type_spec_signature().is_kind(metadata::type_signature::kind::simple_array); });

        type const array_type(
            t.defining_module(),
            metadata::blob(t.get_type_spec_signature().array_type()),
            core::internal_key());

        if (!accumulate_type_name(array_type, without_assembly_qualification(m)))
            return false;

        _buffer.push_back(L'[');
        _buffer.push_back(L']');

        if (t.get_type_spec_signature().is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(array_type, m);
        return true;
    }

    auto type_name_builder::accumulate_variable_type_spec_name(type const& t, mode) -> bool
    {
        core::assert_true([&]{ return t.get_type_spec_signature().is_kind(metadata::type_signature::kind::variable); });

        // TODO Do we need to support class and method variables?  If so, how do we decide when to
        // write them to the type name (i.e., sometimes they definitely do not belong).
        return false;
    }

    auto type_name_builder::accumulate_assembly_qualification_if_required(type const& t, mode const m) -> void
    {
        if (m != mode::assembly_qualified_name)
            return;

        _buffer.push_back(L',');
        _buffer.push_back(L' ');
        _buffer += t.defining_assembly().name().full_name().c_str();
    }

    auto type_name_builder::without_assembly_qualification(mode const m) -> mode
    {
        return m == mode::simple_name ? mode::simple_name : mode::full_name;
    }

} } }

// AMDG //
