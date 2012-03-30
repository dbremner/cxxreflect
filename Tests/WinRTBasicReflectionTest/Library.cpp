
namespace WinRTBasicReflectionTest
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
}
