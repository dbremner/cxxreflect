
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

namespace TestComponents { namespace Alpha {

    public interface class IProvideANumber
    {
        auto GetNumber() -> default::int32;
    };
    
    public ref class ProviderOfZero sealed : IProvideANumber
    {
    public:

        virtual auto GetNumber() -> default::int32 { return 0; }
    };
    
    public ref class ProviderOfOne sealed : IProvideANumber
    {
    public:

        virtual auto GetNumber() -> default::int32 { return 1; }
    };

    public ref class ProviderOfTheAnswer sealed : IProvideANumber
    {
    public:

        virtual auto GetNumber() -> default::int32 { return 42; }
    };

    public ref class UserProvidedNumber sealed : IProvideANumber
    {
    public:

        UserProvidedNumber(default::int32 value)
            : _value(value)
        {
        }

        virtual auto GetNumber() ->default::int32 { return _value; }

    private:

        default::int32 _value;
    };
    
    public ref class UserProvidedMultipliedNumber sealed : public IProvideANumber
    {
    public:

        UserProvidedMultipliedNumber(default::int32 x, default::int32 y)
            : _value(x * 10 + y)
        {
        }

        virtual auto GetNumber() -> default::int32 { return _value; }

    private:

        default::int32 _value;
    };
    
} }

// AMDG //
