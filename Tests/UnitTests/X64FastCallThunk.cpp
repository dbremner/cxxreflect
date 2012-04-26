
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Basic functionality tests for the Metadata::Database and related classes.

#include "Context.hpp"
#include "CxxReflect/CxxReflect.hpp"

#if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64

namespace cxr {

    using namespace CxxReflect;
    using namespace CxxReflect::WindowsRuntime;
    using namespace CxxReflect::WindowsRuntime::Internal;
}

namespace CxxReflectTest { namespace {

    Context* context;

    class GuardedContextInitializer
    {
    public:

        GuardedContextInitializer(Context* const c)
        {
            context = c;
        }

        ~GuardedContextInitializer()
        {
            context = nullptr;
        }
    };

} }

namespace CxxReflectTest { namespace {

    void F0()
    {
    }

    CXXREFLECTTEST_REGISTER(X64FastCallThunk_NoArguments, [](Context& c)
    {
        GuardedContextInitializer const contextGuard(&c);

        cxr::CxxReflectX64FastCallThunk(F0, nullptr, nullptr, 0);
    });

} }

namespace CxxReflectTest { namespace {

    void FI1(cxr::I4 a)
    {
        context->VerifyEquals(a,  1);
    }

    void FI2(cxr::I8 a, cxr::I8 b)
    {
        context->VerifyEquals(a,  1);
        context->VerifyEquals(b, -2);
    }

    void FI3(cxr::I4 a, cxr::I4 b, cxr::I4 c)
    {
        context->VerifyEquals(a,  1);
        context->VerifyEquals(b, -2);
        context->VerifyEquals(c,  3);
    }

    void FI4(cxr::I8 a, cxr::I8 b, cxr::I8 c, cxr::I8 d)
    {
        context->VerifyEquals(a,  1);
        context->VerifyEquals(b, -2);
        context->VerifyEquals(c,  3);
        context->VerifyEquals(d, -4);
    }

    void FI5(cxr::I4 a, cxr::I4 b, cxr::I4 c, cxr::I4 d, cxr::I4 e)
    {
        context->VerifyEquals(a,  1);
        context->VerifyEquals(b, -2);
        context->VerifyEquals(c,  3);
        context->VerifyEquals(d, -4);
        context->VerifyEquals(e,  5);
    }

    void FI6(cxr::I8 a, cxr::I8 b, cxr::I8 c, cxr::I8 d, cxr::I8 e, cxr::I8 f)
    {
        context->VerifyEquals(a,  1);
        context->VerifyEquals(b, -2);
        context->VerifyEquals(c,  3);
        context->VerifyEquals(d, -4);
        context->VerifyEquals(e,  5);
        context->VerifyEquals(f, -6);
    }

    void FI7(cxr::I4 a, cxr::I4 b, cxr::I4 c, cxr::I4 d, cxr::I4 e, cxr::I4 f, cxr::I4 g)
    {
        context->VerifyEquals(a,  1);
        context->VerifyEquals(b, -2);
        context->VerifyEquals(c,  3);
        context->VerifyEquals(d, -4);
        context->VerifyEquals(e,  5);
        context->VerifyEquals(f, -6);
        context->VerifyEquals(g,  7);
    }

    void FI8(cxr::I8 a, cxr::I8 b, cxr::I8 c, cxr::I8 d, cxr::I8 e, cxr::I8 f, cxr::I8 g, cxr::I8 h)
    {
        context->VerifyEquals(a,  1);
        context->VerifyEquals(b, -2);
        context->VerifyEquals(c,  3);
        context->VerifyEquals(d, -4);
        context->VerifyEquals(e,  5);
        context->VerifyEquals(f, -6);
        context->VerifyEquals(g,  7);
        context->VerifyEquals(h, -8);
    }

