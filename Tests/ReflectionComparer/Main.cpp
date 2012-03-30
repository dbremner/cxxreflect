//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is a C++/CLI program that loads an assembly into the reflection-only context and loads the
// same assembly using the CxxReflect library.  We can then do a direct comparison of the results
// returned by each of the APIs.  Currently we do a fully-recursive comparison, which is time
// consuming and expensive, but also gives full coverage of the APIs.

// TODO This is nowhere near complete.

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
    typedef System::Reflection::BindingFlags  BindingFlags;
    typedef System::Reflection::FieldInfo     Field;
    typedef System::Reflection::MethodInfo    Method;
    typedef System::Reflection::ParameterInfo Parameter;
    typedef System::Reflection::PropertyInfo  Property;
    typedef System::Type                      Type;
}

namespace
{
    R::BindingFlags RAllBindingFlags = 
        R::BindingFlags::Public |
        R::BindingFlags::NonPublic |
        R::BindingFlags::Static |
        R::BindingFlags::Instance |
        R::BindingFlags::FlattenHierarchy;

    C::BindingAttribute const CAllBindingFlags =
        C::BindingAttribute::Public |
        C::BindingAttribute::NonPublic |
        C::BindingAttribute::Static |
        C::BindingAttribute::Instance |
        C::BindingAttribute::FlattenHierarchy;

    ref class StateStack;

    ref class StatePopper
    {
    public:

        explicit StatePopper(StateStack% state)
            : _state(%state)
        {
        }

        // This is an auto_ptr-like resource-stealing copy.  It's evil, yes, but C++/CLI doesn't
        // support move semantics (yet?), and the way we use this class, it should be safe.
        StatePopper(StatePopper% other)
            : _state(other._state)
        {
            other._state = nullptr;
        }

        ~StatePopper();

    private:

        StateStack^ _state;
    };

    ref class StateStack
    {
    public:

        StateStack()
            : _message(gcnew System::Text::StringBuilder())
        {
        }

        StatePopper% Push(System::Object^ frame)
        {
            _stack.push_back(frame);
            _isSet = false;

            return *gcnew StatePopper(*this);
        }

        void Pop()
        {
            _stack.pop_back();
            _isSet = false;
        }

        void Report(System::String^ name, System::String^ expected, System::String^ actual)
        {
            if (!_isSet)
            {
                int depth(0);
                for (auto it(_stack.begin()); it != _stack.end(); ++it)
                {
                    _message->AppendLine(System::String::Format(L"{0} * {1}",
                        gcnew System::String(L' ', depth),
                        AsString(*it)));

                    depth += 2;
                }

                _isSet = true;
            }

            System::String^ pad(gcnew System::String(L' ', 2 * _stack.size()));

            _message->AppendLine(System::String::Format(L"{0} * Incorrect Value for [{1}]:", pad, name));

            _message->AppendLine(System::String::Format(L"{0}   Expected [{1}]", pad, expected));
            _message->AppendLine(System::String::Format(L"{0}   Actual   [{1}]", pad, actual));
        }

        System::String^ GetMessages()
        {
            return _message->ToString();
        }

    private:

        System::String^ AsString(System::Object^ o)
        {
            System::Type^ oType(o->GetType());
            if (R::Assembly::typeid->IsAssignableFrom(oType))
            {
                R::Assembly^ a(safe_cast<R::Assembly^>(o));
                return System::String::Format(L"Assembly [{0}] [{1}]", a->FullName, a->CodeBase);
            }
            else if (R::Type::typeid->IsAssignableFrom(oType))
            {
                R::Type^ t(safe_cast<R::Type^>(o));
                return System::String::Format(L"Type [{0}] [${1:x8}]", t->FullName, t->MetadataToken);
            }
            else
            {
                return L"[UNKNOWN]";
            }

            // TODO
        }

        cliext::vector<System::Object^> _stack;
        bool                            _isSet;
        System::Text::StringBuilder^    _message;
    };

    StatePopper::~StatePopper()
    {
        if (_state != nullptr)
            _state->Pop();
    }
}

namespace
{
    template <typename T>
    System::String^ AsSystemString(T const& t)        { return gcnew System::String(t.c_str()); }
    System::String^ AsSystemString(System::String^ t) { return t == nullptr ? L"" : t;          }
    System::String^ AsSystemString(wchar_t const* t)  { return gcnew System::String(t);         }

    template <typename T, typename U>
    bool StringEquals(T t, U u, System::StringComparison mode = System::StringComparison::Ordinal)
    {
        System::String^ tString(AsSystemString(t));
        System::String^ uString(AsSystemString(u));

        return System::String::Equals(tString, uString, mode);
    }

    std::uint32_t GetMetadataToken(C::Method const& x) { return x.GetMetadataToken(); }
    std::uint32_t GetMetadataToken(C::Type   const& x) { return x.GetMetadataToken(); }
    
    std::uint32_t GetMetadataToken(R::Method^       x) { return x->MetadataToken;     }
    std::uint32_t GetMetadataToken(R::Type^         x) { return x->MetadataToken;     }

