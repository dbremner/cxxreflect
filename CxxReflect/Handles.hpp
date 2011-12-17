//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Handles to represent each of the public interface entities.  The handle types are not related to
// each other and allow us to avoid a mess of inclusion dependencies between public headers.
#ifndef CXXREFLECT_HANDLES_HPP_
#define CXXREFLECT_HANDLES_HPP_

#include "CxxReflect/Core.hpp"
#include "CxxReflect/MetadataDatabase.hpp"
#include "CxxReflect/MetadataSignature.hpp"

namespace CxxReflect { namespace Detail {

    class AssemblyHandle
    {
    public:

        AssemblyHandle();
        AssemblyHandle(AssemblyContext const* context);
        AssemblyHandle(Assembly const& assembly);

        Assembly Realize() const;

        bool IsInitialized() const;

    private:

        void VerifyInitialized() const;

        ValueInitialized<AssemblyContext const*> _context;
    };

    bool operator==(AssemblyHandle const&, AssemblyHandle const&);
    bool operator< (AssemblyHandle const&, AssemblyHandle const&);





    class TypeHandle
    {
    public:

        TypeHandle();
        TypeHandle(AssemblyContext            const* assemblyContext,
                   Metadata::ElementReference const& typeReference);
        TypeHandle(Type const& type);

        Type Realize() const;

        bool IsInitialized() const;

    private:

        void VerifyInitialized() const;

        ValueInitialized<AssemblyContext const*> _assemblyContext;
        Metadata::ElementReference               _typeReference;
    };

    bool operator==(TypeHandle const&, TypeHandle const&);
    bool operator< (TypeHandle const&, TypeHandle const&);





    class MethodHandle
    {
    public:

        MethodHandle();
        MethodHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                     Metadata::ElementReference const& reflectedTypeReference,
                     MethodContext              const* methodContext);
        MethodHandle(Method const& method);

        Method Realize() const;

        bool IsInitialized() const;

    private:

        void VerifyInitialized() const;

        ValueInitialized<AssemblyContext const*> _reflectedTypeAssemblyContext;
        Metadata::ElementReference               _reflectedTypeReference;
        ValueInitialized<MethodContext const*>   _methodContext;
    };

    bool operator==(MethodHandle const&, MethodHandle const&);
    bool operator< (MethodHandle const&, MethodHandle const&);

} }

#endif
