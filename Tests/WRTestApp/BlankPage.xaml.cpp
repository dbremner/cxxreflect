
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
                // typedef Windows::UI::Xaml::IDependencyObject IDependencyObject;
                // auto const dependencyObjectTypes = cxr::GetImplementersOf<IDependencyObject>();
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