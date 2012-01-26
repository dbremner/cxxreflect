#include "CxxReflect/CxxReflect.hpp"

namespace WinRTTestStub
{
    public interface class IMakeStrings
    {
    public:

        virtual Platform::String^ MakeHelloWorldString() = 0;

        virtual Platform::String^ MakeNumberString(Platform::IBox<int>^ number) = 0;
    };

    public ref class StringMaker sealed : public IMakeStrings
    {
    public:

        virtual Platform::String^ MakeHelloWorldString() override
        {
            return L"Hello, World!";
        }

        virtual Platform::String^ MakeNumberString(Platform::IBox<int>^ number) override
        {
            return L"No Number For You!";
        }
    };
}

/*
template <typename TThis, typename TArg0>
HRESULT VirtualInvokeThunk(unsigned index, REFIID iid, TThis thisptr, TArg0 const arg0)
{
    return CxxReflect::Private::Invoker::VirtualAbiInvokeReferenceOnly(
        index,
        iid,
        reinterpret_cast<IUnknown*>(thisptr),
        reinterpret_cast<void*>(arg0));
}
*/

#include <oaidl.h>
#include <collection.h>

int main(Platform::Array<Platform::String^>^ arguments)
{
    CxxReflect::BeginWinRTPackageMetadataInitialization();
    try
    {
        auto package(Windows::ApplicationModel::Package::Current);
        auto dependencies(package->Dependencies);

        for (auto it(begin(dependencies)); it != end(dependencies); ++it)
        {
            auto px = (*it)->InstalledLocation->Path;
            auto py = (*it)->Id->FullName;
        }
    }
    catch (Platform::COMException^ ex)
    {
        Platform::String^ message = ex->Message;
    }

    using namespace CxxReflect;
    
    WinRTTestStub::StringMaker^ stringMaker(ref new WinRTTestStub::StringMaker());

    Type const type(WinRTPackageMetadata::GetTypeOf(stringMaker));
    StringReference const typeNamespace(type.GetNamespace());
    StringReference const typeName(type.GetName());

    CxxReflect::BindingFlags bindingFlags(BindingAttribute::Public | BindingAttribute::Instance);

    Method const method(type.GetMethod(L"MakeHelloWorldString", bindingFlags));

    std::vector<Method> const stringMakerMethods(type.BeginMethods(), type.EndMethods());
    
    return 0;
}
