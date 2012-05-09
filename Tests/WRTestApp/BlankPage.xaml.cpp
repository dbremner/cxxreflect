
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "pch.h"
#include "BlankPage.xaml.h"

#include "CxxReflect/CxxReflect.hpp"

namespace WRTestApp { namespace {

    void PrintTypeAndNumber(WRLibrary::IProvideANumber^ const provider)
    {
        std::wstringstream formatter;
        formatter << cxr::GetTypeOf(provider).GetFullName() << L":  " << provider->GetNumber() << L'\n';
        OutputDebugString(formatter.str().c_str());
    }

} }

namespace WRTestApp {

    BlankPage::BlankPage()
    {
	    InitializeComponent();

        cxr::WhenInitializedCall([&]
        {
            {
                using namespace WRLibrary;

                Band^ band = ref new Band();

                IJohn^ john = band;
                IPaul^ paul = band;

                cxr::CreateObjectInstance(
                    cxr::GetType(L"WRLibrary.BandClient"),
                    john,
                    paul,
                    band,
                    band);
            }

            {
                auto enumerators(cxr::GetEnumerators(cxr::GetType(L"WRLibrary.DayOfWeek")));

                // The order of the enumerators is unspecified, so we'll sort them by value:
                std::sort(begin(enumerators), end(enumerators), cxr::EnumeratorUnsignedValueOrdering());

                // Print the enumerators:
                std::for_each(begin(enumerators), end(enumerators), [&](cxr::Enumerator const& e)
                {
                    std::wstringstream formatter;
                    formatter << e.GetName() << L":  " << e.GetValueAsUInt64() << L"\n";
                    OutputDebugString(formatter.str().c_str());
                });
            }

            {
                cxr::Type const awesomeType(cxr::GetType(L"WRLibrary.MyAwesomeType"));

                OutputDebugString(L"Type hierarchy of WRLibrary.MyAwesomeType:\n");
                cxr::Type baseType(awesomeType);
                while (baseType.IsInitialized())
                {
                    std::wstringstream formatter;
                    formatter << baseType.GetFullName() << L"\n";
                    OutputDebugString(formatter.str().c_str());

                    baseType = baseType.GetBaseType();
                }

                OutputDebugString(L"Interfaces implemented by WRLibrary.MyAwesomeType:\n");
                std::for_each(awesomeType.BeginInterfaces(), awesomeType.EndInterfaces(), [&](cxr::Type const& iface)
                {
                    std::wstringstream formatter;
                    formatter << iface.GetFullName() << L"\n";
                    OutputDebugString(formatter.str().c_str());
                });

                OutputDebugString(L"Methods of WRLibrary.MyAwesomeType:\n");
                auto const firstMethod(awesomeType.BeginMethods(cxr::BindingAttribute::AllInstance));
                auto const lastMethod(awesomeType.EndMethods());
                std::for_each(firstMethod, lastMethod, [&](cxr::Method const& method)
                {
                    std::wstringstream formatter;
                    formatter << method.GetName() << L"\n";
                    OutputDebugString(formatter.str().c_str());
                });
            }

            {
                typedef Windows::UI::Xaml::IDependencyObject IDependencyObject;
                auto const dependencyObjectTypes = cxr::GetImplementersOf<IDependencyObject>();
            }

            {
                auto const types(cxr::GetImplementersOf<WRLibrary::IProvideANumber>());
                std::for_each(begin(types), end(types), [](cxr::Type const& type)
                {
                    if (!cxr::IsDefaultConstructible(type))
                        return;

                    auto const instance(cxr::CreateInstance<WRLibrary::IProvideANumber>(type));
        
                    PrintTypeAndNumber(instance);
                });
            }

            {
                cxr::Type const type(cxr::GetType(L"WRLibrary.UserProvidedNumber"));
                for (int i(0); i < 5; ++i)
                {
                    auto const instance(cxr::CreateInstance<WRLibrary::IProvideANumber>(type, i));

                    PrintTypeAndNumber(instance);
                }
            }

            {
                WRLibrary::ProviderOfTheAnswer^ provider = ref new WRLibrary::ProviderOfTheAnswer();
                cxr::Type const wrapperType = cxr::GetType(L"WRLibrary.ProviderOfAWrappedNumber");
                auto const wrapperInstance = cxr::CreateInstance<WRLibrary::IProvideANumber>(wrapperType, provider);
                PrintTypeAndNumber(wrapperInstance);
            }

            {
                cxr::Type const userMultipliedType(cxr::GetType(L"WRLibrary.UserProvidedMultipliedNumber"));
                auto const userMultipliedInstance(cxr::CreateInstance<WRLibrary::IProvideANumber>(userMultipliedType, 2, 4));
                PrintTypeAndNumber(userMultipliedInstance);
            }

        });
    }

    /// <summary>
    /// Invoked when this page is about to be displayed in a Frame.
    /// </summary>
    /// <param name="e">Event data that describes how this page was reached.  The Parameter
    /// property is typically used to configure the page.</param>
    void BlankPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
    {
    }

}