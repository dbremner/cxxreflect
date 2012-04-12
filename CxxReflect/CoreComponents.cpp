//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/CoreComponents.hpp"
#include "CxxReflect/Event.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace Detail {

    AssemblyContext::AssemblyContext(Loader const* const loader, String uri, Metadata::Database&& database)
        : _loader(loader),
          _uri(std::move(uri)),
          _database(std::move(database))
    {
        AssertNotNull(_loader.Get());
        Assert([&]{ return !_uri.empty(); });
    }

    AssemblyContext::AssemblyContext(AssemblyContext&& other)
        : _loader    (std::move(other._loader    )),
          _uri       (std::move(other._uri       )),
          _database  (std::move(other._database  )),
          _name      (std::move(other._name      )),
          _state     (std::move(other._state     ))
    {
        other._loader.Get() = nullptr;
        other._state.Reset();
    }

    AssemblyContext& AssemblyContext::operator=(AssemblyContext&& other)
    {
        Swap(other);
        return *this;
    }

    void AssemblyContext::Swap(AssemblyContext& other)
    {
        using std::swap;
        swap(other._loader,     _loader    );
        swap(other._uri,        _uri       );
        swap(other._database,   _database  );
        swap(other._name,       _name      );
        swap(other._state,      _state     );
    }

    Loader const& AssemblyContext::GetLoader() const
    {
        AssertInitialized();
        return *_loader.Get();
    }

    Metadata::Database const& AssemblyContext::GetDatabase() const
    {
        AssertInitialized();
        return _database;
    }

    String const& AssemblyContext::GetLocation() const
    {
        AssertInitialized();
        return _uri;
    }

    AssemblyName const& AssemblyContext::GetAssemblyName() const
    {
        RealizeName();
        return *_name;
    }

    void AssemblyContext::RealizeName() const
    {
        if (_state.IsSet(RealizedName)) { return; }

        _name.reset(new AssemblyName(
            Assembly(this, InternalKey()),
            Metadata::RowReference(Metadata::TableId::Assembly, 0),
            InternalKey()));

        _state.Set(RealizedName);
    }

    bool AssemblyContext::IsInitialized() const
    {
        return _loader.Get() != nullptr;
    }

    void AssemblyContext::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }





    AssemblyHandle::AssemblyHandle()
    {
    }

    AssemblyHandle::AssemblyHandle(AssemblyContext const* context)
        : _context(context)
    {
        AssertInitialized();
    }

    AssemblyHandle::AssemblyHandle(Assembly const& assembly)
        : _context(&assembly.GetContext(InternalKey()))
    {
        AssertInitialized();
    }

    Assembly AssemblyHandle::Realize() const
    {
        AssertInitialized();
        return Assembly(_context.Get(), InternalKey());
    }

    bool AssemblyHandle::IsInitialized() const
    {
        return _context.Get() != nullptr;
    }

    void AssemblyHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(AssemblyHandle const& lhs, AssemblyHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (AssemblyHandle const& lhs, AssemblyHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    MethodHandle::MethodHandle()
    {
    }

    MethodHandle::MethodHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                               Metadata::ElementReference const& reflectedTypeReference,
                               MethodContext              const* context)
        : _reflectedTypeAssemblyContext(reflectedTypeAssemblyContext),
          _reflectedTypeReference(reflectedTypeReference),
          _context(context)
    {
        AssertInitialized();
    }

    MethodHandle::MethodHandle(Method const& method)
        : _reflectedTypeAssemblyContext(&method.GetReflectedType().GetAssembly().GetContext(InternalKey())),
          _reflectedTypeReference(method.GetReflectedType().GetSelfReference(InternalKey())),
          _context(&method.GetContext(InternalKey()))
    {
        AssertInitialized();
    }

    Method MethodHandle::Realize() const
    {
        AssertInitialized();
        Assembly const assembly(_reflectedTypeAssemblyContext.Get(), InternalKey());

        Type const reflectedType(_reflectedTypeReference.IsRowReference()
            ? Type(assembly, _reflectedTypeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _reflectedTypeReference.AsBlobReference(), InternalKey()));

        return Method(reflectedType, _context.Get(), InternalKey());
    }

    bool MethodHandle::IsInitialized() const
    {
        return _reflectedTypeAssemblyContext.Get() != nullptr
            && _reflectedTypeReference.IsInitialized()
            && _context.Get() != nullptr;
    }

    void MethodHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(MethodHandle const& lhs, MethodHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (MethodHandle const& lhs, MethodHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    ParameterHandle::ParameterHandle()
    {
    }

    ParameterHandle::ParameterHandle(AssemblyContext            const* reflectedTypeAssemblyContext,
                                     Metadata::ElementReference const& reflectedTypeReference,
                                     MethodContext              const* context,
                                     Metadata::RowReference     const& parameterReference,
                                     Metadata::TypeSignature    const& parameterSignature)
        : _reflectedTypeAssemblyContext(reflectedTypeAssemblyContext),
          _reflectedTypeReference(reflectedTypeReference),
          _context(context),
          _parameterReference(parameterReference),
          _parameterSignature(parameterSignature)
    {
        AssertInitialized();
    }

    ParameterHandle::ParameterHandle(Parameter const& parameter)
        : _reflectedTypeAssemblyContext(&parameter.GetDeclaringMethod().GetReflectedType().GetAssembly().GetContext(InternalKey())),
          _reflectedTypeReference(parameter.GetDeclaringMethod().GetReflectedType().GetSelfReference(InternalKey())),
          _context(&parameter.GetDeclaringMethod().GetContext(InternalKey())),
          _parameterReference(parameter.GetSelfReference(InternalKey())),
          _parameterSignature(parameter.GetSelfSignature(InternalKey()))
    {
        AssertInitialized();
    }

    Parameter ParameterHandle::Realize() const
    {
        AssertInitialized();
        Assembly const assembly(_reflectedTypeAssemblyContext.Get(), InternalKey());

        Type const reflectedType(_reflectedTypeReference.IsRowReference()
            ? Type(assembly, _reflectedTypeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _reflectedTypeReference.AsBlobReference(), InternalKey()));

        Method const declaringMethod(reflectedType, _context.Get(), InternalKey());

        /* TODO return Parameter(
            declaringMethod,
            ParameterData(_parameterReference, _parameterSignature, InternalKey()),
            InternalKey());*/

        return Parameter();
    }

    bool ParameterHandle::IsInitialized() const
    {
        return _reflectedTypeAssemblyContext.Get() != nullptr
            && _reflectedTypeReference.IsInitialized()
            && _context.Get() != nullptr
            && _parameterReference.IsInitialized()
            && _parameterSignature.IsInitialized();
    }

    void ParameterHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(ParameterHandle const& lhs, ParameterHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator<(ParameterHandle const& lhs, ParameterHandle const& rhs)
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
        AssertInitialized();
    }

    TypeHandle::TypeHandle(Type const& type)
        : _assemblyContext(&type.GetAssembly().GetContext(InternalKey())),
          _typeReference(type.GetSelfReference(InternalKey()))
    {
        AssertInitialized();
    }

    Type TypeHandle::Realize() const
    {
        AssertInitialized();
        Assembly const assembly(_assemblyContext.Get(), InternalKey());
        return _typeReference.IsRowReference()
            ? Type(assembly, _typeReference.AsRowReference(),  InternalKey())
            : Type(assembly, _typeReference.AsBlobReference(), InternalKey());
    }

    bool TypeHandle::IsInitialized() const
    {
        return _assemblyContext.Get() != nullptr && _typeReference.IsInitialized();
    }

    void TypeHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator< (TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    ParameterData::ParameterData()
    {
    }

    ParameterData::ParameterData(Metadata::RowReference                       const& parameter,
                                 Metadata::MethodSignature::ParameterIterator const& signature,
                                 InternalKey)
        : _parameter(parameter), _signature(signature)
    {
        AssertInitialized();
    }

    bool ParameterData::IsInitialized() const
    {
        return _parameter.IsInitialized();
    }

    void ParameterData::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    ParameterData& ParameterData::operator++()
    {
        AssertInitialized();

        ++_parameter;
        ++_signature;
        return *this;
    }

    ParameterData ParameterData::operator++(int)
    {
        ParameterData const it(*this);
        ++*this;
        return it;
    }

    bool operator==(ParameterData const& lhs, ParameterData const& rhs)
    {
        lhs.AssertInitialized();
        rhs.AssertInitialized();

        return lhs._parameter == rhs._parameter;
    }

    bool operator< (ParameterData const& lhs, ParameterData const& rhs)
    {
        lhs.AssertInitialized();
        rhs.AssertInitialized();

        return lhs._parameter < rhs._parameter;
    }

    Metadata::RowReference const& ParameterData::GetParameter() const
    {
        AssertInitialized();

        return _parameter;
    }

    Metadata::TypeSignature const& ParameterData::GetSignature() const
    {
        AssertInitialized();

        return *_signature;
    }

} }

namespace CxxReflect {

    IAssemblyLocator::~IAssemblyLocator()
    {
        // Virtual destructor required for interface class
    }

    ILoaderConfiguration::~ILoaderConfiguration()
    {
        // Virtual destructor required for interface class
    }

    bool Utility::IsSystemAssembly(Assembly const& assembly)
    {
        Detail::Assert([&]{ return assembly.IsInitialized(); });

        return assembly.GetReferencedAssemblyCount() == 0;
    }

    bool Utility::IsSystemType(Type            const& type,
                               StringReference const& systemTypeNamespace,
                               StringReference const& systemTypeSimpleName)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        return IsSystemAssembly(type.GetAssembly())
            && type.GetNamespace() == systemTypeNamespace
            && type.GetName()      == systemTypeSimpleName;
    }

    bool Utility::IsDerivedFromSystemType(Type                  const& type,
                                          Metadata::ElementType const  systemType,
                                          bool                         includeSelf)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        Type currentType(type);
        if (!includeSelf && currentType.IsInitialized())
            currentType = currentType.GetBaseType();

        Type const targetType(type.GetAssembly().GetContext(InternalKey()).GetLoader().GetFundamentalType(systemType, InternalKey()));

        while (currentType.IsInitialized())
        {
            if (currentType == targetType)
                return true;

            currentType = currentType.GetBaseType();
        }

        return false;
    }

    bool Utility::IsDerivedFromSystemType(Type            const& type,
                                          StringReference const& systemTypeNamespace,
                                          StringReference const& systemTypeSimpleName,
                                          bool                   includeSelf)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        Type currentType(type);
        if (!includeSelf && currentType)
            currentType = type.GetBaseType();

        while (currentType)
        {
            if (IsSystemType(currentType, systemTypeNamespace, systemTypeSimpleName))
                return true;

            currentType = currentType.GetBaseType();
        }

        return false;
    }

    Assembly Utility::GetSystemAssembly(Type const& referenceType)
    {
        Detail::Assert([&]{ return referenceType.IsInitialized(); });

        return GetSystemObjectType(referenceType).GetAssembly();
    }

    Assembly Utility::GetSystemAssembly(Assembly const& referenceAssembly)
    {
        Detail::Assert([&]{ return referenceAssembly.IsInitialized(); });

        return GetSystemObjectType(referenceAssembly).GetAssembly();
    }

    Type Utility::GetSystemObjectType(Type const& referenceType)
    {
        Detail::Assert([&]{ return referenceType.IsInitialized(); });

        Type currentType(referenceType);
        while (currentType.GetBaseType())
            currentType = currentType.GetBaseType();

        // This error is a hard verification because an ill-formed assembly might have a type not
        // derived from the One True Object.
        Detail::Verify([&]{ return currentType.GetName() == L"Object"; }); // TODO Namesapace
        Detail::Verify([&]{ return IsSystemAssembly(currentType.GetAssembly());   });

        return currentType;
    }

    Type Utility::GetSystemObjectType(Assembly const& referenceAssembly)
    {
        Detail::Assert([&]{ return referenceAssembly.IsInitialized(); });

        if (referenceAssembly.BeginTypes() != referenceAssembly.EndTypes())
            return GetSystemObjectType(*referenceAssembly.BeginTypes());

        Loader const& loader(referenceAssembly.GetContext(InternalKey()).GetLoader());

        auto const it(std::find_if(referenceAssembly.BeginReferencedAssemblyNames(),
                                   referenceAssembly.EndReferencedAssemblyNames(),
                                   [&](AssemblyName const& assemblyName) -> bool
        {
            Assembly const assembly(loader.LoadAssembly(assemblyName));
            Detail::Verify([&]{ return assembly.IsInitialized(); });

            return assembly.BeginTypes() != assembly.EndTypes();
        }));

        // This error is a hard verification because an ill-formed assembly might not reference a
        // system assembly (one that defines a One True Object).
        Detail::Verify([&]{ return it != referenceAssembly.EndReferencedAssemblyNames(); });

        Assembly const foundReferenceAssembly(loader.LoadAssembly(*it));
        return GetSystemObjectType(*foundReferenceAssembly.BeginTypes());
    }
}