    CXXREFLECTTEST_REGISTER(X64FastCallThunk_SignedIntegerArguments, [](Context& c)
    {
        GuardedContextInitializer const contextGuard(&c);

        cxr::X64ArgumentFrame frame;
        frame.Push( 1LL);
        frame.Push(-2LL);
        frame.Push( 3LL);
        frame.Push(-4LL);
        frame.Push( 5LL);
        frame.Push(-6LL);
        frame.Push( 7LL);
        frame.Push(-8LL);

        cxr::CxxReflectX64FastCallThunk(FI1, frame.GetArguments(), frame.GetTypes(), 1);
        cxr::CxxReflectX64FastCallThunk(FI2, frame.GetArguments(), frame.GetTypes(), 2);
        cxr::CxxReflectX64FastCallThunk(FI3, frame.GetArguments(), frame.GetTypes(), 3);
        cxr::CxxReflectX64FastCallThunk(FI4, frame.GetArguments(), frame.GetTypes(), 4);
        cxr::CxxReflectX64FastCallThunk(FI5, frame.GetArguments(), frame.GetTypes(), 5);
        cxr::CxxReflectX64FastCallThunk(FI6, frame.GetArguments(), frame.GetTypes(), 6);
        cxr::CxxReflectX64FastCallThunk(FI7, frame.GetArguments(), frame.GetTypes(), 7);
        cxr::CxxReflectX64FastCallThunk(FI8, frame.GetArguments(), frame.GetTypes(), 8);
    });

} }

namespace CxxReflectTest { namespace {

    void FD1(cxr::R8 a)
    {
        context->VerifyEquals(a,  1.0);
    }

    void FD2(cxr::R8 a, cxr::R8 b)
    {
        context->VerifyEquals(a,  1.0);
        context->VerifyEquals(b, -2.0);
    }

    void FD3(cxr::R8 a, cxr::R8 b, cxr::R8 c)
    {
        context->VerifyEquals(a,  1.0);
        context->VerifyEquals(b, -2.0);
        context->VerifyEquals(c,  3.0);
    }

    void FD4(cxr::R8 a, cxr::R8 b, cxr::R8 c, cxr::R8 d)
    {
        context->VerifyEquals(a,  1.0);
        context->VerifyEquals(b, -2.0);
        context->VerifyEquals(c,  3.0);
        context->VerifyEquals(d, -4.0);
    }

    void FD5(cxr::R8 a, cxr::R8 b, cxr::R8 c, cxr::R8 d, cxr::R8 e)
    {
        context->VerifyEquals(a,  1.0);
        context->VerifyEquals(b, -2.0);
        context->VerifyEquals(c,  3.0);
        context->VerifyEquals(d, -4.0);
        context->VerifyEquals(e,  5.0);
    }

    void FD6(cxr::R8 a, cxr::R8 b, cxr::R8 c, cxr::R8 d, cxr::R8 e, cxr::R8 f)
    {
        context->VerifyEquals(a,  1.0);
        context->VerifyEquals(b, -2.0);
        context->VerifyEquals(c,  3.0);
        context->VerifyEquals(d, -4.0);
        context->VerifyEquals(e,  5.0);
        context->VerifyEquals(f, -6.0);
    }

    void FD7(cxr::R8 a, cxr::R8 b, cxr::R8 c, cxr::R8 d, cxr::R8 e, cxr::R8 f, cxr::R8 g)
    {
        context->VerifyEquals(a,  1.0);
        context->VerifyEquals(b, -2.0);
        context->VerifyEquals(c,  3.0);
        context->VerifyEquals(d, -4.0);
        context->VerifyEquals(e,  5.0);
        context->VerifyEquals(f, -6.0);
        context->VerifyEquals(g,  7.0);
    }

    void FD8(cxr::R8 a, cxr::R8 b, cxr::R8 c, cxr::R8 d, cxr::R8 e, cxr::R8 f, cxr::R8 g, cxr::R8 h)
    {
        context->VerifyEquals(a,  1.0);
        context->VerifyEquals(b, -2.0);
        context->VerifyEquals(c,  3.0);
        context->VerifyEquals(d, -4.0);
        context->VerifyEquals(e,  5.0);
        context->VerifyEquals(f, -6.0);
        context->VerifyEquals(g,  7.0);
        context->VerifyEquals(h, -8.0);
    }

