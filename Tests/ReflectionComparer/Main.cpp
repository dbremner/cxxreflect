
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is a C++/CLI program that loads an assembly into the reflection-only context and loads the
// same assembly using the CxxReflect library.  We can then do a direct comparison of the results
// returned by each of the APIs.

// TODO This is nowhere near complete.

#include "CxxReflect/CxxReflect.hpp"

#include <cliext/adapter>
#include <cliext/algorithm>
#include <cliext/vector>

// For convenience, map everything from CxxReflect into the C namespace, then map the corresponding
// types in the CLR Reflection API into the R namespace.

namespace C = CxxReflect;

namespace R
{
    typedef System::Reflection::Assembly            Assembly;
    typedef System::Reflection::BindingFlags        BindingFlags;
    typedef System::Reflection::CustomAttributeData CustomAttribute;
    typedef System::Reflection::FieldInfo           Field;
    typedef System::Reflection::MethodInfo          Method;
    typedef System::Reflection::ParameterInfo       Parameter;
    typedef System::Reflection::PropertyInfo        Property;
    typedef System::Type                            Type;
}

namespace
{
    // TODO The CLR does weird things with many non-public entities.  E.g., it does not report them
    // in reflection, or it manipulates them so they appear differently.  (In all observed cases,
    // this has occurred in mscorlib.dll types, so it's not like user types are affected, except
    // that all types derive from System.Object.)  We need to find an effective and straightforward
    // way to verify private elements.
    R::BindingFlags RAllBindingFlags = 
        R::BindingFlags::Public |
        // R::BindingFlags::NonPublic |
        R::BindingFlags::Static |
        R::BindingFlags::Instance |
        R::BindingFlags::FlattenHierarchy;

    C::BindingAttribute const CAllBindingFlags =
        C::BindingAttribute::Public |
        // C::BindingAttribute::NonPublic |
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
            : _message(gcnew System::Text::StringBuilder()),
              _seenTypes(gcnew System::Collections::Generic::HashSet<R::Type^>()),
              _reportedFrames(0)
        {
        }

        StatePopper% Push(System::Object^ frame)
        {
            _stack.push_back(frame);

            return *gcnew StatePopper(*this);
        }

        void Pop()
        {
            _stack.pop_back();
            if (_reportedFrames > (unsigned)_stack.size())
                _reportedFrames = (unsigned)_stack.size();
        }

        void ReportDifference(System::String^ name, System::String^ expected, System::String^ actual)
        {
            System::String^ pad(WriteMissingFrameHeadersAndGetPad());

            _message->AppendLine(System::String::Format(L"{0} * Incorrect Value for [{1}]:", pad, name));

            _message->AppendLine(System::String::Format(L"{0}   Expected [{1}]", pad, expected));
            _message->AppendLine(System::String::Format(L"{0}   Actual   [{1}]", pad, actual));
        }

        void ReportMessage(System::String^ message)
        {
            System::String^ pad(WriteMissingFrameHeadersAndGetPad());

            _message->AppendLine(System::String::Format(L"{0} {1}", pad, message));
        }

        System::String^ GetMessages()
        {
            return _message->ToString();
        }

        bool ReportTypeAndReturnTrueIfKnown(R::Type^ type)
        {
            return !_seenTypes->Add(type);
        }

    private:

        System::String^ WriteMissingFrameHeadersAndGetPad()
        {
            if (_reportedFrames != (unsigned)_stack.size())
            {
                int depth(0 + 2 * _reportedFrames);
                for (auto it(_stack.begin() + _reportedFrames); it != _stack.end(); ++it)
                {
                    _message->AppendLine(System::String::Format(L"{0} * {1}",
                        gcnew System::String(L' ', depth),
                        AsString(*it)));

                    depth += 2;
                }

                _reportedFrames = _stack.size();
            }

            System::String^ pad(gcnew System::String(L' ', 2 * _stack.size()));

            return pad;
        }

