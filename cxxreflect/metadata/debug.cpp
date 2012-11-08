
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/precompiled_headers.hpp"
#include "cxxreflect/metadata/debug.hpp"
#include "cxxreflect/metadata/rows.hpp"
#include "cxxreflect/metadata/signatures.hpp"

namespace cxxreflect { namespace metadata { namespace debug { namespace {

    template <typename Enumeration>
    auto has_bit_set(Enumeration const value, Enumeration const bit) -> bool
    {
        return (core::as_integer(value) & core::as_integer(bit)) != 0;
    }

    template <typename Enumeration>
    auto has_masked_value(Enumeration const value, Enumeration const mask, Enumeration const bit) -> bool
    {
        return (core::as_integer(value) & core::as_integer(mask)) == core::as_integer(bit);
    }

    template <typename Enumeration>
    auto has_value(Enumeration const value, Enumeration const target) -> bool
    {
        return value == target;
    }

} } } }

namespace cxxreflect { namespace metadata { namespace debug {

    realized_assembly_row::realized_assembly_row(assembly_row const& r)
        : _hash_algorithm(r.hash_algorithm()),
          _version       (r.version()       ),
          _flags         (r.flags()         ),
          _public_key    (r.public_key()    ),
          _name          (r.name()          ),
          _culture       (r.culture()       )
    {
    }

    realized_assembly_os_row::realized_assembly_os_row(assembly_os_row const& r)
        : _platform_id  (r.platform_id()  ),
          _major_version(r.major_version()),
          _minor_version(r.minor_version())
    {
    }
    
    realized_assembly_processor_row::realized_assembly_processor_row(assembly_processor_row const& r)
        : _processor(r.processor())
    {
    }
    
    realized_assembly_ref_row::realized_assembly_ref_row(assembly_ref_row const& r)
        : _version   (r.version()   ),
          _flags     (r.flags()     ),
          _public_key(r.public_key()),
          _name      (r.name()      ),
          _culture   (r.culture()   ),
          _hash_value(r.hash_value())
    {
    }

    realized_assembly_ref_os_row::realized_assembly_ref_os_row(assembly_ref_os_row const& r)
        : _platform_id  (r.platform_id()  ),
          _major_version(r.major_version()),
          _minor_version(r.minor_version()),
          _parent       (r.parent()       )
    {
    }

    realized_assembly_ref_processor_row::realized_assembly_ref_processor_row(assembly_ref_processor_row const& r)
        : _processor(r.processor()),
          _parent   (r.parent()   )
    {
    }

    realized_class_layout_row::realized_class_layout_row(class_layout_row const& r)
        : _packing_size(r.packing_size()),
          _class_size  (r.class_size()  ),
          _parent      (r.parent()      )
    {
    }

    realized_constant_row::realized_constant_row(constant_row const& r)
        : _type      (r.type()      ),
          _parent    (r.parent()    ),
          _parent_raw(r.parent_raw()),
          _value     (r.value()     )
    {
    }

    realized_custom_attribute_row::realized_custom_attribute_row(custom_attribute_row const& r)
        : _parent    (r.parent()    ),
          _parent_raw(r.parent_raw()),
          _type      (r.type()      ),
          _type_raw  (r.type_raw()  ),
          _value     (r.value()     )
    {
    }

    realized_decl_security_row::realized_decl_security_row(decl_security_row const& r)
        : _action        (r.action()        ),
          _parent        (r.parent()        ),
          _parent_raw    (r.parent_raw()    ),
          _permission_set(r.permission_set())
    {
    }

    realized_event_map_row::realized_event_map_row(event_map_row const& r)
        : _parent     (r.parent()     ),
          _first_event(r.first_event()),
          _last_event (r.last_event() )
    {
    }

    realized_event_row::realized_event_row(event_row const& r)
        : _flags   (r.flags()   ),
          _name    (r.name()    ),
          _type    (r.type()    ),
          _type_raw(r.type_raw())
    {
    }

