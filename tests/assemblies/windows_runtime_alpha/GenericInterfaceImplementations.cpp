
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

using Platform::Object;
using Platform::WriteOnlyArray;
using Windows::Foundation::Collections::IIterable;
using Windows::Foundation::Collections::IIterator;
using Windows::Foundation::Collections::IKeyValuePair;
using Windows::Foundation::Collections::IVectorView;

namespace TestComponents { namespace Alpha { namespace GenericInterfaceImplementations {

    public ref struct IterableObject sealed
        : IIterable<Object^>
    {
        virtual auto First() -> IIterator<Object^>^ { return nullptr; }
    };

    public ref struct VectorViewObject sealed
        : IVectorView<Object^>
    {
        property unsigned Size
        {
            virtual auto get() -> unsigned { return 0; }
        }

        virtual auto First() -> IIterator<Object^>^ { return nullptr; }
        virtual auto GetAt(unsigned) -> Object^ { return nullptr; }
        virtual unsigned GetMany(unsigned, WriteOnlyArray<Object^>^) { return 0; }
        virtual auto IndexOf(Object^, unsigned* index) -> bool { *index = 0; return false; }
    };

    public interface struct IIterablePair
        : IIterable<IKeyValuePair<Object^, VectorViewObject^>^>
    {
    };

} } }
