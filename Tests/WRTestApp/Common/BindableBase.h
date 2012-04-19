
//               Copyright James P. McNellis (james@jamesmcnellis.com) 2011 - 2012.               //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#pragma once

#include "pch.h"

namespace WRTestApp
{
	namespace Common
	{
		// Suppress class "not consumable from JavaScript because it's not marked 'sealed'" warning
		// currently emitted despite the WebHostHidden attribute
		#pragma warning(push)
		#pragma warning(disable: 4449)
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
		#pragma warning(pop)
	}
}
