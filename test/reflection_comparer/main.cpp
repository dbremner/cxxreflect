
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //





// This is a C++/CLI program that loads an assembly into the reflection-only context and loads the
// same assembly using the CxxReflect library.  We can then do a direct comparison of the results
// returned by each of the APIs.

// TODO This is nowhere near complete.





#include "cxxreflect/cxxreflect.hpp"

#include <cliext/adapter>
#include <cliext/algorithm>
#include <cliext/vector>

// For convenience, map everything from CxxReflect into the C namespace, then map the corresponding
// types in the CLR Reflection API into the R namespace.

namespace C
{
    using namespace cxxreflect::core;
    using namespace cxxreflect::metadata;
    using namespace cxxreflect::reflection;
}

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
    R::BindingFlags r_all_bindings = 
        R::BindingFlags::Public |
        R::BindingFlags::NonPublic |
        R::BindingFlags::Static |
        R::BindingFlags::Instance |
        R::BindingFlags::FlattenHierarchy;

    C::binding_attribute const c_all_bindings =
        C::binding_attribute::public_ |
        C::binding_attribute::non_public |
        C::binding_attribute::static_ |
        C::binding_attribute::instance |
        C::binding_attribute::flatten_hierarchy;

    ref class state_stack;

    ref class state_popper
    {
    public:

        explicit state_popper(state_stack% state)
            : _state(%state)
        {
        }

        // This is an auto_ptr-like resource-stealing copy.  It's evil, yes, but C++/CLI doesn't
        // support move semantics (yet?), and the way we use this class, it should be safe.
        state_popper(state_popper% other)
            : _state(other._state)
        {
            other._state = nullptr;
        }

        ~state_popper();

    private:

        state_stack^ _state;
    };

    ref class state_stack
    {
    public:

        state_stack()
            : _message(gcnew System::Text::StringBuilder()),
              _seen_types(gcnew System::Collections::Generic::HashSet<R::Type^>()),
              _reported_frames(0)
        {
        }

        auto push(System::Object^ frame) -> state_popper%
        {
            _stack.push_back(frame);

            return *gcnew state_popper(*this);
        }

        auto pop() -> void
        {
            _stack.pop_back();
            if (_reported_frames > (unsigned)_stack.size())
                _reported_frames = (unsigned)_stack.size();
        }

        auto report_difference(System::String^ name, System::String^ expected, System::String^ actual) -> void
        {
            System::String^ pad(write_missing_frame_headers_and_get_pad());

            _message->AppendLine(System::String::Format(L"{0} * Incorrect Value for [{1}]:", pad, name));

            _message->AppendLine(System::String::Format(L"{0}   Expected [{1}]", pad, expected));
            _message->AppendLine(System::String::Format(L"{0}   Actual   [{1}]", pad, actual));
        }

        auto report_message(System::String^ message) -> void
        {
            System::String^ pad(write_missing_frame_headers_and_get_pad());

            _message->AppendLine(System::String::Format(L"{0} {1}", pad, message));
        }

        auto messages() -> System::String^
        {
            return _message->ToString();
        }

        bool report_type_and_return_true_if_known(R::Type^ type)
        {
            return !_seen_types->Add(type);
        }

    private:

        auto write_missing_frame_headers_and_get_pad() -> System::String^
        {
            if (_reported_frames != (unsigned)_stack.size())
            {
                int depth(0 + 2 * _reported_frames);
                for (auto it(_stack.begin() + _reported_frames); it != _stack.end(); ++it)
                {
                    _message->AppendLine(System::String::Format(L"{0} * {1}",
                        gcnew System::String(L' ', depth),
                        as_string(*it)));

                    depth += 2;
                }

                _reported_frames = _stack.size();
            }

            System::String^ pad(gcnew System::String(L' ', 2 * _stack.size()));

            return pad;
        }

        auto as_string(System::Object^ o) -> System::String^
        {
            System::Type^ o_type(o->GetType());
            if (R::Assembly::typeid->IsAssignableFrom(o_type))
            {
                R::Assembly^ a(safe_cast<R::Assembly^>(o));
                return System::String::Format(L"Assembly [{0}] [{1}]", a->FullName, a->CodeBase);
            }
            else if (R::CustomAttribute::typeid->IsAssignableFrom(o_type))
            {
                R::CustomAttribute^ c(safe_cast<R::CustomAttribute^>(o));
                return System::String::Format(L"Custom attribute [{0}]", c->Constructor->DeclaringType->FullName);
            }
            else if (R::Field::typeid->IsAssignableFrom(o_type))
            {
                R::Field^ f(safe_cast<R::Field^>(o));
                return System::String::Format(L"Field [{0}] [${1:x8}]", f->Name, f->MetadataToken);
            }
            else if (R::Method::typeid->IsAssignableFrom(o_type))
            {
                R::Method^ m(safe_cast<R::Method^>(o));
                return System::String::Format(L"Method [{0}] [${1:x8}]", m->Name, m->MetadataToken);
            }
            else if (R::Parameter::typeid->IsAssignableFrom(o_type))
            {
                R::Parameter^ p(safe_cast<R::Parameter^>(o));
                return System::String::Format(L"Parameter [{0}]", p->Name);
            }
            else if (R::Type::typeid->IsAssignableFrom(o_type))
            {
                R::Type^ t(safe_cast<R::Type^>(o));
                return System::String::Format(L"Type [{0}] [${1:x8}]", t->FullName, t->MetadataToken);
            }
            else if (System::String::typeid->IsAssignableFrom(o_type))
            {
                return (System::String^)o;
            }
            else
            {
                return L"[UNKNOWN]";
            }
        }

        cliext::vector<System::Object^>               _stack;
        unsigned                                      _reported_frames;
        System::Text::StringBuilder^                  _message;
        System::Collections::Generic::ISet<R::Type^>^ _seen_types;
    };

    state_popper::~state_popper()
    {
        if (_state != nullptr)
            _state->pop();
    }
}

