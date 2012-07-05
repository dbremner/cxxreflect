
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //





// CXXREFLECT TEST SUITE -- METADATA WRITER CXX
//
// This program loads an assembly using CxxReflect and dumps the metadata tables to a file.  It does
// not report any intertable relationship data; rather, it dumps each table independently.  For a
// higher-level metadata view, including intertable relationships and ownership information, use the
// reflection_writer_cxx program, which uses the CxxReflect reflection APIs.
//
// To use this program, set the input and output paths in the main() function at the bottom of this
// file, recompile, and run.





#include "cxxreflect/cxxreflect.hpp"

#define PP_CONCAT_(x, y) x ## y
#define PP_CONCAT(x, y) PP_CONCAT_(x, y)

#define PP_STRING_(x) # x
#define PP_STRING(x) PP_STRING_(x)

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::externals;
    using namespace cxxreflect::metadata;
}

namespace cxxreflect { namespace metadata {

    auto operator<<(cxr::file_handle& os, cxr::assembly_attribute              const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::assembly_hash_algorithm         const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::binding_attribute               const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::calling_convention              const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::event_attribute                 const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::field_attribute                 const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::file_attribute                  const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::generic_parameter_attribute     const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::manifest_resource_attribute     const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::method_attribute                const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::method_implementation_attribute const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::method_semantics_attribute      const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::parameter_attribute             const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::pinvoke_attribute               const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::property_attribute              const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::signature_attribute             const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::type_attribute                  const& x) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::element_type                    const& x) -> cxr::file_handle&;

    auto operator<<(cxr::file_handle& os, cxr::assembly_row                    const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::assembly_os_row                 const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::assembly_processor_row          const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_row                const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_os_row             const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_processor_row      const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::class_layout_row                const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::constant_row                    const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::custom_attribute_row            const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::decl_security_row               const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::event_map_row                   const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::event_row                       const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::exported_type_row               const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::field_row                       const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::field_layout_row                const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::field_marshal_row               const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::field_rva_row                   const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::file_row                        const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::generic_param_row               const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::generic_param_constraint_row    const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::impl_map_row                    const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::interface_impl_row              const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::manifest_resource_row           const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::member_ref_row                  const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::method_def_row                  const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::method_impl_row                 const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::method_semantics_row            const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::method_spec_row                 const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::module_row                      const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::module_ref_row                  const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::nested_class_row                const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::param_row                       const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::property_row                    const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::property_map_row                const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::standalone_sig_row              const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::type_def_row                    const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::type_ref_row                    const& r) -> cxr::file_handle&;
    auto operator<<(cxr::file_handle& os, cxr::type_spec_row                   const& r) -> cxr::file_handle&;

    auto write(cxr::file_handle& os, cxr::array_shape        const& s, cxr::size_type pad) -> cxr::file_handle&;
    auto write(cxr::file_handle& os, cxr::custom_modifier    const& s, cxr::size_type pad) -> cxr::file_handle&;
    auto write(cxr::file_handle& os, cxr::field_signature    const& s, cxr::size_type pad) -> cxr::file_handle&;
    auto write(cxr::file_handle& os, cxr::method_signature   const& s, cxr::size_type pad) -> cxr::file_handle&;
    auto write(cxr::file_handle& os, cxr::property_signature const& s, cxr::size_type pad) -> cxr::file_handle&;
    auto write(cxr::file_handle& os, cxr::type_signature     const& s, cxr::size_type pad) -> cxr::file_handle&;

    class as_hex
    {
    public:

        template <typename T>
        as_hex(T x) : _x((unsigned)x) { }

        friend auto operator<<(cxr::file_handle& os, as_hex const& o) -> cxr::file_handle&
        {
            os << L"0x" << cxr::hex_format(o._x);
            return os;
        }

    private:

        unsigned _x;
    };

    class as_bytes
    {
    public:

        as_bytes(cxr::blob x) : _x(x) { }

        friend auto operator<<(cxr::file_handle& os, as_bytes const& o) -> cxr::file_handle&
        {
            bool first(true);
            std::for_each(begin(o._x), end(o._x), [&](cxr::byte const v)
            {
                if (!first) os << L" ";
                std::array<wchar_t, 3> r((std::array<wchar_t, 3>()));
                std::swprintf(r.data(), r.size(), L"%02x", (unsigned)v);
                os << r.data();
                first = false;
            });
            return os;
        }

    private:

        cxr::blob _x;
    };

    #define CHECK_WRITE_SOLO(t, n)                         \
        if (((std::uint32_t)x & (std::uint32_t)t::n) != 0) \
        {                                                  \
            if (tag) { os <<L" | "; }                      \
            os << PP_CONCAT(L, PP_STRING(n));              \
            tag = true;                                    \
        }

    #define CHECK_WRITE_MASK(t, m, n)                                        \
        if (((std::uint32_t)x & (std::uint32_t)t::m) == (std::uint32_t)t::n) \
        {                                                                    \
            if (tag) { os <<L" | "; }                                        \
            os << PP_CONCAT(L, PP_STRING(n));                                \
            tag = true;                                                      \
        }