    CXXREFLECTTEST_REGISTER(X64FastCallThunk_DoublePrecisionRealArguments, [](Context& c)
    {
        GuardedContextInitializer const contextGuard(&c);

        cxr::X64ArgumentFrame frame;
        frame.Push( 1.0);
        frame.Push(-2.0);
        frame.Push( 3.0);
        frame.Push(-4.0);
        frame.Push( 5.0);
        frame.Push(-6.0);
        frame.Push( 7.0);
        frame.Push(-8.0);

        cxr::CxxReflectX64FastCallThunk(FD1, frame.GetArguments(), frame.GetTypes(), 1);
        cxr::CxxReflectX64FastCallThunk(FD2, frame.GetArguments(), frame.GetTypes(), 2);
        cxr::CxxReflectX64FastCallThunk(FD3, frame.GetArguments(), frame.GetTypes(), 3);
        cxr::CxxReflectX64FastCallThunk(FD4, frame.GetArguments(), frame.GetTypes(), 4);
        cxr::CxxReflectX64FastCallThunk(FD5, frame.GetArguments(), frame.GetTypes(), 5);
        cxr::CxxReflectX64FastCallThunk(FD6, frame.GetArguments(), frame.GetTypes(), 6);
        cxr::CxxReflectX64FastCallThunk(FD7, frame.GetArguments(), frame.GetTypes(), 7);
        cxr::CxxReflectX64FastCallThunk(FD8, frame.GetArguments(), frame.GetTypes(), 8);
    });

} }

namespace CxxReflectTest { namespace {

    void FS1(cxr::R4 a)
    {
        context->VerifyEquals(a,  1.0f);
    }

    void FS2(cxr::R4 a, cxr::R4 b)
    {
        context->VerifyEquals(a,  1.0f);
        context->VerifyEquals(b, -2.0f);
    }

    void FS3(cxr::R4 a, cxr::R4 b, cxr::R4 c)
    {
        context->VerifyEquals(a,  1.0f);
        context->VerifyEquals(b, -2.0f);
        context->VerifyEquals(c,  3.0f);
    }

    void FS4(cxr::R4 a, cxr::R4 b, cxr::R4 c, cxr::R4 d)
    {
        context->VerifyEquals(a,  1.0f);
        context->VerifyEquals(b, -2.0f);
        context->VerifyEquals(c,  3.0f);
        context->VerifyEquals(d, -4.0f);
    }

    void FS5(cxr::R4 a, cxr::R4 b, cxr::R4 c, cxr::R4 d, cxr::R4 e)
    {
        context->VerifyEquals(a,  1.0f);
        context->VerifyEquals(b, -2.0f);
        context->VerifyEquals(c,  3.0f);
        context->VerifyEquals(d, -4.0f);
        context->VerifyEquals(e,  5.0f);
    }

    void FS6(cxr::R4 a, cxr::R4 b, cxr::R4 c, cxr::R4 d, cxr::R4 e, cxr::R4 f)
    {
        context->VerifyEquals(a,  1.0f);
        context->VerifyEquals(b, -2.0f);
        context->VerifyEquals(c,  3.0f);
        context->VerifyEquals(d, -4.0f);
        context->VerifyEquals(e,  5.0f);
        context->VerifyEquals(f, -6.0f);
    }

    void FS7(cxr::R4 a, cxr::R4 b, cxr::R4 c, cxr::R4 d, cxr::R4 e, cxr::R4 f, cxr::R4 g)
    {
        context->VerifyEquals(a,  1.0f);
        context->VerifyEquals(b, -2.0f);
        context->VerifyEquals(c,  3.0f);
        context->VerifyEquals(d, -4.0f);
        context->VerifyEquals(e,  5.0f);
        context->VerifyEquals(f, -6.0f);
        context->VerifyEquals(g,  7.0f);
    }

    void FS8(cxr::R4 a, cxr::R4 b, cxr::R4 c, cxr::R4 d, cxr::R4 e, cxr::R4 f, cxr::R4 g, cxr::R4 h)
    {
        context->VerifyEquals(a,  1.0f);
        context->VerifyEquals(b, -2.0f);
        context->VerifyEquals(c,  3.0f);
        context->VerifyEquals(d, -4.0f);
        context->VerifyEquals(e,  5.0f);
        context->VerifyEquals(f, -6.0f);
        context->VerifyEquals(g,  7.0f);
        context->VerifyEquals(h, -8.0f);
    }

