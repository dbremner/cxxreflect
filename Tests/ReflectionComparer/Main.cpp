//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is a C++/CLI program that loads an assembly into the reflection-only context and loads the
// same assembly using the CxxReflect library.  We can then do a direct comparison of the results
// returned by each of the APIs.

// TODO This is nowhere near complete.  It's also proven much more cumbersome than we expected.

#include "CxxReflect/CxxReflect.hpp"

#include <cliext/adapter>
#include <cliext/algorithm>
#include <cliext/vector>

// For convenience, map everything from CxxReflect into the C namespace, then map the corresponding
// types in the CLI Reflection API into the R namespace.

namespace C = CxxReflect;

namespace R
{
    typedef System::Reflection::Assembly      Assembly;
    typedef System::Reflection::FieldInfo     Field;
    typedef System::Reflection::MethodInfo    Method;
    typedef System::Reflection::ParameterInfo Parameter;
    typedef System::Reflection::PropertyInfo  Property;
    typedef System::Type                      Type;
}

namespace
{
    class StateStack;

    typedef std::function<System::String^()> StateCall;

    class StateFrame
    {
    public:

    private:

        std::function<System::String^()> _call;
    };

    class StatePopper
    {
    public:

        StatePopper(StateStack* state)
            : _state(state)
        {
        }

        ~StatePopper();

    private:

        StateStack* _state;
    };

    class StateStack
    {
    public:

        StatePopper Push(StateCall frameCall)
        {
            _stack.push_back(frameCall);
            _isSet = false;
        }

        void Pop()
        {
            _stack.pop_back();
            _isSet = false;
        }

    private:

        std::vector<StateCall> _stack;
        bool                   _isSet;
    };

    StatePopper::~StatePopper()
    {
        _state->Pop();
    }
}

namespace
{
    template <typename T>
    System::String^ AsSystemString(T t)               { return gcnew System::String(t.c_str()); }
    System::String^ AsSystemString(System::String^ t) { return t;                               }
    System::String^ AsSystemString(wchar_t const* t)  { return gcnew System::String(t);         }

    template <typename T, typename U>
    bool StringEquals(T t, U u, System::StringComparison mode = System::StringComparison::Ordinal)
    {
        System::String^ tString(AsSystemString(t));
        System::String^ uString(AsSystemString(u));

        return System::String::Equals(tString, uString, mode);
    }

    void Compare(StateStack&, R::Assembly^,  C::Assembly  const&);
    void Compare(StateStack&, R::Type^,      C::Type      const&);
    void Compare(StateStack&, R::Method^,    C::Method    const&);
    void Compare(StateStack&, R::Parameter^, C::Parameter const&);
    
    void Compare(StateStack& state, R::Assembly^ rAssembly, C::Assembly const& cAssembly)
    {
        auto const revert([&]{ return gcnew System::String(L""); });

        cliext::vector<R::Type^>  rTypes(rAssembly->GetTypes());
        C::Assembly::TypeSequence cTypes(cAssembly.GetTypes());

        cliext::sort(rTypes.begin(), rTypes.end(), [](R::Type^ lhs, R::Type^ rhs) -> bool
        {
            return lhs->MetadataToken < rhs->MetadataToken;
        });

        std::sort(cTypes.begin(), cTypes.end(), [](C::Type lhs, C::Type rhs) -> bool
        {
            return lhs.GetMetadataToken() < rhs.GetMetadataToken();
        });

        auto rIt(rTypes.begin());
        auto cIt(cTypes.begin());
        for (; rIt != rTypes.end() && cIt != cTypes.end(); ++rIt, ++cIt)
        {
            if (!StringEquals(rIt->Name, cIt->GetName()))
            {
                int x = 42;
            }
        }
    }
}

int main()
{
    wchar_t const* const assemblyPath(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");

    // Load the assembly using CxxReflect:
    C::Externals::Initialize<CxxReflect::Platform::Win32>();

    C::DirectoryBasedAssemblyLocator::DirectorySet directories;
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319");
    std::auto_ptr<C::IAssemblyLocator> resolver(new C::DirectoryBasedAssemblyLocator(directories));
    C::Loader loader(resolver);
    C::Assembly cAssembly(loader.LoadAssembly(assemblyPath));

    // Load the assembly using Reflection:
    R::Assembly^ rAssembly(R::Assembly::ReflectionOnlyLoadFrom(gcnew System::String(assemblyPath)));

    StateStack state;

    Compare(state, rAssembly, cAssembly);

    return 0;
}
