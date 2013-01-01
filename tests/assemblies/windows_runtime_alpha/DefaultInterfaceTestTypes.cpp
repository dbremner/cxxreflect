
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

using Windows::Foundation::Metadata::DefaultAttribute;

namespace TestComponents { namespace Alpha { namespace DefaultInterface {

    public interface struct IDefaultInterface
    {
        void F() = 0;
    };

    public interface struct IOtherInterface
    {
        void G() = 0;
    };

    public ref struct TestClass sealed
        : [Default] IDefaultInterface, IOtherInterface
    {
        virtual void F() { }
        virtual void G() { }
    };

    public enum struct TestEnum
    {
        Enumerator
    };

    public value struct TestStruct
    {
        int Value;
    };

} } }