    class MetadataTokenStrictWeakOrdering
    {
    public:

        template <typename T>
        bool operator()(T const& lhs, T const& rhs)
        {
            return GetMetadataToken(lhs) < GetMetadataToken(rhs);
        }

        template <typename T>
        bool operator()(T^ lhs, T^ rhs)
        {
            return GetMetadataToken(lhs) < GetMetadataToken(rhs);
        }
    };

    template <typename TExpected, typename TActual>
    void VerifyStringEquals(StateStack% state, System::String^ name, TExpected expected, TActual actual)
    {
        if (StringEquals(expected, actual))
            return;

        state.Report(name, AsSystemString(expected), AsSystemString(actual));
    }

    template <typename T, typename U>
    void VerifyIntegerEquals(StateStack% state, System::String^ name, T expected, U actual)
    {
        if ((unsigned)expected == (unsigned)actual)
            return;

        state.Report(
            name,
            System::String::Format("{0:x8}", (unsigned)expected),
            System::String::Format("{0:x8}", (unsigned)actual));
    }

    void VerifyBooleanEquals(StateStack% state, System::String^ name, bool expected, bool actual)
    {
        if (expected == actual)
            return;

        state.Report(name, System::String::Format("{0}", expected), System::String::Format("{0}", actual));
    }

    void Compare(StateStack%, R::Assembly^,  C::Assembly  const&);
    void Compare(StateStack%, R::Type^,      C::Type      const&);
    void Compare(StateStack%, R::Method^,    C::Method    const&);
    void Compare(StateStack%, R::Parameter^, C::Parameter const&);
    
    void Compare(StateStack% state, R::Assembly^ rAssembly, C::Assembly const& cAssembly)
    {
        auto frame(state.Push(rAssembly));

        cliext::vector<R::Type^>  rTypes(rAssembly->GetTypes());
        C::Assembly::TypeSequence cTypes(cAssembly.GetTypes());

        cliext::sort(rTypes.begin(), rTypes.end(), MetadataTokenStrictWeakOrdering());
        std::sort(cTypes.begin(), cTypes.end(), MetadataTokenStrictWeakOrdering());

        auto rIt(rTypes.begin());
        auto cIt(cTypes.begin());
        for (; rIt != rTypes.end() && cIt != cTypes.end(); ++rIt, ++cIt)
        {   
            Compare(state, *rIt, *cIt);
        }
    }

