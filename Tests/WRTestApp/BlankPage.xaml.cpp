//
// BlankPage.xaml.cpp
// Implementation of the BlankPage.xaml class.
//

#include "pch.h"
#include "BlankPage.xaml.h"

#include "CxxReflect/CxxReflect.hpp"

using namespace WRTestApp;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

BlankPage::BlankPage()
{
	InitializeComponent();

    cxr::CallWhenInitialized([&]
    {
        auto const types(cxr::GetImplementersOf<WRLibrary::IProvideANumber^>());

        std::for_each(begin(types), end(types), [&](cxr::Type const& type)
        {
            // If the type is not default constructible; skip it:
            if (!cxr::IsDefaultConstructible(type))
                return;

            auto const instance(cxr::CreateInstance<WRLibrary::IProvideANumber>(type));
        
            std::wstringstream formatter;
            formatter << type.GetFullName() << L":  " << instance->GetNumber() << L'\n';

            OutputDebugString(formatter.str().c_str());
        });

        Platform::String::typeid->FullName->Data();

        cxr::Type const userType(cxr::GetType(L"WRLibrary.UserProvidedNumber"));

        auto const userInstance(cxr::CreateInstance<WRLibrary::IProvideANumber>(userType, 10));
    });
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void BlankPage::OnNavigatedTo(NavigationEventArgs^ e)
{
}