namespace
{
    template <typename T>
    auto as_system_string(T const&        t) -> System::String^ { return gcnew System::String(t.c_str()); }
    auto as_system_string(System::String^ t) -> System::String^ { return t == nullptr ? L"" : t;          }
    auto as_system_string(wchar_t const*  t) -> System::String^ { return gcnew System::String(t);         }

    template <typename T, typename U>
    auto string_equals(T t, U u, System::StringComparison mode = System::StringComparison::Ordinal) -> bool
    {
        System::String^ const t_string(as_system_string(t));
        System::String^ const u_string(as_system_string(u));

        return System::String::Equals(t_string, u_string, mode);
    }

    auto get_assembly_name(C::custom_attribute const& x) -> System::String^ { return gcnew System::String(x.constructor().declaring_type().defining_assembly().name().full_name().c_str());      }
    auto get_assembly_name(C::field            const& x) -> System::String^ { return gcnew System::String(x.declaring_type().defining_assembly().name().full_name().c_str());                    }
    auto get_assembly_name(C::method           const& x) -> System::String^ { return gcnew System::String(x.declaring_type().defining_assembly().name().full_name().c_str());                    }
    auto get_assembly_name(C::parameter        const& x) -> System::String^ { return gcnew System::String(x.declaring_method().declaring_type().defining_assembly().name().full_name().c_str()); }
    auto get_assembly_name(C::type             const& x) -> System::String^ { return gcnew System::String(x.defining_assembly().name().full_name().c_str());                                     }
    
    auto get_assembly_name(R::CustomAttribute^ x) -> System::String^ { return x->Constructor->DeclaringType->Assembly->FullName; }
    auto get_assembly_name(R::Field^           x) -> System::String^ { return x->DeclaringType->Assembly->FullName;              }
    auto get_assembly_name(R::Method^          x) -> System::String^ { return x->DeclaringType->Assembly->FullName;              }
    auto get_assembly_name(R::Parameter^       x) -> System::String^ { return x->Member->DeclaringType->Assembly->FullName;      }
    auto get_assembly_name(R::Type^            x) -> System::String^ { return x->Assembly->FullName;                             }
    
    auto get_metadata_token(C::custom_attribute const& x) -> std::uint32_t { return x.constructor().metadata_token();    }
    auto get_metadata_token(C::field            const& x) -> std::uint32_t { return x.metadata_token();                  }
    auto get_metadata_token(C::method           const& x) -> std::uint32_t { return x.metadata_token();                  }
    auto get_metadata_token(C::parameter        const& x) -> std::uint32_t { return x.metadata_token();                  }
    auto get_metadata_token(C::type             const& x) -> std::uint32_t { return x.metadata_token();                  }
    
