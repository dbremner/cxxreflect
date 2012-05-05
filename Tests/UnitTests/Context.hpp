#ifndef CXXREFLECT_UNITTESTS_CONTEXT_HPP_
#define CXXREFLECT_UNITTESTS_CONTEXT_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include <algorithm>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>

namespace CxxReflectTest {

    typedef std::wstring String;
    

    class TestError : public std::exception
    {
    public:

        TestError(String const& message = L"")
            : _message(message)
        {
        }

    private:

        String _message;
    };

    namespace KnownProperty { namespace {

        wchar_t const* PrimaryAssemblyPath(L"PrimaryAssemblyPath");

    } }



    
    class Context
    {
    public:

        String GetProperty(String const& key) const
        {
            return L"c:\\windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll";
        }

        void Verify(bool b) const
        {
            if (!b)
                throw TestError(L"");
        }

        template <typename T, typename U>
        void VerifyEquals(T&& t, U&& u) const
        {
            if (!(t == u))
                throw TestError(L"");
        }

        template <typename TForIt0, typename TForIt1>
        void VerifyRangeEquals(TForIt0 first0, TForIt0 last0, TForIt1 first1, TForIt1 last1) const
        {
            while (first0 != last0 && first1 != last1)
            {
                if (!(*first0 == *first1))
                    throw TestError(L"");

                ++first0;
                ++first1;
            }

            if (first0 != last0 || first1 != last1)
                throw TestError(L"Ranges were not of the same size");
        }

        void VerifySuccess(int hresult) const
        {
            if (hresult < 0)
                throw TestError(L"");
        }

        void Fail() const
        {
            throw TestError(L"Unexpectedly failed");
        }

    private:

        std::map<String, String> _properties;
    };




    typedef std::function<void(Context&)> TestFunction;

    
    


    class Index
    {
    public:

        static int RegisterTest(String const& name, TestFunction const& function);

        static void RunAllTests();

    private:

        typedef std::map<String, TestFunction> TestRegistry;

        static TestRegistry& GetOrCreateRegistry();
    };




    #define CXXREFLECTTEST_CONCATENATE_(q, r) q ## r
    #define CXXREFLECTTEST_CONCATENATE(q, r) CXXREFLECTTEST_CONCATENATE_(q, r)

    #define CXXREFLECTTEST_STRINGIZE_(q) # q 
    #define CXXREFLECTTEST_STRINGIZE(q) CXXREFLECTTEST_STRINGIZE_(q)

    #define CXXREFLECTTEST_REGISTER(name, ...)                                       \
        int CXXREFLECTTEST_CONCATENATE(name, RegistrationToken)(Index::RegisterTest( \
            CXXREFLECTTEST_CONCATENATE(L, CXXREFLECTTEST_STRINGIZE(name)),           \
            __VA_ARGS__                                                              \
        ))

    

}

#endif
