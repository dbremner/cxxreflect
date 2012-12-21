
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "tests/unit_tests/windows_runtime/precompiled_headers.hpp"

#include <collection.h>

namespace sdk {

    using namespace CxxReflect::Reflection;
}

namespace cxxreflect_test {

    CXXREFLECTTEST_DEFINE_TEST(basic_alpha_sdk_obtain_loader)
    {
        sdk::ILoader^ const loader(win::sync(sdk::Loader::PackageLoader));
        c.verify(loader != nullptr);
    }

    CXXREFLECTTEST_DEFINE_TEST(basic_alpha_sdk_obtain_namespace)
    {
        sdk::ILoader^ const loader(win::sync(sdk::Loader::PackageLoader));

        sdk::Namespace^ const ns(loader->FindNamespace(L"TestComponents.Alpha"));
        
        c.verify(std::find_if(begin(ns->Types), end(ns->Types), [&](sdk::Type^ t) { return t->Name == L"DayOfWeek"; }) != end(ns->Types));
        for (sdk::Type^ type : ns->Types)
        {

        }
    }

    CXXREFLECTTEST_DEFINE_TEST(basic_alpha_sdk_obtain_type)
    {
        sdk::ILoader^ const loader(win::sync(sdk::Loader::PackageLoader));

        sdk::Type^ const dayOfWeekType(loader->FindType(L"TestComponents.Alpha.DayOfWeek"));
        c.verify(dayOfWeekType != nullptr);
        c.verify_equals(dayOfWeekType->FullName,        ref new Platform::String(L"TestComponents.Alpha.DayOfWeek"));
        c.verify_equals(dayOfWeekType->Name,            ref new Platform::String(L"DayOfWeek"));
        c.verify_equals(dayOfWeekType->Namespace->Name, ref new Platform::String(L"TestComponents.Alpha"));
        c.verify(!dayOfWeekType->IsAbstract);
        c.verify(!dayOfWeekType->IsArray);
        c.verify(!dayOfWeekType->IsByRef);
        c.verify(!dayOfWeekType->IsClass);
        c.verify(dayOfWeekType->IsEnum);
        c.verify(!dayOfWeekType->IsGenericType);
        c.verify(!dayOfWeekType->IsGenericTypeDefinition);
        c.verify(!dayOfWeekType->IsGenericTypeInstantiation);
        c.verify(!dayOfWeekType->IsGenericTypeParameter);
        c.verify(!dayOfWeekType->IsInterface);
        c.verify(!dayOfWeekType->IsPrimitive);
        c.verify(dayOfWeekType->IsSealed);
        c.verify(dayOfWeekType->IsValueType);

        sdk::Type^ const enumType(dayOfWeekType->BaseType);
        c.verify(enumType != nullptr);
        c.verify_equals(enumType->FullName,        ref new Platform::String(L"Platform.Enum"));
        c.verify_equals(enumType->Name,            ref new Platform::String(L"Enum"));
        c.verify_equals(enumType->Namespace->Name, ref new Platform::String(L"Platform"));

        sdk::Type^ const valueTypeType(enumType->BaseType);
        c.verify(valueTypeType != nullptr);
        c.verify_equals(valueTypeType->FullName,        ref new Platform::String(L"Platform.ValueType"));
        c.verify_equals(valueTypeType->Name,            ref new Platform::String(L"ValueType"));
        c.verify_equals(valueTypeType->Namespace->Name, ref new Platform::String(L"Platform"));

        sdk::Type^ const objectType(valueTypeType->BaseType);
        c.verify(objectType != nullptr);
        c.verify_equals(objectType->FullName,        ref new Platform::String(L"Platform.Object"));
        c.verify_equals(objectType->Name,            ref new Platform::String(L"Object"));
        c.verify_equals(objectType->Namespace->Name, ref new Platform::String(L"Platform"));

        sdk::Type^ const nullType(objectType->BaseType);
        c.verify(nullType == nullptr);
    }

}