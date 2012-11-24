
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

using Windows::Foundation::Metadata::DefaultAttribute;

namespace TestComponents { namespace Alpha { namespace MethodImpl {

    public interface struct SimpleInterface
    {
        virtual void InterfaceFunctionShouldNotAppear() = 0;
    };

    public ref struct SimpleBaseClass
        : Windows::UI::Xaml::DependencyObject,
          SimpleInterface
    {
        virtual void BaseClassFunctionShouldNotAppear()
            = SimpleInterface::InterfaceFunctionShouldNotAppear
        {
        }
    };

    public ref struct SimpleDerivedClass sealed
        : SimpleBaseClass
    {
        virtual void DerivedClassFunctionShouldAppear()
            = SimpleInterface::InterfaceFunctionShouldNotAppear
        {
        }
    };

    public interface struct HidingInterface
    {
        virtual void F() = 0;
    };

    public ref struct HidingBaseClass
        : Windows::UI::Xaml::DependencyObject,
          HidingInterface
    {
        virtual void G()
            = HidingInterface::F
        {
        }

        void H()
        {
        }
    };

    public ref struct HidingDerivedClass sealed
        : HidingBaseClass
    {
        virtual void H()
            = HidingInterface::F
        {
        }
    };

} } }
