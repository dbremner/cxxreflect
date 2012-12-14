//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"

#include "cxxreflect/cxxreflect.hpp"
#include "tests/unit_tests/infrastructure/test_driver.hpp"

using namespace unit_tests_windows_runtime_app;

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

MainPage::MainPage()
{
	InitializeComponent();

    cxxreflect::windows_runtime::begin_initialization();
    while (!cxxreflect::windows_runtime::is_initialized());

    cxxreflect_test::driver::start(static_cast<wchar_t const**>(nullptr), static_cast<wchar_t const**>(nullptr));
    // int breakpoint = 0;
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}