    CXXREFLECTTEST_REGISTER(X64FastCallThunk_SinglePrecisionRealArguments, [](Context& c)
    {
        GuardedContextInitializer const contextGuard(&c);

        cxr::X64ArgumentFrame frame;
        frame.Push( 1.0f);
        frame.Push(-2.0f);
        frame.Push( 3.0f);
        frame.Push(-4.0f);
        frame.Push( 5.0f);
        frame.Push(-6.0f);
        frame.Push( 7.0f);
        frame.Push(-8.0f);
        
        cxr::CxxReflectX64FastCallThunk(FS1, frame.GetArguments(), frame.GetTypes(), 1);
        cxr::CxxReflectX64FastCallThunk(FS2, frame.GetArguments(), frame.GetTypes(), 2);
        cxr::CxxReflectX64FastCallThunk(FS3, frame.GetArguments(), frame.GetTypes(), 3);
        cxr::CxxReflectX64FastCallThunk(FS4, frame.GetArguments(), frame.GetTypes(), 4);
        cxr::CxxReflectX64FastCallThunk(FS5, frame.GetArguments(), frame.GetTypes(), 5);
        cxr::CxxReflectX64FastCallThunk(FS6, frame.GetArguments(), frame.GetTypes(), 6);
        cxr::CxxReflectX64FastCallThunk(FS7, frame.GetArguments(), frame.GetTypes(), 7);
        cxr::CxxReflectX64FastCallThunk(FS8, frame.GetArguments(), frame.GetTypes(), 8);
    });

} }

namespace CxxReflectTest { namespace {

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    void FMVerify123456(A a, B b, C c, D d, E e, F f)
    {
        context->VerifyEquals(a, 1);
        context->VerifyEquals(b, 2);
        context->VerifyEquals(c, 3);
        context->VerifyEquals(d, 4);
        context->VerifyEquals(e, 5);
        context->VerifyEquals(f, 6);
    }

    void FMA(cxr::I1 a, cxr::I2 b, cxr::I1 c, cxr::I2 d, cxr::I1 e, cxr::I2 f)
    {
        FMVerify123456(a, b, c, d, e, f);
    }

    void FMB(cxr::I2 a, cxr::I4 b, cxr::I2 c, cxr::I4 d, cxr::I2 e, cxr::I4 f)
    {
        FMVerify123456(a, b, c, d, e, f);
    }

    void FMC(cxr::I4 a, cxr::I8 b, cxr::I4 c, cxr::I8 d, cxr::I4 e, cxr::I8 f)
    {
        FMVerify123456(a, b, c, d, e, f);
    }

    void FMD(cxr::I1 a, cxr::I2 b, cxr::I4 c, cxr::I8 d, cxr::I1 e, cxr::I2 f)
    {
        FMVerify123456(a, b, c, d, e, f);
    }

    void FME(cxr::I1 a, cxr::I8 b, cxr::I1 c, cxr::I8 d, cxr::I1 e, cxr::I8 f)
    {
        FMVerify123456(a, b, c, d, e, f);
    }

    void FMF(cxr::I8 a, cxr::I4 b, cxr::I2 c, cxr::I2 d, cxr::I4 e, cxr::I8 f)
    {
        FMVerify123456(a, b, c, d, e, f);
    }

    CXXREFLECTTEST_REGISTER(X64FastCallThunk_MixedIntegerArguments, [](Context& c)
    {
        GuardedContextInitializer const contextGuard(&c);

        cxr::X64ArgumentFrame frame;
        frame.Push(1LL);
        frame.Push(2LL);
        frame.Push(3LL);
        frame.Push(4LL);
        frame.Push(5LL);
        frame.Push(6LL);
        
        cxr::CxxReflectX64FastCallThunk(FMA, frame.GetArguments(), frame.GetTypes(), 6);
        cxr::CxxReflectX64FastCallThunk(FMB, frame.GetArguments(), frame.GetTypes(), 6);
        cxr::CxxReflectX64FastCallThunk(FMC, frame.GetArguments(), frame.GetTypes(), 6);
        cxr::CxxReflectX64FastCallThunk(FMD, frame.GetArguments(), frame.GetTypes(), 6);
        cxr::CxxReflectX64FastCallThunk(FME, frame.GetArguments(), frame.GetTypes(), 6);
        cxr::CxxReflectX64FastCallThunk(FMF, frame.GetArguments(), frame.GetTypes(), 6);
    });

} }

namespace CxxReflectTest { namespace {

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    cxr::X64ArgumentFrame FNInitFrame(A a, B b, C c, D d, E e, F f)
    {
        cxr::X64ArgumentFrame frame;
        frame.Push(a);
        frame.Push(b);
        frame.Push(c);
        frame.Push(d);
        frame.Push(e);
        frame.Push(f);
        return frame;
    }