        System::String^ AsString(System::Object^ o)
        {
            System::Type^ oType(o->GetType());
            if (R::Assembly::typeid->IsAssignableFrom(oType))
            {
                R::Assembly^ a(safe_cast<R::Assembly^>(o));
                return System::String::Format(L"Assembly [{0}] [{1}]", a->FullName, a->CodeBase);
            }
            else if (R::CustomAttribute::typeid->IsAssignableFrom(oType))
            {
                R::CustomAttribute^ c(safe_cast<R::CustomAttribute^>(o));
                return System::String::Format(L"Custom attribute [{0}]", c->Constructor->DeclaringType->FullName);
            }
            else if (R::Field::typeid->IsAssignableFrom(oType))
            {
                R::Field^ f(safe_cast<R::Field^>(o));
                return System::String::Format(L"Field [{0}] [${1:x8}]", f->Name, f->MetadataToken);
            }
            else if (R::Method::typeid->IsAssignableFrom(oType))
            {
                R::Method^ m(safe_cast<R::Method^>(o));
                return System::String::Format(L"Method [{0}] [${1:x8}]", m->Name, m->MetadataToken);
            }
            else if (R::Parameter::typeid->IsAssignableFrom(oType))
            {
                R::Parameter^ p(safe_cast<R::Parameter^>(o));
                return System::String::Format(L"Parameter [{0}]", p->Name);
            }
            else if (R::Type::typeid->IsAssignableFrom(oType))
            {
                R::Type^ t(safe_cast<R::Type^>(o));
                return System::String::Format(L"Type [{0}] [${1:x8}]", t->FullName, t->MetadataToken);
            }
            else if (System::String::typeid->IsAssignableFrom(oType))
            {
                return (System::String^)o;
            }
            else
            {
                return L"[UNKNOWN]";
            }
        }

        cliext::vector<System::Object^>               _stack;
        unsigned                                      _reportedFrames;
        System::Text::StringBuilder^                  _message;
        System::Collections::Generic::ISet<R::Type^>^ _seenTypes;
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
    
    std::uint32_t GetMetadataToken(C::CustomAttribute const& x) { return x.GetConstructor().GetMetadataToken(); }
    std::uint32_t GetMetadataToken(C::Field           const& x) { return x.GetMetadataToken();                  }
    std::uint32_t GetMetadataToken(C::Method          const& x) { return x.GetMetadataToken();                  }
    std::uint32_t GetMetadataToken(C::Parameter       const& x) { return x.GetMetadataToken();                  }
    std::uint32_t GetMetadataToken(C::Type            const& x) { return x.GetMetadataToken();                  }
    
    std::uint32_t GetMetadataToken(R::CustomAttribute^ x) { return x->Constructor->MetadataToken; }
    std::uint32_t GetMetadataToken(R::Field^           x) { return x->MetadataToken;              }
    std::uint32_t GetMetadataToken(R::Method^          x) { return x->MetadataToken;              }
    std::uint32_t GetMetadataToken(R::Parameter^       x) { return x->MetadataToken;              }
    std::uint32_t GetMetadataToken(R::Type^            x) { return x->MetadataToken;              }

    System::String^ GetBriefString(C::CustomAttribute const& x) { return gcnew System::String(x.GetConstructor().GetDeclaringType().GetAssemblyQualifiedName().c_str()); }
    System::String^ GetBriefString(C::Field           const& x) { return gcnew System::String(x.GetName().c_str());                                                      }
    System::String^ GetBriefString(C::Method          const& x) { return gcnew System::String(x.GetName().c_str());                                                      }
    System::String^ GetBriefString(C::Parameter       const& x) { return gcnew System::String(x.GetName().c_str());                                                      }
    System::String^ GetBriefString(C::Type            const& x) { return gcnew System::String(x.GetAssemblyQualifiedName().c_str());                                     }

