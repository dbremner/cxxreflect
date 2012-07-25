
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "test/unit_tests/test_driver.hpp"
#include "cxxreflect/metadata/metadata.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
}

namespace cxxreflect_test { namespace {

    auto verify_token_uninitialized_state(context const& c) -> void
    {
        // Using an uninitialized token should fail on pretty much every operation:
        cxr::unrestricted_token const t;
        c.verify(!t.is_initialized());

        c.verify_exception<cxr::assertion_error>([&]{ t.scope(); });
        c.verify_exception<cxr::assertion_error>([&]{ t.table(); });
        c.verify_exception<cxr::assertion_error>([&]{ t.index(); });
        c.verify_exception<cxr::assertion_error>([&]{ t.value(); });

        // Comparisons between two uninitialized tokens are allowed and two uninitialized tokens
        // should always compare equal:
        c.verify( (t == t));
        c.verify(!(t != t));
        c.verify(!(t <  t));
        c.verify(!(t >  t));
        c.verify( (t <= t));
        c.verify( (t >= t));

        // However, comparisons between initialized and uninitialized tokens are not allowed:
        cxr::database const* faux_scope(reinterpret_cast<cxr::database const*>(-1));
        cxr::unrestricted_token const u(faux_scope, cxr::table_id::type_def, 0);

        c.verify_exception<cxr::assertion_error>([&]{ t == u; });
        c.verify_exception<cxr::assertion_error>([&]{ t != u; });
        c.verify_exception<cxr::assertion_error>([&]{ t <  u; });
        c.verify_exception<cxr::assertion_error>([&]{ t <= u; });
        c.verify_exception<cxr::assertion_error>([&]{ t >  u; });
        c.verify_exception<cxr::assertion_error>([&]{ t >= u; });
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_token_uninitialized_state, verify_token_uninitialized_state);





    auto verify_token_construction(context const& c) -> void
    {
        cxr::database const* scope(reinterpret_cast<cxr::database const*>(-1));

        // Verify construction from table and index:
        cxr::unrestricted_token t(scope, cxr::table_id::type_def, 0);
        c.verify(t.is_initialized());
        c.verify_equals(t.table(), cxr::table_id::type_def);
        c.verify_equals(t.index(), 0u);
        c.verify_equals(t.value(), 0x02000001u);
        c.verify(t.is<cxr::type_def_token>());
        c.verify(t == t);
        c.verify(!(t < t));

        // Verify construction from token value:
        cxr::unrestricted_token u(scope, 0x02000002u);
        c.verify(u.is_initialized());
        c.verify_equals(u.table(), cxr::table_id::type_def);
        c.verify_equals(u.index(), 1u);
        c.verify_equals(u.value(), 0x02000002u);
        c.verify(u.is<cxr::type_def_token>());
        c.verify(u == u);
        c.verify(!(u < u));

        c.verify(t != u);
        c.verify(t <  u);
        c.verify(u >  t);
        c.verify(t <= u);
        c.verify(u >= t);

        // Verify construction with nullptr scope throws: 
        c.verify_exception<cxr::assertion_error>([&]{ cxr::type_def_token(nullptr, cxr::table_id::type_def, 0); });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::type_def_token(nullptr, 0x02000001);                 });

