
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "pch.h"
#include "BooleanNegationConverter.h"

using namespace WRTestApp::Common;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Interop;

Object^ BooleanNegationConverter::Convert(Object^ value, TypeName targetType, Object^ parameter, String^)
{
	auto boxedBool = dynamic_cast<Box<bool>^>(value);
	auto boolValue = (boxedBool != nullptr && boxedBool->Value);
	return !boolValue;
}

Object^ BooleanNegationConverter::ConvertBack(Object^ value, TypeName targetType, Object^ parameter, String^)
{
	auto boxedBool = dynamic_cast<Box<bool>^>(value);
	auto boolValue = (boxedBool != nullptr && boxedBool->Value);
	return !boolValue;
}
