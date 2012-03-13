#ifndef CXXREFLECT_PROPERTY_HPP_
#define CXXREFLECT_PROPERTY_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect {

    class Property
    {
    public:

        Property();

        Type GetDeclaringType() const;
        Type GetReflectedType() const;

        bool IsInitialized() const;
        bool operator!()     const;

        // Attributes
        // CanRead
        // CanWrite
        // DeclaringType
        // IsSpecialName
        // MemberType
        // MetadataToken
        // Module
        // Name
        // PropertyType
        // ReflectedType

        // GetAccessors
        // GetConstantValue
        // GetCustomAttributes
        // GetGetMethod
        // GetIndexParameters
        // GetOptionalCustomModifiers
        // GetRawConstantValue
        // GetRequiredCustomModifiers
        // GetSetMethod
        // GetValue
        // IsDefined
        // SetValue

        // -- The following members of System.Reflection.PropertyInfo are not implemented --

        friend bool operator==(Property const& lhs, Property const& rhs);
        friend bool operator< (Property const& lhs, Property const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Property)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Property)

    public: // Internal Members

        Property(Type const& reflectedType, Detail::PropertyContext const* context, InternalKey);

        Detail::PropertyContext const& GetContext(InternalKey) const;

    private:

        void AssertInitialized() const;

        Metadata::PropertyRow GetPropertyRow() const;

        Detail::TypeHandle                                       _reflectedType;
        Detail::ValueInitialized<Detail::PropertyContext const*> _context;
    };

}

#endif