    realized_exported_type_row::realized_exported_type_row(exported_type_row const& r)
        : _flags             (r.flags()             ),
          _type_def_id       (r.type_def_id()       ),
          _name              (r.name()              ),
          _namespace_name    (r.namespace_name()    ),
          _implementation    (r.implementation()    ),
          _implementation_raw(r.implementation_raw())
    {
    }

    realized_field_row::realized_field_row(field_row const& r)
        : _flags    (r.flags()    ),
          _name     (r.name()     ),
          _signature(r.signature())
    {
    }

    realized_field_layout_row::realized_field_layout_row(field_layout_row const& r)
        : _offset(r.offset()),
          _parent(r.parent())
    {
    }

    realized_field_marshal_row::realized_field_marshal_row(field_marshal_row const& r)
        : _parent     (r.parent()     ),
          _parent_raw (r.parent_raw() ),
          _native_type(r.native_type())
    {
    }

    realized_field_rva_row::realized_field_rva_row(field_rva_row const& r)
        : _rva   (r.rva()   ),
          _parent(r.parent())
    {
    }

    realized_file_row::realized_file_row(file_row const& r)
        : _flags     (r.flags()     ),
          _name      (r.name()      ),
          _hash_value(r.hash_value())
    {
    }

    realized_generic_param_row::realized_generic_param_row(generic_param_row const& r)
        : _sequence  (r.sequence()  ),
          _flags     (r.flags()     ),
          _parent    (r.parent()    ),
          _parent_raw(r.parent_raw()),
          _name      (r.name()      )
    {
    }

    realized_generic_param_constraint_row::realized_generic_param_constraint_row(generic_param_constraint_row const& r)
        : _parent        (r.parent()        ),
          _constraint    (r.constraint()    ),
          _constraint_raw(r.constraint_raw())
    {
    }

    realized_impl_map_row::realized_impl_map_row(impl_map_row const& r)
        : _flags               (r.flags()               ),
          _member_forwarded    (r.member_forwarded()    ),
          _member_forwarded_raw(r.member_forwarded_raw()),
          _import_name         (r.import_name()         ),
          _import_scope        (r.import_scope()        )
    {
    }

    realized_interface_impl_row::realized_interface_impl_row(interface_impl_row const& r)
        : _parent       (r.parent()       ),
          _interface    (r.interface_()    ),
          _interface_raw(r.interface_raw())
    {
    }

    realized_manifest_resource_row::realized_manifest_resource_row(manifest_resource_row const& r)
        : _offset            (r.offset()            ),
          _flags             (r.flags()             ),
          _name              (r.name()              ),
          _implementation    (r.implementation()    ),
          _implementation_raw(r.implementation_raw())
    {
    }

    realized_member_ref_row::realized_member_ref_row(member_ref_row const& r)
        : _parent    (r.parent()    ),
          _parent_raw(r.parent_raw()),
          _name      (r.name()      ),
          _signature (r.signature() )
    {
    }

    realized_method_def_row::realized_method_def_row(method_def_row const& r)
        : _rva                 (r.rva()                 ),
          _implementation_flags(r.implementation_flags()),
          _flags               (r.flags()               ),
          _name                (r.name()                ),
          _signature           (r.signature()           ),
          _first_parameter     (r.first_parameter()     ),
          _last_parameter      (r.last_parameter()      )
    {
    }

    realized_method_impl_row::realized_method_impl_row(method_impl_row const& r)
        : _parent                (r.parent()                ),
          _method_body           (r.method_body()           ),
          _method_body_raw       (r.method_body_raw()       ),
          _method_declaration    (r.method_declaration()    ),
          _method_declaration_raw(r.method_declaration_raw())
    {
    }

    realized_method_semantics_row::realized_method_semantics_row(method_semantics_row const& r)
        : _semantics (r.semantics() ),
          _method    (r.method()    ),
          _parent    (r.parent()    ),
          _parent_raw(r.parent_raw())
    {
    }

    realized_method_spec_row::realized_method_spec_row(method_spec_row const& r)
        : _method    (r.method()    ),
          _method_raw(r.method_raw()),
          _signature (r.signature() )
    {
    }

