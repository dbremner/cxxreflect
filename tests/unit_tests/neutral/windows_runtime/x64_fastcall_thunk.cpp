
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// These tests do basic verification of the x64 assembly thunk that we use for dynamic invocation on
// x64 for fastcall functions (i.e., all functions, because fastcall is all there is).

#include "tests/unit_tests/neutral/precompiled_headers.hpp"

#if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64

namespace cxr {

    using namespace cxxreflect;
    using namespace cxxreflect::windows_runtime;
    using namespace cxxreflect::windows_runtime::detail;

    typedef std::int8_t   i1;
    typedef std::uint8_t  u1;
    typedef std::int16_t  i2;
    typedef std::uint16_t u2;
    typedef std::int32_t  i4;
    typedef std::uint32_t u4;
    typedef std::int64_t  i8;
    typedef std::uint64_t u8;

    typedef float         r4;
    typedef double        r8;
}

namespace cxxreflect_test { namespace {

    // Because we are testing our ability to call arbitrary functions, we cannot pass a pointer to
    // the current context into each function.  To work around this, we use a global context pointer
    // that gets set at the beginning of each test and unset at the end of the test.
    //
    // If we ever run the test suite in parallel, we'll need to synchronize access to the global
    // context or add some sort of tag that identifies tests as needing to be run in sequence.
    context const* global_context;

    class guarded_context_initializer
    {
    public:

        guarded_context_initializer(context const* const c)
        {
            global_context = c;
        }

        ~guarded_context_initializer()
        {
            global_context = nullptr;
        }
    };

} }

namespace cxxreflect_test {

    static auto f0() -> void
    {
    }

    CXXREFLECTTEST_DEFINE_TEST(windows_runtime_x64_fastcall_thunk_no_arguments)
    {
        guarded_context_initializer const context_guard(&c);

        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(f0, nullptr, nullptr, 0);
    }

}

namespace cxxreflect_test {

    static auto fi1(cxr::i4 a) -> void
    {
        global_context->verify_equals(a,  1);
    }

    static auto fi2(cxr::i8 a, cxr::i8 b) -> void
    {
        global_context->verify_equals(a,  1);
        global_context->verify_equals(b, -2);
    }

    static auto fi3(cxr::i4 a, cxr::i4 b, cxr::i4 c) -> void
    {
        global_context->verify_equals(a,  1);
        global_context->verify_equals(b, -2);
        global_context->verify_equals(c,  3);
    }

    static auto fi4(cxr::i8 a, cxr::i8 b, cxr::i8 c, cxr::i8 d) -> void
    {
        global_context->verify_equals(a,  1);
        global_context->verify_equals(b, -2);
        global_context->verify_equals(c,  3);
        global_context->verify_equals(d, -4);
    }

    static auto fi5(cxr::i4 a, cxr::i4 b, cxr::i4 c, cxr::i4 d, cxr::i4 e) -> void
    {
        global_context->verify_equals(a,  1);
        global_context->verify_equals(b, -2);
        global_context->verify_equals(c,  3);
        global_context->verify_equals(d, -4);
        global_context->verify_equals(e,  5);
    }

    static auto fi6(cxr::i8 a, cxr::i8 b, cxr::i8 c, cxr::i8 d, cxr::i8 e, cxr::i8 f) -> void
    {
        global_context->verify_equals(a,  1);
        global_context->verify_equals(b, -2);
        global_context->verify_equals(c,  3);
        global_context->verify_equals(d, -4);
        global_context->verify_equals(e,  5);
        global_context->verify_equals(f, -6);
    }

    static auto fi7(cxr::i4 a, cxr::i4 b, cxr::i4 c, cxr::i4 d, cxr::i4 e, cxr::i4 f, cxr::i4 g) -> void
    {
        global_context->verify_equals(a,  1);
        global_context->verify_equals(b, -2);
        global_context->verify_equals(c,  3);
        global_context->verify_equals(d, -4);
        global_context->verify_equals(e,  5);
        global_context->verify_equals(f, -6);
        global_context->verify_equals(g,  7);
    }

