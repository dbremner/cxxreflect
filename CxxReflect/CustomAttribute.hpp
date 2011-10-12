//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CUSTOMATTRIBUTE_HPP_
#define CXXREFLECT_CUSTOMATTRIBUTE_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

namespace CxxReflect {

    class CustomAttributeArgument
    {
    public:

        CustomAttributeArgument(); // TODO

        // An argument is either positional or named; these are mutually exclusive.
        bool IsNamedArgument() const; // TODO
        bool IsPositionalArgument() const; // TODO

        // Gets the member that will be used to set the named argument; this is null for positional
        // arguments; it is only valid for named argumemnts.  It is usually a Property.
        MemberHandle GetMember() const; // TODO

        // Gets the type of the argument; this is valid for both named and positional arguments.
        TypeHandle GetArgumentType() const; // TODO

        // Gets the value of the argument; this is valid for both named and positional arguments.
        void /* TODO TYPE? */ GetValue() const; // TODO

    private:

        CXXREFLECT_NONCOPYABLE(CustomAttributeArgument);

    };

    bool operator==(CustomAttributeArgument const& lhs, CustomAttributeArgument const& rhs); // TODO
    bool operator< (CustomAttributeArgument const& lhs, CustomAttributeArgument const& rhs); // TODO

    inline bool operator!=(CustomAttributeArgument const& lhs, CustomAttributeArgument const& rhs) { return !(lhs == rhs); }
    inline bool operator> (CustomAttributeArgument const& lhs, CustomAttributeArgument const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(CustomAttributeArgument const& lhs, CustomAttributeArgument const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(CustomAttributeArgument const& lhs, CustomAttributeArgument const& rhs) { return !(rhs <  lhs); }

    class CustomAttribute
    {
    public:

        typedef CustomAttributeArgumentHandle ArgumentIterator;

        // 'token' names the mdCustomAttribute; 'assembly' names the scope in which 'token' can be
        // resolved (i.e., it owns the import we can use to call GetCustomAttributeProps).
        CustomAttribute(AssemblyHandle assembly, MetadataToken token);

        // Gets the constructor that will be used to apply the attribute to the type.
        MethodHandle GetConstructor() const
        {
            return PrivateGetConstructor();
        }

        ArgumentIterator BeginConstructorArguments() const
        {
            return PrivateGetConstructorArguments().Get();
        }

        ArgumentIterator EndConstructorArguments() const
        {
            return PrivateGetConstructorArguments().Get() + PrivateGetConstructorArguments().GetSize();
        }

        ArgumentIterator BeginNamedArguments() const
        {
            return PrivateGetNamedArguments().Get();
        }

        ArgumentIterator EndNamedArguments() const
        {
            return PrivateGetNamedArguments().Get() + PrivateGetNamedArguments().GetSize();
        }

    private:

        CXXREFLECT_NONCOPYABLE(CustomAttribute);

        enum RealizationState
        {
            RealizedConstructor,
            RealizedArguments
        };

        void RealizeConstructor() const;
        void RealizeArguments() const;

        MethodHandle& PrivateGetConstructor() const
        {
            RealizeConstructor();
            return _constructor;
        }

        Detail::AllocatorBasedArray<CustomAttributeArgument>& PrivateGetConstructorArguments() const
        {
            RealizeArguments();
            return _constructorArguments;
        }

        Detail::AllocatorBasedArray<CustomAttributeArgument>& PrivateGetNamedArguments() const
        {
            RealizeArguments();
            return _namedArguments;
        }

        // Always valid.
        AssemblyHandle _assembly;
        MetadataToken  _token;

        mutable Detail::FlagSet<RealizationState> _state;

        mutable MethodHandle _constructor;

        mutable Detail::AllocatorBasedArray<CustomAttributeArgument> _constructorArguments;
        mutable Detail::AllocatorBasedArray<CustomAttributeArgument> _namedArguments;
    };

    bool operator==(CustomAttribute const& lhs, CustomAttribute const& rhs); // TODO
    bool operator< (CustomAttribute const& lhs, CustomAttribute const& rhs); // TODO

    inline bool operator!=(CustomAttribute const& lhs, CustomAttribute const& rhs) { return !(lhs == rhs); }
    inline bool operator> (CustomAttribute const& lhs, CustomAttribute const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(CustomAttribute const& lhs, CustomAttribute const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(CustomAttribute const& lhs, CustomAttribute const& rhs) { return !(rhs <  lhs); }

}

#endif
