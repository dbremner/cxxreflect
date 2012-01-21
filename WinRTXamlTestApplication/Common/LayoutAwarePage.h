#pragma once

#include "pch.h"
#include <collection.h>

namespace WinRTXamlTestApplication
{
	namespace Common
	{
		/// <summary>
		/// Typical implementation of Page that provides several important conveniences: application
		/// view state to visual state mapping, GoBack and GoHome event handlers, and a default view
		/// model.
		/// </summary>
		[Windows::Foundation::Metadata::WebHostHidden]
		public ref class LayoutAwarePage : Windows::UI::Xaml::Controls::Page
		{
		public:
			LayoutAwarePage();
			void StartLayoutUpdates(Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void StopLayoutUpdates(Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			void InvalidateVisualState();
			void InvalidateVisualState(Windows::UI::ViewManagement::ApplicationViewState viewState);
			property bool UseFilledStateForNarrowWindow
			{
				bool get();
				void set(bool value);
			}
			// TEMPORARY: the DefaultViewModel should be protected, but is temporarily public to work
			// around C++ compiler issue #323005
			property Windows::Foundation::Collections::IObservableMap<Platform::String^, Object^>^ DefaultViewModel
			{
				Windows::Foundation::Collections::IObservableMap<Platform::String^, Object^>^ get();
			}

		protected:
			virtual void GoHome(Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			virtual void GoBack(Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
			virtual Platform::String^ DetermineVisualState(Windows::UI::ViewManagement::ApplicationViewState viewState);

		private:
			bool _useFilledStateForNarrowWindow;
			Platform::Collections::Map<Platform::String^, Object^>^ _defaultViewModel;
			Windows::Foundation::EventRegistrationToken _viewStateEventToken;
			Windows::Foundation::EventRegistrationToken _windowSizeEventToken;
			Platform::Collections::Vector<Windows::UI::Xaml::Controls::Control^>^ _layoutAwareControls;
			void ViewStateChanged(Windows::UI::ViewManagement::ApplicationView^ sender, Windows::UI::ViewManagement::ApplicationViewStateChangedEventArgs^ e);
			void WindowSizeChanged(Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ e);
		};
	}
}
