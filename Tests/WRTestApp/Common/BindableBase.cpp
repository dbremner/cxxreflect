
//               Copyright James P. McNellis (james@jamesmcnellis.com) 2011 - 2012.               //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "pch.h"
#include "BindableBase.h"

using namespace WRTestApp::Common;

using namespace Platform;
using namespace Windows::UI::Xaml::Data;

/// <summary>
/// Notifies listeners that a property value has changed.
/// </summary>
/// <param name="propertyName">Name of the property used to notify listeners.</param>
void BindableBase::OnPropertyChanged(String^ propertyName)
{
	PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
}
