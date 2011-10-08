//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header contains private implementation details only.  
#ifndef CXXREFLECT_INTERNALCXXREFLECTIMPL_HPP_
#define CXXREFLECT_INTERNALCXXREFLECTIMPL_HPP_

#include "CxxReflect/CxxReflect.hpp"
#include "CxxReflect/InternalCorEnumIterator.hpp"
#include "CxxReflect/InternalUtility.hpp"

#include <atlbase.h>
#include <cor.h>

#include <map>

namespace CxxReflect { namespace Detail {

    class EventImpl
    {
    public:

    private:

    };

    class FieldImpl
    {
    public:

    private:

    };

    class MethodImpl
    {
    public:

    private:

    };

    class PropertyImpl
    {
    public:

    private:
    };

    class TypeImpl
    {
    public:

        TypeImpl(AssemblyImpl const* assembly, mdTypeDef token)
            : assembly_(assembly),
              token_(token),
              realizedTypeDefProps_(false),
              baseType_(nullptr),
              resolvedBaseType_(false),
              enclosingType_(nullptr)
        {
            RealizeTypeDefProps();
        }

        String GetAssemblyQualifiedName() const;

        TypeDefToken GetMetadataToken() const
        {
            return token_;
        }

        String GetName() const
        {
            RealizeTypeDefProps();
            std::size_t indexOfLastDot(typeName_.find_last_of(L'.', std::wstring::npos));
            return indexOfLastDot != std::wstring::npos
                ? typeName_.substr(indexOfLastDot + 1, typeName_.size() - indexOfLastDot - 1)
                : typeName_;
        }

        String GetNamespace() const
        {
            String fullName(GetFullName());
            std::size_t indexOfLastDot(fullName.find_last_of(L'.', std::wstring::npos));
            return indexOfLastDot != std::wstring::npos
                ? fullName.substr(0, indexOfLastDot)
                : L"";
        }

        String GetFullName() const
        {
            RealizeEnclosingType();
            String fullName;
            if (enclosingType_ != nullptr)
            {
                fullName += enclosingType_->GetFullName() + L"+";
            }
            fullName += typeName_;
           
            return fullName;
        }

        TypeImpl const* GetBaseType() const { ResolveBaseType(); return baseType_; }

        bool IsAbstract()              const { return (flags_ & tdAbstract) != 0;                            }
        bool IsArray()                 const { return false; } // TODO
        bool IsAutoClass()             const { return (flags_ & tdAutoClass) != 0;                           }
        bool IsAutoLayout()            const { return (flags_ & tdLayoutMask) == tdAutoLayout;               }
        bool IsByRef()                 const { return false; } // TODO
        bool IsClass()                 const { return !IsInterface() && !IsValueType();                      }
        bool IsCOMObject()             const { return (flags_ & tdImport) != 0 && !IsInterface();                              }
        bool IsContextful()            const { return IsDerivedFromSystemType(L"System.ContextBoundObject", true); }

        bool IsEnum()                  const { return IsDerivedFromSystemType(L"System.Enum", false);               }
        bool IsExplicitLayout()        const { return (flags_ & tdLayoutMask) == tdExplicitLayout;           }
        bool IsGenericParameter()      const { return false; } // TODO
        bool IsGenericType()           const { return IsGenericTypeDefinition(); } // TODO
        bool IsGenericTypeDefinition() const
        {
            return GetFullName().find_last_of(L'`', std::wstring::npos) != std::wstring::npos; // TODO
        } // TODO
        bool IsImport()                const { return (flags_ & tdImport) != 0;                              }
        bool IsInterface()             const { return (flags_ & tdClassSemanticsMask) == tdInterface;        }
        bool IsLayoutSequential()      const { return (flags_ & tdLayoutMask) == tdSequentialLayout;         }

        bool IsMarshalByRef()          const { return IsDerivedFromSystemType(L"System.MarshalByRefObject", true); }
        bool IsNested()                const { return (flags_ & tdVisibilityMask) >= tdNestedPublic;         }
        bool IsNestedAssembly()        const { return (flags_ & tdVisibilityMask) == tdNestedAssembly;       }
        bool IsNestedFamANDAssem()     const { return (flags_ & tdVisibilityMask) == tdNestedFamANDAssem;    }
        bool IsNestedFamily()          const { return (flags_ & tdVisibilityMask) == tdNestedFamily;         }
        bool IsNestedPrivate()         const { return (flags_ & tdVisibilityMask) == tdNestedPrivate;        }
        bool IsNestedPublic()          const { return (flags_ & tdVisibilityMask) == tdNestedPublic;         }
        bool IsNotPublic()             const { return (flags_ & tdVisibilityMask) == tdNotPublic;            }

