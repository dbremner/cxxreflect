
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/neutral/precompiled_headers.hpp"
#include "tests/unit_tests/infrastructure/signature_builder.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
}

// Signature Builder Logic
//
// This library defines a set of types and functions for constructing metadata signatures.  The
// logic here is far from optimal, but it's "good enough" for test purposes.  We use this library
// for verifying the CxxReflect signature parser.
namespace sb
{
    using namespace cxxreflect::metadata::signature_builder;
}

namespace cxxreflect_test { namespace {

    auto verify_type_signature_kind(context const& c, cxr::type_signature const& s, bool (cxr::type_signature::*f)() const) -> void
    {
        c.verify_equals(s.is_primitive(),        f == &cxr::type_signature::is_primitive);
        c.verify_equals(s.is_general_array(),    f == &cxr::type_signature::is_general_array);
        c.verify_equals(s.is_simple_array(),     f == &cxr::type_signature::is_simple_array);
        c.verify_equals(s.is_class_type(),       f == &cxr::type_signature::is_class_type);
        c.verify_equals(s.is_value_type(),       f == &cxr::type_signature::is_value_type);
        c.verify_equals(s.is_function_pointer(), f == &cxr::type_signature::is_function_pointer);
        c.verify_equals(s.is_generic_instance(), f == &cxr::type_signature::is_generic_instance);
        c.verify_equals(s.is_pointer(),          f == &cxr::type_signature::is_pointer);
        c.verify_equals(s.is_class_variable(),   f == &cxr::type_signature::is_class_variable);
        c.verify_equals(s.is_method_variable(),  f == &cxr::type_signature::is_method_variable);
    }

} }

namespace cxxreflect_test {

    CXXREFLECTTEST_DEFINE_TEST(metadata_signatures_type_signature_kinds)
    {
        // This test verifies that each of the "is_{kind}()" functions return the correct result for
        // the most basic examples of each kind of type signatures.  (This test is likely to become
        // redundant--hopefully--but for the moment it allows us to verify that the signature build-
        // -ing logic works at least semi-correctly.)

        typedef sb::owned_signature<cxr::type_signature> signature_type;

        signature_type const s0(sb::unscoped(), sb::make_fundamental_type(cxr::element_type::i4));
        verify_type_signature_kind(c, s0.get(), &cxr::type_signature::is_primitive);

        signature_type const s1(sb::unscoped(), sb::make_general_array_type(
            sb::make_fundamental_type(cxr::element_type::i4),
            sb::make_array_shape(1)));
        verify_type_signature_kind(c, s1.get(), &cxr::type_signature::is_general_array);

        signature_type const s2(sb::unscoped(), sb::make_simple_array_type(
            sb::make_fundamental_type(cxr::element_type::i4)));
        verify_type_signature_kind(c, s2.get(), &cxr::type_signature::is_simple_array);

        signature_type const s3(sb::unscoped(), sb::make_class_type(
            cxr::type_def_token(sb::unscoped(), cxr::table_id::type_def, 0x01)));
        verify_type_signature_kind(c, s3.get(), &cxr::type_signature::is_class_type);

        signature_type const s4(sb::unscoped(), sb::make_value_type(
            cxr::type_def_token(sb::unscoped(), cxr::table_id::type_def, 0x01)));
        verify_type_signature_kind(c, s4.get(), &cxr::type_signature::is_value_type);

        signature_type const s5(sb::unscoped(), sb::make_fnptr_type(
            sb::make_method_def(
                cxr::signature_attribute::has_this | cxr::signature_attribute::calling_convention_default,
                sb::make_ret_type(cxr::element_type::void_type),
                std::vector<sb::param_node>())));
        verify_type_signature_kind(c, s5.get(), &cxr::type_signature::is_function_pointer);

        signature_type const s6(sb::unscoped(), sb::make_generic_inst_class_type(
            cxr::type_def_token(sb::unscoped(), cxr::table_id::type_def, 0x01),
            std::vector<sb::type_node>()));
        verify_type_signature_kind(c, s6.get(), &cxr::type_signature::is_generic_instance);

        signature_type const s7(sb::unscoped(), sb::make_void_pointer_type());
        verify_type_signature_kind(c, s7.get(), &cxr::type_signature::is_pointer);

        signature_type const s8(sb::unscoped(), sb::make_class_varibale(0));
        verify_type_signature_kind(c, s8.get(), &cxr::type_signature::is_class_variable);
        
        signature_type const s9(sb::unscoped(), sb::make_method_variable(0));
        verify_type_signature_kind(c, s9.get(), &cxr::type_signature::is_method_variable);
    }

}