    realized_module_row::realized_module_row(module_row const& r)
        : _name(r.name()),
          _mvid(r.mvid())
    {
    }

    realized_module_ref_row::realized_module_ref_row(module_ref_row const& r)
        : _name(r.name())
    {
    }

    realized_nested_class_row::realized_nested_class_row(nested_class_row const& r)
        : _nested_class   (r.nested_class()   ),
          _enclosing_class(r.enclosing_class())
    {
    }

    realized_param_row::realized_param_row(param_row const& r)
        : _flags   (r.flags()   ),
          _sequence(r.sequence()),
          _name    (r.name()    )
    {
    }

    realized_property_row::realized_property_row(property_row const& r)
        : _flags    (r.flags()    ),
          _name     (r.name()     ),
          _signature(r.signature())
    {
    }
    
    realized_property_map_row::realized_property_map_row(property_map_row const& r)
        : _parent        (r.parent()        ),
          _first_property(r.first_property()),
          _last_property (r.last_property() )
    {
    }

    realized_standalone_sig_row::realized_standalone_sig_row(standalone_sig_row const& r)
        : _signature(r.signature())
    {
    }

    realized_type_def_row::realized_type_def_row(type_def_row const& r)
        : _flags         (r.flags()         ),
          _name          (r.name()          ),
          _namespace_name(r.namespace_name()),
          _extends       (r.extends()       ),
          _extends_raw   (r.extends_raw()   ),
          _first_field   (r.first_field()   ),
          _last_field    (r.last_field()    ),
          _first_method  (r.first_method()  ),
          _last_method   (r.last_method()   )
    {
    }

    realized_type_ref_row::realized_type_ref_row(type_ref_row const& r)
        : _resolution_scope    (r.resolution_scope()    ),
          _resolution_scope_raw(r.resolution_scope_raw()),
          _name                (r.name()                ),
          _namespace_name      (r.namespace_name()      )
    {
    }

    realized_type_spec_row::realized_type_spec_row(type_spec_row const& r)
        : _signature(r.signature())
    {
    }





    #define CXXREFLECT_CONCATENATE_(x, y) x ## y
    #define CXXREFLECT_CONCATENATE(x, y) CXXREFLECT_CONCATENATE_(x, y)

    #define CXXREFLECT_STRINGIFY_(x) # x
    #define CXXREFLECT_STRINGIFY(x) CXXREFLECT_STRINGIFY_(x)

    #define CXXREFLECT_WIDE_STRINGIFY_(x) CXXREFLECT_CONCATENATE(L, CXXREFLECT_STRINGIFY(x))
    #define CXXREFLECT_WIDE_STRINGIFY(x) CXXREFLECT_WIDE_STRINGIFY_(x)

    #define CXXREFLECT_WRITE_IF_BIT_SET(t, n)               \
        if (has_bit_set(x, t::n))                           \
        {                                                   \
            if (write_pipe) { os.write(L" | "); }           \
            os.write(CXXREFLECT_WIDE_STRINGIFY(n));         \
            write_pipe = true;                              \
        }

    #define CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(t, m, n)   \
        if (has_masked_value(x, t::m, t::n))                \
        {                                                   \
            if (write_pipe) { os.write(L" | "); }           \
            os.write(CXXREFLECT_WIDE_STRINGIFY(n));         \
            write_pipe = true;                              \
        }

    #define CXXREFLECT_WRITE_IF_EQUAL(t, n)                 \
        if (has_value(x, t::n))                             \
        {                                                   \
            if (write_pipe) { os.write(L" | "); }           \
            os.write(CXXREFLECT_WIDE_STRINGIFY(n));         \
            write_pipe = true;                              \
        }