        bool IsPointer()               const { return false; } // TODO
        bool IsPrimitive()             const;
        bool IsPublic()                const { return (flags_ & tdVisibilityMask) == tdPublic;               }
        bool IsSealed()                const { return (flags_ & tdSealed) != 0;                              }
        bool IsSecurityCritical()      const { return false; } // TODO
        bool IsSecuritySafeCritical()  const { return false; } // TODO
        bool IsSecurityTransparent()   const { return false; } // TODO
        bool IsSerializable()          const
        {
            return (flags_ & tdSerializable) != 0
                || IsEnum()
                || IsDerivedFromSystemType(L"System.MulticastDelegate", false);
        }

        bool IsSpecialName()           const { return (flags_ & tdRTSpecialName) != 0;                       }
        bool IsUnicodeClass()          const { return (flags_ & tdStringFormatMask) == tdUnicodeClass;       }
        bool IsValueType()             const;

        bool IsVisible() const
        { 
            if (IsPublic())
                return true;

            if (!IsNestedPublic())
                return false;

            RealizeEnclosingType();
            return enclosingType_->IsVisible();
        }

        AssemblyImpl GetAssembly() const;

    private:

        bool IsDerivedFromSystemType(String const& typeName, bool includeSelf) const;

        void RealizeTypeDefProps() const;

        void RealizeEnclosingType() const;

        void ResolveBaseType() const;

        AssemblyImpl const* assembly_;
        mutable TypeDefToken token_;

        mutable bool realizedTypeDefProps_;
        mutable String typeName_;
        mutable unsigned flags_;
        mutable MetadataToken baseToken_;

        mutable bool resolvedBaseType_;
        mutable TypeImpl const* baseType_;

        mutable TypeImpl const* enclosingType_; // For nested classes

        enum RealizationFlags
        {
            EventsRealized,
            FieldsRealized,
            MethodsRealized,
            PropertiesRealized,
            EnclosingTypeRealized
        };

        mutable FlagSet<RealizationFlags> realizationState_;

        mutable std::vector<EventImpl>    events_;
        mutable std::vector<FieldImpl>    fields_;
        mutable std::vector<MethodImpl>   methods_;
        mutable std::vector<PropertyImpl> properties_;
    };

    class AssemblyImpl
    {
    public:

        AssemblyImpl(MetadataReaderImpl const* metadataReader, String const& path, IMetaDataImport2* import);

        AssemblyName const& GetName() const { RealizeName(); return name_; }

        AssemblySequence GetReferencedAssemblies() const
        {
            RealizeReferencedAssemblies();
            AssemblySequence referencedAssemblies;
            std::transform(referencedAssemblies_.begin(), referencedAssemblies_.end(), std::back_inserter(referencedAssemblies), [](AssemblyImpl const* impl) { return Assembly(impl); });
            return referencedAssemblies;
        }

        TypeSequence GetTypes() const;

        Type AssemblyImpl::GetType(Detail::String const& name, bool throwOnError = false, bool ignoreCase = false) const;

        TypeImpl const* ResolveTypeDef(TypeDefToken typeDef) const
        {
            auto it(std::find_if(types_.begin(), types_.end(), [&](TypeImpl const& t)
            {
                return t.GetMetadataToken() == typeDef;
            }));

            return it != types_.end() ? &*it : nullptr;
        }

        bool IsSystemAssembly() const
        {
            RealizeReferencedAssemblies();
            return referencedAssemblies_.empty();
        }

        IMetaDataImport2* UnsafeGetImport() const
        {
            return import_;
        }

    private:

        enum RealizationFlags
        {
            NameRealized                 = 0x01,
            ReferencedAssembliesRealized = 0x02,
            TypesRealized                = 0x04
        };

        void RealizeName() const;
        void RealizeReferencedAssemblies() const;
        void RealizeTypes() const;

        
            
        String path_;
        MetadataReaderImpl const* metadataReader_;
        CComPtr<IMetaDataImport2> import_;

        mutable FlagSet<RealizationFlags> state_;

        mutable AssemblyName name_;
        mutable std::vector<AssemblyImpl const*> referencedAssemblies_;

        // The types defined in this assembly, sorted by token
        mutable std::vector<TypeImpl> types_;
    };

    class MetadataReaderImpl : public RefCounted
    {
    public:

        MetadataReaderImpl(std::unique_ptr<IReferenceResolver> referenceResolver);

        Assembly GetAssembly(String const& path) const;

        Assembly GetAssemblyByName(AssemblyName const& name) const;

        IMetaDataDispenserEx* UnsafeGetDispenser() const
        {
            return dispenser_;
        }

    private:

