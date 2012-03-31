#include "CxxReflect/CxxReflect.hpp"

#include <sstream>

#include <inspectable.h>
#include <Windows.h>

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
        auto const instance(cxr::CreateInstance<WinRTBasicReflectionTest::IProvideANumber>(type));
        
        std::wstringstream formatter;
        formatter << type.GetFullName() << L":  " << instance->GetNumber() << L'\n';

        OutputDebugString(formatter.str().c_str());
    });

    cxr::Type t = cxr::GetType(L"WinRTBasicReflectionTest.TestClass");
    std::for_each(t.BeginCustomAttributes(), t.EndCustomAttributes(), [&](cxr::CustomAttribute const& a)
    {
        OutputDebugString(L"CA:");
        OutputDebugString(a.GetConstructor().GetDeclaringType().GetFullName().c_str());
        OutputDebugString(L"\n");
    });

    cxr::CreateInspectableInstance(t, 42);
}