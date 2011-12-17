//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/Handles.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace Detail {

    AssemblyHandle::AssemblyHandle()
    {
    }

    AssemblyHandle::AssemblyHandle(AssemblyContext const* context)
        : _context(context)
    {
        VerifyInitialized();
    }

    AssemblyHandle::AssemblyHandle(Assembly const& assembly)
        : _context(&assembly.GetContext(InternalKey()))
    {
        VerifyInitialized();
    }

    Assembly AssemblyHandle::Realize() const
    {
        VerifyInitialized();
        return Assembly(_context.Get(), InternalKey());
    }

    bool AssemblyHandle::IsInitialized() const
    {
        return _context.Get() != nullptr;
    }

    void AssemblyHandle::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }

    bool operator==(AssemblyHandle const& lhs, AssemblyHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (AssemblyHandle const& lhs, AssemblyHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    TypeHandle::TypeHandle()
    {
    }

    TypeHandle::TypeHandle(AssemblyContext            const* assemblyContext,
                           Metadata::ElementReference const& typeReference)
        : _assemblyContext(assemblyContext),
          _typeReference(typeReference)
    {
        VerifyInitialized();
    }

    TypeHandle::TypeHandle(Type const& type)
        : _assemblyContext(&type.GetAssembly().GetContext(InternalKey())),
          _typeReference(type.GetSelfReference(InternalKey()))
    {
        VerifyInitialized();
    }

    Type TypeHandle::Realize() const
    {
        VerifyInitialized();
        Assembly const assembly(_assemblyContext.Get(), InternalKey());
        return _typeReference.IsRowReference()
            ? Type(assembly, _typeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _typeReference.AsBlobReference(), InternalKey());
    }

    bool TypeHandle::IsInitialized() const
    {
        return _assemblyContext.Get() != nullptr && _typeReference.IsInitialized();
    }

    void TypeHandle::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }

    bool operator==(TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    MethodHandle::MethodHandle()
    {
    }

    MethodHandle::MethodHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                               Metadata::ElementReference const& reflectedTypeReference,
                               MethodContext              const* methodContext)
        : _reflectedTypeAssemblyContext(reflectedTypeAssemblyContext),
          _reflectedTypeReference(reflectedTypeReference),
          _methodContext(methodContext)
    {
        VerifyInitialized();
    }

    MethodHandle::MethodHandle(Method const& method)
        : _reflectedTypeAssemblyContext(&method.GetReflectedType().GetAssembly().GetContext(InternalKey())),
          _reflectedTypeReference(method.GetReflectedType().GetSelfReference(InternalKey())),
          _methodContext(&method.GetContext(InternalKey()))
    {
        VerifyInitialized();
    }

    Method MethodHandle::Realize() const
    {
        VerifyInitialized();
        Assembly const assembly(_reflectedTypeAssemblyContext.Get(), InternalKey());

        Type const reflectedType(_reflectedTypeReference.IsRowReference()
            ? Type(assembly, _reflectedTypeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _reflectedTypeReference.AsBlobReference(), InternalKey()));

        return Method(reflectedType, _methodContext.Get(), InternalKey());
    }

    bool MethodHandle::IsInitialized() const
    {
        return _reflectedTypeAssemblyContext.Get() != nullptr
            && _reflectedTypeReference.IsInitialized()
            && _methodContext.Get() != nullptr;
    }

    void MethodHandle::VerifyInitialized() const
    {
        Verify([&]{ return IsInitialized(); });
    }

    bool operator==(MethodHandle const& lhs, MethodHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (MethodHandle const& lhs, MethodHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }

} }