        mutable std::unique_ptr<IReferenceResolver> referenceResolver_;
        mutable CComPtr<IMetaDataDispenserEx> dispenser_;
        mutable std::map<std::wstring, AssemblyImpl> assemblies_;
    };

    template <typename T>
    class OpaqueIteratorUnderlyingType;

    template <> class OpaqueIteratorUnderlyingType<Assembly>  { typedef AssemblyImpl  Type; };
    template <> class OpaqueIteratorUnderlyingType<Module>    { typedef ModuleImpl    Type; };
    template <> class OpaqueIteratorUnderlyingType<Type>      { typedef TypeImpl      Type; };
    template <> class OpaqueIteratorUnderlyingType<Event>     { typedef EventImpl     Type; };
    template <> class OpaqueIteratorUnderlyingType<Field>     { typedef FieldImpl     Type; };
    template <> class OpaqueIteratorUnderlyingType<Method>    { typedef MethodImpl    Type; };
    template <> class OpaqueIteratorUnderlyingType<Property>  { typedef PropertyImpl  Type; };
    template <> class OpaqueIteratorUnderlyingType<Parameter> { typedef ParameterImpl Type; };

    template <typename T>
    template <typename U>
    OpaqueIterator<T>::OpaqueIterator(U const* element)
        : element_(element)
    {
        static_assert(std::is_same<U, OpaqueIteratorUnderlyingType<T>>::value, "Invalid instantiation");
    }

    template <typename T>
    typename OpaqueIterator<T>::pointer OpaqueIterator<T>::operator->() const
    {

    }

    inline MetadataReaderImpl::MetadataReaderImpl(std::unique_ptr<IReferenceResolver> referenceResolver)
        : referenceResolver_(std::move(referenceResolver))
    {
        ThrowOnFailure(CoCreateInstance(
            CLSID_CorMetaDataDispenser,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IMetaDataDispenserEx,
            reinterpret_cast<void**>(&dispenser_)));
    }

    inline Assembly MetadataReaderImpl::GetAssembly(String const& path) const
    {
        auto const it(assemblies_.find(path));
        if (it != assemblies_.end())
        {
            return Assembly(&it->second);
        }

        CComPtr<IMetaDataImport2> import;
        dispenser_->OpenScope(
            path.c_str(),
            ofReadOnly,
            IID_IMetaDataImport2,
            reinterpret_cast<IUnknown**>(&import));

        auto const result(assemblies_.insert(std::make_pair(path, AssemblyImpl(this, path, import))));
        return Assembly(&result.first->second);
    }

    inline Assembly MetadataReaderImpl::GetAssemblyByName(AssemblyName const& name) const
    {
        return GetAssembly(referenceResolver_->ResolveAssembly(name));
    }

    inline AssemblyImpl::AssemblyImpl(MetadataReaderImpl const* metadataReader,
                               String const& path,
                               IMetaDataImport2* import)
        : path_(path), metadataReader_(metadataReader), import_(import)
    {
    }

    inline TypeSequence AssemblyImpl::GetTypes() const
    {
        RealizeTypes();

        TypeSequence result;
        std::transform(types_.begin(), types_.end(), std::back_inserter(result), [&](TypeImpl const& impl)
        {
            return Type(&impl);
        });
        return result;
    }

    inline Type AssemblyImpl::GetType(Detail::String const& name, bool throwOnError, bool ignoreCase) const
    {
        int (*compare)(wchar_t const*, wchar_t const*) = ignoreCase ? _wcsicmp : wcscmp;

        auto it(std::find_if(types_.begin(), types_.end(), [&](TypeImpl const& impl)
        {
            return compare(impl.GetFullName().c_str(), name.c_str()) == 0;
        }));

        if (it == types_.end())
        {
            return !throwOnError ? Type(nullptr) : throw std::logic_error("wtf");
        }
        else
        {
            return Type(&*it);
        }
    }

