//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Parameter::Parameter()
    {
    }

    Parameter::Parameter(Method                const& method,
                         Detail::ParameterData const& parameterData,
                         InternalKey)
        : _method(method), _parameter(parameterData.GetParameter()), _signature(parameterData.GetSignature())
    {
        VerifyInitialized();
    }

    ParameterFlags Parameter::GetAttributes() const
    {
        return GetParamRow().GetFlags();
    }

    bool Parameter::IsIn() const
    {
        return GetAttributes().IsSet(ParameterAttribute::In);
    }

    bool Parameter::IsLcid() const
    {
        Detail::VerifyFail("NYI");
        return false;
    }

    bool Parameter::IsOptional() const
    {
        return GetAttributes().IsSet(ParameterAttribute::Optional);
    }

    bool Parameter::IsOut() const
    {
        return GetAttributes().IsSet(ParameterAttribute::Out);
    }

    bool Parameter::IsRetVal() const
    {
        Detail::VerifyFail("NYI");
        return false;
    }

    Method Parameter::GetDeclaringMethod() const
    {
        VerifyInitialized();
        return _method.Realize();
    }

    SizeType Parameter::GetMetadataToken() const
    {
        VerifyInitialized();
        return _parameter.GetToken();
    }

    StringReference Parameter::GetName() const
    {
        return GetParamRow().GetName();
    }

    Type Parameter::GetType() const
    {
        VerifyInitialized();

        Assembly           const  assembly(_method.Realize().GetDeclaringType().GetAssembly());
        Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());

        // TODO It might be useful to create a helper for this in Database; we do it several places.
        ByteIterator const blobsStart(database.GetBlobs().Begin());
        SizeType     const signatureFirst(_signature.BeginBytes() - blobsStart);
        SizeType     const signatureLast (_signature.EndBytes()   - blobsStart);

        return Type(
            _method.Realize().GetDeclaringType().GetAssembly(),
            Metadata::BlobReference(signatureFirst, signatureLast - signatureFirst),
            InternalKey());
    }

    SizeType Parameter::GetPosition() const
    {
        return GetParamRow().GetSequence();
    }

    Metadata::RowReference  const& Parameter::GetSelfReference(InternalKey) const
    {
        VerifyInitialized();
        return _parameter;
    }

    Metadata::TypeSignature const& Parameter::GetSelfSignature(InternalKey) const
    {
        VerifyInitialized();
        return _signature;
    }

    Metadata::ParamRow Parameter::GetParamRow() const
    {
        VerifyInitialized();
        return _method.Realize()
            .GetDeclaringType()
            .GetAssembly()
            .GetContext(InternalKey())
            .GetDatabase()
            .GetRow<Metadata::TableId::Param>(_parameter);
    }

    bool Parameter::IsInitialized() const
    {
        return _method.IsInitialized() && _parameter.IsInitialized() && _signature.IsInitialized();
    }

    bool Parameter::operator!() const
    {
        return !IsInitialized();
    }

    void Parameter::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }

    bool operator==(Parameter const& lhs, Parameter const& rhs)
    {
        return lhs._method == rhs._method
            && lhs._parameter == rhs._parameter
            && lhs._signature.BeginBytes() == rhs._signature.BeginBytes();
    }

    bool operator<(Parameter const& lhs, Parameter const& rhs)
    {
        if (lhs._method < rhs._method)       { return true;  }
        if (lhs._method > rhs._method)       { return false; }
        if (lhs._parameter < rhs._parameter) { return true;  }
        if (lhs._parameter > rhs._parameter) { return false; }

        return std::less<ByteIterator>()(lhs._signature.BeginBytes(), rhs._signature.BeginBytes());
    }

}