    System::String^ GetBriefString(R::CustomAttribute^ x) { return x->Constructor->DeclaringType->AssemblyQualifiedName; }
    System::String^ GetBriefString(R::Field^           x) { return x->Name;                                              }
    System::String^ GetBriefString(R::Method^          x) { return x->Name;                                              }
    System::String^ GetBriefString(R::Parameter^       x) { return x->Name;                                              }
    System::String^ GetBriefString(R::Type^            x) { return x->AssemblyQualifiedName;                             }

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

        state.ReportDifference(name, AsSystemString(expected), AsSystemString(actual));
    }

    template <typename T, typename U>
    void VerifyIntegerEquals(StateStack% state, System::String^ name, T expected, U actual)
    {
        if ((unsigned)expected == (unsigned)actual)
            return;

        state.ReportDifference(
            name,
            System::String::Format("{0:x8}", (unsigned)expected),
            System::String::Format("{0:x8}", (unsigned)actual));
    }

    void VerifyBooleanEquals(StateStack% state, System::String^ name, bool expected, bool actual)
    {
        if (expected == actual)
            return;

        state.ReportDifference(name, System::String::Format("{0}", expected), System::String::Format("{0}", actual));
    }

    void Compare(StateStack%, R::Assembly^,        C::Assembly        const&);
    void Compare(StateStack%, R::CustomAttribute^, C::CustomAttribute const&);
    void Compare(StateStack%, R::Field^,           C::Field           const&);
    void Compare(StateStack%, R::Method^,          C::Method          const&);
    void Compare(StateStack%, R::Parameter^,       C::Parameter       const&);
    void Compare(StateStack%, R::Type^,            C::Type            const&);

    template <typename TRElement, typename TCElement>
    void CompareRanges(StateStack                % state,
                       System::String            ^ name,
                       cliext::vector<TRElement^>  rElements,
                       std::vector<TCElement>      cElements)
    {
        VerifyIntegerEquals(state, System::String::Format(L"{0} Count", name), rElements.size(), cElements.size());
        if ((unsigned)rElements.size() == cElements.size())
        {
            auto rIt(rElements.begin());
            auto cIt(cElements.begin());
            for (; rIt != rElements.end() && cIt != cElements.end(); ++rIt, ++cIt)
            {
                Compare(state, *rIt, *cIt);
            }
        }
        else
        {
            {
                auto frame(state.Push(System::String::Format(L"Expected {0}s", name)));
                for (auto it(rElements.begin()); it != rElements.end(); ++it)
                {
                    state.ReportMessage(GetBriefString(*it));
                }
            }
            {
                auto frame(state.Push(System::String::Format(L"Actual {0}s", name)));
                for (auto it(cElements.begin()); it != cElements.end(); ++it)
                {
                    state.ReportMessage(GetBriefString(*it));
                }
            }
        }
    }

    template <typename TRElement, typename TCElement>
    void CompareCustomAttributesOf(StateStack% state, TRElement^ rElement, TCElement const& cElement)
    {
        auto frame(state.Push(L"Custom Attributes"));
            
        cliext::vector<R::CustomAttribute^> rAttributes(rElement->GetCustomAttributesData());
        std::vector<C::CustomAttribute>     cAttributes(cElement.BeginCustomAttributes(), cElement.EndCustomAttributes());

        // TODO We do not correctly handle SerializableAttribute.  It isn't actually a custom
        // attribute, but the Reflection API reports it as if it is.  To determine whether a
        // type is serializable using CxxReflect, you can use the IsSerializable property.
        rAttributes.erase(cliext::remove_if(rAttributes.begin(), rAttributes.end(), [](R::CustomAttribute^ a)
        {
            return a->Constructor->DeclaringType->Name == L"SerializableAttribute";
        }), rAttributes.end());

        sort(rAttributes.begin(), rAttributes.end(), MetadataTokenStrictWeakOrdering());
        sort(cAttributes.begin(), cAttributes.end(), MetadataTokenStrictWeakOrdering());

        CompareRanges(state, L"Attribute", rAttributes, cAttributes);
    }
    
    void Compare(StateStack% state, R::Assembly^ rAssembly, C::Assembly const& cAssembly)
    {
        auto frame(state.Push(rAssembly));

        cliext::vector<R::Type^>  rTypes(rAssembly->GetTypes());
        C::Assembly::TypeSequence cTypes(cAssembly.GetTypes());

        sort(rTypes.begin(), rTypes.end(), MetadataTokenStrictWeakOrdering());
        sort(cTypes.begin(), cTypes.end(), MetadataTokenStrictWeakOrdering());

        auto rIt(rTypes.begin());
        auto cIt(cTypes.begin());
        for (; rIt != rTypes.end() && cIt != cTypes.end(); ++rIt, ++cIt)
        {   
            Compare(state, *rIt, *cIt);
        }
    }

    void Compare(StateStack% state, R::CustomAttribute^ rAttribute, C::CustomAttribute const& cAttribute)
    {
        auto frame(state.Push(rAttribute));
    }

    void Compare(StateStack% state, R::Field^ rField, C::Field const& cField)
    {
        auto frame(state.Push(rField));

        // TODO Support for generic fields
        if (rField->FieldType->IsGenericType)
            return;

        //
        // Properties
        //

        VerifyIntegerEquals(state, L"Attributes", rField->Attributes, cField.GetAttributes().GetIntegral());

        VerifyStringEquals(state, L"DeclaringType(Name)",
            rField->DeclaringType->AssemblyQualifiedName,
            cField.GetDeclaringType().GetAssemblyQualifiedName());

        {
            auto frame(state.Push(L"DeclaringType"));
            Compare(state, rField->DeclaringType, cField.GetDeclaringType());
        }

        // FieldHandle -- Not implemented in CxxReflect

        VerifyStringEquals(state, L"FieldType(Name)",
            rField->FieldType->AssemblyQualifiedName,
            cField.GetType().GetAssemblyQualifiedName());

        {
            auto frame(state.Push(L"FieldType"));
            Compare(state, rField->FieldType, cField.GetType());
        }

        VerifyBooleanEquals(state, L"IsAssembly",          rField->IsAssembly,          cField.IsAssembly());
        VerifyBooleanEquals(state, L"IsFamily",            rField->IsFamily,            cField.IsFamily());
        VerifyBooleanEquals(state, L"IsFamilyAndAssembly", rField->IsFamilyAndAssembly, cField.IsFamilyAndAssembly());
        VerifyBooleanEquals(state, L"IsFamilyOrAssembly",  rField->IsFamilyOrAssembly,  cField.IsFamilyOrAssembly());
        VerifyBooleanEquals(state, L"IsInitOnly",          rField->IsInitOnly,          cField.IsInitOnly());
        VerifyBooleanEquals(state, L"IsLiteral",           rField->IsLiteral,           cField.IsLiteral());
        VerifyBooleanEquals(state, L"IsNotSerialized",     rField->IsNotSerialized,     cField.IsNotSerialized());
        VerifyBooleanEquals(state, L"IsPinvokeImpl",       rField->IsPinvokeImpl,       cField.IsPinvokeImpl());
        VerifyBooleanEquals(state, L"IsPrivate",           rField->IsPrivate,           cField.IsPrivate());
        VerifyBooleanEquals(state, L"IsPublic",            rField->IsPublic,            cField.IsPublic());
        // IsSecurityCritical     -- Not implemented in CxxReflect
        // IsSecuritySafeCritical -- Not implemented in CxxReflect
        // IsSecurityTransparent  -- Not implemented in CxxReflect
        VerifyBooleanEquals(state, L"IsSpecialName",       rField->IsSpecialName,       cField.IsSpecialName());
        VerifyBooleanEquals(state, L"IsStatic",            rField->IsStatic,            cField.IsStatic());

        // MemberType -- Not implemented in CxxReflect

        VerifyIntegerEquals(state, L"MetadataToken", rField->MetadataToken, cField.GetMetadataToken());

        // TODO Module

        VerifyStringEquals(state, L"Name", rField->Name, cField.GetName());

        VerifyStringEquals(state, L"ReflectedType(Name)",
            rField->ReflectedType->AssemblyQualifiedName,
            cField.GetReflectedType().GetAssemblyQualifiedName());

        {
            auto frame(state.Push(L"ReflectedType"));
            Compare(state, rField->ReflectedType, cField.GetReflectedType());
        }

        //
        // Methods
        //

        // TODO GetCustomAttributes()
        // TODO GetOptionalCustomModifiers()
        // TODO GetRawConstantValue()
        // TODO GetRequiredCustomModifiers()
    }
    
    void Compare(StateStack% state, R::Method^ rMethod, C::Method const& cMethod)
    {
        auto frame(state.Push(rMethod));

        // TODO Support for generic methods
        if (rMethod->IsGenericMethod)
            return;

        //
        // Properties
        //

        VerifyIntegerEquals(state, L"Attributes", rMethod->Attributes, cMethod.GetAttributes().GetIntegral());
        // TODO VerifyIntegerEquals(state, L"CallingConvention", rMethod->CallingConvention, cMethod.GetCallingConvention());

        // TODO ContainsGenericParameters

        {
            auto frame(state.Push(L"DeclaringType"));
            Compare(state, rMethod->DeclaringType, cMethod.GetDeclaringType());
        }

        VerifyBooleanEquals(state, L"IsAbstract",                rMethod->IsAbstract,                cMethod.IsAbstract());
        VerifyBooleanEquals(state, L"IsAssembly",                rMethod->IsAssembly,                cMethod.IsAssembly());
        VerifyBooleanEquals(state, L"IsConstructor",             rMethod->IsConstructor,             cMethod.IsConstructor());
        VerifyBooleanEquals(state, L"IsFamily",                  rMethod->IsFamily,                  cMethod.IsFamily());
        VerifyBooleanEquals(state, L"IsFamilyAndAssembly",       rMethod->IsFamilyAndAssembly,       cMethod.IsFamilyAndAssembly());
        VerifyBooleanEquals(state, L"IsFamilyOrAssembly",        rMethod->IsFamilyOrAssembly,        cMethod.IsFamilyOrAssembly());
        VerifyBooleanEquals(state, L"IsFinal",                   rMethod->IsFinal,                   cMethod.IsFinal());
        VerifyBooleanEquals(state, L"IsGenericMethod",           rMethod->IsGenericMethod,           cMethod.IsGenericMethod());
        VerifyBooleanEquals(state, L"IsGenericMethodDefinition", rMethod->IsGenericMethodDefinition, cMethod.IsGenericMethodDefinition());
        VerifyBooleanEquals(state, L"IsHideBySig",               rMethod->IsHideBySig,               cMethod.IsHideBySig());
        VerifyBooleanEquals(state, L"IsPrivate",                 rMethod->IsPrivate,                 cMethod.IsPrivate());
        VerifyBooleanEquals(state, L"IsPublic",                  rMethod->IsPublic,                  cMethod.IsPublic());
        // IsSecurityCritical     -- Not implemented in CxxReflect
        // IsSecuritySafeCritical -- Not implemented in CxxReflect
        // IsSecurityTransparent  -- Not implemented in CxxReflect
        VerifyBooleanEquals(state, L"IsSpecialName",             rMethod->IsSpecialName,             cMethod.IsSpecialName());
        VerifyBooleanEquals(state, L"IsStatic",                  rMethod->IsStatic,                  cMethod.IsStatic());
        VerifyBooleanEquals(state, L"IsVirtual",                 rMethod->IsVirtual,                 cMethod.IsVirtual());

        // MemberType -- Not implemented in CxxReflect

        VerifyIntegerEquals(state, L"MetadataToken", rMethod->MetadataToken, cMethod.GetMetadataToken());

        // TODO Module

        VerifyStringEquals(state, L"Name", rMethod->Name, cMethod.GetName());

        {
            auto frame(state.Push(L"ReflectedType"));
            Compare(state, rMethod->ReflectedType, cMethod.GetReflectedType());
        }

        // TODO if (rMethod->ReturnParameter != nullptr)
        // {
        //     auto frame(state.Push(L"ReturnParameter"));
        //     Compare(state, rMethod->ReturnParameter, cMethod.GetReturnParameter());
        // }

        // TODO {
        //     auto frame(state.Push(L"ReturnType"));
        //     Compare(state, rMethod->ReturnType, cMethod.GetReturnType());
        // }

        // TODO ReturnTypeCustomAttributes

        // TODO GetBaseDefinition()
        // TODO GetCustomAttributes()
        // TODO GetGenericArguments()
        // TODO GetGenericMethodDefinition()
        // GetMethodBody() -- NotImplemented in CxxReflect
        // TODO GetMethodImplementationFlags()

        cliext::vector<R::Parameter^> rParameters(rMethod->GetParameters());
        std::vector<C::Parameter>     cParameters(cMethod.BeginParameters(), cMethod.EndParameters());

        sort(rParameters.begin(), rParameters.end(), MetadataTokenStrictWeakOrdering());
        sort(cParameters.begin(), cParameters.end(), MetadataTokenStrictWeakOrdering());

        CompareRanges(state, L"Parameter", rParameters, cParameters);
    }

    void Compare(StateStack% state, R::Parameter^ rParameter, C::Parameter const& cParameter)
    {
        auto frame(state.Push(rParameter));

        //
        // Properties
        //

        VerifyIntegerEquals(state, L"Attributes", rParameter->Attributes, cParameter.GetAttributes().GetIntegral());

        // TODO DefaultValue

        VerifyBooleanEquals(state, L"IsIn",       rParameter->IsIn,       cParameter.IsIn());
        // TODO VerifyBooleanEquals(state, L"IsLcid",     rParameter->IsLcid,     cParameter.IsLcid());
        VerifyBooleanEquals(state, L"IsOptional", rParameter->IsOptional, cParameter.IsOptional());
        VerifyBooleanEquals(state, L"IsOut",      rParameter->IsOut,      cParameter.IsOut());
        // TODO VerifyBooleanEquals(state, L"IsRetval",   rParameter->IsRetval,   cParameter.IsRetVal());

        // TODO Member

        VerifyIntegerEquals(state, L"MetadataToken", rParameter->MetadataToken, cParameter.GetMetadataToken());

        VerifyStringEquals(state, L"Name", rParameter->Name, cParameter.GetName());
        
        
        if (rParameter->ParameterType->HasElementType)
        {
            Compare(state, rParameter->ParameterType, cParameter.GetType());
        }
        else
        {
            VerifyStringEquals(state, L"ParameterType(Name)",
                rParameter->ParameterType->AssemblyQualifiedName, 
                cParameter.GetType().GetAssemblyQualifiedName());
        }

        VerifyIntegerEquals(state, L"Position", rParameter->Position, cParameter.GetPosition());

        // TODO RawDefaultValue

        //
        // Methods
        //

        // TODO GetCustomAttributes()
        // CompareCustomAttributesOf(state, rParameter, cParameter); // GetCustomAttributes and friends

        // TODO GetOptionalCustomModifiers()
        // TODO GetRequiredCustomModifiers()
    }

    void Compare(StateStack% state, R::Type^ rType, C::Type const& cType)
    {
        // Prevent infinite recursion by ensuring we only visit each type once (this also prevents
        // us from doing more work than we need to do, since types are immutable, we only need to
        // compare them once).
        if (state.ReportTypeAndReturnTrueIfKnown(rType))
            return;

        // TODO Support for generic types
        if (rType->IsGenericType)
            return;

        auto frame(state.Push(rType));

        // TODO Assembly
        VerifyStringEquals(state, L"AssemblyQualifiedName", rType->AssemblyQualifiedName, cType.GetAssemblyQualifiedName());
        VerifyIntegerEquals(state, L"Attributes", rType->Attributes, cType.GetAttributes().GetIntegral());

        {
            auto frame(state.Push(L"Base Type"));
            if (rType->BaseType != nullptr)
            {
                Compare(state, rType->BaseType, cType.GetBaseType());
            }
        }

        // TODO VerifyBooleanEquals(state, L"ContainsGenericParameters", rType->ContainsGenericParameters, cType.ContainsGenericParameters());
        
        CompareCustomAttributesOf(state, rType, cType); // GetCustomAttributes() and friends

        // TODO DeclaringMethods
        VerifyStringEquals(state, L"FullName", rType->FullName, cType.GetFullName());
        // TODO GenericParameterAttributes
        // TODO GenericParameterPosition
        // TODO GenericTypeArguments
        // TODO GetArrayRank
        // TODO GetConstructor
        // TODO GetConstructors
        // TODO GetDefaultMembers
        // TODO GetElementType
        // TODO GetEnumName
        // TODO GetEnumNames
        // TODO GetEnumUnderlyingType
        // TODO GetEnumValues
        // TODO GetEvent
        // TODO GetEvents
        // TODO GetField
        // TODO GetFields
        // TODO GetGenericArguments
        // TODO GetGenericParameterConstraints
        // TODO GetGenericTypeDefinition
        // TODO GetInterface
        
        cliext::vector<R::Type^> rInterfaces(rType->GetInterfaces());
        std::vector<C::Type>     cInterfaces(cType.BeginInterfaces(), cType.EndInterfaces());

        sort(rInterfaces.begin(), rInterfaces.end(), MetadataTokenStrictWeakOrdering());
        sort(cInterfaces.begin(), cInterfaces.end(), MetadataTokenStrictWeakOrdering());

        CompareRanges(state, L"Interfaces", rInterfaces, cInterfaces);

        // TODO GetMember
        // TODO GetMembers
        // TODO GetMethod
        // TODO GetMethods

        cliext::vector<R::Method^> rMethods(rType->GetMethods(RAllBindingFlags));
        std::vector<C::Method>     cMethods(cType.BeginMethods(CAllBindingFlags), cType.EndMethods());

        sort(rMethods.begin(), rMethods.end(), MetadataTokenStrictWeakOrdering());
        sort(cMethods.begin(), cMethods.end(), MetadataTokenStrictWeakOrdering());

        CompareRanges(state, L"Methods", rMethods, cMethods);

        // TODO GetNestedType
        // TODO GetNestedTypes
        // TODO GetProperties
        // TODO GetProperty
        // TODO GUID
        // TODO HasElementType

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
        //        IsSecurityCritical     -- Not implemented in CxxReflect
        //        IsSecuritySafeCritical -- Not implemented in CxxReflect
        //        IsSecurityTransparent  -- Not implemented in CxxReflect
        VERIFY_IS(IsSerializable,          IsSerializable           );
        VERIFY_IS(IsSpecialName,           IsSpecialName            );
        VERIFY_IS(IsUnicodeClass,          IsUnicodeClass           );
        VERIFY_IS(IsValueType,             IsValueType              );
        VERIFY_IS(IsVisible,               IsVisible                );

        #undef VERIFY_IS

        // MemberType -- Not implemented in CxxReflect
        // TODO Module

        VerifyStringEquals(state, L"Name", rType->Name, cType.GetName());
        VerifyStringEquals(state, L"Namespace", rType->Namespace, cType.GetNamespace());

        // TODO ReflectedType
        // TODO StructLayoutAttribute
        // TODO TypeHandle
        // TODO TypeInitializer
    }

    
}

int main()
{
    wchar_t const* const assemblyPath(L"C:\\jm\\CxxReflect\\Build\\Output\\Win32\\Debug\\TestAssemblies\\A0.dat");

    C::Externals::Initialize<CxxReflect::Platform::Win32>();

    // Load the assembly using CxxReflect:
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