    auto get_metadata_token(R::CustomAttribute^ x) -> std::uint32_t { return x->Constructor->MetadataToken; }
    auto get_metadata_token(R::Field^           x) -> std::uint32_t { return x->MetadataToken;              }
    auto get_metadata_token(R::Method^          x) -> std::uint32_t { return x->MetadataToken;              }
    auto get_metadata_token(R::Parameter^       x) -> std::uint32_t { return x->MetadataToken;              }
    auto get_metadata_token(R::Type^            x) -> std::uint32_t { return x->MetadataToken;              }

    auto get_brief_string(C::custom_attribute const& x) -> System::String^ { return gcnew System::String(x.constructor().declaring_type().assembly_qualified_name().c_str()); }
    auto get_brief_string(C::field            const& x) -> System::String^ { return gcnew System::String(x.name().c_str());                                                   }
    auto get_brief_string(C::method           const& x) -> System::String^ { return gcnew System::String(x.name().c_str());                                                   }
    auto get_brief_string(C::parameter        const& x) -> System::String^ { return gcnew System::String(x.name().c_str());                                                   }
    auto get_brief_string(C::type             const& x) -> System::String^ { return gcnew System::String(x.assembly_qualified_name().c_str());                                }

    auto get_brief_string(R::CustomAttribute^ x) -> System::String^ { return x->Constructor->DeclaringType->AssemblyQualifiedName; }
    auto get_brief_string(R::Field^           x) -> System::String^ { return x->Name;                                              }
    auto get_brief_string(R::Method^          x) -> System::String^ { return x->Name;                                              }
    auto get_brief_string(R::Parameter^       x) -> System::String^ { return x->Name;                                              }
    auto get_brief_string(R::Type^            x) -> System::String^ { return x->AssemblyQualifiedName;                             }

    class metadata_token_strict_weak_ordering
    {
    public:

        template <typename T>
        auto operator()(T const& lhs, T const& rhs) -> bool
        {
            if (get_assembly_name(lhs) < get_assembly_name(rhs))
                return true;

            if (get_assembly_name(lhs) > get_assembly_name(rhs))
                return false;

            return get_metadata_token(lhs) < get_metadata_token(rhs);
        }

        template <typename T>
        auto operator()(T^ lhs, T^ rhs) -> bool
        {
            if (get_assembly_name(lhs) < get_assembly_name(rhs))
                return true;

            if (get_assembly_name(lhs) > get_assembly_name(rhs))
                return false;

            return get_metadata_token(lhs) < get_metadata_token(rhs);
        }
    };

    template <typename TExpected, typename TActual>
    auto verify_string_equals(state_stack% state, System::String^ name, TExpected expected, TActual actual) -> void
    {
        if (string_equals(expected, actual))
            return;

        state.report_difference(name, as_system_string(expected), as_system_string(actual));
    }

    template <typename T, typename U>
    auto verify_integer_equals(state_stack% state, System::String^ name, T expected, U actual) -> void
    {
        if ((unsigned)expected == (unsigned)actual)
            return;

        state.report_difference(
            name,
            System::String::Format("{0:x8}", (unsigned)expected),
            System::String::Format("{0:x8}", (unsigned)actual));
    }

    auto verify_boolean_equals(state_stack% state, System::String^ name, bool expected, bool actual) -> void
    {
        if (expected == actual)
            return;

        state.report_difference(name, System::String::Format("{0}", expected), System::String::Format("{0}", actual));
    }

    auto compare(state_stack%, R::Assembly^,        C::assembly         const&) -> void;
    auto compare(state_stack%, R::CustomAttribute^, C::custom_attribute const&) -> void;
    auto compare(state_stack%, R::Field^,           C::field            const&) -> void;
    auto compare(state_stack%, R::Method^,          C::method           const&) -> void;
    auto compare(state_stack%, R::Parameter^,       C::parameter        const&) -> void;
    auto compare(state_stack%, R::Type^,            C::type             const&) -> void;

    template <typename TRElement, typename TCElement>
    auto compare_ranges(state_stack                % state,
                        System::String             ^ name,
                        cliext::vector<TRElement^>   r_elements,
                        std::vector<TCElement>       c_elements) -> void
    {
        verify_integer_equals(state, System::String::Format(L"{0} Count", name), r_elements.size(), c_elements.size());
        if ((unsigned)r_elements.size() == c_elements.size())
        {
            auto rIt(r_elements.begin());
            auto cIt(c_elements.begin());
            for (; rIt != r_elements.end() && cIt != c_elements.end(); ++rIt, ++cIt)
            {
                compare(state, *rIt, *cIt);
            }
        }
        else
        {
            {
                auto frame(state.push(System::String::Format(L"Expected {0}s", name)));
                for (auto it(r_elements.begin()); it != r_elements.end(); ++it)
                {
                    state.report_message(get_brief_string(*it));
                }
            }
            {
                auto frame(state.push(System::String::Format(L"Actual {0}s", name)));
                for (auto it(c_elements.begin()); it != c_elements.end(); ++it)
                {
                    state.report_message(get_brief_string(*it));
                }
            }
        }
    }