    static auto fi8(cxr::i8 a, cxr::i8 b, cxr::i8 c, cxr::i8 d, cxr::i8 e, cxr::i8 f, cxr::i8 g, cxr::i8 h) -> void
    {
        global_context->verify_equals(a,  1);
        global_context->verify_equals(b, -2);
        global_context->verify_equals(c,  3);
        global_context->verify_equals(d, -4);
        global_context->verify_equals(e,  5);
        global_context->verify_equals(f, -6);
        global_context->verify_equals(g,  7);
        global_context->verify_equals(h, -8);
    }

    CXXREFLECTTEST_DEFINE_TEST(windows_runtime_x64_fastcall_thunk_signed_integer_arguments)
    {
        guarded_context_initializer const context_guard(&c);

        cxr::x64_argument_frame frame;
        frame.push( 1LL);
        frame.push(-2LL);
        frame.push( 3LL);
        frame.push(-4LL);
        frame.push( 5LL);
        frame.push(-6LL);
        frame.push( 7LL);
        frame.push(-8LL);

        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fi1, frame.arguments(), frame.types(), 1);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fi2, frame.arguments(), frame.types(), 2);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fi3, frame.arguments(), frame.types(), 3);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fi4, frame.arguments(), frame.types(), 4);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fi5, frame.arguments(), frame.types(), 5);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fi6, frame.arguments(), frame.types(), 6);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fi7, frame.arguments(), frame.types(), 7);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fi8, frame.arguments(), frame.types(), 8);
    }

}

namespace cxxreflect_test {

    static auto fd1(cxr::r8 a) -> void
    {
        global_context->verify_equals(a,  1.0);
    }

    static auto fd2(cxr::r8 a, cxr::r8 b) -> void
    {
        global_context->verify_equals(a,  1.0);
        global_context->verify_equals(b, -2.0);
    }

    static auto fd3(cxr::r8 a, cxr::r8 b, cxr::r8 c) -> void
    {
        global_context->verify_equals(a,  1.0);
        global_context->verify_equals(b, -2.0);
        global_context->verify_equals(c,  3.0);
    }

    static auto fd4(cxr::r8 a, cxr::r8 b, cxr::r8 c, cxr::r8 d) -> void
    {
        global_context->verify_equals(a,  1.0);
        global_context->verify_equals(b, -2.0);
        global_context->verify_equals(c,  3.0);
        global_context->verify_equals(d, -4.0);
    }

    static auto fd5(cxr::r8 a, cxr::r8 b, cxr::r8 c, cxr::r8 d, cxr::r8 e) -> void
    {
        global_context->verify_equals(a,  1.0);
        global_context->verify_equals(b, -2.0);
        global_context->verify_equals(c,  3.0);
        global_context->verify_equals(d, -4.0);
        global_context->verify_equals(e,  5.0);
    }

    static auto fd6(cxr::r8 a, cxr::r8 b, cxr::r8 c, cxr::r8 d, cxr::r8 e, cxr::r8 f) -> void
    {
        global_context->verify_equals(a,  1.0);
        global_context->verify_equals(b, -2.0);
        global_context->verify_equals(c,  3.0);
        global_context->verify_equals(d, -4.0);
        global_context->verify_equals(e,  5.0);
        global_context->verify_equals(f, -6.0);
    }

    static auto fd7(cxr::r8 a, cxr::r8 b, cxr::r8 c, cxr::r8 d, cxr::r8 e, cxr::r8 f, cxr::r8 g) -> void
    {
        global_context->verify_equals(a,  1.0);
        global_context->verify_equals(b, -2.0);
        global_context->verify_equals(c,  3.0);
        global_context->verify_equals(d, -4.0);
        global_context->verify_equals(e,  5.0);
        global_context->verify_equals(f, -6.0);
        global_context->verify_equals(g,  7.0);
    }

    static auto fd8(cxr::r8 a, cxr::r8 b, cxr::r8 c, cxr::r8 d, cxr::r8 e, cxr::r8 f, cxr::r8 g, cxr::r8 h) -> void
    {
        global_context->verify_equals(a,  1.0);
        global_context->verify_equals(b, -2.0);
        global_context->verify_equals(c,  3.0);
        global_context->verify_equals(d, -4.0);
        global_context->verify_equals(e,  5.0);
        global_context->verify_equals(f, -6.0);
        global_context->verify_equals(g,  7.0);
        global_context->verify_equals(h, -8.0);
    }

