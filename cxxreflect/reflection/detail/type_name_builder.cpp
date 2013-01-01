
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/assembly_context.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/detail/type_name_builder.hpp"
#include "cxxreflect/reflection/detail/type_policy.hpp"
#include "cxxreflect/reflection/detail/type_resolution.hpp"
#include "cxxreflect/reflection/assembly_name.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    auto type_name_builder::build_type_name(metadata::type_def_ref_or_signature const& t, mode const m) -> core::string
    {
        return type_name_builder(t, m);
    }

    type_name_builder::type_name_builder(metadata::type_def_ref_or_signature const& t, mode const m)
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

    auto type_name_builder::accumulate_type_name(metadata::type_def_ref_or_signature const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_initialized(); });

        if (t.is_blob())
            return accumulate_type_signature_name(t.as_blob().as<metadata::type_signature>(), m);

        metadata::type_def_ref_token const& token(t.as_token());
        switch (token.table())
        {
        case metadata::table_id::type_def: return accumulate_type_def_name(token.as<metadata::type_def_token>(), m);
        case metadata::table_id::type_ref: return accumulate_type_ref_name(token.as<metadata::type_ref_token>(), m);
        default: core::assert_unreachable();
        }
    }

    auto type_name_builder::accumulate_type_def_name(metadata::type_def_token const& t, mode const m) -> bool
    {
        metadata::type_def_row const& r(row_from(t));
        type_policy const& policy(type_policy::get_for(t));

        if (m == mode::simple_name)
        {
            _buffer += r.name().c_str();
            return true;
        }

        // Otherwise, we have either a simple name or an assembly qualified name:
        if (policy.is_nested(t))
        {
            accumulate_type_def_name(policy.declaring_type(t).as_token().as<metadata::type_def_token>(), mode::full_name);
            _buffer.push_back(L'+');
        }
        else if (r.namespace_name().size() > 0)
        {
            _buffer += r.namespace_name().c_str();
            _buffer.push_back(L'.');
        }

        _buffer += r.name().c_str();

        accumulate_assembly_qualification_if_required(t, m);
        return true;
    }

    auto type_name_builder::accumulate_type_ref_name(metadata::type_ref_token const& t, mode const m) -> bool
    {
        metadata::type_ref_row const& r(row_from(t));
        type_policy const& policy(type_policy::get_for(t));

        if (m == mode::simple_name)
        {
            _buffer += r.name().c_str();
            return true;
        }

        // Otherwise, we have either a simple name or an assembly qualified name:
        if (policy.is_nested(t))
        {
            accumulate_type_def_name(policy.declaring_type(t).as_token().as<metadata::type_def_token>(), mode::full_name);
            _buffer.push_back(L'+');
        }
        else if (r.namespace_name().size() > 0)
        {
            _buffer += r.namespace_name().c_str();
            _buffer.push_back(L'.');
        }

        _buffer += r.name().c_str();

        accumulate_assembly_qualification_if_required(t, m);
        return true;
    }

    auto type_name_builder::accumulate_type_signature_name(metadata::type_signature const& t, mode const m) -> bool
    {
        // A TypeSpec for an uninstantiated generic type has no name:
        if (m != mode::simple_name && metadata::signature_instantiator::requires_instantiation(t))
            return false;

        switch (t.get_kind())
        {
        case metadata::type_signature::kind::class_type:       return accumulate_class_type_signature_name           (t, m);
        case metadata::type_signature::kind::function_pointer: return accumulate_function_pointer_signature_name     (t, m);
        case metadata::type_signature::kind::general_array:    return accumulate_general_array_type_signature_name   (t, m);
        case metadata::type_signature::kind::generic_instance: return accumulate_generic_instance_type_signature_name(t, m);
        case metadata::type_signature::kind::pointer:          return accumulate_pointer_type_signature_name         (t, m);
        case metadata::type_signature::kind::primitive:        return accumulate_primitive_type_signature_name       (t, m);
        case metadata::type_signature::kind::simple_array:     return accumulate_simple_array_type_signature_name    (t, m);
        case metadata::type_signature::kind::variable:         return accumulate_variable_type_signature_name        (t, m);
        default: core::assert_unreachable();
        }
    }

    auto type_name_builder::accumulate_class_type_signature_name(metadata::type_signature const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_kind(metadata::type_signature::kind::class_type); });

        metadata::type_def_ref_or_signature const& class_type(compute_type(t.class_type()));

        if (!accumulate_type_name(class_type, without_assembly_qualification(m)))
            return false;

        if (t.is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(class_type, m);
        return true;
    }

    auto type_name_builder::accumulate_function_pointer_signature_name(metadata::type_signature const& t, mode) -> bool
    {
        core::assert_true([&]{ return t.is_kind(metadata::type_signature::kind::function_pointer); });

        // TODO We need to figure out how to write the function pointer form:
        core::assert_not_yet_implemented();
    }

    auto type_name_builder::accumulate_general_array_type_signature_name(metadata::type_signature const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_kind(metadata::type_signature::kind::general_array); });

        metadata::type_def_ref_or_signature const& array_type(compute_type(metadata::blob(t.array_type())));

        if (!accumulate_type_name(array_type, without_assembly_qualification(m)))
            return false;

        _buffer.push_back(L'[');
        for (unsigned i(1); i < t.array_shape().rank(); ++i)
            _buffer.push_back(L',');
        _buffer.push_back(L']');

        if (t.is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(array_type, m);
        return true;
    }

    auto type_name_builder::accumulate_generic_instance_type_signature_name(metadata::type_signature const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_kind(metadata::type_signature::kind::generic_instance); });

        metadata::type_def_ref_or_signature const& generic_type(compute_type(t.generic_type()));

        if (!accumulate_type_name(generic_type, without_assembly_qualification(m)))
            return false;

        if (m == mode::simple_name)
        {
            if (t.is_by_ref())
                _buffer.push_back(L'&');
            return true;
        }

        _buffer.push_back(L'[');

        bool is_first(true);
        bool const success(core::all(t.generic_arguments(), [&](metadata::type_signature const& arg) -> bool
        {
            if (!is_first)
                _buffer.push_back(L',');

            is_first = false;

            _buffer.push_back(L'[');

            metadata::type_def_ref_or_signature const& argument_type(compute_type(metadata::blob(arg)));
            if (!accumulate_type_name(argument_type, mode::assembly_qualified_name))
                return false;

            _buffer.push_back(L']');
            return true;
        }));

        if (!success)
            return false;

        _buffer.push_back(L']');

        if (t.is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(generic_type, m);
        return true;
    }

    auto type_name_builder::accumulate_pointer_type_signature_name(metadata::type_signature const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_kind(metadata::type_signature::kind::pointer); });

        metadata::type_def_ref_or_signature const& pointer_type(compute_type(metadata::blob(t.pointer_type())));

        if (!accumulate_type_name(pointer_type, without_assembly_qualification(m)))
            return false;

        _buffer.push_back(L'*');

        if (t.is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(pointer_type, m);
        return true;
    }

    auto type_name_builder::accumulate_primitive_type_signature_name(metadata::type_signature const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_kind(metadata::type_signature::kind::primitive); });

        metadata::element_type const e_type(t.primitive_type());

        detail::loader_context const& loader(detail::loader_context::from(t.scope()));

        metadata::type_def_token const type_def(loader.resolve_fundamental_type(e_type));

        if (!accumulate_type_name(type_def, without_assembly_qualification(m)))
            return false;

        if (t.is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(type_def, m);
        return true;
    }

    auto type_name_builder::accumulate_simple_array_type_signature_name(metadata::type_signature const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_kind(metadata::type_signature::kind::simple_array); });

        metadata::type_def_ref_or_signature const& array_type(compute_type(metadata::blob(t.array_type())));

        if (!accumulate_type_name(array_type, without_assembly_qualification(m)))
            return false;

        _buffer.push_back(L'[');
        _buffer.push_back(L']');

        if (t.is_by_ref())
            _buffer.push_back(L'&');

        accumulate_assembly_qualification_if_required(array_type, m);
        return true;
    }

    auto type_name_builder::accumulate_variable_type_signature_name(metadata::type_signature const& t, mode const m) -> bool
    {
        core::assert_true([&]{ return t.is_kind(metadata::type_signature::kind::variable); });

        if (m == mode::assembly_qualified_name || m == mode::full_name)
            return false;

        core::assert_true([&]() -> bool
        {
            metadata::element_type const type_code(t.get_element_type());
            return type_code == metadata::element_type::annotated_mvar || type_code == metadata::element_type::annotated_var;
        });

        core::size_type                    const variable_number(t.variable_number());
        metadata::type_or_method_def_token const variable_context(t.variable_context());

        core::assert_initialized(variable_context);

        metadata::generic_param_row_range const generic_parameters(metadata::find_generic_params(variable_context));

        if (generic_parameters.size() < variable_number)
            throw core::runtime_error(L"invalid generic parameter number");

        metadata::generic_param_row const parameter_row(*(generic_parameters.begin() + variable_number));

        _buffer += parameter_row.name().c_str();

        if (t.is_by_ref())
            _buffer.push_back(L'&');

        return true;
    }

    auto type_name_builder::accumulate_assembly_qualification_if_required(metadata::type_def_ref_or_signature const& t, mode const m) -> void
    {
        if (m != mode::assembly_qualified_name)
            return;

        _buffer.push_back(L',');
        _buffer.push_back(L' ');
        _buffer += module_context::from(t.scope()).assembly().name().full_name().c_str();
    }

    auto type_name_builder::without_assembly_qualification(mode const m) -> mode
    {
        return m == mode::simple_name ? mode::simple_name : mode::full_name;
    }

} } }
