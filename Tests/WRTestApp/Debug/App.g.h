﻿

#pragma once
//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------

#include "XamlTypeInfo.g.h"

namespace WRTestApp
{
    partial ref class App :  public Windows::UI::Xaml::Application, public  Windows::UI::Xaml::Markup::IXamlMetadataProvider, public Windows::UI::Xaml::Markup::IComponentConnector
    {
        public:
            void InitializeComponent();
            virtual void Connect(int connectionId, Platform::Object^ pTarget);

            virtual Windows::UI::Xaml::Markup::IXamlType^ GetXamlType(Windows::UI::Xaml::Interop::TypeName t)
            {
                return GetXamlType(t.Name);
            }

            virtual Windows::UI::Xaml::Markup::IXamlType^ GetXamlType(Platform::String^ typeName)
            {
                if(_provider == nullptr)
                {
                    _provider = ref new XamlTypeInfo::InfoProvider::XamlTypeInfoProvider();
                }
                return _provider->GetXamlTypeByName(typeName);
            }

            virtual Platform::Array<Windows::UI::Xaml::Markup::XmlnsDefinition>^ GetXmlnsDefinitions()
            {
                return ref new Platform::Array<Windows::UI::Xaml::Markup::XmlnsDefinition>(0);
            }

        private:
            XamlTypeInfo::InfoProvider::XamlTypeInfoProvider^ _provider;
            bool _contentLoaded;

        };
}