        // Verify construction with disallowed table throws:
        c.verify_exception<cxr::assertion_error>([&]{ cxr::type_def_token(scope, cxr::table_id::assembly, 0); });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::type_def_token(scope, 0x20000000);                 });

        // Verify construction with out-of-range value throws:
        c.verify_exception<cxr::assertion_error>([&]{ cxr::type_def_token(scope, (cxr::table_id)0xff,     0);          });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::type_def_token(scope, cxr::table_id::type_def, 0x10000000); });
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_token_construction, verify_token_construction);





    auto verify_token_static_conversions(context const& c) -> void
    {
        // Verify that a unique token type is convertible to itself:
        c.verify(std::is_convertible<cxr::assembly_token,   cxr::assembly_token  >::value);
        c.verify(std::is_convertible<cxr::method_def_token, cxr::method_def_token>::value);
        c.verify(std::is_convertible<cxr::type_def_token,   cxr::type_def_token  >::value);

        // Verify that a unique token type is not convertible to another unique token type:
        c.verify(!std::is_convertible<cxr::assembly_token, cxr::type_def_token>::value);
        c.verify(!std::is_convertible<cxr::type_def_token, cxr::assembly_token>::value);

        // Verify that a non-unique token type is convertible to itself:
        c.verify(std::is_convertible<cxr::type_def_ref_spec_token, cxr::type_def_ref_spec_token>::value);
        c.verify(std::is_convertible<cxr::has_constant_token,      cxr::has_constant_token     >::value);

        // Verify that a non-unique token type is not convertible to an incompatible token type:
        c.verify(!std::is_convertible<cxr::type_def_ref_spec_token, cxr::has_constant_token     >::value);
        c.verify(!std::is_convertible<cxr::has_constant_token,      cxr::type_def_ref_spec_token>::value);

        // Verify that valid "widening" conversions are allowed:
        c.verify(std::is_convertible<cxr::type_def_token,  cxr::type_def_ref_spec_token>::value);
        c.verify(std::is_convertible<cxr::type_ref_token,  cxr::type_def_ref_spec_token>::value);
        c.verify(std::is_convertible<cxr::type_spec_token, cxr::type_def_ref_spec_token>::value);

        c.verify(std::is_convertible<cxr::type_def_spec_token, cxr::type_def_ref_spec_token>::value);

        // Verify that invalid "widening" conversions are disallowed:
        c.verify(!std::is_convertible<cxr::assembly_token, cxr::type_def_ref_spec_token>::value);

        // Verify that narrowing conversions are disallowed:
        c.verify(!std::is_convertible<cxr::type_def_ref_spec_token, cxr::type_def_token>::value);
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_token_static_conversions, verify_token_static_conversions);
    




    auto verify_token_dynamic_conversions(context const& c) -> void
    {
        // We need a non-null scope pointer to avoid tripping the null checks; we'll never perform
        // indirection through this pointer, so we pick the value -1 arbitrarily.
        cxr::database const* const faux_scope(reinterpret_cast<cxr::database const*>(-1));

        c.verify_exception<cxr::assertion_error>([&]
        {
            cxr::type_def_ref_spec_token(faux_scope, cxr::table_id::type_spec, 1).as<cxr::type_def_token>();
        });
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_token_dynamic_conversions, verify_token_dynamic_conversions);





    auto verify_token_arithmetic(context const& c) -> void
    {
        // We need a non-null scope pointer to avoid tripping the null checks; we'll never perform
        // indirection through this pointer, so we pick the value -1 arbitrarily.
        cxr::database const* const faux_scope(reinterpret_cast<cxr::database const*>(-1));

        cxr::type_def_token const original_token(faux_scope, cxr::table_id::type_def, 0);

        typedef cxr::token_with_arithmetic<cxr::type_def_token>::type type_def_ops_token;

        type_def_ops_token ops_token(original_token);
        c.verify(ops_token == original_token);
        c.verify(ops_token.index() == 0);

        c.verify((++ops_token).index() == 1);
        c.verify(ops_token.index() == 1);

        c.verify((--ops_token).index() == 0);
        c.verify(ops_token.index() == 0);

        c.verify((ops_token++).index() == 0);
        c.verify(ops_token.index() == 1);

        c.verify((ops_token--).index() == 1);
        c.verify(ops_token.index() == 0);

        c.verify((ops_token += 4).index() == 4);
        c.verify(ops_token.index() == 4);

        c.verify((ops_token -= 4).index() == 0);
        c.verify(ops_token.index() == 0);

        c.verify((ops_token + 4).index() == 4);
        c.verify(ops_token.index() == 0);

        ops_token += 4;
        c.verify((ops_token - 4).index() == 0);
        c.verify(ops_token.index() == 4);
        ops_token -= 4;

        c.verify((ops_token + 42) - ops_token == 42);

        cxr::type_def_token const final_token(ops_token);
        c.verify(final_token == original_token);
        c.verify(final_token == ops_token);
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_token_arithmetic, verify_token_arithmetic);





    auto verify_token_comparability(context const& c) -> void
    {
        // We need non-null scope pointers to avoid tripping the null checks; we'll never perform
        // indirection through this pointer, so we pick the values -1 and -2 arbitrarily.
        cxr::database const* const faux_scope_a(reinterpret_cast<cxr::database const*>(-1));
        cxr::database const* const faux_scope_b(reinterpret_cast<cxr::database const*>(-2));

        // Equal tokens compare equal:
        {
            cxr::unrestricted_token const a(faux_scope_a, cxr::table_id::type_def, 0);
            cxr::unrestricted_token const b(faux_scope_a, cxr::table_id::type_def, 0);

            c.verify( (a == b));
            c.verify(!(a != b));
            c.verify(!(a <  b));
            c.verify(!(a >  b));
            c.verify( (a <= b));
            c.verify( (a >= b));
        }

        // Tokens with different indices should compare not equal:
        {
            cxr::unrestricted_token const a(faux_scope_a, cxr::table_id::type_def, 0);
            cxr::unrestricted_token const b(faux_scope_a, cxr::table_id::type_def, 1);

            c.verify(!(a == b));
            c.verify( (a != b));
            c.verify( (a <  b));
            c.verify(!(a >  b));
            c.verify( (a <= b));
            c.verify(!(a >= b));
        }

        // Tokens with different table identifiers should compare not equal:
        {
            cxr::unrestricted_token const a(faux_scope_a, cxr::table_id::type_def,   0);
            cxr::unrestricted_token const b(faux_scope_a, cxr::table_id::method_def, 0);

            c.verify(!(a == b));
            c.verify( (a != b));
            c.verify( (a <  b));
            c.verify(!(a >  b));
            c.verify( (a <= b));
            c.verify(!(a >= b));
        }

        // Tokens with different table identifiers and indices should compare not equal:
        {
            // Note that the table identifier should have higher precedence than the index
            cxr::unrestricted_token const a(faux_scope_a, cxr::table_id::type_def,   1);
            cxr::unrestricted_token const b(faux_scope_a, cxr::table_id::method_def, 0);

            c.verify(!(a == b));
            c.verify( (a != b));
            c.verify( (a <  b));
            c.verify(!(a >  b));
            c.verify( (a <= b));
            c.verify(!(a >= b));
        }

        // Tokens with different scopes should compare not equal:
        {
            cxr::unrestricted_token const a(faux_scope_a, cxr::table_id::type_def, 0);
            cxr::unrestricted_token const b(faux_scope_b, cxr::table_id::type_def, 0);

            c.verify(!(a == b));
            c.verify( (a != b));
            c.verify(!(a <  b));
            c.verify( (a >  b));
            c.verify(!(a <= b));
            c.verify( (a >= b));
        }
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_token_comparability, verify_token_comparability);





    auto verify_blob_uninitialized_state(context const& c) -> void
    {
        // Using an uninitialized blob should fail on pretty much every operation:
        cxr::blob const t;
        c.verify(!t.is_initialized());

        c.verify_exception<cxr::assertion_error>([&]{ t.scope(); });
        c.verify_exception<cxr::assertion_error>([&]{ t.begin(); });
        c.verify_exception<cxr::assertion_error>([&]{ t.end();   });

        // Comparisons between two uninitialized tokens are allowed and two uninitialized tokens
        // should always compare equal:
        c.verify( (t == t));
        c.verify(!(t != t));
        c.verify(!(t <  t));
        c.verify(!(t >  t));
        c.verify( (t <= t));
        c.verify( (t >= t));

        // However, comparisons between initialized and uninitialized tokens are not allowed:
        cxr::database const*     const faux_scope(reinterpret_cast<cxr::database const*>(-1));
        cxr::const_byte_iterator const faux_begin(reinterpret_cast<cxr::const_byte_iterator>(1));
        cxr::const_byte_iterator const faux_end  (reinterpret_cast<cxr::const_byte_iterator>(2));

        cxr::blob const u(faux_scope, faux_begin, faux_end);

        c.verify_exception<cxr::assertion_error>([&]{ t == u; });
        c.verify_exception<cxr::assertion_error>([&]{ t != u; });
        c.verify_exception<cxr::assertion_error>([&]{ t <  u; });
        c.verify_exception<cxr::assertion_error>([&]{ t <= u; });
        c.verify_exception<cxr::assertion_error>([&]{ t >  u; });
        c.verify_exception<cxr::assertion_error>([&]{ t >= u; });
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_blob_uninitialized_state, verify_blob_uninitialized_state);




    auto verify_blob_construction(context const& c) -> void
    {
        cxr::database const*     const faux_scope(reinterpret_cast<cxr::database const*>(-1));
        cxr::const_byte_iterator const faux_begin(reinterpret_cast<cxr::const_byte_iterator>(1));
        cxr::const_byte_iterator const faux_end  (reinterpret_cast<cxr::const_byte_iterator>(2));

        // Verify nominal from table and index:
        cxr::blob const t(faux_scope, faux_begin, faux_end);
        c.verify(t.is_initialized());
        c.verify_equals(&t.scope(), faux_scope);
        c.verify_equals(t.begin(),  faux_begin);
        c.verify_equals(t.end(),    faux_end);
        c.verify( (t == t));
        c.verify(!(t <  t));

        // Verify signature round-tripping:
        cxr::type_signature const ts(t.as<cxr::type_signature>());
        c.verify(ts.is_initialized());
        cxr::blob const u(ts);
        c.verify( (t == u));
        c.verify(!(t <  u));

        // Verify uniqueness:
        cxr::blob const v(faux_scope, faux_end, faux_begin);
        c.verify(!(t == v));
        c.verify( (t <  v));

        // Verify construction with nullptr argument throws:
        c.verify_exception<cxr::assertion_error>([&]{ cxr::blob(nullptr,    faux_begin, faux_end); });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::blob(faux_scope, nullptr,    faux_end); });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::blob(faux_scope, faux_begin, nullptr ); });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::blob(nullptr,    nullptr,    faux_end); });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::blob(nullptr,    faux_begin, nullptr ); });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::blob(faux_scope, nullptr,    nullptr ); });
        c.verify_exception<cxr::assertion_error>([&]{ cxr::blob(nullptr,    nullptr,    nullptr ); });
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_blob_construction, verify_blob_construction);





    auto verify_hybrid_uninitialized_state(context const& c) -> void
    {
        // Using an uninitialized blob should fail on pretty much every operation:
        auto const verify_uninitialized([&](cxr::type_def_ref_spec_or_signature const& t)
        {
            c.verify(!t.is_initialized());
            c.verify(!t.is_blob());
            c.verify(!t.is_token());

            c.verify_exception<cxr::assertion_error>([&]{ t.scope();    });
            c.verify_exception<cxr::assertion_error>([&]{ t.as_token(); });
            c.verify_exception<cxr::assertion_error>([&]{ t.as_blob();  });
        });

        // A default-constructed hybrid should be uninitialized:
        {
            cxr::type_def_ref_spec_or_signature const t;
            verify_uninitialized(t);
        }

        // A hybrid constructed from an uninitialized token or blob should be uninitialized:
        {
            cxr::type_def_ref_spec_or_signature const t((cxr::type_def_ref_spec_token()));
            verify_uninitialized(t);
        }

        {
            cxr::type_def_ref_spec_or_signature const t((cxr::blob()));
            verify_uninitialized(t);
        }

        {
            cxr::type_def_ref_spec_or_signature const t((cxr::type_def_ref_spec_or_signature()));
            verify_uninitialized(t);
        }

        {
            cxr::type_def_ref_spec_or_signature const t((cxr::type_def_token()));
            verify_uninitialized(t);
        }

        // Comparisons between two uninitialized tokens are allowed and two uninitialized tokens
        // should always compare equal:
        {
            cxr::type_def_ref_spec_or_signature const t;
            c.verify( (t == t));
            c.verify(!(t != t));
            c.verify(!(t <  t));
            c.verify(!(t >  t));
            c.verify( (t <= t));
            c.verify( (t >= t));
        }

        // However, comparisons between initialized and uninitialized tokens are not allowed:
        {
            cxr::database const*     const faux_scope(reinterpret_cast<cxr::database const*>(-1));
            cxr::const_byte_iterator const faux_begin(reinterpret_cast<cxr::const_byte_iterator>(1));
            cxr::const_byte_iterator const faux_end  (reinterpret_cast<cxr::const_byte_iterator>(2));

            cxr::type_def_ref_spec_or_signature const t;
            cxr::type_def_ref_spec_or_signature const u(cxr::blob(faux_scope, faux_begin, faux_end));

            c.verify_exception<cxr::assertion_error>([&]{ t == u; });
            c.verify_exception<cxr::assertion_error>([&]{ t != u; });
            c.verify_exception<cxr::assertion_error>([&]{ t <  u; });
            c.verify_exception<cxr::assertion_error>([&]{ t <= u; });
            c.verify_exception<cxr::assertion_error>([&]{ t >  u; });
            c.verify_exception<cxr::assertion_error>([&]{ t >= u; });
        }
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_hybrid_uninitialized_state, verify_hybrid_uninitialized_state);





    auto verify_hybrid_construction(context const& c) -> void
    {
        cxr::database const*     const faux_scope(reinterpret_cast<cxr::database const*>(-1));
        cxr::const_byte_iterator const faux_begin(reinterpret_cast<cxr::const_byte_iterator>(1));
        cxr::const_byte_iterator const faux_end  (reinterpret_cast<cxr::const_byte_iterator>(2));

        // Verify construction from token:
        {
            cxr::type_def_token        const t(faux_scope, cxr::table_id::type_def, 0);
            cxr::type_def_or_signature const u(t);
            c.verify(u.is_initialized());
            c.verify(u.is_token());
            c.verify(!u.is_blob());
            c.verify_equals(u.as_token(), t);
            c.verify_exception<cxr::assertion_error>([&]{ u.as_blob(); });
        }

        // Verify construction from blob:
        {
            cxr::blob                  const t(faux_scope, faux_begin, faux_end);
            cxr::type_def_or_signature const u(t);
            c.verify(u.is_initialized());
            c.verify(!u.is_token());
            c.verify(u.is_blob());
            c.verify_equals(u.as_blob(), t);
            c.verify_exception<cxr::assertion_error>([&]{ u.as_token(); });
        }

        // Verify widening construction:
        {
            cxr::blob                           const t(faux_scope, faux_begin, faux_end);
            cxr::type_def_or_signature          const u(t);
            cxr::type_def_ref_spec_or_signature const v(u);
            c.verify(v.is_initialized());
            c.verify(!v.is_token());
            c.verify(v.is_blob());
            c.verify_equals(v.as_blob(), t);
            c.verify_exception<cxr::assertion_error>([&]{ v.as_token(); });
        }
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_hybrid_construction, verify_hybrid_construction);





    auto verify_hybrid_comparability(context const& c) -> void
    {
        // We need non-null scope pointers to avoid tripping the null checks; we'll never perform
        // indirection through this pointer, so we pick the values -1 and -2 arbitrarily.
        cxr::database const* const faux_scope_a(reinterpret_cast<cxr::database const*>(-1));
        cxr::database const* const faux_scope_b(reinterpret_cast<cxr::database const*>(-2));

        // Equal hybrids compare equal:
        {
            cxr::type_def_or_signature const a(cxr::type_def_token(faux_scope_a, cxr::table_id::type_def, 0));
            cxr::type_def_or_signature const b(cxr::type_def_token(faux_scope_a, cxr::table_id::type_def, 0));

            c.verify( (a == b));
            c.verify(!(a != b));
            c.verify(!(a <  b));
            c.verify(!(a >  b));
            c.verify( (a <= b));
            c.verify( (a >= b));
        }

        // Tokens with different indices should compare not equal:
        {
            cxr::type_def_or_signature const a(cxr::type_def_token(faux_scope_a, cxr::table_id::type_def, 0));
            cxr::type_def_or_signature const b(cxr::type_def_token(faux_scope_a, cxr::table_id::type_def, 1));

            c.verify(!(a == b));
            c.verify( (a != b));
            c.verify( (a <  b));
            c.verify(!(a >  b));
            c.verify( (a <= b));
            c.verify(!(a >= b));
        }

        // Tokens with different scopes should compare not equal:
        {
            cxr::type_def_or_signature const a(cxr::type_def_token(faux_scope_a, cxr::table_id::type_def, 0));
            cxr::type_def_or_signature const b(cxr::type_def_token(faux_scope_b, cxr::table_id::type_def, 0));

            c.verify(!(a == b));
            c.verify( (a != b));
            c.verify(!(a <  b));
            c.verify( (a >  b));
            c.verify(!(a <= b));
            c.verify( (a >= b));
        }
    }

    CXXREFLECTTEST_REGISTER(metadata_tokens_hybrid_comparability, verify_hybrid_comparability);

} }
