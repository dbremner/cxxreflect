
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

using Windows::Foundation::Metadata::DefaultAttribute;

namespace TestComponents { namespace Alpha { namespace InterfaceDeclarer {

    public interface struct IF
    {
        void F0() = 0;
        void F1() = 0;
        void F2() = 0;
    };

    public interface struct IG
    {
        void G0() = 0;
    };

    public ref struct C sealed
        : IF, IG
    {
        virtual void F0() { }
        virtual void F1() { }
        virtual void F2() { }
        virtual void G0() { }
    };

} } }