    auto insert_into_stream(core::base_wostream_wrapper& os, assembly_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_BIT_SET(assembly_attribute, public_key);
        CXXREFLECT_WRITE_IF_BIT_SET(assembly_attribute, retargetable);
        CXXREFLECT_WRITE_IF_BIT_SET(assembly_attribute, disable_jit_compile_optimizer);
        CXXREFLECT_WRITE_IF_BIT_SET(assembly_attribute, enable_jit_compile_tracking);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(assembly_attribute, content_type_mask, default_content_type);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(assembly_attribute, content_type_mask, windows_runtime_content_type);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, assembly_hash_algorithm const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_EQUAL(assembly_hash_algorithm, none);
        CXXREFLECT_WRITE_IF_EQUAL(assembly_hash_algorithm, md5);
        CXXREFLECT_WRITE_IF_EQUAL(assembly_hash_algorithm, sha1);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, binding_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_EQUAL(binding_attribute, default_);
        CXXREFLECT_WRITE_IF_BIT_SET(binding_attribute, ignore_case);
        CXXREFLECT_WRITE_IF_BIT_SET(binding_attribute, declared_only);
        CXXREFLECT_WRITE_IF_BIT_SET(binding_attribute, instance);
        CXXREFLECT_WRITE_IF_BIT_SET(binding_attribute, static_);
        CXXREFLECT_WRITE_IF_BIT_SET(binding_attribute, public_);
        CXXREFLECT_WRITE_IF_BIT_SET(binding_attribute, non_public);
        CXXREFLECT_WRITE_IF_BIT_SET(binding_attribute, flatten_hierarchy);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, calling_convention const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_BIT_SET(calling_convention, standard);
        CXXREFLECT_WRITE_IF_BIT_SET(calling_convention, varargs);
        CXXREFLECT_WRITE_IF_BIT_SET(calling_convention, has_this);
        CXXREFLECT_WRITE_IF_BIT_SET(calling_convention, explicit_this);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, event_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_BIT_SET(event_attribute, special_name);
        CXXREFLECT_WRITE_IF_BIT_SET(event_attribute, runtime_special_name);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, field_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(field_attribute, field_access_mask, compiler_controlled);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(field_attribute, field_access_mask, private_);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(field_attribute, field_access_mask, family_and_assembly);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(field_attribute, field_access_mask, assembly);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(field_attribute, field_access_mask, family);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(field_attribute, field_access_mask, family_or_assembly);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(field_attribute, field_access_mask, public_);

        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, static_);
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, init_only);
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, literal);
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, not_serialized);
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, special_name);
        
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, pinvoke_impl);
   
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, runtime_special_name);    
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, has_field_marshal);
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, has_default);
        CXXREFLECT_WRITE_IF_BIT_SET(field_attribute, has_field_rva);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, file_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_EQUAL(file_attribute, contains_metadata);
        CXXREFLECT_WRITE_IF_EQUAL(file_attribute, contains_no_metadata);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, generic_parameter_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(generic_parameter_attribute, variance_mask, none);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(generic_parameter_attribute, variance_mask, covariant);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(generic_parameter_attribute, variance_mask, contravariant);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(generic_parameter_attribute, special_constraint_mask, reference_type_constraint);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(generic_parameter_attribute, special_constraint_mask, non_nullable_value_type_constraint);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(generic_parameter_attribute, special_constraint_mask, default_constructor_constraint);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, manifest_resource_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(manifest_resource_attribute, visibility_mask, public_);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(manifest_resource_attribute, visibility_mask, private_);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, method_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, member_access_mask, compiler_controlled);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, member_access_mask, private_);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, member_access_mask, family_and_assembly);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, member_access_mask, assembly);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, member_access_mask, family);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, member_access_mask, family_or_assembly);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, member_access_mask, public_);

        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, static_);
        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, final);
        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, virtual_);
        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, hide_by_sig);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, vtable_layout_mask, reuse_slot);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_attribute, vtable_layout_mask, new_slot);

        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, strict);
        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, abstract);
        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, special_name);

        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, pinvoke_impl);
        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, runtime_special_name);
        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, has_security);
        CXXREFLECT_WRITE_IF_BIT_SET(method_attribute, require_security_object);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, method_implementation_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_implementation_attribute, code_type_mask, il);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_implementation_attribute, code_type_mask, native);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_implementation_attribute, code_type_mask, runtime);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_implementation_attribute, managed_mask, unmanaged);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(method_implementation_attribute, managed_mask, managed);

        CXXREFLECT_WRITE_IF_BIT_SET(method_implementation_attribute, forward_ref);
        CXXREFLECT_WRITE_IF_BIT_SET(method_implementation_attribute, preserve_sig);
        CXXREFLECT_WRITE_IF_BIT_SET(method_implementation_attribute, internal_call);
        CXXREFLECT_WRITE_IF_BIT_SET(method_implementation_attribute, synchronized);
        CXXREFLECT_WRITE_IF_BIT_SET(method_implementation_attribute, no_inlining);
        CXXREFLECT_WRITE_IF_BIT_SET(method_implementation_attribute, no_optimization);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, method_semantics_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_BIT_SET(method_semantics_attribute, setter);
        CXXREFLECT_WRITE_IF_BIT_SET(method_semantics_attribute, getter);
        CXXREFLECT_WRITE_IF_BIT_SET(method_semantics_attribute, other);
        CXXREFLECT_WRITE_IF_BIT_SET(method_semantics_attribute, add_on);
        CXXREFLECT_WRITE_IF_BIT_SET(method_semantics_attribute, remove_on);
        CXXREFLECT_WRITE_IF_BIT_SET(method_semantics_attribute, fire);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, parameter_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_BIT_SET(parameter_attribute, in);
        CXXREFLECT_WRITE_IF_BIT_SET(parameter_attribute, out);
        CXXREFLECT_WRITE_IF_BIT_SET(parameter_attribute, optional);
        CXXREFLECT_WRITE_IF_BIT_SET(parameter_attribute, has_default);
        CXXREFLECT_WRITE_IF_BIT_SET(parameter_attribute, has_field_marshal);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, pinvoke_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_BIT_SET(pinvoke_attribute, no_mangle);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, character_set_mask, character_set_mask_not_specified);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, character_set_mask, character_set_mask_ansi);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, character_set_mask, character_set_mask_unicode);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, character_set_mask, character_set_mask_auto);

        CXXREFLECT_WRITE_IF_BIT_SET(pinvoke_attribute, supports_last_error);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, calling_convention_mask, calling_convention_platform_api);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, calling_convention_mask, calling_convention_cdecl);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, calling_convention_mask, calling_convention_stdcall);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, calling_convention_mask, calling_convention_thiscall);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(pinvoke_attribute, calling_convention_mask, calling_convention_fastcall);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, property_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_BIT_SET(property_attribute, special_name);
        CXXREFLECT_WRITE_IF_BIT_SET(property_attribute, runtime_special_name);
        CXXREFLECT_WRITE_IF_BIT_SET(property_attribute, has_default);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, signature_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_BIT_SET(signature_attribute, has_this);
        CXXREFLECT_WRITE_IF_BIT_SET(signature_attribute, explicit_this);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, calling_convention_default);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, calling_convention_cdecl);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, calling_convention_stdcall);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, calling_convention_thiscall);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, calling_convention_fastcall);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, calling_convention_varargs);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, field);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, local);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(signature_attribute, calling_convention_mask, property_);

        CXXREFLECT_WRITE_IF_BIT_SET(signature_attribute, generic_);

        CXXREFLECT_WRITE_IF_EQUAL(signature_attribute, sentinel);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, type_attribute const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, visibility_mask, not_public);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, visibility_mask, public_);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, visibility_mask, nested_public);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, visibility_mask, nested_private);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, visibility_mask, nested_family);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, visibility_mask, nested_assembly);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, visibility_mask, nested_family_and_assembly);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, visibility_mask, nested_family_or_assembly);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, layout_mask, auto_layout);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, layout_mask, sequential_layout);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, layout_mask, explicit_layout);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, class_semantics_mask, class_);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, class_semantics_mask, interface_);

        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, abstract_);
        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, sealed);
        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, special_name);

        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, import);
        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, serializable);

        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, string_format_mask, ansi_class);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, string_format_mask, unicode_class);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, string_format_mask, auto_class);
        CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE(type_attribute, string_format_mask, custom_format_class);

        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, before_field_init);
        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, runtime_special_name);
        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, has_security);
        CXXREFLECT_WRITE_IF_BIT_SET(type_attribute, is_type_forwarder);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, element_type const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_EQUAL(element_type, end);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, void_type);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, boolean);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, character);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, i1);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, u1);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, i2);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, u2);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, i4);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, u4);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, i8);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, u8);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, r4);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, r8);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, string);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, ptr);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, by_ref);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, value_type);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, class_type);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, var);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, array);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, generic_inst);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, typed_by_ref);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, i);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, u);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, fn_ptr);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, object);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, sz_array);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, mvar);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, custom_modifier_required);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, custom_modifier_optional);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, internal);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, modifier);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, sentinel);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, pinned);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, type);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, custom_attribute_boxed_object);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, custom_attribute_field);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, custom_attribute_property);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, custom_attribute_enum);
        CXXREFLECT_WRITE_IF_EQUAL(element_type, cross_module_type_reference);
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, table_id const x) -> void
    {
        bool write_pipe(false);

        CXXREFLECT_WRITE_IF_EQUAL(table_id, module);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, type_ref);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, type_def);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, field);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, method_def);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, param);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, interface_impl);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, member_ref);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, constant);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, custom_attribute);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, field_marshal);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, decl_security);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, class_layout);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, field_layout);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, standalone_sig);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, event_map);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, event);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, property_map);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, property);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, method_semantics);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, method_impl);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, module_ref);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, type_spec);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, impl_map);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, field_rva);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, assembly);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, assembly_processor);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, assembly_os);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, assembly_ref);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, assembly_ref_processor);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, assembly_ref_os);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, file);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, exported_type);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, manifest_resource);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, nested_class);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, generic_param);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, method_spec);
        CXXREFLECT_WRITE_IF_EQUAL(table_id, generic_param_constraint);
    }

    #undef CXXREFLECT_WRITE_IF_BIT_SET
    #undef CXXREFLECT_WRITE_IF_HAS_MASKED_VALUE
    #undef CXXREFLECT_WRITE_IF_EQUAL

    auto insert_into_stream(core::base_wostream_wrapper& os, unrestricted_token const x) -> void
    {
        os.write(L"{0x");

        os.write(core::hex_format(x.value()));
        os.write(L"|");

        insert_into_stream(os, x.table());
        
        os.write(L":");
        os.write(x.index());

        os.write(L"}");
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, array_shape const& x) -> void
    {
        os.write(L"[rank:");

        os.write(x.rank());

        if (x.size_count() > 0)
        {
            os.write(L"/sizes:");
            bool is_first(true);
            core::for_all(x.sizes(), [&](core::size_type const n)
            {
                if (!is_first) { os.write(L","); }
                os.write(n);
                is_first = false;
            });
        }

        if (x.low_bound_count() > 0)
        {
            os.write(L"/bounds:");
            bool is_first(true);
            core::for_all(x.low_bounds(), [&](core::size_type const n)
            {
                if (!is_first) { os.write(L","); }
                os.write(n);
                is_first = false;
            });
        }

        os.write(L"]");
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, custom_modifier const& x) -> void
    {
        os.write(L"{");

        if (x.is_optional())
            os.write(L"mod_opt:");
        else if (x.is_required())
            os.write(L"mod_req:");

        insert_into_stream(os, x.type());

        os.write(L"}");
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, field_signature const& x) -> void
    {
        os.write(L"{field:");
        insert_into_stream(os, x.type());
        os.write(L"}");
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, method_signature const& x) -> void
    {
        os.write(L"{method:");

        if (x.has_this())
            os.write(L"has_this:");

        if (x.has_explicit_this())
            os.write(L"explicit_this:");

        insert_into_stream(os, x.calling_convention());
        os.write(L":");

        if (x.is_generic())
        {
            os.write(L"generic:");
            os.write(x.generic_parameter_count());
            os.write(L":");
        }

        os.write(L"returns:");
        insert_into_stream(os, x.return_type());

        os.write(L"parameters:(");
        {
            bool is_first(true);
            core::for_all(x.parameters(), [&](type_signature const& p)
            {
                if (!is_first) { os.write(L","); }
                insert_into_stream(os, p);
                is_first = false;
            });
        }
        os.write(L")");

        os.write(L"varargs:(");
        {
            bool is_first(true);
            core::for_all(x.vararg_parameters(), [&](type_signature const& p)
            {
                if (!is_first) { os.write(L","); }
                insert_into_stream(os, p);
                is_first = false;
            });
        }
        os.write(L")");

        os.write(L"}");
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, property_signature const& x) -> void
    {
        os.write(L"{property:");
        
        if (x.has_this())
            os.write(L"has_this:");

        os.write(L"type:");
        insert_into_stream(os, x.type());

        if (x.parameter_count() > 0)
        {
            os.write(L"parameters:(");
            bool is_first(true);
            core::for_all(x.parameters(), [&](type_signature const& p)
            {
                if (!is_first) { os.write(L","); }
                insert_into_stream(os, p);
                is_first = false;
            });
            os.write(L")");
        }

        os.write(L"}");
    }

    auto insert_into_stream(core::base_wostream_wrapper& os, type_signature const& x) -> void
    {
        os.write(L"{");

        core::for_all(x.custom_modifiers(), [&](custom_modifier const& m)
        {
            insert_into_stream(os, m);
        });

        if (x.is_by_ref())
            os.write(L"by_ref:");

        switch (x.get_element_type())
        {
        case element_type::void_type:
        case element_type::boolean:
        case element_type::character:
        case element_type::i1:
        case element_type::u1:
        case element_type::i2:
        case element_type::u2:
        case element_type::i4:
        case element_type::u4:
        case element_type::i8:
        case element_type::u8:
        case element_type::r4:
        case element_type::r8:
        case element_type::i:
        case element_type::u:
        case element_type::string:
        case element_type::object:
        case element_type::typed_by_ref:
            os.write(L"primitive:");
            insert_into_stream(os, x.primitive_type());
            break;

        case element_type::array:
            os.write(L"array:");
            insert_into_stream(os, x.array_type());
            insert_into_stream(os, x.array_shape());
            break;

        case element_type::sz_array:
            os.write(L"array:");
            insert_into_stream(os, x.array_type());
            break;

        case element_type::class_type:
            os.write(L"class:");
            insert_into_stream(os, x.class_type());
            break;

        case element_type::value_type:
            os.write(L"value_type:");
            insert_into_stream(os, x.class_type());
            break;

        case element_type::fn_ptr:
            os.write(L"fn_ptr:");
            insert_into_stream(os, x.function_type());
            break;

        case element_type::generic_inst:
            os.write(L"generic_inst:");
            insert_into_stream(os, x.generic_type());
            os.write(L"<");
            core::for_all(x.generic_arguments(), [&](type_signature const& s)
            {
                insert_into_stream(os, s);
            });
            os.write(L">");
            break;

        case element_type::ptr:
            os.write(L"pointer:");
            insert_into_stream(os, x.pointer_type());
            break;

        case element_type::annotated_mvar:
            os.write(L"mvar:");
            os.write(x.variable_number());
            os.write(L"/scope:");
            insert_into_stream(os, x.variable_context());
            break;

        case element_type::annotated_var:
            os.write(L"var:");
            os.write(x.variable_number());
            os.write(L"/scope:");
            insert_into_stream(os, x.variable_context());
            break;

        case element_type::mvar:
            os.write(L"mvar:");
            os.write(x.variable_number());
            break;

        case element_type::var:
            os.write(L"var:");
            os.write(x.variable_number());
            break;

        default:
            os.write(L"UNKNOWN");
            break;
        }

        os.write(L"}");
    }

} } }
