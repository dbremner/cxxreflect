
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Constant.hpp"

namespace CxxReflect { namespace { namespace Private {

    template <typename T>
    T CheckAndReadSinglePrimitive(Metadata::BlobReference const& blob)
    {
        if (Detail::Distance(blob.Begin(), blob.End()) != sizeof(T))
            throw RuntimeError(L"Attempted an invalid reinterpretation");

        return *reinterpret_cast<T const*>(blob.Begin());
    }

} } }

namespace CxxReflect {

    Constant::Constant()
    {
    }

    Constant::Constant(Metadata::FullReference const& constant, InternalKey)
        : _constant(constant)
    {
        Detail::Assert([&]{ return constant.IsRowReference();                                           });
        Detail::Assert([&]{ return constant.AsRowReference().GetTable() == Metadata::TableId::Constant; });
    }

    bool Constant::IsInitialized() const
    {
        return _constant.IsInitialized();
    }

    void Constant::AssertInitialized() const
    {
        Detail::Assert([&]{ return _constant.IsInitialized(); });
    }

    Metadata::ConstantRow Constant::GetConstantRow() const
    {
        AssertInitialized();

        return _constant.GetDatabase().GetRow<Metadata::TableId::Constant>(_constant);
    }

    Constant::Kind Constant::GetKind() const
    {
        if (!IsInitialized())
            return Kind::Unknown;

        switch (GetConstantRow().GetElementType())
        {
        case Metadata::ElementType::Boolean: return Kind::Boolean;
        case Metadata::ElementType::Char:    return Kind::Char;
        case Metadata::ElementType::I1:      return Kind::Int8;
        case Metadata::ElementType::U1:      return Kind::UInt8;
        case Metadata::ElementType::I2:      return Kind::Int16;
        case Metadata::ElementType::U2:      return Kind::UInt16;
        case Metadata::ElementType::I4:      return Kind::Int32;
        case Metadata::ElementType::U4:      return Kind::UInt32;
        case Metadata::ElementType::I8:      return Kind::Int64;
        case Metadata::ElementType::U8:      return Kind::UInt64;
        case Metadata::ElementType::R4:      return Kind::Float;
        case Metadata::ElementType::R8:      return Kind::Double;
        case Metadata::ElementType::String:  return Kind::String;
        case Metadata::ElementType::Class:   return Kind::Class;
        default:                             return Kind::Unknown;
        }
    }

    bool Constant::AsBoolean() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::uint8_t>(GetConstantRow().GetValue()) != 0;
    }

    wchar_t Constant::AsChar() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<wchar_t>(GetConstantRow().GetValue());
    }

    std::int8_t Constant::AsInt8() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::int8_t>(GetConstantRow().GetValue());
    }

    std::uint8_t Constant::AsUInt8() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::uint8_t>(GetConstantRow().GetValue());
    }

    std::int16_t Constant::AsInt16() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::int16_t>(GetConstantRow().GetValue());
    }

    std::uint16_t Constant::AsUInt16() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::uint16_t>(GetConstantRow().GetValue());
    }

    std::int32_t Constant::AsInt32() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::int32_t>(GetConstantRow().GetValue());
    }

    std::uint32_t Constant::AsUInt32() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::uint32_t>(GetConstantRow().GetValue());
    }

    std::int64_t Constant::AsInt64() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::int64_t>(GetConstantRow().GetValue());
    }

    std::uint64_t Constant::AsUInt64() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<std::uint64_t>(GetConstantRow().GetValue());
    }

    float Constant::AsFloat() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<float>(GetConstantRow().GetValue());
    }

    double Constant::AsDouble() const
    {
        AssertInitialized();

        return Private::CheckAndReadSinglePrimitive<double>(GetConstantRow().GetValue());
    }

    Constant Constant::For(Metadata::FullReference const& parent, InternalKey)
    {
        Metadata::ConstantRow const constantRow(Metadata::GetConstant(parent));
        return constantRow.IsInitialized()
            ? Constant(constantRow.GetSelfFullReference(), InternalKey())
            : Constant();
    }

}
