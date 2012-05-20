#ifndef CXXREFLECT_PARAMETER_HPP_
#define CXXREFLECT_PARAMETER_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect { namespace Detail {

    /*
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
    */

} }

namespace CxxReflect {

    /// \ingroup cxxreflect_public_interface
    ///
    /// @{





    class Parameter
    {
    public:

        typedef void /* TODO */ OptionalCustomModifierIterator;
        typedef void /* TODO */ RequiredCustomModifierIterator;

        Parameter();

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

        OptionalCustomModifierIterator BeginOptionalCustomModifiers() const; // TODO
        OptionalCustomModifierIterator EndOptionalCustomModifiers()   const; // TODO

        RequiredCustomModifierIterator BeginRequiredCustomModifiers() const; // TODO
        RequiredCustomModifierIterator EndRequiredCustomModifiers()   const; // TODO

        // TODO GetCustomAttributes
        // TODO IsDefined

        bool IsInitialized() const;
        bool operator!()     const;

        friend bool operator==(Parameter const&, Parameter const&);
        friend bool operator< (Parameter const&, Parameter const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Parameter)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(Parameter)

        // -- The following members of System.Reflection.ParameterInfo are not implemented --
        // Member     Use GetDeclaringMethod;

    public: // Internal Members

        Parameter(Method                const& method,
                  Detail::ParameterData const& parameterData,
                  InternalKey);

        Parameter(Method                  const& method,
                  Metadata::RowReference  const& parameter,
                  Metadata::TypeSignature const& signature,
                  InternalKey);

        Metadata::RowReference  const& GetSelfReference(InternalKey) const;
        Metadata::TypeSignature const& GetSelfSignature(InternalKey) const;

    private:

        void AssertInitialized() const;

        Metadata::ParamRow GetParamRow() const;

        Detail::MethodHandle    _method;
        Metadata::RowReference  _parameter;
        Metadata::TypeSignature _signature;
    };

    /// @}

}

#endif