    CXXREFLECTTEST_DEFINE_TEST(windows_runtime_x64_fastcall_thunk_double_precision_real_arguments)
    {
        guarded_context_initializer const context_guard(&c);

        cxr::x64_argument_frame frame;
        frame.push( 1.0);
        frame.push(-2.0);
        frame.push( 3.0);
        frame.push(-4.0);
        frame.push( 5.0);
        frame.push(-6.0);
        frame.push( 7.0);
        frame.push(-8.0);

        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fd1, frame.arguments(), frame.types(), 1);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fd2, frame.arguments(), frame.types(), 2);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fd3, frame.arguments(), frame.types(), 3);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fd4, frame.arguments(), frame.types(), 4);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fd5, frame.arguments(), frame.types(), 5);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fd6, frame.arguments(), frame.types(), 6);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fd7, frame.arguments(), frame.types(), 7);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fd8, frame.arguments(), frame.types(), 8);
    }

}

namespace cxxreflect_test {

    static auto fs1(cxr::r4 a) -> void
    {
        global_context->verify_equals(a,  1.0f);
    }

    static auto fs2(cxr::r4 a, cxr::r4 b) -> void
    {
        global_context->verify_equals(a,  1.0f);
        global_context->verify_equals(b, -2.0f);
    }

    static auto fs3(cxr::r4 a, cxr::r4 b, cxr::r4 c) -> void
    {
        global_context->verify_equals(a,  1.0f);
        global_context->verify_equals(b, -2.0f);
        global_context->verify_equals(c,  3.0f);
    }

    static auto fs4(cxr::r4 a, cxr::r4 b, cxr::r4 c, cxr::r4 d) -> void
    {
        global_context->verify_equals(a,  1.0f);
        global_context->verify_equals(b, -2.0f);
        global_context->verify_equals(c,  3.0f);
        global_context->verify_equals(d, -4.0f);
    }

    static auto fs5(cxr::r4 a, cxr::r4 b, cxr::r4 c, cxr::r4 d, cxr::r4 e) -> void
    {
        global_context->verify_equals(a,  1.0f);
        global_context->verify_equals(b, -2.0f);
        global_context->verify_equals(c,  3.0f);
        global_context->verify_equals(d, -4.0f);
        global_context->verify_equals(e,  5.0f);
    }

    static auto fs6(cxr::r4 a, cxr::r4 b, cxr::r4 c, cxr::r4 d, cxr::r4 e, cxr::r4 f) -> void
    {
        global_context->verify_equals(a,  1.0f);
        global_context->verify_equals(b, -2.0f);
        global_context->verify_equals(c,  3.0f);
        global_context->verify_equals(d, -4.0f);
        global_context->verify_equals(e,  5.0f);
        global_context->verify_equals(f, -6.0f);
    }

    static auto fs7(cxr::r4 a, cxr::r4 b, cxr::r4 c, cxr::r4 d, cxr::r4 e, cxr::r4 f, cxr::r4 g) -> void
    {
        global_context->verify_equals(a,  1.0f);
        global_context->verify_equals(b, -2.0f);
        global_context->verify_equals(c,  3.0f);
        global_context->verify_equals(d, -4.0f);
        global_context->verify_equals(e,  5.0f);
        global_context->verify_equals(f, -6.0f);
        global_context->verify_equals(g,  7.0f);
    }

    static auto fs8(cxr::r4 a, cxr::r4 b, cxr::r4 c, cxr::r4 d, cxr::r4 e, cxr::r4 f, cxr::r4 g, cxr::r4 h) -> void
    {
        global_context->verify_equals(a,  1.0f);
        global_context->verify_equals(b, -2.0f);
        global_context->verify_equals(c,  3.0f);
        global_context->verify_equals(d, -4.0f);
        global_context->verify_equals(e,  5.0f);
        global_context->verify_equals(f, -6.0f);
        global_context->verify_equals(g,  7.0f);
        global_context->verify_equals(h, -8.0f);
    }

