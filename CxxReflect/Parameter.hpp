//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_PARAMETER_HPP_
#define CXXREFLECT_PARAMETER_HPP_

#include "CxxReflect/Core.hpp"

namespace CxxReflect {

    class Parameter
        : public Detail::SafeBoolConvertible<Parameter>,
          public Detail::EqualityComparable<Parameter>,
          public Detail::RelationalComparable<Parameter>
    {
    public:

        typedef void /* TODO */ OptionalCustomModifierIterator;
        typedef void /* TODO */ RequiredCustomModifierIterator;

        ParameterFlags GetAttributes() const;
        // DefaultValue
        bool IsIn()       const;
        bool IsLcid()     const;
        bool IsOptional() const;
        bool IsOut()      const;
        bool IsRetVal()   const;

        Method GetDeclaringMethod() const;

        std::uint32_t GetMetadataToken() const;

        StringReference GetName()     const;

        Type GetType();
        SizeType        GetPosition() const;
        // RawDefaultValue

        OptionalCustomModifierIterator BeginOptionalCustomModifiers() const;
        OptionalCustomModifierIterator EndOptionalCustomModifiers()   const;

        RequiredCustomModifierIterator BeginRequiredCustomModifiers() const;
        RequiredCustomModifierIterator EndRequiredCustomModifiers()   const;

        // GetCustomAttributes
        // IsDefined

        friend bool operator==(Parameter const& lhs, Parameter const& rhs); // TODO
        friend bool operator< (Parameter const& lhs, Parameter const& rhs); // TODO

        // -- The following members of System.Reflection.ParameterInfo are not implemented --
        // Member     Use GetDeclaringMethod;

    private:

        Parameter(Parameter const&);
        Parameter& operator=(Parameter const&);

    };

}

#endif