    template <typename A, typename B, typename C, typename D, typename E, typename F>
    void FNVerify123456(A a, B b, C c, D d, E e, F f)
    {
        context->VerifyEquals(a, 1);
        context->VerifyEquals(b, 2);
        context->VerifyEquals(c, 3);
        context->VerifyEquals(d, 4);
        context->VerifyEquals(e, 5);
        context->VerifyEquals(f, 6);
    }

    void FNA(cxr::R8 a, cxr::I8 b, cxr::R8 c, cxr::I8 d, cxr::R8 e, cxr::I8 f)
    {
        FNVerify123456(a, b, c, d, e, f);
    }

    void FNB(cxr::I8 a, cxr::R8 b, cxr::R8 c, cxr::I8 d, cxr::I8 e, cxr::R8 f)
    {
        FNVerify123456(a, b, c, d, e, f);
    }

    void FNC(cxr::I8 a, cxr::R4 b, cxr::R4 c, cxr::I8 d, cxr::I8 e, cxr::R4 f)
    {
        FNVerify123456(a, b, c, d, e, f);
    }

    void FND(cxr::I4 a, cxr::R4 b, cxr::R8 c, cxr::I8 d, cxr::R4 e, cxr::R8 f)
    {
        FNVerify123456(a, b, c, d, e, f);
    }

    CXXREFLECTTEST_REGISTER(X64FastCallThunk_MixedRealAndIntegerArguments, [](Context& c)
    {
        GuardedContextInitializer const contextGuard(&c);

        cxr::X64ArgumentFrame frameA(FNInitFrame(1.0, 2LL, 3.0, 4LL, 5.0, 6LL));
        cxr::CxxReflectX64FastCallThunk(FNA, frameA.GetArguments(), frameA.GetTypes(), 6);

        cxr::X64ArgumentFrame frameB(FNInitFrame(1LL, 2.0, 3.0, 4LL, 5LL, 6.0));
        cxr::CxxReflectX64FastCallThunk(FNB, frameB.GetArguments(), frameB.GetTypes(), 6);

        cxr::X64ArgumentFrame frameC(FNInitFrame(1LL, 2.0f, 3.0f, 4LL, 5LL, 6.0f));
        cxr::CxxReflectX64FastCallThunk(FNC, frameC.GetArguments(), frameC.GetTypes(), 6);

        cxr::X64ArgumentFrame frameD(FNInitFrame(1LL, 2.0f, 3.0, 4LL, 5.0f, 6.0));
        cxr::CxxReflectX64FastCallThunk(FND, frameD.GetArguments(), frameD.GetTypes(), 6);
    });

} }

namespace CxxReflectTest { namespace {

    struct BasicStruct
    {
        cxr::U8 x;
        cxr::U8 y;
        cxr::U8 z;
    };

    void FBasicStruct(BasicStruct s)
    {
        context->VerifyEquals(s.x, 1);
        context->VerifyEquals(s.y, 2);
        context->VerifyEquals(s.z, 3);
    }

    CXXREFLECTTEST_REGISTER(X64FastCallThunk_Structures, [](Context& c)
    {
        GuardedContextInitializer const contextGuard(&c);

        BasicStruct x = { 1, 2, 3 };

        cxr::X64ArgumentFrame frame;
        frame.Push(&x);
        
        cxr::CxxReflectX64FastCallThunk(FBasicStruct, frame.GetArguments(), frame.GetTypes(), 1);
    });

} }

namespace CxxReflectTest { namespace {

    class FException { };

    void FThrows(int, int, int, int, int, int)
    {
        throw FException();
    }

    CXXREFLECTTEST_REGISTER(X64FastCallThunk_ExceptionalReturn, [](Context& c)
    {
        GuardedContextInitializer const contextGuard(&c);

        cxr::X64ArgumentFrame frame;
        frame.Push(1LL);
        frame.Push(2LL);
        frame.Push(3LL);
        frame.Push(4LL);
        frame.Push(5LL);
        frame.Push(6LL);

        try
        {
            cxr::CxxReflectX64FastCallThunk(FThrows, frame.GetArguments(), frame.GetTypes(), 6);
            c.Fail();
        }
        catch (FException const&)
        {
        }
    });

} }

#endif // CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64
