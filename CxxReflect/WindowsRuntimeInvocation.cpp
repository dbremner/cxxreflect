
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/WindowsRuntimeCommon.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "CxxReflect/WindowsRuntimeInspection.hpp"
#include "CxxReflect/WindowsRuntimeInvocation.hpp"
#include "CxxReflect/WindowsRuntimeLoader.hpp"
#include "CxxReflect/WindowsRuntimeInternals.hpp"

#include <inspectable.h>
#include <wrl/client.h>

// I hate Windows.h.
#undef GetMessage

using Microsoft::WRL::ComPtr;

namespace CxxReflect { namespace WindowsRuntime { namespace {

    /// Calls `callable`, handles all exceptions and rethrows them as `InvocationError`
    template <typename TCallable>
    auto CallWithInvocationConvention(TCallable&& callable) -> decltype(callable())
    {
        try
        {
            return callable();
        }
        catch (InvocationError const&)
        {
            // Don't mess with exceptions that are already InvocationErrors:
            throw;
        }
        catch (LogicError const&)
        {
            // Rethrow logic errors as-is; this is required so that the next block doesn't handle it
            throw;
        }
        catch (Exception const& e)
        {
            throw InvocationError(e.GetMessage().c_str());
        }
        catch (...)
        {
            throw InvocationError(L"An unknown failure occurred during invocation");
        }
    }

} } }

namespace CxxReflect { namespace WindowsRuntime { namespace Internal {

    UnresolvedVariantArgument::UnresolvedVariantArgument(Metadata::ElementType const type,
                                                         SizeType              const valueIndex,
                                                         SizeType              const valueSize,
                                                         SizeType              const typeNameIndex,
                                                         SizeType              const typeNameSize)
        : _type         (type         ),
          _valueIndex   (valueIndex   ),
          _valueSize    (valueSize    ),
          _typeNameIndex(typeNameIndex),
          _typeNameSize (typeNameSize )
    {
    }

    Metadata::ElementType UnresolvedVariantArgument::GetElementType() const
    {
        return _type.Get();
    }

    SizeType UnresolvedVariantArgument::GetValueIndex() const
    {
        return _valueIndex.Get();
    }

    SizeType UnresolvedVariantArgument::GetValueSize() const
    {
        return _valueSize.Get();
    }

    SizeType UnresolvedVariantArgument::GetTypeNameIndex() const
    {
        return _typeNameIndex.Get();
    }

    SizeType UnresolvedVariantArgument::GetTypeNameSize() const
    {
        return _typeNameSize.Get();
    }





    ResolvedVariantArgument::ResolvedVariantArgument(Metadata::ElementType const type,
                                                     ConstByteIterator     const valueFirst,
                                                     ConstByteIterator     const valueLast,
                                                     ConstByteIterator     const typeNameFirst,
                                                     ConstByteIterator     const typeNameLast)
        : _type         (type         ),
          _valueFirst   (valueFirst   ),
          _valueLast    (valueLast    ), 
          _typeNameFirst(typeNameFirst),
          _typeNameLast (typeNameLast )
    {
        Detail::AssertNotNull(_valueFirst.Get());
        Detail::AssertNotNull(_valueLast.Get());
        // Note:  _typeNameFirst and _typeNameLast may be null.
    }
    
    Metadata::ElementType ResolvedVariantArgument::GetElementType() const
    {
        return _type.Get();
    }

    Type ResolvedVariantArgument::GetType() const
    {
        if (_type.Get() == Metadata::ElementType::Class)
        {
            // First, see if we have a known type name (i.e., GetTypeName() returns a string).  If 
            // we have one, we use that to get the type of the argument.
            StringReference const knownTypeName(GetTypeName());
            if (!knownTypeName.empty())
            {
                Type const type(WindowsRuntime::GetType(knownTypeName));

                // If the static type of the object was Platform::Object, we'll instead try to use
                // its dynamic type for overload resolution:
                if (type.IsInitialized() && type != WindowsRuntime::GetType(L"Platform", L"Object"))
                    return type;
            }

            // Otherwise, see if we can get the type from the IInspectable argument:
            Detail::Verify([&]{ return sizeof(IInspectable*) == Detail::Distance(BeginValue(), EndValue()); });

            IInspectable* value(nullptr);
            Detail::RangeCheckedCopy(BeginValue(), EndValue(), Detail::BeginBytes(value), Detail::EndBytes(value));

            // If we have an IInspectable object, try to get its runtime class name:
            if (value != nullptr)
            {
                Utility::SmartHString inspectableTypeName;
                Detail::VerifySuccess(value->GetRuntimeClassName(inspectableTypeName.proxy()));

                Type const type(WindowsRuntime::GetType(inspectableTypeName.c_str()));
                if (type.IsInitialized())
                    return type;
            }

            // TODO For nullptr, we should probably allow conversion to any interface with equal
            // conversion rank.  How to do this cleanly, though, is a good question.

            // Finally, fall back to use Platform::Object^:
            Type const type(WindowsRuntime::GetType(L"Platform", L"Object"));
            Detail::Verify([&]{ return type.IsInitialized(); });
            return type;
        }
        else if (_type.Get() == Metadata::ElementType::ValueType)
        {
            throw LogicError(L"Not yet implemented"); // TODO
        }
        else
        {
            Detail::LoaderContext const& loader(WindowsRuntime::GlobalLoaderContext::Get()
                .GetLoader()
                .GetContext(InternalKey()));

            return Type(loader, loader.ResolveFundamentalType(_type.Get()), InternalKey());
        }
    }