    CXXREFLECTTEST_DEFINE_TEST(windows_runtime_x64_fastcall_thunk_single_precision_real_arguments)
    {
        guarded_context_initializer const context_guard(&c);

        cxr::x64_argument_frame frame;
        frame.push( 1.0f);
        frame.push(-2.0f);
        frame.push( 3.0f);
        frame.push(-4.0f);
        frame.push( 5.0f);
        frame.push(-6.0f);
        frame.push( 7.0f);
        frame.push(-8.0f);

        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fs1, frame.arguments(), frame.types(), 1);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fs2, frame.arguments(), frame.types(), 2);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fs3, frame.arguments(), frame.types(), 3);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fs4, frame.arguments(), frame.types(), 4);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fs5, frame.arguments(), frame.types(), 5);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fs6, frame.arguments(), frame.types(), 6);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fs7, frame.arguments(), frame.types(), 7);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fs8, frame.arguments(), frame.types(), 8);
    }

}

namespace cxxreflect_test {

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    static auto fm_verify_123456(A a, B b, C c, D d, E e, F f) -> void
    {
        global_context->verify_equals(a, 1);
        global_context->verify_equals(b, 2);
        global_context->verify_equals(c, 3);
        global_context->verify_equals(d, 4);
        global_context->verify_equals(e, 5);
        global_context->verify_equals(f, 6);
    }

    static auto fma(cxr::i1 a, cxr::i2 b, cxr::i1 c, cxr::i2 d, cxr::i1 e, cxr::i2 f) -> void
    {
        fm_verify_123456(a, b, c, d, e, f);
    }

    static auto fmb(cxr::i2 a, cxr::i4 b, cxr::i2 c, cxr::i4 d, cxr::i2 e, cxr::i4 f) -> void
    {
        fm_verify_123456(a, b, c, d, e, f);
    }

    static auto fmc(cxr::i4 a, cxr::i8 b, cxr::i4 c, cxr::i8 d, cxr::i4 e, cxr::i8 f) -> void
    {
        fm_verify_123456(a, b, c, d, e, f);
    }

    static auto fmd(cxr::i1 a, cxr::i2 b, cxr::i4 c, cxr::i8 d, cxr::i1 e, cxr::i2 f) -> void
    {
        fm_verify_123456(a, b, c, d, e, f);
    }

    static auto fme(cxr::i1 a, cxr::i8 b, cxr::i1 c, cxr::i8 d, cxr::i1 e, cxr::i8 f) -> void
    {
        fm_verify_123456(a, b, c, d, e, f);
    }

    static auto fmf(cxr::i8 a, cxr::i4 b, cxr::i2 c, cxr::i2 d, cxr::i4 e, cxr::i8 f) -> void
    {
        fm_verify_123456(a, b, c, d, e, f);
    }

    CXXREFLECTTEST_DEFINE_TEST(windows_runtime_x64_fastcall_thunk_mixed_integer_arguments)
    {
        guarded_context_initializer const context_guard(&c);

        cxr::x64_argument_frame frame;
        frame.push(1LL);
        frame.push(2LL);
        frame.push(3LL);
        frame.push(4LL);
        frame.push(5LL);
        frame.push(6LL);

        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fma, frame.arguments(), frame.types(), 6);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fmb, frame.arguments(), frame.types(), 6);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fmc, frame.arguments(), frame.types(), 6);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fmd, frame.arguments(), frame.types(), 6);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fme, frame.arguments(), frame.types(), 6);
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fmf, frame.arguments(), frame.types(), 6);
    }

}

namespace cxxreflect_test {

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    static auto fn_init_frame(A a, B b, C c, D d, E e, F f) -> cxr::x64_argument_frame
    {
        cxr::x64_argument_frame frame;
        frame.push(a);
        frame.push(b);
        frame.push(c);
        frame.push(d);
        frame.push(e);
        frame.push(f);
        return frame;
    }

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    static auto fn_verify_123456(A a, B b, C c, D d, E e, F f) -> void
    {
        global_context->verify_equals(a, 1);
        global_context->verify_equals(b, 2);
        global_context->verify_equals(c, 3);
        global_context->verify_equals(d, 4);
        global_context->verify_equals(e, 5);
        global_context->verify_equals(f, 6);
    }

    static auto fna(cxr::r8 a, cxr::i8 b, cxr::r8 c, cxr::i8 d, cxr::r8 e, cxr::i8 f) -> void
    {
        fn_verify_123456(a, b, c, d, e, f);
    }

    static auto fnb(cxr::i8 a, cxr::r8 b, cxr::r8 c, cxr::i8 d, cxr::i8 e, cxr::r8 f) -> void
    {
        fn_verify_123456(a, b, c, d, e, f);
    }

    static auto fnc(cxr::i8 a, cxr::r4 b, cxr::r4 c, cxr::i8 d, cxr::i8 e, cxr::r4 f) -> void
    {
        fn_verify_123456(a, b, c, d, e, f);
    }

    static auto fnd(cxr::i4 a, cxr::r4 b, cxr::r8 c, cxr::i8 d, cxr::r4 e, cxr::r8 f) -> void
    {
        fn_verify_123456(a, b, c, d, e, f);
    }

    CXXREFLECTTEST_DEFINE_TEST(windows_runtime_x64_fastcall_thunk_mixed_integer_and_real_arguments)
    {
        guarded_context_initializer const context_guard(&c);

        cxr::x64_argument_frame const frame_a(fn_init_frame(1.0, 2LL, 3.0, 4LL, 5.0, 6LL));
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fna, frame_a.arguments(), frame_a.types(), 6);

        cxr::x64_argument_frame const frame_b(fn_init_frame(1LL, 2.0, 3.0, 4LL, 5LL, 6.0));
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fnb, frame_b.arguments(), frame_b.types(), 6);

        cxr::x64_argument_frame const frame_c(fn_init_frame(1LL, 2.0f, 3.0f, 4LL, 5LL, 6.0f));
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fnc, frame_c.arguments(), frame_c.types(), 6);

        cxr::x64_argument_frame const frame_d(fn_init_frame(1LL, 2.0f, 3.0, 4LL, 5.0f, 6.0));
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(fnd, frame_d.arguments(), frame_d.types(), 6);
    }

}

namespace cxxreflect_test {

    struct windows_runtime_x64_fastcall_thunk_basic_struct
    {
        cxr::u8 x;
        cxr::u8 y;
        cxr::u8 z;
    };

    static auto f_basic_struct(windows_runtime_x64_fastcall_thunk_basic_struct s) -> void
    {
        global_context->verify_equals(s.x, 1);
        global_context->verify_equals(s.y, 2);
        global_context->verify_equals(s.z, 3);
    }

    CXXREFLECTTEST_DEFINE_TEST(windows_runtime_x64_fastcall_thunk_struct_arguments)
    {
        guarded_context_initializer const context_guard(&c);

        windows_runtime_x64_fastcall_thunk_basic_struct x = { 1, 2, 3 };

        cxr::x64_argument_frame frame;
        frame.push(&x);
        
        cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(f_basic_struct, frame.arguments(), frame.types(), 1);
    }

}

namespace cxxreflect_test {

    class windows_runtime_x64_fastcall_thunk_f_exception { };

    static auto f_throws(int, int, int, int, int, int) -> void
    {
        throw windows_runtime_x64_fastcall_thunk_f_exception();
    }

    CXXREFLECTTEST_DEFINE_TEST(windows_runtime_x64_fastcall_thunk_exceptional_return)
    {
        guarded_context_initializer const context_guard(&c);

        cxr::x64_argument_frame frame;
        frame.push(1LL);
        frame.push(2LL);
        frame.push(3LL);
        frame.push(4LL);
        frame.push(5LL);
        frame.push(6LL);

        try
        {
            cxr::cxxreflect_windows_runtime_x64_fastcall_thunk(f_throws, frame.arguments(), frame.types(), 6);
            c.fail();
        }
        catch (windows_runtime_x64_fastcall_thunk_f_exception const&)
        {
        }
    }

}

#endif // CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64
