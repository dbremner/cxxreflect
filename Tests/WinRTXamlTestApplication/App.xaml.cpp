﻿//
// App.xaml.cpp
// Implementation of the App.xaml class.
//

#include "pch.h"
#include "BlankPage.xaml.h"

using namespace WinRTXamlTestApplication;

using namespace Platform;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

// The Split Application template is documented at http://go.microsoft.com/fwlink/?LinkId=234228

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
	InitializeComponent();
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used when the application is launched to open a specific file, to display
/// search results, and so forth.
/// </summary>
/// <param name="args">Details about the launch request and process.</param>
void App::OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ pArgs)
{
	// Create a Frame to act navigation context and navigate to the first page
	auto rootFrame = ref new Frame();
	TypeName pageType = { BlankPage::typeid->FullName, TypeKind::Metadata };
	rootFrame->Navigate(pageType);

	// Place the frame in the current Window and ensure that it is active
	Window::Current->Content = rootFrame;
	Window::Current->Activate();
}
