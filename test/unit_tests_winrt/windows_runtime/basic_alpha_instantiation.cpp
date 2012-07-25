
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "test/unit_tests/test_driver.hpp"
#include "cxxreflect/cxxreflect.hpp"

namespace cxr
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
    using namespace cxxreflect::windows_runtime;
}

namespace co
{
    using namespace TestComponents::Alpha;
}

namespace cxxreflect_test { namespace {

    auto verify_alpha_instantiation_number_providers(context const& c) -> void
    {
        // ProviderOfZero
        {
            auto const t(cxr::get_type(L"TestComponents.Alpha.ProviderOfZero"));
            c.verify(t.is_initialized());

            auto const i(cxr::create_instance<co::IProvideANumber>(t));
            c.verify(i != nullptr);

            auto const v(i->GetNumber());
            c.verify_equals(v, 0);
        }

        // ProviderOfOne
        {
            auto const t(cxr::get_type(L"TestComponents.Alpha.ProviderOfOne"));
            c.verify(t.is_initialized());

            auto const i(cxr::create_instance<co::IProvideANumber>(t));
            c.verify(i != nullptr);

            auto const v(i->GetNumber());
            c.verify_equals(v, 1);
        }

        // ProviderOfTheAnswer
        {
            auto const t(cxr::get_type(L"TestComponents.Alpha.ProviderOfTheAnswer"));
            c.verify(t.is_initialized());

            auto const i(cxr::create_instance<co::IProvideANumber>(t));
            c.verify(i != nullptr);

            auto const v(i->GetNumber());
            c.verify_equals(v, 42);
        }

        // UserProvidedNumber(12345)
        {
            auto const t(cxr::get_type(L"TestComponents.Alpha.UserProvidedNumber"));
            c.verify(t.is_initialized());

            auto const i(cxr::create_instance<co::IProvideANumber>(t, 12345));
            c.verify(i != nullptr);

            auto const v(i->GetNumber());
            c.verify_equals(v, 12345);
        }

        // UserProvidedNumber(12345)
        {
            auto const t(cxr::get_type(L"TestComponents.Alpha.UserProvidedNumber"));
            c.verify(t.is_initialized());

            auto const i(cxr::create_instance<co::IProvideANumber>(t, 12345));
            c.verify(i != nullptr);

            auto const v(i->GetNumber());
            c.verify_equals(v, 12345);
        }
    }

    CXXREFLECTTEST_REGISTER(alpha_instantiation_number_providers, verify_alpha_instantiation_number_providers);





    auto verify_alpha_instantiation_number_provider_implementers(context const& c) -> void
    {
        auto const implementers(cxr::get_implementers<co::IProvideANumber>());
        c.verify_equals(implementers.size(), 5u);

        unsigned count(0);
        std::for_each(begin(implementers), end(implementers), [&](cxr::type const& implementer)
        {
            if (!cxr::is_default_constructible(implementer))
                return;

            auto const instance(cxr::create_instance<co::IProvideANumber>(implementer));
            c.verify(instance != nullptr);

            auto const value(instance->GetNumber());
            c.verify(value >= 0);

            ++count;
        });

        c.verify_equals(count, 3u);
    }

    CXXREFLECTTEST_REGISTER(alpha_instantiation_number_provider_implementers, verify_alpha_instantiation_number_provider_implementers);

} }