    ConstByteIterator ResolvedVariantArgument::BeginValue() const
    {
        return _valueFirst.Get();
    }

    ConstByteIterator ResolvedVariantArgument::EndValue() const
    {
        return _valueLast.Get();
    }

    StringReference ResolvedVariantArgument::GetTypeName() const
    {
        // If the type name is an empty range, then we have no type name:
        if (_typeNameFirst.Get() == _typeNameLast.Get())
            return StringReference();

        return StringReference(
            reinterpret_cast<ConstCharacterIterator>(_typeNameFirst.Get()),
            reinterpret_cast<ConstCharacterIterator>(_typeNameLast.Get()));
    }





    InspectableWithTypeName::InspectableWithTypeName()
    {
    }

    InspectableWithTypeName::InspectableWithTypeName(IInspectable* const inspectable, StringReference const typeName)
        : _inspectable(inspectable), _typeName(typeName.c_str())
    {
    }

    IInspectable* InspectableWithTypeName::GetInspectable() const
    {
        return _inspectable.Get();
    }

    StringReference InspectableWithTypeName::GetTypeName() const
    {
        return _typeName.c_str();
    }






    SizeType VariantArgumentPack::GetArity() const
    {
        return Detail::ConvertInteger(_arguments.size());
    }

    VariantArgumentPack::UnresolvedArgumentIterator VariantArgumentPack::Begin() const
    {
        return _arguments.begin();
    }

    VariantArgumentPack::UnresolvedArgumentIterator VariantArgumentPack::End() const
    {
        return _arguments.end();
    }

    VariantArgumentPack::ReverseUnresolvedArgumentIterator VariantArgumentPack::ReverseBegin() const
    {
        return _arguments.rbegin();
    }

    VariantArgumentPack::ReverseUnresolvedArgumentIterator VariantArgumentPack::ReverseEnd() const
    {
        return _arguments.rend();
    }

    ResolvedVariantArgument VariantArgumentPack::Resolve(UnresolvedVariantArgument const& argument) const
    {
        ConstByteIterator const typeNameFirst(argument.GetTypeNameSize() != 0
            ? _data.data() + argument.GetTypeNameIndex()
            : nullptr);

        ConstByteIterator const typeNameLast(argument.GetTypeNameSize() != 0
            ? _data.data() + argument.GetTypeNameIndex() + argument.GetTypeNameSize()
            : nullptr);

        return ResolvedVariantArgument(
            argument.GetElementType(),
            _data.data() + argument.GetValueIndex(),
            _data.data() + argument.GetValueIndex() + argument.GetValueSize(),
            typeNameFirst,
            typeNameLast);
    }

