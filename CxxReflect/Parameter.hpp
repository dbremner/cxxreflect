//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_PARAMETER_HPP_
#define CXXREFLECT_PARAMETER_HPP_

#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataSignature.hpp"

namespace CxxReflect { namespace Detail {

    template <bool (Metadata::TypeSignature::CustomModifierIterator::*FFilter)()>
    class CustomModifierIterator
    {
    public:

        typedef std::forward_iterator_tag                       iterator_category;
        typedef Type                                            value_type;
        typedef Type                                            reference;
        typedef Dereferenceable<Type>                           pointer;
        typedef std::ptrdiff_t                                  difference_type;

        typedef value_type                                      ValueType;
        typedef reference                                       Reference;
        typedef pointer                                         Pointer;
        typedef Metadata::TypeSignature::CustomModifierIterator InnerIterator;

        CustomModifierIterator()
        {
        }

        CustomModifierIterator(Method const& method, InnerIterator const& current)
            : _method(method), _current(current)
        {
        }

    private:

        enum Kind : std::uint8_t
        {
            Optional,
            Required
        };

        void AdvanceCurrentToNext()
        {
            while ((_kind.Get() == Optional ? _current->IsOptional() : _current->IsRequired())
                && (_current != InnerIterator()))
            {
                ++_current;
            }
        }

        ValueInitialized<Kind> _kind;
        InnerIterator          _current;
    };

} }

namespace CxxReflect {

    class Parameter
    {
    public:

        typedef void /* TODO */ OptionalCustomModifierIterator;
        typedef void /* TODO */ RequiredCustomModifierIterator;

        Parameter();

        Parameter(Method                  const& method,
                  Metadata::RowReference  const& parameter,
                  Metadata::TypeSignature const& signature);

        ParameterFlags GetAttributes() const;

        bool IsIn()       const;
        bool IsLcid()     const;
        bool IsOptional() const;
        bool IsOut()      const;
        bool IsRetVal()   const;

        Method GetDeclaringMethod() const;

        SizeType        GetMetadataToken() const;

        StringReference GetName()          const;

        Type            GetType()          const;
        SizeType        GetPosition()      const;

        // TODO DefaultValue
        // TODO RawDefaultValue

        OptionalCustomModifierIterator BeginOptionalCustomModifiers() const;
        OptionalCustomModifierIterator EndOptionalCustomModifiers()   const;

        RequiredCustomModifierIterator BeginRequiredCustomModifiers() const;
        RequiredCustomModifierIterator EndRequiredCustomModifiers()   const;

        // TODO GetCustomAttributes
        // TODO IsDefined

        bool IsInitialized() const;

        bool operator!() const;

        friend bool operator==(Parameter const& lhs, Parameter const& rhs); // TODO
        friend bool operator< (Parameter const& lhs, Parameter const& rhs); // TODO

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Parameter)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Parameter)

        // -- The following members of System.Reflection.ParameterInfo are not implemented --
        // Member     Use GetDeclaringMethod;

    private:

        void VerifyInitialized() const;

        Method                  _method;
        Metadata::RowReference  _parameter;
        Metadata::TypeSignature _signature;
    };

}

#endif