    #define CHECK_WRITE_BITS(t, n)            \
        if (x == t::n)                        \
        {                                     \
            if (tag) { os <<L" | "; }         \
            os << PP_CONCAT(L, PP_STRING(n)); \
            tag = true;                       \
        }

    auto operator<<(cxr::file_handle& os, cxr::assembly_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_SOLO(cxr::assembly_attribute, public_key);
        CHECK_WRITE_SOLO(cxr::assembly_attribute, retargetable);
        CHECK_WRITE_SOLO(cxr::assembly_attribute, disable_jit_compile_optimizer);
        CHECK_WRITE_SOLO(cxr::assembly_attribute, enable_jit_compile_tracking);
        CHECK_WRITE_MASK(cxr::assembly_attribute, content_type_mask, default_content_type);
        CHECK_WRITE_MASK(cxr::assembly_attribute, content_type_mask, windows_runtime_content_type);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_hash_algorithm const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_BITS(cxr::assembly_hash_algorithm, none);
        CHECK_WRITE_BITS(cxr::assembly_hash_algorithm, md5);
        CHECK_WRITE_BITS(cxr::assembly_hash_algorithm, sha1);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::binding_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_BITS(cxr::binding_attribute, default_);
        CHECK_WRITE_SOLO(cxr::binding_attribute, ignore_case);
        CHECK_WRITE_SOLO(cxr::binding_attribute, declared_only);
        CHECK_WRITE_SOLO(cxr::binding_attribute, instance);
        CHECK_WRITE_SOLO(cxr::binding_attribute, static_);
        CHECK_WRITE_SOLO(cxr::binding_attribute, public_);
        CHECK_WRITE_SOLO(cxr::binding_attribute, non_public);
        CHECK_WRITE_SOLO(cxr::binding_attribute, flatten_hierarchy);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::calling_convention const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_SOLO(cxr::calling_convention, standard);
        CHECK_WRITE_SOLO(cxr::calling_convention, varargs);
        CHECK_WRITE_SOLO(cxr::calling_convention, has_this);
        CHECK_WRITE_SOLO(cxr::calling_convention, explicit_this);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::event_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_SOLO(cxr::event_attribute, special_name);
        CHECK_WRITE_SOLO(cxr::event_attribute, runtime_special_name);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_MASK(cxr::field_attribute, field_access_mask, compiler_controlled);
        CHECK_WRITE_MASK(cxr::field_attribute, field_access_mask, private_);
        CHECK_WRITE_MASK(cxr::field_attribute, field_access_mask, family_and_assembly);
        CHECK_WRITE_MASK(cxr::field_attribute, field_access_mask, assembly);
        CHECK_WRITE_MASK(cxr::field_attribute, field_access_mask, family);
        CHECK_WRITE_MASK(cxr::field_attribute, field_access_mask, family_or_assembly);
        CHECK_WRITE_MASK(cxr::field_attribute, field_access_mask, public_);

        CHECK_WRITE_SOLO(cxr::field_attribute, static_);
        CHECK_WRITE_SOLO(cxr::field_attribute, init_only);
        CHECK_WRITE_SOLO(cxr::field_attribute, literal);
        CHECK_WRITE_SOLO(cxr::field_attribute, not_serialized);
        CHECK_WRITE_SOLO(cxr::field_attribute, special_name);
        
        CHECK_WRITE_SOLO(cxr::field_attribute, pinvoke_impl);
   
        CHECK_WRITE_SOLO(cxr::field_attribute, runtime_special_name);    
        CHECK_WRITE_SOLO(cxr::field_attribute, has_field_marshal);
        CHECK_WRITE_SOLO(cxr::field_attribute, has_default);
        CHECK_WRITE_SOLO(cxr::field_attribute, has_field_rva);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::file_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_BITS(cxr::file_attribute, contains_metadata);
        CHECK_WRITE_BITS(cxr::file_attribute, contains_no_metadata);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::generic_parameter_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_MASK(cxr::generic_parameter_attribute, variance_mask, none);
        CHECK_WRITE_MASK(cxr::generic_parameter_attribute, variance_mask, covariant);
        CHECK_WRITE_MASK(cxr::generic_parameter_attribute, variance_mask, contravariant);

        CHECK_WRITE_MASK(cxr::generic_parameter_attribute, special_constraint_mask, reference_type_constraint);
        CHECK_WRITE_MASK(cxr::generic_parameter_attribute, special_constraint_mask, non_nullable_value_type_constraint);
        CHECK_WRITE_MASK(cxr::generic_parameter_attribute, special_constraint_mask, default_constructor_constraint);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::manifest_resource_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_MASK(cxr::manifest_resource_attribute, visibility_mask, public_);
        CHECK_WRITE_MASK(cxr::manifest_resource_attribute, visibility_mask, private_);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_MASK(cxr::method_attribute, member_access_mask, compiler_controlled);
        CHECK_WRITE_MASK(cxr::method_attribute, member_access_mask, private_);
        CHECK_WRITE_MASK(cxr::method_attribute, member_access_mask, family_and_assembly);
        CHECK_WRITE_MASK(cxr::method_attribute, member_access_mask, assembly);
        CHECK_WRITE_MASK(cxr::method_attribute, member_access_mask, family);
        CHECK_WRITE_MASK(cxr::method_attribute, member_access_mask, family_or_assembly);
        CHECK_WRITE_MASK(cxr::method_attribute, member_access_mask, public_);

        CHECK_WRITE_SOLO(cxr::method_attribute, static_);
        CHECK_WRITE_SOLO(cxr::method_attribute, final);
        CHECK_WRITE_SOLO(cxr::method_attribute, virtual_);
        CHECK_WRITE_SOLO(cxr::method_attribute, hide_by_sig);

        CHECK_WRITE_MASK(cxr::method_attribute, vtable_layout_mask, reuse_slot);
        CHECK_WRITE_MASK(cxr::method_attribute, vtable_layout_mask, new_slot);

        CHECK_WRITE_SOLO(cxr::method_attribute, strict);
        CHECK_WRITE_SOLO(cxr::method_attribute, abstract);
        CHECK_WRITE_SOLO(cxr::method_attribute, special_name);

        CHECK_WRITE_SOLO(cxr::method_attribute, pinvoke_impl);
        CHECK_WRITE_SOLO(cxr::method_attribute, runtime_special_name);
        CHECK_WRITE_SOLO(cxr::method_attribute, has_security);
        CHECK_WRITE_SOLO(cxr::method_attribute, require_security_object);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_implementation_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_MASK(cxr::method_implementation_attribute, code_type_mask, il);
        CHECK_WRITE_MASK(cxr::method_implementation_attribute, code_type_mask, native);
        CHECK_WRITE_MASK(cxr::method_implementation_attribute, code_type_mask, runtime);

        CHECK_WRITE_MASK(cxr::method_implementation_attribute, managed_mask, unmanaged);
        CHECK_WRITE_MASK(cxr::method_implementation_attribute, managed_mask, managed);

        CHECK_WRITE_SOLO(cxr::method_implementation_attribute, forward_ref);
        CHECK_WRITE_SOLO(cxr::method_implementation_attribute, preserve_sig);
        CHECK_WRITE_SOLO(cxr::method_implementation_attribute, internal_call);
        CHECK_WRITE_SOLO(cxr::method_implementation_attribute, synchronized);
        CHECK_WRITE_SOLO(cxr::method_implementation_attribute, no_inlining);
        CHECK_WRITE_SOLO(cxr::method_implementation_attribute, no_optimization);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_semantics_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_SOLO(cxr::method_semantics_attribute, setter);
        CHECK_WRITE_SOLO(cxr::method_semantics_attribute, getter);
        CHECK_WRITE_SOLO(cxr::method_semantics_attribute, other);
        CHECK_WRITE_SOLO(cxr::method_semantics_attribute, add_on);
        CHECK_WRITE_SOLO(cxr::method_semantics_attribute, remove_on);
        CHECK_WRITE_SOLO(cxr::method_semantics_attribute, fire);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::parameter_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_SOLO(cxr::parameter_attribute, in);
        CHECK_WRITE_SOLO(cxr::parameter_attribute, out);
        CHECK_WRITE_SOLO(cxr::parameter_attribute, optional);
        CHECK_WRITE_SOLO(cxr::parameter_attribute, has_default);
        CHECK_WRITE_SOLO(cxr::parameter_attribute, has_field_marshal);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::pinvoke_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_SOLO(cxr::pinvoke_attribute, no_mangle);

        CHECK_WRITE_MASK(cxr::pinvoke_attribute, character_set_mask, character_set_mask_not_specified);
        CHECK_WRITE_MASK(cxr::pinvoke_attribute, character_set_mask, character_set_mask_ansi);
        CHECK_WRITE_MASK(cxr::pinvoke_attribute, character_set_mask, character_set_mask_unicode);
        CHECK_WRITE_MASK(cxr::pinvoke_attribute, character_set_mask, character_set_mask_auto);

        CHECK_WRITE_SOLO(cxr::pinvoke_attribute, supports_last_error);

        CHECK_WRITE_MASK(cxr::pinvoke_attribute, calling_convention_mask, calling_convention_platform_api);
        CHECK_WRITE_MASK(cxr::pinvoke_attribute, calling_convention_mask, calling_convention_cdecl);
        CHECK_WRITE_MASK(cxr::pinvoke_attribute, calling_convention_mask, calling_convention_stdcall);
        CHECK_WRITE_MASK(cxr::pinvoke_attribute, calling_convention_mask, calling_convention_thiscall);
        CHECK_WRITE_MASK(cxr::pinvoke_attribute, calling_convention_mask, calling_convention_fastcall);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::property_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_SOLO(cxr::property_attribute, special_name);
        CHECK_WRITE_SOLO(cxr::property_attribute, runtime_special_name);
        CHECK_WRITE_SOLO(cxr::property_attribute, has_default);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::signature_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_SOLO(cxr::signature_attribute, has_this);
        CHECK_WRITE_SOLO(cxr::signature_attribute, explicit_this);

        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, calling_convention_default);
        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, calling_convention_cdecl);
        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, calling_convention_stdcall);
        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, calling_convention_thiscall);
        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, calling_convention_fastcall);
        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, calling_convention_varargs);
        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, field);
        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, local);
        CHECK_WRITE_MASK(cxr::signature_attribute, calling_convention_mask, property_);

        CHECK_WRITE_SOLO(cxr::signature_attribute, generic_);

        CHECK_WRITE_BITS(cxr::signature_attribute, sentinel);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::type_attribute const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_MASK(cxr::type_attribute, visibility_mask, not_public);
        CHECK_WRITE_MASK(cxr::type_attribute, visibility_mask, public_);
        CHECK_WRITE_MASK(cxr::type_attribute, visibility_mask, nested_public);
        CHECK_WRITE_MASK(cxr::type_attribute, visibility_mask, nested_private);
        CHECK_WRITE_MASK(cxr::type_attribute, visibility_mask, nested_family);
        CHECK_WRITE_MASK(cxr::type_attribute, visibility_mask, nested_assembly);
        CHECK_WRITE_MASK(cxr::type_attribute, visibility_mask, nested_family_and_assembly);
        CHECK_WRITE_MASK(cxr::type_attribute, visibility_mask, nested_family_or_assembly);

        CHECK_WRITE_MASK(cxr::type_attribute, layout_mask, auto_layout);
        CHECK_WRITE_MASK(cxr::type_attribute, layout_mask, sequential_layout);
        CHECK_WRITE_MASK(cxr::type_attribute, layout_mask, explicit_layout);

        CHECK_WRITE_MASK(cxr::type_attribute, class_semantics_mask, class_);
        CHECK_WRITE_MASK(cxr::type_attribute, class_semantics_mask, interface);

        CHECK_WRITE_SOLO(cxr::type_attribute, abstract_);
        CHECK_WRITE_SOLO(cxr::type_attribute, sealed);
        CHECK_WRITE_SOLO(cxr::type_attribute, special_name);

        CHECK_WRITE_SOLO(cxr::type_attribute, import);
        CHECK_WRITE_SOLO(cxr::type_attribute, serializable);

        CHECK_WRITE_MASK(cxr::type_attribute, string_format_mask, ansi_class);
        CHECK_WRITE_MASK(cxr::type_attribute, string_format_mask, unicode_class);
        CHECK_WRITE_MASK(cxr::type_attribute, string_format_mask, auto_class);
        CHECK_WRITE_MASK(cxr::type_attribute, string_format_mask, custom_format_class);

        CHECK_WRITE_SOLO(cxr::type_attribute, before_field_init);
        CHECK_WRITE_SOLO(cxr::type_attribute, runtime_special_name);
        CHECK_WRITE_SOLO(cxr::type_attribute, has_security);
        CHECK_WRITE_SOLO(cxr::type_attribute, is_type_forwarder);

        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::element_type const& x) -> cxr::file_handle&
    {
        bool tag(false);

        CHECK_WRITE_BITS(cxr::element_type, end);
        CHECK_WRITE_BITS(cxr::element_type, void_type);
        CHECK_WRITE_BITS(cxr::element_type, boolean);
        CHECK_WRITE_BITS(cxr::element_type, character);
        CHECK_WRITE_BITS(cxr::element_type, i1);
        CHECK_WRITE_BITS(cxr::element_type, u1);
        CHECK_WRITE_BITS(cxr::element_type, i2);
        CHECK_WRITE_BITS(cxr::element_type, u2);
        CHECK_WRITE_BITS(cxr::element_type, i4);
        CHECK_WRITE_BITS(cxr::element_type, u4);
        CHECK_WRITE_BITS(cxr::element_type, i8);
        CHECK_WRITE_BITS(cxr::element_type, u8);
        CHECK_WRITE_BITS(cxr::element_type, r4);
        CHECK_WRITE_BITS(cxr::element_type, r8);
        CHECK_WRITE_BITS(cxr::element_type, string);
        CHECK_WRITE_BITS(cxr::element_type, ptr);
        CHECK_WRITE_BITS(cxr::element_type, by_ref);
        CHECK_WRITE_BITS(cxr::element_type, value_type);
        CHECK_WRITE_BITS(cxr::element_type, class_type);
        CHECK_WRITE_BITS(cxr::element_type, var);
        CHECK_WRITE_BITS(cxr::element_type, array);
        CHECK_WRITE_BITS(cxr::element_type, generic_inst);
        CHECK_WRITE_BITS(cxr::element_type, typed_by_ref);
        CHECK_WRITE_BITS(cxr::element_type, i);
        CHECK_WRITE_BITS(cxr::element_type, u);
        CHECK_WRITE_BITS(cxr::element_type, fn_ptr);
        CHECK_WRITE_BITS(cxr::element_type, object);
        CHECK_WRITE_BITS(cxr::element_type, sz_array);
        CHECK_WRITE_BITS(cxr::element_type, mvar);
        CHECK_WRITE_BITS(cxr::element_type, custom_modifier_required);
        CHECK_WRITE_BITS(cxr::element_type, custom_modifier_optional);
        CHECK_WRITE_BITS(cxr::element_type, internal);
        CHECK_WRITE_BITS(cxr::element_type, modifier);
        CHECK_WRITE_BITS(cxr::element_type, sentinel);
        CHECK_WRITE_BITS(cxr::element_type, pinned);
        CHECK_WRITE_BITS(cxr::element_type, type);
        CHECK_WRITE_BITS(cxr::element_type, custom_attribute_boxed_object);
        CHECK_WRITE_BITS(cxr::element_type, custom_attribute_field);
        CHECK_WRITE_BITS(cxr::element_type, custom_attribute_property);
        CHECK_WRITE_BITS(cxr::element_type, custom_attribute_enum);
        CHECK_WRITE_BITS(cxr::element_type, cross_module_type_reference);

        return os;
    }

    #undef CHECK_WRITE_BITS
    #undef CHECK_WRITE_SOLO
    #undef CHECK_WRITE_MASK

    auto operator<<(cxr::file_handle& os, cxr::assembly_row const& r) -> cxr::file_handle&
    {
        os << L"// assembly [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * hash_algorithm [" << as_hex(r.hash_algorithm()) << L"] [" <<  r.hash_algorithm() << L"]\n";
        os << L"//  * version ["
               << r.version().major() << L"."
               << r.version().minor() << L"."
               << r.version().build() << L"."
               << r.version().revision()
           << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * public_key [" << as_bytes(r.public_key()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * culture [" << r.culture().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_os_row const& r) -> cxr::file_handle&
    {
        os << L"// assembly_os [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * platform_id [" << r.platform_id() << L"]\n";
        os << L"//  * major_version [" << r.major_version() << L"]\n";
        os << L"//  * minor_version [" << r.minor_version() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_processor_row const& r) -> cxr::file_handle&
    {
        os << L"// assembly_processor [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * processor [" << r.processor() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_row const& r) -> cxr::file_handle&
    {
        os << L"// assembly_ref [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * version ["
               << r.version().major() << L"."
               << r.version().minor() << L"."
               << r.version().build() << L"."
               << r.version().revision()
           << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * public_key [" << as_bytes(r.public_key()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * culture [" << r.culture().c_str() << L"]\n";
        os << L"//  * hash_value [" << as_bytes(r.hash_value()) << L"]\n";
        os << L"// \n";
        return os;
    }


    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_os_row const& r) -> cxr::file_handle&
    {
        os << L"// assembly_ref_os [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * platform_id [" << r.platform_id() << L"]\n";
        os << L"//  * major_version [" << r.major_version() << L"]\n";
        os << L"//  * minor_version [" << r.minor_version() << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::assembly_ref_processor_row const& r) -> cxr::file_handle&
    {
        os << L"// assembly_ref_processor [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * processor [" << r.processor() << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::class_layout_row const& r) -> cxr::file_handle&
    {
        os << L"// class_layout [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * packing_size [" << r.packing_size() << L"]\n";
        os << L"//  * class_size [" << r.class_size() << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::constant_row const& r) -> cxr::file_handle&
    {
        os << L"// constant [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * type [" << r.type() << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * value [" << as_bytes(r.value()) << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::custom_attribute_row const& r) -> cxr::file_handle&
    {
        os << L"// custom_attribute [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * type [" << as_hex(r.type().value()) << L"]\n";
        os << L"//  * value [" << as_bytes(r.value()) << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::decl_security_row const& r) -> cxr::file_handle&
    {
        os << L"// decl_security [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * action [" << r.action() << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * permission_set [" << as_bytes(r.permission_set()) << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::event_map_row const& r) -> cxr::file_handle&
    {
        os << L"// event_map [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * events ["
               << as_hex(r.first_event().value()) << L" ~ "
               << as_hex(r.last_event().value())
           << L"]\n";
        os << L"\n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::event_row const& r) -> cxr::file_handle&
    {
        os << L"// event [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * type [" << as_hex(r.type().value()) << L"]\n";
        os << L"// \n";
        return os;
    }
    
    auto operator<<(cxr::file_handle& os, cxr::exported_type_row const& r) -> cxr::file_handle&
    {
        os << L"// exported_type [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * type_def_id [" << r.type_def_id() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * namespace_name [" << r.namespace_name().c_str() << L"]\n";
        os << L"//  * implementation [" << as_hex(r.implementation().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_row const& r) -> cxr::file_handle&
    {
        os << L"// field [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        write(os, r.signature().as<cxr::field_signature>(), 0);
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_layout_row const& r) -> cxr::file_handle&
    {
        os << L"// field_layout [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * offset [" << r.offset() << L"]\n"; 
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_marshal_row const& r) -> cxr::file_handle&
    {
        os << L"// field_marshal [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * native_type [" << as_bytes(r.native_type()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::field_rva_row const& r) -> cxr::file_handle&
    {
        os << L"// field_rva [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * rva [" << as_hex(r.rva()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::file_row const& r) -> cxr::file_handle&
    {
        os << L"// file [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * hash_value [" << as_bytes(r.hash_value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::generic_param_row const& r) -> cxr::file_handle&
    {
        os << L"// generic_param [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * sequence [" << r.sequence() << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::generic_param_constraint_row const& r) -> cxr::file_handle&
    {
        os << L"// generic_param_constraint [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * constraint [" << as_hex(r.constraint().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::impl_map_row const& r) -> cxr::file_handle&
    {
        os << L"// impl_map [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * mapping_flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * member_forwarded [" << as_hex(r.member_forwarded().value()) << L"]\n";
        os << L"//  * import_name [" << r.import_name().c_str() << L"]\n";
        os << L"//  * import_scope [" << as_hex(r.import_scope().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::interface_impl_row const& r) -> cxr::file_handle&
    {
        os << L"// interface_impl [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * interface [" << as_hex(r.interface().value()) << L"]\n";
        os << L"// \n";
        return os;
    }
    
    auto operator<<(cxr::file_handle& os, cxr::manifest_resource_row const& r) -> cxr::file_handle&
    {
        os << L"// manifest_resource [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * offset [" << as_hex(r.offset()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        if (r.implementation().is_initialized())
            os << L"//  * implementation [" << as_hex(r.implementation().value()) << L"]\n";
        else
            os << L"//  * implementation [<none>]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::member_ref_row const& r) -> cxr::file_handle&
    {
        os << L"// member_ref [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        // TODO Determine signature type
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_def_row const& r) -> cxr::file_handle&
    {
        os << L"// method_def [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * rva [" << as_hex(r.rva()) << L"]\n";
        os << L"//  * implementation_flags [" << as_hex(r.implementation_flags().integer()) << L"] [" << r.implementation_flags().enumerator() << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        write(os, r.signature().as<cxr::method_signature>(), 0);
        os << L"//  * parameters ["
               << as_hex(r.first_parameter().value()) << L" ~ "
               << as_hex(r.last_parameter().value())
           << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_impl_row const& r) -> cxr::file_handle&
    {
        os << L"// method_impl [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * method_body [" << as_hex(r.method_body().value()) << L"]\n";
        os << L"//  * method_declaration [" << as_hex(r.method_declaration().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::method_semantics_row const& r) -> cxr::file_handle&
    {
        os << L"// method_semantics [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * semantics [" << as_hex(r.semantics().integer()) << L"] [" << r.semantics().enumerator() << L"]\n";
        os << L"//  * method [" << as_hex(r.method().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"// \n";
        return os;
    }
    
    auto operator<<(cxr::file_handle& os, cxr::method_spec_row const& r) -> cxr::file_handle&
    {
        os << L"// method_spec [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * method [" << as_hex(r.method().value()) << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        write(os, r.signature().as<cxr::type_signature>(), 0);
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::module_row const& r) -> cxr::file_handle&
    {
        os << L"// module [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * mvid [" << as_bytes(r.mvid()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::module_ref_row const& r) -> cxr::file_handle&
    {
        os << L"// module_ref [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::nested_class_row const& r) -> cxr::file_handle&
    {
        os << L"// nested_class [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * nested_class [" << as_hex(r.nested_class().value()) << L"]\n";
        os << L"//  * enclosing_class [" << as_hex(r.enclosing_class().value()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::param_row const& r) -> cxr::file_handle&
    {
        os << L"// param [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * sequence [" << r.sequence() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::property_row const& r) -> cxr::file_handle&
    {
        os << L"// property [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        write(os, r.signature().as<cxr::property_signature>(), 0);
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::property_map_row const& r) -> cxr::file_handle&
    {
        os << L"// property_map [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * parent [" << as_hex(r.parent().value()) << L"]\n";
        os << L"//  * properties ["
               << as_hex(r.first_property().value()) << L" ~ "
               << as_hex(r.last_property().value())
           << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::standalone_sig_row const& r) -> cxr::file_handle&
    {
        os << L"// standalone_sig [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::type_def_row const& r) -> cxr::file_handle&
    {
        os << L"// type_def [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * flags [" << as_hex(r.flags().integer()) << L"] [" << r.flags().enumerator() << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * namespace_name [" << r.namespace_name().c_str() << L"]\n";
        if (r.extends().is_initialized())
            os << L"//  * extends [" << as_hex(r.extends().value()) << L"]\n";
        else
            os << L"//  * extends [<none>]\n";
        os << L"//  * fields ["
               << as_hex(r.first_field().value()) << L" ~ "
               << as_hex(r.last_field().value())
           << L"]\n";
        os << L"//  * methods ["
               << as_hex(r.first_method().value()) << L" ~ "
               << as_hex(r.last_method().value())
           << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::type_ref_row const& r) -> cxr::file_handle&
    {
        os << L"// type_ref [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * resolution_scope [" << as_hex(r.resolution_scope().value()) << L"]\n";
        os << L"//  * name [" << r.name().c_str() << L"]\n";
        os << L"//  * namespace_name [" << r.namespace_name().c_str() << L"]\n";
        os << L"// \n";
        return os;
    }

    auto operator<<(cxr::file_handle& os, cxr::type_spec_row const& r) -> cxr::file_handle&
    {
        os << L"// type_spec [index:" << r.token().index() << L"] [" << as_hex(r.token().value()) << L"]\n";
        os << L"//  * signature [" << as_bytes(r.signature()) << L"]\n";
        write(os, r.signature().as<cxr::type_signature>(), 0);
        os << L"// \n";
        return os;
    }

    auto write(cxr::file_handle& os, cxr::array_shape const& s, cxr::size_type pad) -> cxr::file_handle&
    {
        cxr::string const start(L"//    " + cxr::string(pad, L' '));

        os << start.c_str();
        os << L"[array_shape | rank: ";
        os << s.rank();
        os << L" | sizes: ";
        std::for_each(s.begin_sizes(), s.end_sizes(), [&](cxr::size_type const x)
        {
            os << x << L" ";
        });
        os << L" | low_bounds: ";
        std::for_each(s.begin_low_bounds(), s.end_low_bounds(), [&](cxr::size_type const x)
        {
            os << x << L" ";
        });
        os << L"]\n";

        return os;
    }

    auto write(cxr::file_handle& os, cxr::custom_modifier const& s, cxr::size_type pad) -> cxr::file_handle&
    {
        cxr::string const start(L"//    " + cxr::string(pad, L' '));

        os << start.c_str();
        os << L"[custom_modifier | ";
        os << (s.is_optional() ? L"optional" : L"required");
        os << L" | token: ";
        os << as_hex(s.type().value());
        os << L"]\n";

        return os;
    }

    auto write(cxr::file_handle& os, cxr::field_signature const& s, cxr::size_type pad) -> cxr::file_handle&
    {
        cxr::string const start(L"//    " + cxr::string(pad, L' '));

        os << start.c_str() << L"[field]\n";
        write(os, s.type(), pad + 4);
        os << start.c_str() << L"[end_field]\n";

        return os;
    }

    auto write(cxr::file_handle& os, cxr::method_signature const& s, cxr::size_type pad) -> cxr::file_handle&
    {
        cxr::string const start(L"//    " + cxr::string(pad, L' '));

        os << start.c_str() << L"[method: ";
        os << (s.has_this() ? L"has_this " : L"");
        os << (s.has_explicit_this() ? L"explicit_this " : L"");
        os << L" | convention: ";
        os << (cxr::calling_convention)s.calling_convention();
        os << L" | is_generic: " << (s.is_generic() ? L"true" : L"false");
        if (s.is_generic())
            os << L" (arity: " << s.generic_parameter_count() << L")";
        os << L"]\n";
        os << start.c_str() << L"    [return_type]\n";
        write(os, s.return_type(), pad + 8);
        os << start.c_str() << L"    [end_return_type]\n";
        os << start.c_str() << L"    [parameters: " << s.parameter_count() << L"]\n";
        std::for_each(s.begin_parameters(), s.end_parameters(), [&](cxr::type_signature const& x)
        {
            write(os, x, pad + 8);
        });
        os << start.c_str() << L"    [end_parameters]\n";
        os << start.c_str() << L"    [vararg_parameters: " << s.parameter_count() << L"]\n";
        std::for_each(s.begin_vararg_parameters(), s.end_vararg_parameters(), [&](cxr::type_signature const& x)
        {
            write(os, x, pad + 8);
        });
        os << start.c_str() << L"    [end_vararg_parameters]\n";
        os << start.c_str() << L"[end_method]\n";

        return os;
    }

    auto write(cxr::file_handle& os, cxr::property_signature const& s, cxr::size_type pad) -> cxr::file_handle&
    {
        cxr::string const start(L"//    " + cxr::string(pad, L' '));

        os << start.c_str();
        os << L"[property | has_this: ";
        os << (s.has_this() ? L"true" : L"false");
        os << L"]\n";
        os << start.c_str() << L"    [property_parameters]\n";
        std::for_each(s.begin_parameters(), s.end_parameters(), [&](type_signature const& p)
        {
            write(os, p, pad + 8);
        });
        os << start.c_str() << L"    [end_property_parameters]\n";
        os << start.c_str() << L"    [property_type]\n";
        write(os, s.type(), pad + 8);
        os << start.c_str() << L"    [end_property_type]\n";
        os << start.c_str() << L"[end_property]\n";

        return os;
    }

    auto write(cxr::file_handle& os, cxr::type_signature const& s, cxr::size_type pad) -> cxr::file_handle&
    {
        cxr::string const start(L"//    " + cxr::string(pad, L' '));

        os << start.c_str();
        os << L"[type | kind: ";
        switch (s.get_kind())
        {
        case cxr::type_signature::kind::unknown:          throw cxr::logic_error(L"unknown kind");
        case cxr::type_signature::kind::primitive:        os << L"primitive";    break;
        case cxr::type_signature::kind::general_array:    os << L"array";        break;
        case cxr::type_signature::kind::simple_array:     os << L"sz_array";     break;
        case cxr::type_signature::kind::class_type:       os << L"class_type";   break;
        case cxr::type_signature::kind::function_pointer: os << L"fn_ptr";       break;
        case cxr::type_signature::kind::generic_instance: os << L"generic_inst"; break;
        case cxr::type_signature::kind::pointer:          os << L"ptr";          break;
        case cxr::type_signature::kind::variable:         os << L"var";          break;
        default:                                          throw cxr::logic_error(L"unknown kind");
        }
        os << L" | element_type: ";
        os << s.get_element_type();
        os << L" | by_ref: ";
        os << (s.is_by_ref() ? L"true" : L"false");
        os << L"]\n";
        os << start.c_str() << L"    [custom_modifiers]\n";
        std::for_each(s.begin_custom_modifiers(), s.end_custom_modifiers(), [&](cxr::custom_modifier const& m)
        {
            write(os, m, pad + 8);
        });
        os << start.c_str() << L"    [end_custom_modifiers]\n";

        switch (s.get_kind())
        {
        case cxr::type_signature::kind::primitive:
            os << start.c_str() << L"    [primitive: " << s.primitive_type() << L"]\n";
            break;

        case cxr::type_signature::kind::general_array:
        case cxr::type_signature::kind::simple_array:
            os << start.c_str() << L"    [array: " << (s.is_general_array() ? L"general" : L"simple") << L"]\n";
            os << start.c_str() << L"        [array_type]\n";
            write(os, s.array_type(), pad + 12);
            os << start.c_str() << L"        [end_array_type]\n";
            if (s.is_general_array())
            {
                os << start.c_str() << L"        [array_shape]\n";
                write(os, s.array_shape(), pad + 12);
                os << start.c_str() << L"        [end_array_shape]\n";
            }
            os << start.c_str() << L"    [end_array]\n";
            break;

        case cxr::type_signature::kind::class_type:
            os << start.c_str() << L"    [" << (s.is_class_type() ? L"class_type" : L"value_type")
               << L": " << as_hex(s.class_type().value()) << L"]\n";
            break;

        case cxr::type_signature::kind::function_pointer:
            os << start.c_str() << L"    [fn_ptr]\n";
            write(os, s.function_type(), pad + 8);
            os << start.c_str() << L"    [end_fn_ptr]\n";
            break;

        case cxr::type_signature::kind::generic_instance:
            os << start.c_str() << L"    [generic_inst: ";
            os << (s.is_generic_class_type_instance() ? L"class" : L"value") << L" | type: ";
            os << as_hex(s.generic_type().value()) << L" | arity: ";
            os << s.generic_argument_count() << L"]\n";
            std::for_each(s.begin_generic_arguments(), s.end_generic_arguments(), [&](cxr::type_signature const& x)
            {
                write(os, x, pad + 8);
            });
            os << start.c_str() << L"    [end_generic_inst]\n";
            break;

        case cxr::type_signature::kind::pointer:
            os << start.c_str() << L"    [pointer]\n";
            write(os, s.pointer_type(), pad + 8);
            os << start.c_str() << L"    [end_pointer]\n";
            break;

        case cxr::type_signature::kind::variable:
            os << start.c_str() << L"    [" << (s.is_class_variable() ? L"class" : L"method")
               << L"_variable: " << s.variable_number() << L"]\n";
            break;
        }

        os << start.c_str() << L"[end_type]\n";

        return os;
    }

} }

namespace {

    template <cxr::table_id Id>
    auto write_table(cxr::file_handle        & os,
                     cxr::database      const& scope,
                     cxr::string        const& table_name) -> cxr::file_handle&
    {
        typedef typename cxr::row_type_for_table_id<Id>::type row_type;

        os << L"\n";
        os << L"\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        os << L"// TABLE [" << table_name.c_str() << L"]\n";
        os << L"////////////////////////////////////////////////////////////////////////////////\n";
        std::for_each(scope.begin<Id>(), scope.end<Id>(), [&](decltype(*scope.begin<Id>()) x)
        {
            os << x;
        });

        return os;
    }
}

auto main() -> int
{
    cxr::externals::initialize(cxr::win32_externals());

    cxr::string const input_path(L"c:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");
    cxr::string const output_path(L"c:\\jm\\metadata_writer_cxx.txt");

    cxr::database scope(cxr::database::create_from_file(input_path.c_str()));

    cxr::file_handle of(output_path.c_str(), cxr::file_mode::write);

    #define WRITE_TABLE(x) write_table<cxr::table_id::x>(of, scope, PP_CONCAT(L, PP_STRING(x)))

    WRITE_TABLE(assembly);
    WRITE_TABLE(assembly_os);
    WRITE_TABLE(assembly_processor);
    WRITE_TABLE(assembly_ref);
    WRITE_TABLE(assembly_ref_os);
    WRITE_TABLE(assembly_ref_processor);
    WRITE_TABLE(class_layout);
    WRITE_TABLE(constant);
    WRITE_TABLE(custom_attribute);
    WRITE_TABLE(decl_security);
    WRITE_TABLE(event_map);
    WRITE_TABLE(event);
    WRITE_TABLE(exported_type);
    WRITE_TABLE(field);
    WRITE_TABLE(field_layout);
    WRITE_TABLE(field_marshal);
    WRITE_TABLE(field_rva);
    WRITE_TABLE(file);
    WRITE_TABLE(generic_param);
    WRITE_TABLE(generic_param_constraint);
    WRITE_TABLE(impl_map);
    WRITE_TABLE(interface_impl);
    WRITE_TABLE(manifest_resource);
    WRITE_TABLE(member_ref);
    WRITE_TABLE(method_def);
    WRITE_TABLE(method_impl);
    WRITE_TABLE(method_semantics);
    WRITE_TABLE(method_spec);
    WRITE_TABLE(module);
    WRITE_TABLE(module_ref);
    WRITE_TABLE(nested_class);
    WRITE_TABLE(param);
    WRITE_TABLE(property);
    WRITE_TABLE(property_map);
    WRITE_TABLE(standalone_sig);
    WRITE_TABLE(type_def);
    WRITE_TABLE(type_ref);
    WRITE_TABLE(type_spec);

    #undef WRITE_TABLE

    return 0;
}

// AMDG //
