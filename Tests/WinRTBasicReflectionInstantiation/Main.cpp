#include "CxxReflect/CxxReflect.hpp"

#include <sstream>

#include <inspectable.h>
#include <Windows.h>
#include <future>

namespace cxr
{
    using namespace CxxReflect;
    using namespace CxxReflect::WindowsRuntime;
}

int main(Platform::Array<Platform::String^>^)
{
    cxr::BeginPackageInitialization();

    auto const types(cxr::GetImplementersOf<WinRTBasicReflectionTest::IProvideANumber^>());

    std::for_each(begin(types), end(types), [&](cxr::Type const& type)
    {
        // If the type is not default constructible; skip it:
        if (!cxr::IsDefaultConstructible(type))
            return;

        auto const instance(cxr::CreateInstance<WinRTBasicReflectionTest::IProvideANumber>(type));
        
        std::wstringstream formatter;
        formatter << type.GetFullName() << L":  " << instance->GetNumber() << L'\n';

        OutputDebugString(formatter.str().c_str());
    });

    Platform::String::typeid->FullName->Data();

    cxr::Type const userType(cxr::GetType(L"WinRTBasicReflectionTest.UserProvidedNumber"));

    auto const userInstance(cxr::CreateInstance<WinRTBasicReflectionTest::IProvideANumber>(userType, 10));
}
