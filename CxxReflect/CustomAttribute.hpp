//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CUSTOMATTRIBUTE_HPP_
#define CXXREFLECT_CUSTOMATTRIBUTE_HPP_

#include "CxxReflect/CoreInternals.hpp"

namespace CxxReflect {

    class CustomAttribute
    {
    public:

        typedef void /* TODO */ PositionalArgumentIterator;
        typedef void /* TODO */ NamedArgumentIterator;

        CustomAttribute();
        CustomAttribute(Assembly const& assembly, Metadata::RowReference const& customAttribute, InternalKey);

        SizeType GetMetadataToken() const;

        // TODO Implement some sort of GetParent() functionality.

        Method GetConstructor() const;

        PositionalArgumentIterator BeginPositionalArguments() const;
        PositionalArgumentIterator EndPositionalArguments()   const;

        NamedArgumentIterator BeginNamedArguments() const;
        NamedArgumentIterator EndNamedArguments()   const;

        bool IsInitialized() const;

        bool operator!() const { return !IsInitialized(); }

        friend bool operator==(CustomAttribute const&, CustomAttribute const&);
        friend bool operator< (CustomAttribute const&, CustomAttribute const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(CustomAttribute)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(CustomAttribute)

    public: // Internals

        static CustomAttributeIterator BeginFor(Assembly const& assembly, Metadata::RowReference const& parent, InternalKey);
        static CustomAttributeIterator EndFor  (Assembly const& assembly, Metadata::RowReference const& parent, InternalKey);

    private:

        void AssertInitialized() const;

        // The value, if non-null, is resolved in the same scope as the parent.
        Metadata::FullReference _parent;
        Metadata::RowReference  _attribute;
        Detail::MethodHandle    _constructor;
    };

}

#endif
