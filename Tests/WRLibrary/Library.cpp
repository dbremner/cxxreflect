
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

namespace WRLibrary
{
    public interface class IProvideANumber
    {
        default::int32 GetNumber();
    };

    public ref class ProviderOfZero sealed : IProvideANumber
    {
    public:

        default::int32 GetNumber() { return 0; }
    };

    public ref class ProviderOfOne sealed : IProvideANumber
    {
    public:

        default::int32 GetNumber() { return 1; }
    };

    public ref class ProviderOfTheAnswer sealed : IProvideANumber
    {
    public:

        default::int32 GetNumber() { return 42; }
    };

    public ref class UserProvidedNumber sealed : IProvideANumber
    {
    public:

        UserProvidedNumber(default::int32 value)
            : _value(value)
        {
        }

        default::int32 GetNumber() { return _value; }

    private:

        default::int32 _value;
    };
}
