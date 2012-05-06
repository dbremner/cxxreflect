#ifndef CXXREFLECT_FIELD_HPP_
#define CXXREFLECT_FIELD_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect {

    class Field
    {
    public:

        Field();

        Type       GetDeclaringType()    const;
        Type       GetReflectedType()    const;

        FieldFlags GetAttributes()       const;
        Type       GetType()             const;

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

        SizeType   GetMetadataToken()    const;

        Constant   GetConstantValue()    const;

        StringReference GetName() const;

        bool IsInitialized() const;
        bool operator!()     const;

        // Module

        // GetCustomAttributes
        // GetOptionalCustomModifiers
        // GetRawConstantValue
        // GetRequiredCustomModifiers
        
        

        // -- The following members of System.Reflection.FieldInfo are not implemented --
        // FieldHandle
        // GetValue()             N/A in reflection only
        // GetValueDirect()       N/A in reflection only
        // IsDefined()
        // IsSecurityCritical
        // IsSecuritySafeCritical
        // IsSecurityTransparent
        // MemberType
        // SetValue()             N/A in reflection only
        // SetValueDirect()       N/A in reflection only
        //
        // The 'FieldType' property has been named 'Type'

        friend bool operator==(Field const& lhs, Field const& rhs);
        friend bool operator< (Field const& lhs, Field const& rhs);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Field)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Field)

    public: // Internal Members

        Field(Type const& reflectedType, Detail::FieldContext const* context, InternalKey);

        Detail::FieldContext const& GetContext(InternalKey) const;

    private:

        void AssertInitialized() const;

        Metadata::FieldRow GetFieldRow() const;

        Detail::TypeHandle                                    _reflectedType;
        Detail::ValueInitialized<Detail::FieldContext const*> _context;
    };

}

#endif
