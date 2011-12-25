//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_FIELD_HPP_
#define CXXREFLECT_FIELD_HPP_

#include "CxxReflect/CoreInternals.hpp"

namespace CxxReflect {

    class Field
    {
    public:

        Field();
        Field(Type const& reflectedType, Detail::FieldContext const* context, InternalKey);

        Type       GetDeclaringType()    const;
        Type       GetReflectedType()    const;

        FieldFlags GetAttributes()       const;
        Type       GetFieldType()        const;
        bool       IsAssembly()          const;
        bool       IsFamily()            const;
        bool       IsFamilyAndAssembly() const;
        bool       IsFamilyOrAssembly()  const;
        bool       IsInitOnly()          const;
        bool       IsLiteral()           const;
        bool       IsNotSerialized()     const;
        bool       IsPinvokeImpl()       const;
        bool       IsPrivate()           const;
        bool       IsPublic()            const;
        bool       IsSpecialName()       const;
        bool       IsStatic()            const;

        SizeType GetMetadataToken() const;

        StringReference GetName() const;

        bool IsInitialized() const;
        bool operator!()     const;

        // Module

        // GetCustomAttributes
        // GetOptionalCustomModifiers
        // GetRawConstantValue
        // GetRequiredCustomModifiers
        // GetValue
        // GetValueDirect
        // IsDefined
        // SetValue
        // SetValueDirect

        // UNIMPLEMENTED:
        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent

        friend bool operator==(Field const& lhs, Field const& rhs);
        friend bool operator< (Field const& lhs, Field const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Field)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Field)

    public: // Internal Members

        Detail::FieldContext const& GetContext(InternalKey) const;

    private:

        void VerifyInitialized() const;

        Metadata::FieldRow GetFieldRow() const;

        Detail::TypeHandle                                    _reflectedType;
        Detail::ValueInitialized<Detail::FieldContext const*> _context;

    };

}

#endif
