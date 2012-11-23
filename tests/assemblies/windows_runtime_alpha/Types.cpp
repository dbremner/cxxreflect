
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

namespace TestComponents { namespace Alpha {

    public enum class DayOfWeek
    {
        Sunday    = 0,
        Monday    = 1,
        Tuesday   = 2,
        Wednesday = 3,
        Thursday  = 4,
        Friday    = 5,
        Saturday  = 6
    };

    public interface struct SimpleMethodImplTestInterface
    {
        virtual void InterfaceFunctionShouldNotAppear() = 0;
    };

    public ref struct SimpleMethodImplTestBaseClass
        : Windows::UI::Xaml::DependencyObject,
          SimpleMethodImplTestInterface
    {
        virtual void BaseClassFunctionShouldNotAppear()
            = SimpleMethodImplTestInterface::InterfaceFunctionShouldNotAppear
        {
        }
    };

    public ref struct SimpleMethodImplTestDerivedClass sealed
        : SimpleMethodImplTestBaseClass
    {
        virtual void DerivedClassFunctionShouldAppear()
            = SimpleMethodImplTestInterface::InterfaceFunctionShouldNotAppear
        {
        }
    };

    public interface struct HidingMethodImplTestInterface
    {
        virtual void F() = 0;
    };

    public ref struct HidingMethodImplTestBaseClass
        : Windows::UI::Xaml::DependencyObject,
          HidingMethodImplTestInterface
    {
        virtual void G()
            = HidingMethodImplTestInterface::F
        {
        }

        void H()
        {
        }
    };

    public ref struct HidingMethodImplTestDerivedClass sealed
        : HidingMethodImplTestBaseClass
    {
        virtual void H()
            = HidingMethodImplTestInterface::F
        {
        }
    };

} }