    template <typename TRElement, typename TCElement>
    auto compare_custom_attributes_of(state_stack% state, TRElement^ r_element, TCElement const& c_element) -> void
    {
        auto frame(state.push(L"Custom Attributes"));
            
        cliext::vector<R::CustomAttribute^> r_attributes(r_element->GetCustomAttributesData());
        std::vector<C::custom_attribute>    c_attributes(c_element.begin_custom_attributes(), c_element.end_custom_attributes());

        // TODO We do not correctly handle SerializableAttribute.  It isn't actually a custom
        // attribute, but the Reflection API reports it as if it is.  To determine whether a
        // type is serializable using CxxReflect, you can use the IsSerializable property.
        r_attributes.erase(cliext::remove_if(r_attributes.begin(), r_attributes.end(), [](R::CustomAttribute^ a)
        {
            return a->Constructor->DeclaringType->Name == L"SerializableAttribute"
                || a->Constructor->DeclaringType->Name == L"ComImportAttribute"
                || a->Constructor->DeclaringType->Name == L"SecurityPermissionAttribute"
                || a->Constructor->DeclaringType->Name == L"HostProtectionAttribute"
                || a->Constructor->DeclaringType->Name == L"FileIOPermissionAttribute"
                || a->Constructor->DeclaringType->Name == L"PermissionSetAttribute";
        }), r_attributes.end());

        sort(r_attributes.begin(), r_attributes.end(), metadata_token_strict_weak_ordering());
        sort(c_attributes.begin(), c_attributes.end(), metadata_token_strict_weak_ordering());

        compare_ranges(state, L"Attribute", r_attributes, c_attributes);
    }

    auto compare(state_stack% state, R::Assembly^ r_assembly, C::assembly const& c_assembly) -> void
    {
        auto frame(state.push(r_assembly));

        cliext::vector<R::Type^>  r_types(r_assembly->GetTypes());
        std::vector<C::type>      c_types(c_assembly.begin_types(), c_assembly.end_types());

        sort(r_types.begin(), r_types.end(), metadata_token_strict_weak_ordering());
        sort(c_types.begin(), c_types.end(), metadata_token_strict_weak_ordering());

        r_types.erase(cliext::remove_if(r_types.begin(), r_types.end(), [](R::Type^ const x)
        {
            return x->FullName == L"System.__ComObject"
                || x->FullName == L"System.StubHelpers.HStringMarshaler"
                || x->FullName == L"System.Runtime.InteropServices.WindowsRuntime.DisposableRuntimeClass";
        }), r_types.end());

        c_types.erase(std::remove_if(begin(c_types), end(c_types), [](C::type const& x)
        {
            return (x.namespace_name() == L"System"                                        && x.simple_name() == L"__ComObject")
                || (x.namespace_name() == L"System.Runtime.Remoting.Proxies"               && x.simple_name() == L"__TransparentProxy")
                || (x.namespace_name() == L"System.Runtime.InteropServices.WindowsRuntime" && x.simple_name() == L"DisposableRuntimeClass")
                || (x.namespace_name() == L"System.StubHelpers"                            && x.simple_name() == L"HStringMarshaler");
        }), end(c_types));

        auto r_it(r_types.begin());
        auto c_it(c_types.begin());
        for (; r_it != r_types.end() && c_it != c_types.end(); ++r_it, ++c_it)
        {   
            compare(state, *r_it, *c_it);
        }
    }

    auto compare(state_stack% state, R::CustomAttribute^ r_attribute, C::custom_attribute const& c_attribute) -> void
    {
        auto frame(state.push(r_attribute));
    }

    auto compare(state_stack% state, R::Field^ r_field, C::field const& c_field) -> void
    {
        auto frame(state.push(r_field));

        // TODO Support for generic fields
        if (r_field->FieldType->IsGenericType)
            return;

        //
        // Properties
        //

        verify_integer_equals(state, L"Attributes", r_field->Attributes, c_field.attributes().integer());

        verify_string_equals(state, L"DeclaringType(Name)",
            r_field->DeclaringType->AssemblyQualifiedName,
            c_field.declaring_type().assembly_qualified_name());

        {
            auto frame(state.push(L"DeclaringType"));
            compare(state, r_field->DeclaringType, c_field.declaring_type());
        }

        // FieldHandle -- Not implemented in CxxReflect

        verify_string_equals(state, L"FieldType(Name)",
            r_field->FieldType->AssemblyQualifiedName,
            c_field.field_type().assembly_qualified_name());

        {
            auto frame(state.push(L"FieldType"));
            compare(state, r_field->FieldType, c_field.field_type());
        }

        verify_boolean_equals(state, L"IsAssembly",          r_field->IsAssembly,          c_field.is_assembly());
        verify_boolean_equals(state, L"IsFamily",            r_field->IsFamily,            c_field.is_family());
        verify_boolean_equals(state, L"IsFamilyAndAssembly", r_field->IsFamilyAndAssembly, c_field.is_family_and_assembly());
        verify_boolean_equals(state, L"IsFamilyOrAssembly",  r_field->IsFamilyOrAssembly,  c_field.is_family_or_assembly());
        verify_boolean_equals(state, L"IsInitOnly",          r_field->IsInitOnly,          c_field.is_init_only());
        verify_boolean_equals(state, L"IsLiteral",           r_field->IsLiteral,           c_field.is_literal());
        verify_boolean_equals(state, L"IsNotSerialized",     r_field->IsNotSerialized,     c_field.is_not_serialized());
        verify_boolean_equals(state, L"IsPinvokeImpl",       r_field->IsPinvokeImpl,       c_field.is_pinvoke_impl());
        verify_boolean_equals(state, L"IsPrivate",           r_field->IsPrivate,           c_field.is_private());
        verify_boolean_equals(state, L"IsPublic",            r_field->IsPublic,            c_field.is_public());
        // IsSecurityCritical     -- Not implemented in CxxReflect
        // IsSecuritySafeCritical -- Not implemented in CxxReflect
        // IsSecurityTransparent  -- Not implemented in CxxReflect
        verify_boolean_equals(state, L"IsSpecialName",       r_field->IsSpecialName,       c_field.is_special_name());
        verify_boolean_equals(state, L"IsStatic",            r_field->IsStatic,            c_field.is_static());

        // MemberType -- Not implemented in CxxReflect

        verify_integer_equals(state, L"MetadataToken", r_field->MetadataToken, c_field.metadata_token());

        // TODO Module

        verify_string_equals(state, L"Name", r_field->Name, c_field.name());

        verify_string_equals(state, L"ReflectedType(Name)",
            r_field->ReflectedType->AssemblyQualifiedName,
            c_field.reflected_type().assembly_qualified_name());

        {
            auto frame(state.push(L"ReflectedType"));
            compare(state, r_field->ReflectedType, c_field.reflected_type());
        }

        //
        // Methods
        //

        // TODO GetCustomAttributes()
        // TODO GetOptionalCustomModifiers()
        // TODO GetRawConstantValue()
        // TODO GetRequiredCustomModifiers()
    }
    
