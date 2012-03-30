#include "CxxReflect/CxxReflect.hpp"

namespace cxr = CxxReflect::WindowsRuntime;

int main(Platform::Array<Platform::String^>^)
{
    CxxReflect::Externals::Initialize<CxxReflect::Platform::Win32>();
    CxxReflect::BeginWinRTPackageMetadataInitialization();

    auto const numberProviders(cxr::GetImplementersOf<WinRTBasicReflectionTest::IProvideANumber^>());

    WinRTBasicReflectionTest::IProvideANumber^ numberProvider = ref new WinRTBasicReflectionTest::ProviderOfTheAnswer();
    int x = numberProvider->GetNumber();
}