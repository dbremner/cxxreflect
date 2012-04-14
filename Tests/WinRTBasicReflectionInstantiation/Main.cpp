#include "CxxReflect/CxxReflect.hpp"

#include <sstream>

#include <inspectable.h>
#include <Windows.h>
#include <future>

namespace w
{
    using namespace Windows::UI::Xaml;
}

namespace cxr
{
    using namespace CxxReflect;
    using namespace CxxReflect::WindowsRuntime;
}

namespace
{
    std::future<void> asyncContext;

    void Run(w::ApplicationInitializationCallbackParams^)
    {
        cxr::BeginPackageInitialization();

        cxr::CallWhenInitialized([&]
        {
            int x = 42;
        });

        asyncContext = std::async(std::launch::async, [&]
        {
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
        });
    }
}

namespace WinRTBasicReflectionInstantiation
{
    ref class App sealed : public w::Application
	{
	public:

		App()
        {
            
        }

		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ pArgs) override
        {
            Run(nullptr);
        }

	private:
		void OnSuspending(Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
	};
}

int main(Platform::Array<Platform::String^>^)
{
    w::Application^ app = ref new w::Application();
    app->Start(ref new w::ApplicationInitializationCallback(Run));
}
