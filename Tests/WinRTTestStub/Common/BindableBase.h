#pragma once

#include "pch.h"

namespace WinRTTestStub
{
	namespace Common
	{
		/// <summary>
		/// Implementation of <see cref="INotifyPropertyChanged"/> to simplify models.
		/// </summary>
		[Windows::Foundation::Metadata::WebHostHidden]
		public ref class BindableBase : Windows::UI::Xaml::Data::INotifyPropertyChanged
		{
		public:
			event Windows::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

		protected:
			void OnPropertyChanged(Platform::String^ propertyName);
		};
	}
}