    void VariantArgumentPack::PushArgument(bool const value)
    {
        PushArgument(Metadata::ElementType::Boolean, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(wchar_t const value)
    {
        PushArgument(Metadata::ElementType::Char, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(std::int8_t const value)
    {
        PushArgument(Metadata::ElementType::I1, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(std::uint8_t const value)
    {
        PushArgument(Metadata::ElementType::U1, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(std::int16_t const value)
    {
        PushArgument(Metadata::ElementType::I2, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(std::uint16_t const value)
    {
        PushArgument(Metadata::ElementType::U2, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(std::int32_t const value)
    {
        PushArgument(Metadata::ElementType::I4, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(std::uint32_t const value)
    {
        PushArgument(Metadata::ElementType::U4, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(std::int64_t const value)
    {
        PushArgument(Metadata::ElementType::I8, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(std::uint64_t const value)
    {
        PushArgument(Metadata::ElementType::U8, Detail::BeginBytes(value), Detail::EndBytes(value));
    }
        
    void VariantArgumentPack::PushArgument(float const value)
    {
        PushArgument(Metadata::ElementType::R4, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(double const value)
    {
        PushArgument(Metadata::ElementType::R8, Detail::BeginBytes(value), Detail::EndBytes(value));
    }

    void VariantArgumentPack::PushArgument(InspectableWithTypeName const& argument)
    {
        IInspectable* const value(argument.GetInspectable());

        SizeType const valueIndex(Detail::ConvertInteger(_data.size()));
        std::copy(Detail::BeginBytes(value), Detail::EndBytes(value), std::back_inserter(_data));

        SizeType const nameIndex(Detail::ConvertInteger(_data.size()));
        std::copy(reinterpret_cast<ConstByteIterator>(argument.GetTypeName().begin()),
                  reinterpret_cast<ConstByteIterator>(argument.GetTypeName().end()),
                  std::back_inserter(_data));

        // Null-terminate the buffer:
        _data.resize(_data.size() + sizeof(Character));

        _arguments.push_back(UnresolvedVariantArgument(
            Metadata::ElementType::Class,
            valueIndex,
            Detail::ConvertInteger(sizeof(value)),
            nameIndex,
            Detail::ConvertInteger((argument.GetTypeName().size() + 1) * sizeof(Character))));
    }

    void VariantArgumentPack::PushArgument(Metadata::ElementType const type,
                                           ConstByteIterator     const first,
                                           ConstByteIterator     const last)
    {
        SizeType const index(Detail::ConvertInteger(_data.size()));

        std::copy(first, last, std::back_inserter(_data));
        _arguments.push_back(UnresolvedVariantArgument(type, index, Detail::Distance(first, last)));
    }





    bool ConvertingOverloadResolver::Succeeded() const
    {
        EnsureEvaluated();
        return _state.Get() == State::MatchFound;
    }

    Method ConvertingOverloadResolver::GetResult() const
    {
        EnsureEvaluated();
        if (_state.Get() != State::MatchFound)
            throw LogicError(L"Matching method not found.  Call Succeeded() first.");

        return _result;
    }

    void ConvertingOverloadResolver::EnsureEvaluated() const
    {
        if (_state.Get() != State::NotEvaluated)
            return;

        // Accumulate the argument types once, for performance:
        std::vector<Type> argumentTypes;
        std::transform(_arguments.Begin(), _arguments.End(), std::back_inserter(argumentTypes),
                        [&](UnresolvedVariantArgument const& a) -> Type
        {
            return _arguments.Resolve(a).GetType();
        });

        _state.Get() = State::MatchNotFound;

        auto                        bestMatch(end(_candidates));
        std::vector<ConversionRank> bestMatchRank(argumentTypes.size(), ConversionRank::NoMatch);

        for (auto methodIt(begin(_candidates)); methodIt != end(_candidates); ++methodIt)
        {
            // First, check to see if the arity matches:
            if (Detail::Distance(methodIt->BeginParameters(), methodIt->EndParameters()) != argumentTypes.size())
                continue;

            std::vector<ConversionRank> currentRank(argumentTypes.size(), ConversionRank::NoMatch);

            // Compute the conversion rank of this method:
            unsigned parameterNumber(0);
            auto parameterIt(methodIt->BeginParameters());
            for (; parameterIt != methodIt->EndParameters(); ++parameterIt, ++parameterNumber)
            {
                currentRank[parameterNumber] = ComputeConversionRank(
                    parameterIt->GetType(),
                    argumentTypes[parameterNumber]);

                // If any parameter is not a match, the whole method is not a match:
                if (currentRank[parameterNumber] == ConversionRank::NoMatch)
                    break;
            }

            bool betterMatch(false);
            bool worseMatch(false);
            bool noMatch(false);
            for (unsigned i(0); i < argumentTypes.size(); ++i)
            {
                if (currentRank[i] == ConversionRank::NoMatch)
                    noMatch = true;
                else if (currentRank[i] < bestMatchRank[i])
                    betterMatch = true;
                else if (currentRank[i] > bestMatchRank[i])
                    worseMatch = true;
            }

            if (noMatch)
            {
                continue;
            }

            // This is an unambiguously better match than the current best match:
            if (betterMatch && !worseMatch)
            {
                bestMatch     = methodIt;
                bestMatchRank = currentRank;
                continue;
            }

            // This is an unambiguously worse match than the current best match:
            if (worseMatch && !betterMatch)
            {
                continue;
            }

            // There is an ambiguity between this match and the current best match:
            bestMatch = end(_candidates);
            for (unsigned i(0); i < argumentTypes.size(); ++i)
            {
                bestMatchRank[i] = bestMatchRank[i] < currentRank[i] ? bestMatchRank[i] : currentRank[i];
            }
        }

        if (bestMatch != end(_candidates))
            _result = *bestMatch;

        if (_result.IsInitialized())
            _state.Get() = State::MatchFound;
    }

    ConvertingOverloadResolver::ConversionRank
    ConvertingOverloadResolver::ComputeConversionRank(Type const& parameterType, Type const& argumentType)
    {
        Detail::Assert([&]{ return parameterType.IsInitialized() && argumentType.IsInitialized(); });

        Metadata::ElementType const pType(ComputeOverloadElementType(parameterType));
        Metadata::ElementType const aType(ComputeOverloadElementType(argumentType));

        // Exact match of any kind.
        if (parameterType == argumentType)
        {
            return ConversionRank::ExactMatch;
        }

        // Value Types, Boolean, Char, and String only match exactly; there are no conversions.
        if (pType == Metadata::ElementType::ValueType || aType == Metadata::ElementType::ValueType ||
            pType == Metadata::ElementType::Boolean   || aType == Metadata::ElementType::Boolean   ||
            pType == Metadata::ElementType::Char      || aType == Metadata::ElementType::Char      ||
            pType == Metadata::ElementType::String    || aType == Metadata::ElementType::String)
        {
            return ConversionRank::NoMatch;
        }

        // A Class Type may be converted to another Class Type.
        if (pType == Metadata::ElementType::Class && aType == Metadata::ElementType::Class)
        {
            return ComputeClassConversionRank(parameterType, argumentType);
        }
        else if (pType == Metadata::ElementType::Class || aType == Metadata::ElementType::Class)
        {
            return ConversionRank::NoMatch;
        }

        // Numeric conversions:
        if (IsNumericElementType(pType) && IsNumericElementType(aType))
        {
            return ComputeNumericConversionRank(pType, aType);
        }

        throw LogicError(L"Not yet implemented");
        // return ConversionRank::NoMatch;
    }

    ConvertingOverloadResolver::ConversionRank
    ConvertingOverloadResolver::ComputeClassConversionRank(Type const& parameterType, Type const& argumentType)
    {
        Detail::Assert([&]{ return !parameterType.IsValueType() && !argumentType.IsValueType(); });
        Detail::Assert([&]{ return parameterType != argumentType;                               });

        // First check to see if there is a derived-to-base conversion:
        if (parameterType.IsClass())
        {
            unsigned baseDistance(1);
            Type baseType(argumentType.GetBaseType());
            while (baseType.IsInitialized())
            {
                if (baseType == parameterType)
                    return ConversionRank::DerivedToBaseConversion | static_cast<ConversionRank>(baseDistance);

                baseType = baseType.GetBaseType();
                ++baseDistance;
            }
        }

        // Next check to see if there is an interface conversion.  Note that all interface
        // conversions are of equal rank.
        if (parameterType.IsInterface())
        {
            Type currentType(argumentType);
            while (currentType.IsInitialized())
            {
                auto const it(std::find(currentType.BeginInterfaces(), currentType.EndInterfaces(), parameterType));
                if (it != currentType.EndInterfaces())
                    return ConversionRank::DerivedToInterfaceConversion;

                currentType = currentType.GetBaseType();
            }
        }

        return ConversionRank::NoMatch;
    }

    ConvertingOverloadResolver::ConversionRank
    ConvertingOverloadResolver::ComputeNumericConversionRank(Metadata::ElementType const pType,
                                                             Metadata::ElementType const aType)
    {
        Detail::Assert([&]{ return IsNumericElementType(pType) && IsNumericElementType(aType); });
        Detail::Assert([&]{ return pType != aType;                                             });

        if (IsIntegralElementType(pType) && IsIntegralElementType(aType))
        {
            // Signed -> Unsigned and Unsigned -> Signed conversions are not permitted.
            if (IsSignedIntegralElementType(pType) != IsSignedIntegralElementType(aType))
                return ConversionRank::NoMatch;

            // Narrowing conversions are not permitted.
            if (pType < aType)
                return ConversionRank::NoMatch;

            unsigned const rawConversionDistance(static_cast<unsigned>(pType) - static_cast<unsigned>(aType));
            Detail::Assert([&]{ return rawConversionDistance % 2 == 0; });
            unsigned const conversionDistance(rawConversionDistance / 2);

            return ConversionRank::IntegralPromotion | static_cast<ConversionRank>(conversionDistance);
        }

        // Real -> Integral conversions are not permitted.
        if (IsIntegralElementType(pType))
            return ConversionRank::NoMatch;

        // Integral -> Real conversion is permitted.
        if (IsIntegralElementType(aType))
            return ConversionRank::RealConversion;

        Detail::Assert([&]{ return IsRealElementType(pType) && IsRealElementType(aType); });

        // R8 -> R4 narrowing is not permitted.
        if (pType == Metadata::ElementType::R4 && aType == Metadata::ElementType::R8)
            return ConversionRank::NoMatch;

        // R4 -> R8 widening is permitted.
        return ConversionRank::RealConversion;
    }





    Metadata::ElementType ComputeOverloadElementType(Type const& type)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        // Shortcut:  If 'type' isn't from the system assembly, it isn't one of the system types:
        if (!Detail::IsSystemAssembly(type.GetAssembly()))
            return type.IsValueType() ? Metadata::ElementType::ValueType : Metadata::ElementType::Class;

        Detail::LoaderContext const& loader(type.GetAssembly().GetContext(InternalKey()).GetLoader());

        #define CXXREFLECT_GENERATE(A)                                                                        \
            if (Type(loader, loader.ResolveFundamentalType(Metadata::ElementType::A), InternalKey()) == type) \
            {                                                                                                 \
                return Metadata::ElementType::A;                                                              \
            }

        CXXREFLECT_GENERATE(Boolean)
        CXXREFLECT_GENERATE(Char)
        CXXREFLECT_GENERATE(I1)
        CXXREFLECT_GENERATE(U1)
        CXXREFLECT_GENERATE(I2)
        CXXREFLECT_GENERATE(U2)
        CXXREFLECT_GENERATE(I4)
        CXXREFLECT_GENERATE(U4)
        CXXREFLECT_GENERATE(I8)
        CXXREFLECT_GENERATE(U8)
        CXXREFLECT_GENERATE(R4)
        CXXREFLECT_GENERATE(R8)

        #undef CXXREFLECT_GENERATE

        return type.IsValueType() ? Metadata::ElementType::ValueType : Metadata::ElementType::Class;
    }





    I4 ConvertToI4(ResolvedVariantArgument const& argument)
    {
        return VerifyInRangeAndConvertTo<I4>(ConvertToI8(argument));
    }

    I8 ConvertToI8(ResolvedVariantArgument const& argument)
    {
        switch (argument.GetElementType())
        {
        case Metadata::ElementType::I1: return ReinterpretAs<I1>(argument);
        case Metadata::ElementType::I2: return ReinterpretAs<I2>(argument);
        case Metadata::ElementType::I4: return ReinterpretAs<I4>(argument);
        case Metadata::ElementType::I8: return ReinterpretAs<I8>(argument);
        default: throw LogicError(L"Unsupported conversion requested");
        }
    }

    U4 ConvertToU4(ResolvedVariantArgument const& argument)
    {
        return VerifyInRangeAndConvertTo<U4>(ConvertToU8(argument));
    }

    U8 ConvertToU8(ResolvedVariantArgument const& argument)
    {
        switch (argument.GetElementType())
        {
        case Metadata::ElementType::U1: return ReinterpretAs<U1>(argument);
        case Metadata::ElementType::U2: return ReinterpretAs<U2>(argument);
        case Metadata::ElementType::U4: return ReinterpretAs<U4>(argument);
        case Metadata::ElementType::U8: return ReinterpretAs<U8>(argument);
        default: throw LogicError(L"Unsupported conversion requested");
        }
    }

    R4 ConvertToR4(ResolvedVariantArgument const& argument)
    {
        return VerifyInRangeAndConvertTo<R4>(ConvertToR8(argument));
    }

    R8 ConvertToR8(ResolvedVariantArgument const& argument)
    {
        switch (argument.GetElementType())
        {
        case Metadata::ElementType::R4: return ReinterpretAs<R4>(argument);
        case Metadata::ElementType::R8: return ReinterpretAs<R8>(argument);
        default: throw LogicError(L"Unsupported conversion requested");
        }
    }

    IInspectable* ConvertToInterface(ResolvedVariantArgument const& argument, Guid const& interfaceGuid)
    {
        if (argument.GetElementType() != Metadata::ElementType::Class)
            throw LogicError(L"Invalid source argument:  argument must be a runtime class");

        IInspectable* const inspectableObject(ReinterpretAs<IInspectable*>(argument));

        // A nullptr argument is valid:
        if (inspectableObject == nullptr)
            return nullptr;

        ComPtr<IInspectable> inspectableInterface;
        HResult const queryResult(inspectableObject->QueryInterface(
            ToComGuid(interfaceGuid),
            reinterpret_cast<void**>(inspectableInterface.ReleaseAndGetAddressOf())));

        if (queryResult != 0)
            throw LogicError(L"Unsupported conversion requested:  interface not implemented");

        // Reference counting note:  our reference to the QI'ed interface pointer will be
        // released when this function returns.  In order for this to work, we rely on there
        // being One True IUnknown for the runtime object.  We are relying on the upstream
        // caller to keep a reference to the inspectable runtime object so that it is not
        // destroyed prematurely.
        return inspectableInterface.Get();
    }

    template <typename T>
    T ReinterpretAs(ResolvedVariantArgument const& argument)
    {
        if (Detail::Distance(argument.BeginValue(), argument.EndValue()) != sizeof(T))
            throw LogicError(L"Invalid reinterpretation target:  size does not match");

        // We pack arguments into the range without respecting alignment, so when we read them
        // back out here we must copy bytes individually to ensure we aren't accessing an
        // insufficiently aligned object.  (Strictly speaking this may not be required on our
        // target platforms, but better safe than sorry. :-D)
        T value;
        Detail::RangeCheckedCopy(argument.BeginValue(), argument.EndValue(),
                                 Detail::BeginBytes(value), Detail::EndBytes(value));
        return value;
    }

    template <typename TTarget, typename TSource>
    TTarget VerifyInRangeAndConvertTo(TSource const& value)
    {
        // Only widening conversions are permitted, so this check should never fail:
        if (value < static_cast<TSource>(std::numeric_limits<TTarget>::min()) ||
            value > static_cast<TSource>(std::numeric_limits<TTarget>::max()))
            throw LogicError(L"Unsupported conversion requested:  argument out of range");

        return static_cast<TTarget>(value);
    }





    void const* ComputeFunctionPointer(void const* const instance, unsigned const slot)
    {
        Detail::AssertNotNull(instance);

        // There are two levels of indirection to get to the function pointer:
        //
        //                  object            vtable
        //               +----------+      +----------+
        // instance ---> | vptr     | ---> | slot 0   |
        //               |~~~~~~~~~~|      | slot 1   |
        //                                 | slot 2   |
        //                                 |~~~~~~~~~~|
        //
        // This is fundamentally unsafe, so be very careful when calling. :-)
        return (*reinterpret_cast<void const* const* const*>(instance))[slot];
    }





    #if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X86 
    ConstByteIterator X86ArgumentFrame::Begin() const
    {
        return _data.data();
    }

    ConstByteIterator X86ArgumentFrame::End() const
    {
        return _data.data() + _data.size();
    }

    ConstByteIterator X86ArgumentFrame::GetData() const
    {
        return _data.data(); 
    }

    SizeType X86ArgumentFrame::GetSize() const
    {
        return Detail::ConvertInteger(_data.size());
    }

    // Aligns the end of the frame to an index evenly divisible by 'alignment'.
    void X86ArgumentFrame::AlignTo(SizeType const alignment)
    {
        if (_data.empty())
            return;

        SizeType const bytesToInsert(alignment - (_data.size() % alignment));
        _data.resize(_data.size() + bytesToInsert);
    }

    void X86ArgumentFrame::Push(ConstByteIterator const first, ConstByteIterator const last)
    {
        _data.insert(_data.end(), first, last);
    }





    void X86ArgumentConverter::ConvertAndInsert(Type                    const& parameterType,
                                                ResolvedVariantArgument const& argument,
                                                X86ArgumentFrame             & frame)
    {
        switch (ComputeOverloadElementType(parameterType))
        {
        case Metadata::ElementType::Boolean:
        {
            throw LogicError(L"Not yet implemented");
            break;
        }

        case Metadata::ElementType::Char:
        {
            throw LogicError(L"Not yet implemented");
            break;
        }

        case Metadata::ElementType::I1:
        case Metadata::ElementType::I2:
        case Metadata::ElementType::I4:
        {
            I4 const value(ConvertToI4(argument));
            frame.Push(Detail::BeginBytes(value), Detail::EndBytes(value));
            break;
        }

        case Metadata::ElementType::I8:
        {
            I8 const value(ConvertToI8(argument));
            frame.Push(Detail::BeginBytes(value), Detail::EndBytes(value));
            break;
        }

        case Metadata::ElementType::U1:
        case Metadata::ElementType::U2:
        case Metadata::ElementType::U4:
        {
            U4 const value(ConvertToU4(argument));
            frame.Push(Detail::BeginBytes(value), Detail::EndBytes(value));
            break;
        }

        case Metadata::ElementType::U8:
        {
            U8 const value(ConvertToU8(argument));
            frame.Push(Detail::BeginBytes(value), Detail::EndBytes(value));
            break;
        }

        case Metadata::ElementType::R4:
        {
            R4 const value(ConvertToR4(argument));
            frame.Push(Detail::BeginBytes(value), Detail::EndBytes(value));
            break;
        }

        case Metadata::ElementType::R8:
        {
            R8 const value(ConvertToR8(argument));
            frame.Push(Detail::BeginBytes(value), Detail::EndBytes(value));
            break;
        }

        case Metadata::ElementType::Class:
        {
            IInspectable* const value(ConvertToInterface(argument, GetGuid(parameterType)));
            frame.Push(Detail::BeginBytes(value), Detail::EndBytes(value));
            break;
        }

        case Metadata::ElementType::ValueType:
        {
            throw LogicError(L"Not yet implemented");
            break;
        }

        default:
        {
            throw LogicError(L"Element type not supported");
        }
        }
    }





    HResult X86StdCallInvoker::Invoke(Method              const& method,
                                      IInspectable             * instance,
                                      void                     * result,
                                      VariantArgumentPack const& arguments)
    {
        // We can only call a method defined by an interface implemented by the runtime type, so
        // we re-resolve the method against the interfaces of its declaring type.  If it has
        // already been resolved to an interface method, this is a no-op transformation.
        Method const interfaceMethod(FindMatchingInterfaceMethod(method));
        if (!interfaceMethod.IsInitialized())
            throw RuntimeError(L"Failed to find interface that defines method.");

        // Next, we need to compute the vtable slot of the method and QI to get the correct
        // interface pointer in order to obtain the function pointer.
        SizeType const methodSlot(ComputeMethodSlotIndex(interfaceMethod));
        auto const interfacePointer(QueryInterface(instance, interfaceMethod.GetDeclaringType()));
            
        // We compute the function pointer from the vtable.  '6' is the well-known offset of all
        // Windows Runtime interface methods (IUnknown has three functions, and IInspectable has
        // an additional three functions).
        void const* functionPointer(ComputeFunctionPointer(interfacePointer.get(), methodSlot + 6));

        // We construct the argument frame, by converting each argument to the correct type and
        // appending it to an array.  In stdcall, arguments are pushed onto the stack left-to-right.
        // Because the stack is upside-down (i.e., it grows top-to-bottom), we push the arguments
        // into our argument frame right-to-left.
        X86ArgumentFrame frame;

        // Every function is called via an interface pointer.  That is always the first argument:
        void const* const rawInterfacePointer(interfacePointer.get());
        frame.Push(Detail::BeginBytes(rawInterfacePointer), Detail::EndBytes(rawInterfacePointer));

        // Next, we iterate over the arguments and parameters, convert each argument to the correct
        // parameter type, and push the argument into the frame:
        auto pIt(method.BeginParameters());
        auto aIt(arguments.Begin());
        for (; pIt != method.EndParameters() && aIt != arguments.End(); ++pIt, ++aIt)
        {
            X86ArgumentConverter::ConvertAndInsert(pIt->GetType(), arguments.Resolve(*aIt), frame);
        }

        if (pIt != method.EndParameters() || aIt != arguments.End())
        {
            throw RuntimeError(L"Method arity does not match argument count");
        }

        // TODO We need to check the return type, but GetReturnType() is not yet implemented.
        // if (method.GetReturnType() != GetType(L"Platform", L"Void"))
        {
            frame.Push(Detail::BeginBytes(result), Detail::EndBytes(result));
        }
        // else if (result != nullptr)
        // {
        //     throw RuntimeError(L"Attempted to call a void-returning function with a result pointer");
        // }
            
        // Due to promotion and padding, all argument frames should have a size divisible by 4.
        // In order to avoid writing inline assembly to move the arguments frame onto the stack
        // and issue the call instruction, we have a set of function template instantiations
        // that handle invocation for us.
        switch (frame.GetSize())
        {
        case  4: return InvokeWithFrame< 4>(functionPointer, frame.Begin());
        case  8: return InvokeWithFrame< 8>(functionPointer, frame.Begin());
        case 12: return InvokeWithFrame<12>(functionPointer, frame.Begin());
        case 16: return InvokeWithFrame<16>(functionPointer, frame.Begin());
        case 20: return InvokeWithFrame<20>(functionPointer, frame.Begin());
        case 24: return InvokeWithFrame<24>(functionPointer, frame.Begin());
        case 28: return InvokeWithFrame<28>(functionPointer, frame.Begin());
        case 32: return InvokeWithFrame<32>(functionPointer, frame.Begin());
        case 36: return InvokeWithFrame<36>(functionPointer, frame.Begin());
        case 40: return InvokeWithFrame<40>(functionPointer, frame.Begin());
        case 44: return InvokeWithFrame<44>(functionPointer, frame.Begin());
        case 48: return InvokeWithFrame<48>(functionPointer, frame.Begin());
        case 52: return InvokeWithFrame<52>(functionPointer, frame.Begin());
        case 56: return InvokeWithFrame<56>(functionPointer, frame.Begin());
        case 60: return InvokeWithFrame<60>(functionPointer, frame.Begin());
        case 64: return InvokeWithFrame<64>(functionPointer, frame.Begin());
        }

        // If we hit this, we just need to add additional cases above.
        throw LogicError(L"Size of requested frame is out of range.");
    }

    template <SizeType FrameSize>
    HResult X86StdCallInvoker::InvokeWithFrame(void const* const functionPointer, ConstByteIterator const frameBytes)
    {
        struct FrameType { Byte _value[FrameSize]; };
        typedef HResult (__stdcall* FunctionPointer)(FrameType);

        FrameType const* const frame(reinterpret_cast<FrameType const*>(frameBytes));

        FunctionPointer typedFunctionPointer(reinterpret_cast<FunctionPointer>(functionPointer));

        return typedFunctionPointer(*frame);
    }
    #endif





    #if CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64
    void const* X64ArgumentFrame::GetArguments() const
    {
        return _arguments.data();
    }

    void const* X64ArgumentFrame::GetTypes() const
    {
        return _types.data();
    }

    std::uint64_t X64ArgumentFrame::GetCount() const
    {
        return _types.size();
    }
    
    void X64ArgumentFrame::Push(float const x)
    {
        static_assert(sizeof(float) == 4, "Attempted to push unaligned argument");
        _arguments.insert(_arguments.end(), Detail::BeginBytes(x), Detail::EndBytes(x));
        _arguments.resize(_arguments.size() + 4);
        _types.push_back(X64ArgumentType::SinglePrecisionReal);
    }

    void X64ArgumentFrame::Push(double const x)
    {
        static_assert(sizeof(double) == 8, "Attempted to push unaligned argument");
        _arguments.insert(_arguments.end(), Detail::BeginBytes(x), Detail::EndBytes(x));
        _types.push_back(X64ArgumentType::DoublePrecisionReal);
    }





    void X64ArgumentConverter::ConvertAndInsert(Type                    const& parameterType,
                                                ResolvedVariantArgument const& argument,
                                                X64ArgumentFrame             & frame)
    {
        switch (ComputeOverloadElementType(parameterType))
        {
        case Metadata::ElementType::Boolean:
        {
            throw LogicError(L"Not yet implemented");
            break;
        }

        case Metadata::ElementType::Char:
        {
            throw LogicError(L"Not yet implemented");
            break;
        }

        case Metadata::ElementType::I1:
        case Metadata::ElementType::I2:
        case Metadata::ElementType::I4:
        case Metadata::ElementType::I8:
        {
            I8 const value(ConvertToI8(argument));
            frame.Push(value);
            break;
        }

        case Metadata::ElementType::U1:
        case Metadata::ElementType::U2:
        case Metadata::ElementType::U4:
        case Metadata::ElementType::U8:
        {
            U8 const value(ConvertToU8(argument));
            frame.Push(value);
            break;
        }

        case Metadata::ElementType::R4:
        {
            R4 const value(ConvertToR4(argument));
            frame.Push(value);
            break;
        }

        case Metadata::ElementType::R8:
        {
            R8 const value(ConvertToR8(argument));
            frame.Push(value);
            break;
        }

        case Metadata::ElementType::Class:
        {
            IInspectable* const value(ConvertToInterface(argument, GetGuid(parameterType)));
            frame.Push(value);
            break;
        }

        case Metadata::ElementType::ValueType:
        {
            throw LogicError(L"Not yet implemented");
            break;
        }

        default:
        {
            throw LogicError(L"Element type not supported");
        }
        }
    }





    HResult X64FastCallInvoker::Invoke(Method              const& method,
                                       IInspectable             * instance,
                                       void                     * result,
                                       VariantArgumentPack const& arguments)
    {
        // We can only call a method defined by an interface implemented by the runtime type, so
        // we re-resolve the method against the interfaces of its declaring type.  If it has
        // already been resolved to an interface method, this is a no-op transformation.
        Method const interfaceMethod(FindMatchingInterfaceMethod(method));
        if (!interfaceMethod.IsInitialized())
            throw RuntimeError(L"Failed to find interface that defines method.");

        // Next, we need to compute the vtable slot of the method and QI to get the correct
        // interface pointer in order to obtain the function pointer.
        SizeType const methodSlot(ComputeMethodSlotIndex(interfaceMethod));
        auto const interfacePointer(QueryInterface(instance, interfaceMethod.GetDeclaringType()));
            
        // We compute the function pointer from the vtable.  '6' is the well-known offset of all
        // Windows Runtime interface methods (IUnknown has three functions, and IInspectable has
        // an additional three functions).
        void const* functionPointer(ComputeFunctionPointer(interfacePointer.get(), methodSlot + 6));

        // We construct the argument frame, by converting each argument to the correct type and
        // appending it to the frame, with basic type information for determining enregistration.
        X64ArgumentFrame frame;

        // Every function is called via an interface pointer.  That is always the first argument:
        void const* const rawInterfacePointer(interfacePointer.get());
        frame.Push(rawInterfacePointer);

        // Next, we iterate over the arguments and parameters, convert each argument to the correct
        // parameter type, and push the argument into the frame:
        auto pIt(method.BeginParameters());
        auto aIt(arguments.Begin());
        for (; pIt != method.EndParameters() && aIt != arguments.End(); ++pIt, ++aIt)
        {
            X64ArgumentConverter::ConvertAndInsert(pIt->GetType(), arguments.Resolve(*aIt), frame);
        }

        if (pIt != method.EndParameters() || aIt != arguments.End())
        {
            throw RuntimeError(L"Method arity does not match argument count");
        }

        // TODO We need to check the return type, but GetReturnType() is not yet implemented.
        // if (method.GetReturnType() != GetType(L"Platform", L"Void"))
        {
            frame.Push(result);
        }
        // else if (result != nullptr)
        // {
        //     throw RuntimeError(L"Attempted to call a void-returning function with a result pointer");
        // }
            
        return CxxReflectX64FastCallThunk(functionPointer, frame.GetArguments(), frame.GetTypes(), frame.GetCount());
    }
    #endif





    HResult ArmApcCallInvoker::Invoke(Method              const& method,
                                      IInspectable             * instance,
                                      void                     * result,
                                      VariantArgumentPack const& arguments)
    {
        throw LogicError(L"Not yet implemented");
    }





    SizeType ComputeMethodSlotIndex(Method const& method)
    {
        Detail::Assert([&]{ return method.IsInitialized(); });

        // TODO We should add this as a member function on 'method'.  We can trivially compute this
        // by determining the index of the method in its element context table.
        Type const type(method.GetReflectedType());

        SizeType slotIndex(0);
        for (auto it(type.BeginMethods(BindingAttribute::AllInstance)); it != type.EndMethods(); ++it)
        {
            if (*it == method)
                break;

            ++slotIndex;
        }

        return slotIndex;
    }

    Method FindMatchingInterfaceMethod(Method const& runtimeTypeMethod)
    {
        Detail::Assert([&]{ return runtimeTypeMethod.IsInitialized(); });

        BindingFlags const bindingFlags(BindingAttribute::Public | BindingAttribute::Instance);

        Type const runtimeType(runtimeTypeMethod.GetReflectedType());
        if (runtimeType.IsInterface())
            return runtimeTypeMethod;

        for (auto interfaceIt(runtimeType.BeginInterfaces()); interfaceIt != runtimeType.EndInterfaces(); ++interfaceIt)
        {
            for (auto methodIt(interfaceIt->BeginMethods(bindingFlags)); methodIt != interfaceIt->EndMethods(); ++methodIt)
            {
                if (methodIt->GetName() != runtimeTypeMethod.GetName())
                    continue;

                if (methodIt->GetReturnType() != runtimeTypeMethod.GetReturnType())
                    continue;

                if (!Detail::RangeCheckedEqual(
                        methodIt->BeginParameters(), methodIt->EndParameters(),
                        runtimeTypeMethod.BeginParameters(), runtimeTypeMethod.EndParameters()))
                    continue;

                return *methodIt;
            }
        }

        return Method();
    }





    UniqueInspectable CreateInspectableInstance(Type const& type, VariantArgumentPack const& arguments)
    {
        return CallWithInvocationConvention([&]
        {
            Detail::Verify([&]{ return type.IsInitialized() && arguments.GetArity() > 0; });

            // Get the activation factory for the type:
            Type const factoryType(GlobalLoaderContext::Get().GetActivationFactoryType(type));
            Guid const factoryGuid(GetGuid(factoryType));

            auto const factory(GetActivationFactoryInterface(type.GetFullName(), factoryGuid));

            if (factory == nullptr)
                throw RuntimeError(L"Failed to obtain activation factory for type");

            // Enumerate the candidate activation methods and perform overload resolution:
            auto const candidates(Detail::CreateStaticFilteredRange(
                factoryType.BeginMethods(BindingAttribute::AllInstance),
                factoryType.EndMethods(),
                [&](Method const& m) { return m.GetName() == L"CreateInstance" && m.GetReturnType() == type; }));

            ConvertingOverloadResolver const overloadResolver(candidates.Begin(), candidates.End(), arguments);

            if (!overloadResolver.Succeeded())
                throw RuntimeError(L"Failed to find activation method matching provided arguments");

            // Invoke the activation method to create the instance:
            ComPtr<IInspectable> newInstance;
            HResult result(CallInvoker::Invoke(
                overloadResolver.GetResult(),
                factory.get(),
                reinterpret_cast<void**>(newInstance.ReleaseAndGetAddressOf()),
                arguments));

            Detail::VerifySuccess(result, L"Failed to create instance of type");

            return WindowsRuntime::UniqueInspectable(newInstance.Detach());
        });
    }

} } }





namespace CxxReflect { namespace WindowsRuntime {

    InvocationError::InvocationError(StringReference const message)
        : RuntimeError(message.c_str())
    {
    }





    UniqueInspectable CreateInspectableInstance(Type const& type)
    {
        Detail::Verify([&]{ return type.IsInitialized(); });

        if (!type.IsClass())
            throw InvocationError(L"Type is not a reference type; only reference types may be created");

        Utility::SmartHString const typeFullName(type.GetFullName().c_str());

        ComPtr<IInspectable> instance;
        if (Detail::Failed(::RoActivateInstance(typeFullName.value(), instance.GetAddressOf())) || instance == nullptr)
            throw InvocationError(L"Failed to create instance of type");

        return UniqueInspectable(instance.Detach());
    }

} }

#endif