    auto compare(state_stack% state, R::Method^ r_method, C::method const& c_method) -> void
    {
        auto frame(state.push(r_method));

        // TODO Support for generic methods
        if (r_method->IsGenericMethod)
            return;

        //
        // Properties
        //

        verify_integer_equals(state, L"Attributes", r_method->Attributes, c_method.attributes().integer());
        // TODO VerifyIntegerEquals(state, L"CallingConvention", rMethod->CallingConvention, cMethod.GetCallingConvention());

        // TODO ContainsGenericParameters

        {
            auto frame(state.push(L"DeclaringType"));
            compare(state, r_method->DeclaringType, c_method.declaring_type());
        }

        verify_boolean_equals(state, L"IsAbstract",                r_method->IsAbstract,                c_method.is_abstract());
        verify_boolean_equals(state, L"IsAssembly",                r_method->IsAssembly,                c_method.is_assembly());
        verify_boolean_equals(state, L"IsConstructor",             r_method->IsConstructor,             c_method.is_constructor());
        verify_boolean_equals(state, L"IsFamily",                  r_method->IsFamily,                  c_method.is_family());
        verify_boolean_equals(state, L"IsFamilyAndAssembly",       r_method->IsFamilyAndAssembly,       c_method.is_family_and_assembly());
        verify_boolean_equals(state, L"IsFamilyOrAssembly",        r_method->IsFamilyOrAssembly,        c_method.is_family_or_assembly());
        verify_boolean_equals(state, L"IsFinal",                   r_method->IsFinal,                   c_method.is_final());
        verify_boolean_equals(state, L"IsGenericMethod",           r_method->IsGenericMethod,           c_method.is_generic_method());
        verify_boolean_equals(state, L"IsGenericMethodDefinition", r_method->IsGenericMethodDefinition, c_method.is_generic_method_definition());
        verify_boolean_equals(state, L"IsHideBySig",               r_method->IsHideBySig,               c_method.is_hide_by_signature());
        verify_boolean_equals(state, L"IsPrivate",                 r_method->IsPrivate,                 c_method.is_private());
        verify_boolean_equals(state, L"IsPublic",                  r_method->IsPublic,                  c_method.is_public());
        // IsSecurityCritical     -- Not implemented in CxxReflect
        // IsSecuritySafeCritical -- Not implemented in CxxReflect
        // IsSecurityTransparent  -- Not implemented in CxxReflect
        verify_boolean_equals(state, L"IsSpecialName",             r_method->IsSpecialName,             c_method.is_special_name());
        verify_boolean_equals(state, L"IsStatic",                  r_method->IsStatic,                  c_method.is_static());
        verify_boolean_equals(state, L"IsVirtual",                 r_method->IsVirtual,                 c_method.is_virtual());

        // MemberType -- Not implemented in CxxReflect

        verify_integer_equals(state, L"MetadataToken", r_method->MetadataToken, c_method.metadata_token());

        // TODO Module

        verify_string_equals(state, L"Name", r_method->Name, c_method.name());

        {
            auto frame(state.push(L"ReflectedType"));
            compare(state, r_method->ReflectedType, c_method.reflected_type());
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

        cliext::vector<R::Parameter^> r_parameters(r_method->GetParameters());
        std::vector<C::parameter>     c_parameters(c_method.begin_parameters(), c_method.end_parameters());

        sort(r_parameters.begin(), r_parameters.end(), metadata_token_strict_weak_ordering());
        sort(c_parameters.begin(), c_parameters.end(), metadata_token_strict_weak_ordering());

        compare_ranges(state, L"Parameter", r_parameters, c_parameters);
    }

    auto compare(state_stack% state, R::Parameter^ r_parameter, C::parameter const& c_parameter) -> void
    {
        auto frame(state.push(r_parameter));

        //
        // Properties
        //

        verify_integer_equals(state, L"Attributes", r_parameter->Attributes, c_parameter.attributes().integer());

        // TODO DefaultValue

        verify_boolean_equals(state, L"IsIn",       r_parameter->IsIn,       c_parameter.is_in());
        // TODO VerifyBooleanEquals(state, L"IsLcid",     r_parameter->IsLcid,     c_parameter.IsLcid());
        verify_boolean_equals(state, L"IsOptional", r_parameter->IsOptional, c_parameter.is_optional());
        verify_boolean_equals(state, L"IsOut",      r_parameter->IsOut,      c_parameter.is_out());
        // TODO VerifyBooleanEquals(state, L"IsRetval",   r_parameter->IsRetval,   c_parameter.IsRetVal());

        // TODO Member

        verify_integer_equals(state, L"MetadataToken", r_parameter->MetadataToken, c_parameter.metadata_token());

        verify_string_equals(state, L"Name", r_parameter->Name, c_parameter.name());
        
        
        if (r_parameter->ParameterType->HasElementType)
        {
            compare(state, r_parameter->ParameterType, c_parameter.parameter_type());
        }
        else
        {
            verify_string_equals(state, L"ParameterType(Name)",
                r_parameter->ParameterType->AssemblyQualifiedName, 
                c_parameter.parameter_type().assembly_qualified_name());
        }

        verify_integer_equals(state, L"Position", r_parameter->Position, c_parameter.position());

        // TODO RawDefaultValue

        //
        // Methods
        //

        // TODO GetCustomAttributes()
        // CompareCustomAttributesOf(state, rParameter, cParameter); // GetCustomAttributes and friends

        // TODO GetOptionalCustomModifiers()
        // TODO GetRequiredCustomModifiers()
    }

    auto compare(state_stack% state, R::Type^ r_type, C::type const& c_type) -> void
    {
        // Prevent infinite recursion by ensuring we only visit each type once (this also prevents
        // us from doing more work than we need to do, since types are immutable, we only need to
        // compare them once).
        if (state.report_type_and_return_true_if_known(r_type))
            return;

        // TODO Support for generic types
        // TODO Support for array types
        if (r_type->IsGenericType || r_type->IsArray)
            return;

        auto frame(state.push(r_type));

        if (r_type->Name == L"ValueType")
        {
            int x = 42;
        }

        // TODO Assembly
        verify_string_equals(state, L"AssemblyQualifiedName", r_type->AssemblyQualifiedName, c_type.assembly_qualified_name());
        verify_integer_equals(state, L"Attributes", r_type->Attributes, c_type.attributes().integer());

        {
            auto frame(state.push(L"Base Type"));
            if (r_type->BaseType != nullptr)
            {
                compare(state, r_type->BaseType, c_type.base_type());
            }
        }

        // TODO VerifyBooleanEquals(state, L"ContainsGenericParameters", rType->ContainsGenericParameters, cType.ContainsGenericParameters());
        
        compare_custom_attributes_of(state, r_type, c_type); // GetCustomAttributes() and friends

        // TODO DeclaringMethods
        verify_string_equals(state, L"FullName", r_type->FullName, c_type.full_name());
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
        
        cliext::vector<R::Type^> r_interfaces(r_type->GetInterfaces());
        std::vector<C::type>     c_interfaces(c_type.begin_interfaces(), c_type.end_interfaces());

        sort(r_interfaces.begin(), r_interfaces.end(), metadata_token_strict_weak_ordering());
        sort(c_interfaces.begin(), c_interfaces.end(), metadata_token_strict_weak_ordering());

        compare_ranges(state, L"Interface", r_interfaces, c_interfaces);

        // TODO GetMember
        // TODO GetMembers
        // TODO GetMethod
        // TODO GetMethods

        cliext::vector<R::Method^> r_methods(r_type->GetMethods(r_all_bindings));
        std::vector<C::method>     c_methods(c_type.begin_methods(c_all_bindings), c_type.end_methods());

        sort(r_methods.begin(), r_methods.end(), metadata_token_strict_weak_ordering());
        sort(c_methods.begin(), c_methods.end(), metadata_token_strict_weak_ordering());

        compare_ranges(state, L"Method", r_methods, c_methods);

        // TODO GetNestedType
        // TODO GetNestedTypes
        // TODO GetProperties
        // TODO GetProperty
        // TODO GUID
        // TODO HasElementType

        verify_boolean_equals(state, L"IsAbstract",              r_type->IsAbstract,              c_type.is_abstract());
        verify_boolean_equals(state, L"IsAnsiClass",             r_type->IsAnsiClass,             c_type.is_ansi_class());
        verify_boolean_equals(state, L"IsArray",                 r_type->IsArray,                 c_type.is_array());
        verify_boolean_equals(state, L"IsAutoClass",             r_type->IsAutoClass,             c_type.is_auto_class());
        verify_boolean_equals(state, L"IsAutoLayout",            r_type->IsAutoLayout,            c_type.is_auto_layout());
        verify_boolean_equals(state, L"IsByRef",                 r_type->IsByRef,                 c_type.is_by_ref());
        verify_boolean_equals(state, L"IsClass",                 r_type->IsClass,                 c_type.is_class());
        verify_boolean_equals(state, L"IsCOMObject",             r_type->IsCOMObject,             c_type.is_com_object());
        verify_boolean_equals(state, L"IsContextful",            r_type->IsContextful,            c_type.is_contextful());
        verify_boolean_equals(state, L"IsEnum",                  r_type->IsEnum,                  c_type.is_enum());
        verify_boolean_equals(state, L"IsExplicitLayout",        r_type->IsExplicitLayout,        c_type.is_explicit_layout());
        verify_boolean_equals(state, L"IsGenericParameter",      r_type->IsGenericParameter,      c_type.is_generic_parameter());
        verify_boolean_equals(state, L"IsGenericType",           r_type->IsGenericType,           c_type.is_generic_type());
        verify_boolean_equals(state, L"IsGenericTypeDefinition", r_type->IsGenericTypeDefinition, c_type.is_generic_type_definition());
        verify_boolean_equals(state, L"IsImport",                r_type->IsImport,                c_type.is_import());
        verify_boolean_equals(state, L"IsInterface",             r_type->IsInterface,             c_type.is_interface());
        verify_boolean_equals(state, L"IsLayoutSequential",      r_type->IsLayoutSequential,      c_type.is_layout_sequential());
        verify_boolean_equals(state, L"IsMarshalByRef",          r_type->IsMarshalByRef,          c_type.is_marshal_by_ref());
        verify_boolean_equals(state, L"IsNested",                r_type->IsNested,                c_type.is_nested());
        verify_boolean_equals(state, L"IsNestedAssembly",        r_type->IsNestedAssembly,        c_type.is_nested_assembly());
        verify_boolean_equals(state, L"IsNestedFamANDAssem",     r_type->IsNestedFamANDAssem,     c_type.is_nested_family_and_assembly());
        verify_boolean_equals(state, L"IsNestedFamily",          r_type->IsNestedFamily,          c_type.is_nested_family());
        verify_boolean_equals(state, L"IsNestedFamORAssem",      r_type->IsNestedFamORAssem,      c_type.is_nested_family_or_assembly());
        verify_boolean_equals(state, L"IsNestedPrivate",         r_type->IsNestedPrivate,         c_type.is_nested_private());
        verify_boolean_equals(state, L"IsNestedPublic",          r_type->IsNestedPublic,          c_type.is_nested_public());
        verify_boolean_equals(state, L"IsNotPublic",             r_type->IsNotPublic,             c_type.is_not_public());
        verify_boolean_equals(state, L"IsPointer",               r_type->IsPointer,               c_type.is_pointer());
        verify_boolean_equals(state, L"IsPrimitive",             r_type->IsPrimitive,             c_type.is_primitive());
        verify_boolean_equals(state, L"IsPublic",                r_type->IsPublic,                c_type.is_public());
        verify_boolean_equals(state, L"IsSealed",                r_type->IsSealed,                c_type.is_sealed());
        //        IsSecurityCritical     -- Not implemented in CxxReflect
        //        IsSecuritySafeCritical -- Not implemented in CxxReflect
        //        IsSecurityTransparent  -- Not implemented in CxxReflect
        verify_boolean_equals(state, L"IsSerializable",          r_type->IsSerializable,          c_type.is_serializable());
        verify_boolean_equals(state, L"IsSpecialName",           r_type->IsSpecialName,           c_type.is_special_name());
        verify_boolean_equals(state, L"IsUnicodeClass",          r_type->IsUnicodeClass,          c_type.is_unicode_class());
        verify_boolean_equals(state, L"IsValueType",             r_type->IsValueType,             c_type.is_value_type());
        verify_boolean_equals(state, L"IsVisible",               r_type->IsVisible,               c_type.is_visible());

        // MemberType -- Not implemented in CxxReflect
        // TODO Module

        verify_string_equals(state, L"Name",      r_type->Name,      c_type.simple_name());
        verify_string_equals(state, L"Namespace", r_type->Namespace, c_type.namespace_name());

        // TODO ReflectedType
        // TODO StructLayoutAttribute
        // TODO TypeHandle
        // TODO TypeInitializer
    }
}

auto main() -> int
{
    wchar_t const* const mscorlib_path(L"C:\\windows\\Microsoft.NET\\Framework\\v4.0.30319\\mscorlib.dll");
    wchar_t const* const assembly_path(L"c:\\jm\\cxxreflect\\build\\output\\Win32\\Debug\\test_assemblies\\alpha.dll");

    C::externals::initialize(cxxreflect::externals::win32_externals());

    // Load the assembly using CxxReflect:
    C::directory_based_module_locator::directory_set directories;
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319");
    directories.insert(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\wpf");
    C::loader root((C::directory_based_module_locator(directories)));

    C::assembly  c_mscorlib(root.load_assembly(C::module_location(mscorlib_path)));
    R::Assembly^ r_mscorlib(R::Assembly::LoadFrom(gcnew System::String(mscorlib_path)));

    C::assembly  c_assembly(root.load_assembly(C::module_location(assembly_path)));
    R::Assembly^ r_assembly(R::Assembly::LoadFrom(gcnew System::String(assembly_path)));

    state_stack state;
    // compare(state, r_mscorlib->GetType(gcnew System::String(L"System.ValueType")), c_mscorlib.find_type(L"System.ValueType"));
    compare(state, r_assembly, c_assembly);

    System::IO::StreamWriter^ result_file(gcnew System::IO::StreamWriter(L"c:\\jm\\reflectresult.txt"));
    result_file->Write(state.messages());
    result_file->Close();

    return 0;
}

// AMDG //
