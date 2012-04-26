#ifndef CXXREFLECT_WINDOWSRUNTIMEINVOCATION_HPP_
#define CXXREFLECT_WINDOWSRUNTIMEINVOCATION_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header defines the public interface for runtime method invocation using the Windows Runtime.

#include "CxxReflect/WindowsRuntimeCommon.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace CxxReflect { namespace WindowsRuntime { namespace Internal {

    // Represents an unresolved argument in the VariantArgumentPack.  We store arguments unresolved,
    // which basically means that they have no pointers, only indices into the temporary storage.
    // This ensures that we don't muck up the arguments when we add new data to the buffers.  When
    // we need to use an argument, we resolve it into a ResolvedVariantArgument, which has pointers
    // to the data that it owns.
    class UnresolvedVariantArgument
    {
    public:

        UnresolvedVariantArgument(Metadata::ElementType type,
                                  SizeType              valueIndex,
                                  SizeType              valueSize,
                                  SizeType              typeNameIndex = 0,
                                  SizeType              typeNameSize  = 0);

        Metadata::ElementType GetElementType()   const;
        SizeType              GetValueIndex()    const;
        SizeType              GetValueSize()     const;
        SizeType              GetTypeNameIndex() const;
        SizeType              GetTypeNameSize()  const;

    private:

        Detail::ValueInitialized<Metadata::ElementType> _type;
        Detail::ValueInitialized<SizeType>              _valueIndex;
        Detail::ValueInitialized<SizeType>              _valueSize;
        Detail::ValueInitialized<SizeType>              _typeNameIndex;
        Detail::ValueInitialized<SizeType>              _typeNameSize;
    };





    // A resolved argument from a VariantArgumentPack.  The resolved argument points to its value as
    // a raw sequence of bytes, along with all known type information (element type, actual type, and
    // type name).
    class ResolvedVariantArgument
    {
    public:

        ResolvedVariantArgument(Metadata::ElementType elementType,
                                ConstByteIterator     valueFirst,
                                ConstByteIterator     valueLast,
                                ConstByteIterator     typeNameFirst,
                                ConstByteIterator     typeNameLast);

        Metadata::ElementType GetElementType() const;
        Type                  GetType()        const;
        ConstByteIterator     BeginValue()     const;
        ConstByteIterator     EndValue()       const;
        StringReference       GetTypeName()    const;

    private:

        Detail::ValueInitialized<Metadata::ElementType> _type;
        Detail::ValueInitialized<ConstByteIterator>     _valueFirst;
        Detail::ValueInitialized<ConstByteIterator>     _valueLast;
        Detail::ValueInitialized<ConstByteIterator>     _typeNameFirst;
        Detail::ValueInitialized<ConstByteIterator>     _typeNameLast;
    };





    // Represents an IInspectable along with a known type name.  Note that it is not sufficient to
    // call IInspectable::GetRuntimeClassName() because we may have additional static type info that
    // we want to make use of.  For example, if a user has type Foo that implements IFoo, and passes
    // an instance of Foo into one of the invocation methods via a pointer to its IFoo interface, we
    // want to treat it as an IFoo, not as a Foo, for the purposes of overload resolution.
    class InspectableWithTypeName
    {
    public:

        InspectableWithTypeName();
        InspectableWithTypeName(IInspectable* inspectable, StringReference typeName);

        IInspectable*   GetInspectable() const;
        StringReference GetTypeName()    const;

    private:

        Detail::ValueInitialized<IInspectable*> _inspectable;
        String                                  _typeName;
    };





    // A pack of variant arguments.  When we make an invocation with arguments, we need a way to 
    // store the arguments and type information while we process the invocation.  We do not want to
    // include the entirety of the invocation logic here, both because there is a lot of code and
    // because doing so would leak many messy implementation details.
    //
    // So, we accumulate the argument values into an array of bytes, and store with them all known
    // type information.  We can then resolve the arguments and their types during the invocation.
    class VariantArgumentPack
    {
    public:

        typedef std::vector<UnresolvedVariantArgument>             UnresolvedArgumentSequence;
        typedef UnresolvedArgumentSequence::const_iterator         UnresolvedArgumentIterator;
        typedef UnresolvedArgumentSequence::const_reverse_iterator ReverseUnresolvedArgumentIterator;

        SizeType GetArity() const;

        UnresolvedArgumentIterator Begin() const;
        UnresolvedArgumentIterator End()   const;

        ReverseUnresolvedArgumentIterator ReverseBegin() const;
        ReverseUnresolvedArgumentIterator ReverseEnd()   const;

        // Resolves an unresolved argument into a ResolvedVariantArgument.  The lifetime of the
        // resolved argument cannot extend beyond the lifetime of the VariantArgumentPack from which
        // it was obtained (i.e., treat it as an iterator into the VariantArgumentPack).
        ResolvedVariantArgument Resolve(UnresolvedVariantArgument const&) const;

        void PushArgument(bool);

        void PushArgument(wchar_t);

        void PushArgument(std::int8_t  );
        void PushArgument(std::uint8_t );
        void PushArgument(std::int16_t );
        void PushArgument(std::uint16_t);
        void PushArgument(std::int32_t );
        void PushArgument(std::uint32_t);
        void PushArgument(std::int64_t );
        void PushArgument(std::uint64_t);
        
        void PushArgument(float );
        void PushArgument(double);

        void PushArgument(InspectableWithTypeName const&);

        // TODO Add support for strings (std::string, wchar_t const*, HSTRING, etc.)
        // TODO Add support for arbitrary value types

    private:
        
        void PushArgument(Metadata::ElementType type, ConstByteIterator first, ConstByteIterator last);

        std::vector<UnresolvedVariantArgument> _arguments;
        std::vector<Byte>                      _data;
    };





    // We use this PreprocessArgument function to inject type information from a hatted type into
    // the argument before we store it in the VariantArgumentPack.  The function is a no-op for 
    // bare-headed types.
    template <typename T>
    T PreprocessArgument(T const& value)
    {
        return value;
    }

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template <typename T>
    InspectableWithTypeName PreprocessArgument(T^ const value)
    {
        return InspectableWithTypeName(
            reinterpret_cast<IInspectable*>(value),
            T::typeid->FullName->Data());
    }
    #endif





    // PackArguments packs a set of strongly-typed argument values into a VariantArgumentPack and
    // returns the resulting argument pack.  We use these functions to bridge the gap between the
    // statically typed invocation function templates, and the untyped functions that use the
    // VariantArgumentPack.
    template <typename P0>
    VariantArgumentPack PackArguments(P0&& a0)
    {
        VariantArgumentPack pack;
        pack.PushArgument(PreprocessArgument(std::forward<P0>(a0)));
        return pack;
    }

    template <typename P0, typename P1>
    VariantArgumentPack PackArguments(P0&& a0, P1&& a1)
    {
        VariantArgumentPack pack;
        pack.PushArgument(PreprocessArgument(std::forward<P0>(a0)));
        pack.PushArgument(PreprocessArgument(std::forward<P1>(a1)));
        return pack;
    }

    template <typename P0, typename P1, typename P2>
    VariantArgumentPack PackArguments(P0&& a0, P1&& a1, P2&& a2)
    {
        VariantArgumentPack pack;
        pack.PushArgument(PreprocessArgument(std::forward<P0>(a0)));
        pack.PushArgument(PreprocessArgument(std::forward<P1>(a1)));
        pack.PushArgument(PreprocessArgument(std::forward<P2>(a2)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3>
    VariantArgumentPack PackArguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3)
    {
        VariantArgumentPack pack;
        pack.PushArgument(PreprocessArgument(std::forward<P0>(a0)));
        pack.PushArgument(PreprocessArgument(std::forward<P1>(a1)));
        pack.PushArgument(PreprocessArgument(std::forward<P2>(a2)));
        pack.PushArgument(PreprocessArgument(std::forward<P3>(a3)));
        return pack;
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    VariantArgumentPack PackArguments(P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
    {
        VariantArgumentPack pack;
        pack.PushArgument(PreprocessArgument(std::forward<P0>(a0)));
        pack.PushArgument(PreprocessArgument(std::forward<P1>(a1)));
        pack.PushArgument(PreprocessArgument(std::forward<P2>(a2)));
        pack.PushArgument(PreprocessArgument(std::forward<P3>(a3)));
        pack.PushArgument(PreprocessArgument(std::forward<P4>(a4)));
        return pack;
    }





    // A basic converting overload resolver. 
    //
    // TODO Document overload resolution behavior.
    class ConvertingOverloadResolver
    {
    public:

        enum class ConversionRank : std::uint32_t
        {
            NoMatch                      = 0xffffffff,

            ExactMatch                   = 0x00000000,
            IntegralPromotion            = 0x00010000,
            RealConversion               = 0x00020000,
            DerivedToBaseConversion      = 0x00040000,
            DerivedToInterfaceConversion = 0x00080000
        };

        enum class State
        {
            NotEvaluated,
            MatchFound,
            MatchNotFound
        };

        template <typename TInputIt>
        ConvertingOverloadResolver(TInputIt const first, TInputIt last, VariantArgumentPack const& arguments)
            : _candidates(first, last), _arguments(arguments)
        {
        }

        bool   Succeeded() const;
        Method GetResult() const;

    private:

        static ConversionRank ComputeConversionRank     (Type const& parameterType, Type const& argumentType);
        static ConversionRank ComputeClassConversionRank(Type const& parameterType, Type const& argumentType);

        static ConversionRank ComputeNumericConversionRank(Metadata::ElementType parameterType,
                                                           Metadata::ElementType argumentType);

        void EnsureEvaluated() const;

        Detail::ValueInitialized<State> mutable _state;
        Method                          mutable _result;

        std::vector<Method> _candidates;
        VariantArgumentPack _arguments;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(ConvertingOverloadResolver::ConversionRank);





    // Computes the ElementType of 'type' for use in overload resolution.  This is not necessarily
    // the actual ElementType that best fits 'type'.  For example, Object^ has its own ElementType,
    // but this will still return Class for Object^, because that is how it is considered during
    // overloading and conversion.
    Metadata::ElementType ComputeOverloadElementType(Type const& type);





    typedef std::int8_t   I1;
    typedef std::uint8_t  U1;
    typedef std::int16_t  I2;
    typedef std::uint16_t U2;
    typedef std::int32_t  I4;
    typedef std::uint32_t U4;
    typedef std::int64_t  I8;
    typedef std::uint64_t U8;
    typedef float         R4;
    typedef double        R8;





    I4 ConvertToI4(ResolvedVariantArgument const& argument); 
    I8 ConvertToI8(ResolvedVariantArgument const& argument);

    U4 ConvertToU4(ResolvedVariantArgument const& argument); 
    U8 ConvertToU8(ResolvedVariantArgument const& argument);

    R4 ConvertToR4(ResolvedVariantArgument const& argument); 
    R8 ConvertToR8(ResolvedVariantArgument const& argument);

    IInspectable* ConvertToInterface(ResolvedVariantArgument const& argument, Guid const& interfaceGuid);

    template <typename T>
    T ReinterpretAs(ResolvedVariantArgument const& argument);

    template <typename TTarget, typename TSource>
    TTarget VerifyInRangeAndConvertTo(TSource const& value);





    // Computes the pointer to the function at 'slot' in the vtable pointed to by 'instance'.
    static void const* ComputeFunctionPointer(void const* instance, unsigned slot);





    #if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86 
    // The ArgumentFrame is used to align arguments for a call.  It does not maintain any type
    // information, it just keeps an array of bytes that can be aligned and padded sufficiently to
    // mock a function call.
    class X86ArgumentFrame
    {
    public:

        ConstByteIterator Begin()   const;
        ConstByteIterator End()     const;
        ConstByteIterator GetData() const;
        SizeType          GetSize() const;

        // Aligns the end of the frame to an index evenly divisible by 'alignment'.
        void AlignTo(SizeType alignment);

        // Pushes the provided range of bytes into the frame, at the end, after any existing data.
        // This function does not perform any alignment.
        void Push(ConstByteIterator first, ConstByteIterator last);

    private:

        std::vector<Byte> _data;
    };

    // ArgumentConverter
    //
    // TODO Document behavior
    class X86ArgumentConverter
    {
    public:

        static void ConvertAndInsert(Type                    const& parameterType,
                                     ResolvedVariantArgument const& argument,
                                     X86ArgumentFrame             & frame);
    };

    // Invocation logic for x86 stdcall functions.
    class X86StdCallInvoker
    {
    public:

        static HResult Invoke(Method              const& method,
                              IInspectable             * instance,
                              void                     * result,
                              VariantArgumentPack const& arguments);

    private:

        template <SizeType FrameSize>
        static HResult InvokeWithFrame(void const* functionPointer, ConstByteIterator frameBytes);
    };
    #endif




    
    #if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64
    extern "C"
    {
        // This is a thunk for dynamically invoking an x64 __fastcall function.  For documentation,
        // see its implementation in X64FastCallThunk.asm.  Its declaration here must exactly match
        // the declaration described in the .asm file.  Be very, very carfeul when calling this :-)
        int CxxReflectX64FastCallThunk(void const*   fp,
                                       void const*   arguments,
                                       void const*   types,
                                       std::uint64_t count);
    }

    // These are the flags used in the 'types' array passed to the CxxReflectX64FastCallThunk.  The
    // enumerator values must match the values described in the documentation of the thunk procedure.
    enum class X64ArgumentType : std::uint64_t
    {
        Integer             = 0,
        DoublePrecisionReal = 1,
        SinglePrecisionReal = 2
    };

    // This frame is used for accumulating arguments for dynamic invocation on x64.  The arguments,
    // types, and count are maintained such that they may be passed to the CxxReflectX64FastCallThunk.
    class X64ArgumentFrame
    {
    public:

        void const*   GetArguments() const;
        void const*   GetTypes()     const;
        std::uint64_t GetCount()     const;

        void Push(float  x);
        void Push(double x);

        template <typename T>
        void Push(T const& x)
        {
            static_assert(sizeof(T) == 8, "Attempted to push unaligned argument");
            _arguments.insert(_arguments.end(), Detail::BeginBytes(x), Detail::EndBytes(x));
            _types.push_back(X64ArgumentType::Integer);
        }

    private:

        std::vector<Byte>            _arguments;
        std::vector<X64ArgumentType> _types;
    };

    class X64ArgumentConverter
    {
    public:

        static void ConvertAndInsert(Type                    const& parameterType,
                                     ResolvedVariantArgument const& argument,
                                     X64ArgumentFrame             & frame);
    };

    // Invocation logic for x64 fastcall functions.
    class X64FastCallInvoker
    {
    public:

        static HResult Invoke(Method              const& method,
                              IInspectable             * instance,
                              void                     * result,
                              VariantArgumentPack const& arguments);

    private:
    };
    #endif

    // Invocation logic for ARM Procedure Call functions.  TODO Not yet implemented.  This exists
    // only so we have something to typedef below.  If you call Invoke(), it will throw.  This
    // class will be implemented per the ARM Procedure Call Standard, documented at the following
    // PDF:  http://infocenter.arm.com/help/topic/com.arm.doc.ihi0042d/IHI0042D_aapcs.pdf (Note:
    // this is speculation; we'll have to do substantial investigation into the code generated by
    // Visual C++ when targeting ARM in order to ensure that we implement the right thing.)
    class ArmApcCallInvoker
    {
    public:

        static HResult Invoke(Method              const& method,
                              IInspectable             * instance,
                              void                     * result,
                              VariantArgumentPack const& arguments);
    };

    #if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86 
    typedef X86StdCallInvoker CallInvoker;
    #elif CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64
    typedef X64FastCallInvoker CallInvoker;
    #elif CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_ARM
    typedef ArmApcCallInvoker CallInvoker;
    #else
    #error Architecture not specified or unknown architecture specified
    #endif





    // TODO Documentation
    SizeType ComputeMethodSlotIndex(Method const& method);
    Method FindMatchingInterfaceMethod(Method const& runtimeTypeMethod);




    // These internal invocation functions are what actually perform method invocation or type
    // instantiation for any call that has arguments.  All of the public interface functions call
    // into one of these functions.  (A different path is taken for some calls that do not have
    // arguments.)
    UniqueInspectable CreateInspectableInstance(Type const& type, VariantArgumentPack const& arguments);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    inline ::Platform::Object^ CreateObjectInstance(Type const& type, VariantArgumentPack const& arguments)
    {
        // TODO:  Is this reference counting correct?
        return reinterpret_cast<::Platform::Object^>(CreateInspectableInstance(type, arguments).release());
    }

    template <typename TTarget>
    TTarget^ CreateInstance(Type const& type, VariantArgumentPack const& arguments)
    {
        return dynamic_cast<TTarget^>(CreateObjectInstance(type, arguments));
    }
    #endif

} } }





namespace CxxReflect { namespace WindowsRuntime {

    // INVOCATION ERROR
    //
    // This exception is thrown when an invocation fails.

    class InvocationError : public RuntimeError
    {
    public:

        explicit InvocationError(StringReference message);
    };





    // TYPE INSTANTIATION VIA DEFAULT CONSTRUCTOR
    //
    // These functions create an instance of the requested 'type' using its default constructor (or,
    // to be more precise, using the RoActivateInstance Windows Runtime function).  If the creation
    // fails, an InvocationError is thrown.

    UniqueInspectable CreateInspectableInstance(Type const& type);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    inline ::Platform::Object^ CreateObjectInstance(Type const& type)
    {
        return reinterpret_cast<::Platform::Object^>(CreateInspectableInstance(type).release());
    }

    template <typename T>
    T^ CreateInstance(Type const& type)
    {
        return dynamic_cast<T^>(CreateObjectInstance(type));
    }
    #endif





    // TYPE INSTANTIATION USING ARGUMENTS
    //
    // These functions create an instance of the requested 'type' using the provided arguments.
    // They use a basic overload resolution and argument conversion algorithm, which is implemented
    // as follows:
    //
    // * If the argument is a reference type (i.e., IInspectable* or Object^), it may be converted
    //   to any interface that it implements.  No interface is preferable to any other interface, so
    //   all runtime class -> interface conversions have equal conversion rank.
    //
    // * If the argument is an integer type, it may be promoted to any wider integer type with the
    //   same signedness.  So, int16_t -> int32_t and uint8_t -> uint64_t are both allowed, but
    //   int32_t -> int16_t is not allowed because it is a narrowing conversion, which is not valid.
    //   Likewise, uint32_t -> int32_t is not allowed because the signedness of the argument does
    //   not match the signedness of the parameter.
    //
    //   Note that this 'integer type' category really means integer types; it does not include bool
    //   or wchar_t (which is the native Windows Runtime character type).
    //
    //   During overload resolution, shorter conversions are preferred, so int8_t -> int16_t is a
    //   better conversion than int8_t -> int32_t.
    //
    // * If the argument is a real type, it may be promoted to a wider real type.  Basically, we
    //   allow float -> double.  There are no other conversions in this category.
    //
    // * If the argument is of type bool or wchar_t, it can only match a parameter of type bool or
    //   wchar_t, respectively.
    //
    // If the creation fails, an InvocationError is thrown.
    //
    // We support up to five arguments, though that upper limit is arbitrary; additional overloads
    // may be very easily written to support additional arities.
    //
    // TODO We do not yet support strings, enumerations, or arbitrary value types.  Other kinds of
    // conversions are also not fully supported, including derived-to-base conversions (which may
    // not even be viable, since we can only QueryInterface an interface pointer, not a runtime
    // class pointer).

    template <typename P0>
    UniqueInspectable CreateInspectableInstance(Type const& type, P0&& a0)
    {
        return Internal::CreateInspectableInstance(type, Internal::PackArguments(
            std::forward<P0>(a0)));
    }

    template <typename P0, typename P1>
    UniqueInspectable CreateInspectableInstance(Type const& type, P0&& a0, P1&& a1)
    {
        return Internal::CreateInspectableInstance(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1)));
    }

    template <typename P0, typename P1, typename P2>
    UniqueInspectable CreateInspectableInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2)
    {
        return Internal::CreateInspectableInstance(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2)));
    }

    template <typename P0, typename P1, typename P2, typename P3>
    UniqueInspectable CreateInspectableInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2, P3&& a3)
    {
        return Internal::CreateInspectableInstance(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    UniqueInspectable CreateInspectableInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
    {
        return Internal::CreateInspectableInstance(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3), 
            std::forward<P4>(a4)));
    }

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template <typename P0>
    ::Platform::Object^ CreateObjectInstance(Type const& type, P0&& a0)
    {
        return Internal::CreateObjectInstance(type, Internal::PackArguments(
            std::forward<P0>(a0)));
    }

    template <typename P0, typename P1>
    ::Platform::Object^ CreateObjectInstance(Type const& type, P0&& a0, P1&& a1)
    {
        return Internal::CreateObjectInstance(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1)));
    }

    template <typename P0, typename P1, typename P2>
    ::Platform::Object^ CreateObjectInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2)
    {
        return Internal::CreateObjectInstance(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2)));
    }

    template <typename P0, typename P1, typename P2, typename P3>
    ::Platform::Object^ CreateObjectInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2, P3&& a3)
    {
        return Internal::CreateObjectInstance(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    ::Platform::Object^ CreateObjectInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
    {
        return Internal::CreateObjectInstance(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3), 
            std::forward<P4>(a4)));
    }

    template <typename TTarget, typename P0>
    TTarget^ CreateInstance(Type const& type, P0&& a0)
    {
        return Internal::CreateInstance<TTarget>(type, Internal::PackArguments(
            std::forward<P0>(a0)));
    }

    template <typename TTarget, typename P0, typename P1>
    TTarget^ CreateInstance(Type const& type, P0&& a0, P1&& a1)
    {
        return Internal::CreateInstance<TTarget>(type, Internal::PackArguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1)));
    }

    template <typename TTarget, typename P0, typename P1, typename P2>
    TTarget^ CreateInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2)
    {
        return Internal::CreateInstance<TTarget>(type, Internal::PackArguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2)));
    }

    template <typename TTarget, typename P0, typename P1, typename P2, typename P3>
    TTarget^ CreateInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2, P3&& a3)
    {
        return Internal::CreateInstance<TTarget>(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3)));
    }

    template <typename TTarget, typename P0, typename P1, typename P2, typename P3, typename P4>
    TTarget^ CreateInstance(Type const& type, P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4)
    {
        return Internal::CreateInstance<TTarget>(type, Internal::PackArguments(
            std::forward<P0>(a0), 
            std::forward<P1>(a1), 
            std::forward<P2>(a2), 
            std::forward<P3>(a3), 
            std::forward<P4>(a4)));
    }
    #endif





    // METHOD INVOCATION WITHOUT ARGUMENTS
    //
    // These functions invoke either a static method or an instance method.  The 'instance' pointer
    // cannot be null for an instance method call.  If the invocation fails, an InvocationError is
    // thrown.
    //
    // TODO Finish documentation and specification when we implement these.
    //
    // TODO None of these are implemented yet, and once we do implement them, we probably won't have
    // full support for value type return types.  We'll need to find some way to box them.

    // TODO void InvokeStaticWithVoidReturn  (Method const& method);
    // TODO void InvokeInstanceWithVoidReturn(Method const& method, IInspectable* instance);

    // TODO UniqueInspectable InvokeStaticWithInspectableReturn  (Method const& method);
    // TODO UniqueInspectable InvokeInstanceWithInspectableReturn(Method const& method, IInspectable* instance);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    // TODO inline ::Platform::Object^ InvokeStaticWithObjectReturn(Method const& method)
    // {
    //     return reinterpret_cast<::Platform::Object^>(InvokeStaticWithInspectableReturn(method).release());
    // }

    // TODO inline ::Platform::Object^ InvokeInstanceWithObjectReturn(Method const& method, IInspectable* const instance)
    // {
    //     return reinterpret_cast<::Platform::Object^>(InvokeInstanceWithInspectableReturn(method, instance).release());
    // }

    // TODO template <typename TTarget>
    // TTarget^ InvokeStatic(Method const& method)
    // {
    //     return dynamic_cast<TTarget^>(InvokeStaticWithObjectReturn(method));
    // }

    // TODO template <typename TTarget>
    // TTarget^ InvokeInstance(Method const& method, IInspectable* const instance)
    // {
    //     return dynamic_cast<TTarget^>(InvokeInstanceWithObjectReturn(method, instance));
    // }
    #endif





    // METHOD INVOCATION WITH ARGUMENTS
    //
    // TODO Finish documentation and such

} }

#endif // CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif
