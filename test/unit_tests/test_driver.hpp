//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_UNITTESTS_TEST_DRIVER_HPP_
#define CXXREFLECT_UNITTESTS_TEST_DRIVER_HPP_

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace cxxreflect_test
{
    typedef std::size_t  size_type;
    typedef std::wstring string;

    class test_error : public std::exception
    {
    public:

        test_error(string const& m = L"") : _message(m) { }

        auto message() const -> string const& { return _message; }

    private:

        string _message;
    };

    namespace known_property { namespace {

        auto framework_path()        -> string { return L"framework_path"; }
        auto primary_assembly_path() -> string { return L"primary_assembly_path"; }
        auto test_assemblies_path()  -> string { return L"test_assemblies_path";  }
    } }

    class context
    {
    public:

        auto get_property(string const& p) const -> string
        {
            if (p == known_property::framework_path())
                return L"c:\\windows\\Microsoft.NET\\Framework\\v4.0.30319";
            else if (p == known_property::primary_assembly_path())
                return L"c:\\windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll";
            else if (p == known_property::test_assemblies_path())
                return L"c:\\jm\\cxxreflect\\build\\output\\Win32\\Debug\\test_assemblies";
            else
                throw test_error(L"failed to find property:  " + p);
        }

        auto verify(bool b) const -> void
        {
            if (!b)
                throw test_error(L"expected true; got false");
        }

        template <typename T, typename U>
        auto verify_equals(T&& t, U&& u) const -> void
        {
            if (!(t == u))
                throw test_error(L"objects did not compare equal");
        }

        template <typename ForwardIterator0, typename ForwardIterator1>
        auto verify_range_equals(ForwardIterator0 first0, ForwardIterator0 const last0,
                                 ForwardIterator1 first1, ForwardIterator1 const last1) const -> void
        {
            while (first0 != last0 && first1 != last1)
            {
                if (!(*first0 == *first1))
                    throw test_error(L"pair of elements in range did not compare equal");

                ++first0;
                ++first1;
            }

            if (first0 != last0 || first1 != last1)
                throw test_error(L"ranges were not of the same size");
        }

        auto verify_success(int hr) const -> void
        {
            if (hr < 0)
                throw test_error(L"operation failed with hresult " + std::to_wstring(hr));
        }

        template <typename Exception, typename Callable>
        auto verify_exception(Callable&& c) const -> void
        {
            try
            {
                c();
                throw test_error(L"expected exception; no exception was thrown");
            }
            catch (Exception const&)
            {
                return;
            }
            catch (...)
            {
                throw test_error(L"expected exception; wrong exception was thrown");
            }
        }

        auto fail(string const& m = L"unexpected catastrophic failure") const -> void
        {
            throw test_error(m);
        }

    private:


    };

    typedef std::function<void(context const&)> test_function;
    typedef std::map<string, test_function>     test_registry;

    class driver
    {
    public:

        static auto register_test(string const& name, test_function const& function) -> size_type
        {
            if (!global_registry_state())
                return 0;

            if (!global_registry().insert(std::make_pair(name, function)).second)
                throw test_error(L"test name already registered");

            return global_registry().size();
        }

        static auto register_solo(string const& name) -> size_type
        {
            test_function const function(global_registry()[name]);
            global_registry().clear();
            global_registry().insert(std::make_pair(name, function));
            global_registry_state() = false;
            return 1;
        }

        template <typename ForwardIterator>
        static auto start(ForwardIterator const first_argument, ForwardIterator const last_argument) -> void
        {
            std::vector<string> const arguments(first_argument, last_argument);

            run_all_tests(context());
        }

    private:

        static auto run_all_tests(context const& c) -> void
        {
            typedef test_registry::const_reference reference;

            size_type test_count(0);
            size_type pass_count(0);

            output() << L"================================================================================" << std::endl;
            output() << L"Starting Test Run..." << std::endl;
            output() << L"================================================================================" << std::endl;

            auto const& registry(global_registry());
            std::for_each(begin(registry), end(registry), [&](reference test) -> void
            {
                ++test_count;

                if (run_test(test.first, test.second, c))
                    ++pass_count;
            });

            output() << L"================================================================================" << std::endl;
            output() << L"Test Run Completed:  " << pass_count << L" passed out of " << test_count << L"." << std::endl;
            output() << L"================================================================================" << std::endl;
        }

        static auto run_test(string const& name, test_function const& call, context const& c) -> bool
        {
            output() << L"Running test [" << std::setw(70) << std::left << name << L"]:  ";
            try
            {
                call(c);
                output() << L"PASSED" << std::endl;
                return true;
            }
            catch (test_error const& error)
            {
                output() << L"FAILED" << std::endl;
                output() << L"    Failure:  " << error.message() << std::endl;
                return false;
            }
            catch (...)
            {
                output() << L"FAILED" << std::endl;
                output() << L"    An unknown exception occurred during execution." << std::endl;
                return false;
            }
        }

        static auto global_registry() -> test_registry&
        {
            static test_registry r;
            return r;
        }

        static auto global_registry_state() -> bool&
        {
            static bool state(true);
            return state;
        }

        static auto output(std::wostream* p = nullptr) -> std::wostream&
        {
            static std::wostream* o(&std::wcout);
            if (p != nullptr)
                o = p;

            if (o == nullptr)
                throw test_error(L"output stream not initialized");

            return *o;
        }
    };


    #define CXXREFLECTTEST_CONCATENATE_(q, r) q ## r
    #define CXXREFLECTTEST_CONCATENATE(q, r) CXXREFLECTTEST_CONCATENATE_(q, r)

    #define CXXREFLECTTEST_STRINGIZE_(q) # q 
    #define CXXREFLECTTEST_STRINGIZE(q) CXXREFLECTTEST_STRINGIZE_(q)

    #define CXXREFLECTTEST_REGISTER(name, ...)                                                   \
    auto CXXREFLECTTEST_CONCATENATE(test_function_, name)(context const& test_c) -> void         \
    {                                                                                            \
        (__VA_ARGS__)(test_c);                                                                   \
    }                                                                                            \
                                                                                                 \
    std::size_t CXXREFLECTTEST_CONCATENATE(test_function_token_, name)(driver::register_test(    \
        CXXREFLECTTEST_CONCATENATE(L, CXXREFLECTTEST_STRINGIZE(name)),                           \
        CXXREFLECTTEST_CONCATENATE(test_function_, name)))

    #define CXXREFLECTTEST_REGISTER_SOLO(name, ...)      \
        CXXREFLECTTEST_REGISTER(name, __VA_ARGS__);      \
        std::size_t solo_tag(driver::register_solo(CXXREFLECTTEST_CONCATENATE(L, CXXREFLECTTEST_STRINGIZE(name))))

}

#endif

// AMDG //
