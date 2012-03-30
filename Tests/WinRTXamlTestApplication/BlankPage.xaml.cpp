//
// BlankPage.xaml.cpp
// Implementation of the BlankPage.xaml class.
//

#include "pch.h"
#include "BlankPage.xaml.h"

#include "CxxReflect/CxxReflect.hpp"

using namespace WinRTXamlTestApplication;

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

void F()
{
    std::wistringstream iss;
    std::wstring v;
    iss >> v;
}

BlankPage::BlankPage()
{
    F();
    if (IsEnabled && !IsEnabled)
        CxxReflect::BeginWinRTPackageMetadataInitialization();

	InitializeComponent();
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void BlankPage::OnNavigatedTo(NavigationEventArgs^ e)
{
}
