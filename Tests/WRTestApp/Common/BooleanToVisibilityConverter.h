
//               Copyright James P. McNellis (james@jamesmcnellis.com) 2011 - 2012.               //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#pragma once

#include "pch.h"

namespace WRTestApp
{
	namespace Common
	{
		/// <summary>
		/// Value converter that translates true to <see cref="Visibility.Visible"/> and false
		/// to <see cref="Visibility.Collapsed"/>.
		/// </summary>
		public ref class BooleanToVisibilityConverter sealed : Windows::UI::Xaml::Data::IValueConverter
		{
		public:
			virtual Object^ Convert(Object^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object^ parameter, Platform::String^);
			virtual Object^ ConvertBack(Object^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object^ parameter, Platform::String^);
		};
	}
}