    inline void AssemblyImpl::RealizeName() const
    {
        if (state_.IsSet(NameRealized)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(import_);
        VerifyNotNull(assemblyImport);

        mdAssembly assemblyToken;
        ThrowOnFailure(assemblyImport->GetAssemblyFromScope(&assemblyToken));
        name_ = GetAssemblyNameFromToken(assemblyImport, assemblyToken);

        state_.Set(NameRealized);
    }

    inline void AssemblyImpl::RealizeReferencedAssemblies() const
    {
        if (state_.IsSet(ReferencedAssembliesRealized)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(import_);

        using Detail::AssemblyRefIterator;
        std::transform(AssemblyRefIterator(assemblyImport),
                       AssemblyRefIterator(),
                       std::back_inserter(referencedAssemblies_),
                       [&](mdAssemblyRef token) -> AssemblyImpl const*
        {
            AssemblyName name(GetAssemblyNameFromToken(assemblyImport, token));
            Assembly assembly(metadataReader_->GetAssemblyByName(name));
            return assembly.impl_;
        });

        state_.Set(ReferencedAssembliesRealized);
    }

    inline void AssemblyImpl::RealizeTypes() const
    {
        if (state_.IsSet(TypesRealized)) { return; }

        std::transform(TypeDefIterator(import_), TypeDefIterator(), std::back_inserter(types_), [&](mdTypeDef token)
        {
            return TypeImpl(this, token);
        });

        std::sort(types_.begin(), types_.end(), [](TypeImpl const& lhs, TypeImpl const& rhs)
        {
            return lhs.GetMetadataToken() < rhs.GetMetadataToken();
        });

        types_.erase(std::unique(types_.begin(), types_.end(), [](TypeImpl const& lhs, TypeImpl const& rhs)
        {
            return lhs.GetMetadataToken() == rhs.GetMetadataToken(); 
        }), types_.end());

        state_.Set(TypesRealized);
    }
    
    inline String TypeImpl::GetAssemblyQualifiedName() const
    {
        return GetFullName() + L", " + assembly_->GetName().GetFullName();
    }

    inline void TypeImpl::RealizeEnclosingType() const
    {
        if (realizationState_.IsSet(EnclosingTypeRealized))
            return;

        RealizeTypeDefProps();

        if (IsTdNested(flags_))
        {
            mdTypeDef enclosingType;
            ThrowOnFailure(assembly_->UnsafeGetImport()->GetNestedClassProps(token_.Get(), &enclosingType));

            enclosingType_ = assembly_->ResolveTypeDef(enclosingType);
        }
        
        realizationState_.Set(EnclosingTypeRealized);
    }

    inline void TypeImpl::RealizeTypeDefProps() const
    {
        if (realizedTypeDefProps_)
            return;

        IMetaDataImport2* import(assembly_->UnsafeGetImport());
        std::array<wchar_t, 512> nameBuffer;
        ULONG count;
        DWORD flags;
        mdToken extends;
        ThrowOnFailure(import->GetTypeDefProps(token_.Get(), nameBuffer.data(), nameBuffer.size(), &count, &flags, &extends));
        
        typeName_ = String(nameBuffer.begin(), nameBuffer.begin() + count - 1);
        flags_ = flags;
        baseToken_ = extends;
        realizedTypeDefProps_ = true;
    }

    inline bool TypeImpl::IsPrimitive() const
    {
        if (!assembly_->IsSystemAssembly())
            return false;

        String fullName(GetFullName());
        return fullName == L"System.Boolean"
            || fullName == L"System.Byte"
            || fullName == L"System.SByte"
            || fullName == L"System.Int16"
            || fullName == L"System.UInt16"
            || fullName == L"System.Int32"
            || fullName == L"System.UInt32"
            || fullName == L"System.Int64"
            || fullName == L"System.UInt64"
            || fullName == L"System.IntPtr"
            || fullName == L"System.UIntPtr"
            || fullName == L"System.Char"
            || fullName == L"System.Double"
            || fullName == L"System.Single";
    }

    inline bool TypeImpl::IsValueType() const
    {
        // System.Enum is derived from System.ValueType but is not itself a value type.  Go figure.
        if (assembly_->IsSystemAssembly() && GetFullName() == L"System.Enum")
            return false;
            
        return IsDerivedFromSystemType(L"System.ValueType", false);
    }

    inline bool TypeImpl::IsDerivedFromSystemType(String const& typeName, bool includeSelf) const
    {
        TypeImpl const* current(this);
        if (!includeSelf)
        {
            current->ResolveBaseType();
            current = current->baseType_;
        }

        while (current)
        {
            if (current->assembly_->IsSystemAssembly() && current->GetFullName() == typeName)
                return true;

            current->ResolveBaseType();
            current = current->baseType_;
        }

        return false; // TODO
    }

    inline void TypeImpl::ResolveBaseType() const
    {
        if (resolvedBaseType_)
            return;

        RealizeTypeDefProps();

        if (baseToken_.Get() == 0)
        {
            resolvedBaseType_ = true;
            return;
        }

        if (baseToken_.GetType() == mdtTypeDef)
        {
            baseType_ = assembly_->ResolveTypeDef(baseToken_);
        }
        else if (baseToken_.GetType() == mdtTypeRef)
        {
            // TODO
        }
        else if (baseToken_.GetType() == mdtTypeSpec)
        {
            // TODO 
        }

        resolvedBaseType_ = true;
    }

} }

#endif