    void Compare(StateStack% state, R::Type^ rType, C::Type const& cType)
    {
        auto frame(state.Push(rType));

        // Assembly
        VerifyStringEquals(state, L"AssemblyQualifiedName", rType->AssemblyQualifiedName, cType.GetAssemblyQualifiedName());
        VerifyIntegerEquals(state, L"Attributes", rType->Attributes, cType.GetAttributes().GetIntegral());

        {
            auto frame(state.Push(L"Base Type"));
            if (rType->BaseType != nullptr)
            {
                Compare(state, rType->BaseType, cType.GetBaseType());
            }
        }
        // BaseType

        // VerifyBooleanEquals(state, L"ContainsGenericParameters", rType->ContainsGenericParameters, cType.ContainsGenericParameters());
        // CustomAttributes
        // DeclaringMethods
        VerifyStringEquals(state, L"FullName", rType->FullName, cType.GetFullName());
        // GenericParameterAttributes
        // GenericParameterPosition
        // GenericTypeArguments
        // GetArrayRank
        // GetConstructor
        // GetConstructors
        // GetDefaultMembers
        // GetElementType
        // GetEnumName
        // GetEnumNames
        // GetEnumUnderlyingType
        // GetEnumValues
        // GetEvent
        // GetEvents
        // GetField
        // GetFields
        // GetGenericArguments
        // GetGenericParameterConstraints
        // GetGenericTypeDefinition
        // GetInterface
        
        cliext::vector<R::Type^> rInterfaces(rType->GetInterfaces());
        std::vector<C::Type>     cInterfaces(cType.BeginInterfaces(), cType.EndInterfaces());

        cliext::sort(rInterfaces.begin(), rInterfaces.end(), MetadataTokenStrictWeakOrdering());
        std::sort(cInterfaces.begin(), cInterfaces.end(), MetadataTokenStrictWeakOrdering());

        VerifyIntegerEquals(state, L"Interface Count", rInterfaces.size(), cInterfaces.size());
        auto rInterfaceIt(rInterfaces.begin());
        auto cInterfaceIt(cInterfaces.begin());
        for (; rInterfaceIt != rInterfaces.end() && cInterfaceIt != cInterfaces.end(); ++rInterfaceIt, ++cInterfaceIt)
        {
            VerifyStringEquals(state, L"Interface Name", (*rInterfaceIt)->FullName, cInterfaceIt->GetFullName());
        }

        // GetMember
        // GetMembers
        // GetMethod
        // GetMethods

        cliext::vector<R::Method^> rMethods(rType->GetMethods(RAllBindingFlags));
        std::vector<C::Method>     cMethods(cType.BeginMethods(CAllBindingFlags), cType.EndMethods());

        cliext::sort(rMethods.begin(), rMethods.end(), MetadataTokenStrictWeakOrdering());
        std::sort(cMethods.begin(), cMethods.end(), MetadataTokenStrictWeakOrdering());

        VerifyIntegerEquals(state, L"Method Count", rMethods.size(), cMethods.size());
        auto rMethodIt(rMethods.begin());
        auto cMethodIt(cMethods.begin());
        for (; rMethodIt != rMethods.end() && cMethodIt != cMethods.end(); ++rMethodIt, ++cMethodIt)
        {
            // TODO DO METHOD COMPARISON
            VerifyStringEquals(state, L"Method Name", (*rMethodIt)->Name, cMethodIt->GetName());
        }

        // GetNestedType
        // GetNestedTypes
        // GetProperties
        // GetProperty
        // GUID
        // HasElementType

        #define VERIFY_IS(r, c) VerifyBooleanEquals(state, # r, rType->r, cType.c());

        VERIFY_IS(IsAbstract,              IsAbstract               );
        VERIFY_IS(IsAnsiClass,             IsAnsiClass              );
        VERIFY_IS(IsArray,                 IsArray                  );
        VERIFY_IS(IsAutoClass,             IsAutoClass              );
        VERIFY_IS(IsAutoLayout,            IsAutoLayout             );
        VERIFY_IS(IsByRef,                 IsByRef                  );
        VERIFY_IS(IsClass,                 IsClass                  );
        VERIFY_IS(IsCOMObject,             IsComObject              );
        VERIFY_IS(IsContextful,            IsContextful             );
        VERIFY_IS(IsEnum,                  IsEnum                   );
        VERIFY_IS(IsExplicitLayout,        IsExplicitLayout         );
        VERIFY_IS(IsGenericParameter,      IsGenericParameter       );
        VERIFY_IS(IsGenericType,           IsGenericType            );
        VERIFY_IS(IsGenericTypeDefinition, IsGenericTypeDefinition  );
        VERIFY_IS(IsImport,                IsImport                 );
        VERIFY_IS(IsInterface,             IsInterface              );
        VERIFY_IS(IsLayoutSequential,      IsLayoutSequential       );
        VERIFY_IS(IsMarshalByRef,          IsMarshalByRef           );
        VERIFY_IS(IsNested,                IsNested                 );
        VERIFY_IS(IsNestedAssembly,        IsNestedAssembly         );
        VERIFY_IS(IsNestedFamANDAssem,     IsNestedFamilyAndAssembly);
        VERIFY_IS(IsNestedFamily,          IsNestedFamily           );
        VERIFY_IS(IsNestedFamORAssem,      IsNestedFamilyOrAssembly );
        VERIFY_IS(IsNestedPrivate,         IsNestedPrivate          );
        VERIFY_IS(IsNestedPublic,          IsNestedPublic           );
        VERIFY_IS(IsNotPublic,             IsNotPublic              );
        VERIFY_IS(IsPointer,               IsPointer                );
        VERIFY_IS(IsPrimitive,             IsPrimitive              );
        VERIFY_IS(IsPublic,                IsPublic                 );
        VERIFY_IS(IsSealed,                IsSealed                 );
        //        IsSecurityCritical
        //        IsSecuritySafeCritical
        //        IsSecurityTransparent
        VERIFY_IS(IsSerializable,          IsSerializable           );
        VERIFY_IS(IsSpecialName,           IsSpecialName            );
        VERIFY_IS(IsUnicodeClass,          IsUnicodeClass           );
        VERIFY_IS(IsValueType,             IsValueType              );
        VERIFY_IS(IsVisible,               IsVisible                );

        #undef VERIFY_IS

        // MemberType
        // Module

        VerifyStringEquals(state, L"Name", rType->Name, cType.GetName());
        VerifyStringEquals(state, L"Namespace", rType->Namespace, cType.GetNamespace());

        // ReflectedType
        // StructLayoutAttribute
        // TypeHandle
        // TypeInitializer
    }
}

int main()
{
    wchar_t const* const assemblyPath(L"C:\\jm\\CxxReflect\\Build\\Output\\Win32\\Debug\\TestAssemblies\\A0.dat");

    // Load the assembly using CxxReflect:
    C::Externals::Initialize<CxxReflect::Platform::Win32>();

    C::DirectoryBasedAssemblyLocator::DirectorySet directories;
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319");
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\wpf");
    std::auto_ptr<C::IAssemblyLocator> resolver(new C::DirectoryBasedAssemblyLocator(directories));
    C::Loader loader(resolver);
    C::Assembly cAssembly(loader.LoadAssembly(assemblyPath));

    // Load the assembly using Reflection:
    R::Assembly^ rAssembly(R::Assembly::LoadFrom(gcnew System::String(assemblyPath)));

    StateStack state;

    Compare(state, rAssembly, cAssembly);

    System::IO::StreamWriter^ resultFile(gcnew System::IO::StreamWriter(L"c:\\jm\\reflectresult.txt"));
    resultFile->Write(state.GetMessages());
    resultFile->Close();

    return 0;
}
