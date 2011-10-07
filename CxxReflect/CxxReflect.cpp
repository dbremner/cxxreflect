#include "CxxReflect.hpp"

#include "CorEnumIterator.hxx"
#include "Utility.hxx"

#include <atlbase.h>
#include <cor.h>

#include <array>
#include <exception>
#include <iostream>
#include <map>
#include <type_traits>

namespace
{
    #pragma region Debug Runtime Checks

    namespace RuntimeCheck {

        #if !defined(NDEBUG) && !defined(ENABLE_CXXREFLECT_RUNTIME_CHECKS)
        #define ENABLE_CXXREFLECT_RUNTIME_CHECKS true
        #endif

        #ifdef ENABLE_CXXREFLECT_RUNTIME_CHECKS

        void VerifyNotNull(void const* p)
        {
            if (!p)
            {
                throw std::logic_error("wtf");
            }
        }

        void Verify(bool b)
        {
            if (!b)
            {
                throw std::logic_error("wtf");
            }
        }

        template <typename Callable>
        void Verify(Callable const& callable)
        {
            if (!callable())
            {
                throw std::logic_error("wtf");
            }
        }

        #else

        void VerifyNotNull(void const*) { }
        void Verify(bool) { }

        template <typename Callable>
        void Verify(Callable const&) { }

        #endif
    }

    #pragma endregion

    #pragma region LinearAllocator

    template <typename T, std::size_t N>
    class LinearAllocator
    {
    public:

        typedef T              ValueType;
        typedef T*             Pointer;
        typedef std::size_t    SizeType;

        enum { BlockSize = N };

        LinearAllocator()
            : next_()
        {
        }

        Pointer Allocate()
        {
            if (blocks_.size() == 0 || next_ == blocks_.back()->end())
            {
                blocks_.emplace_back(new BlockType);
                next_ = blocks_.back()->begin();
            }

            Pointer p(&*next_);
            next_ += 1;

            return p;
        }

    private:

        LinearAllocator(LinearAllocator const&);
        LinearAllocator& operator=(LinearAllocator const&);

        typedef std::array<ValueType, BlockSize>    BlockType;
        typedef std::unique_ptr<BlockType>          BlockPointer;
        typedef std::vector<BlockPointer>           BlockSequence;
        typedef typename BlockSequence::iterator    BlockSequenceIterator;

        BlockSequence            blocks_;
        BlockSequenceIterator    next_;
    };

    #pragma endregion

    #pragma region FlagSet

    template <typename T>
    class FlagSet
    {
    public:

        FlagSet()
            : value_()
        {
        }

        void Set(T x)         { value_ = static_cast<T>(value_ | x); }
        void Unset(T x)       { value_ = static_cast<T>(value_ ^ x); }
        bool IsSet(T x) const { return (value_ & x) != 0;            }

    private:

        T value_;
    };

    #pragma endregion

    #pragma region MetadataToken

    static const mdToken InvalidMetadataTokenValue = 0xFFFFFFFF;
    static const mdToken MetadataTokenTypeMask     = 0xFF000000;

    class MetadataToken
    {
    public:

        MetadataToken()
            : token_(InvalidMetadataTokenValue)
        {
        }

        MetadataToken(mdToken token)
            : token_(token)
        {
        }

        void Set(mdToken token)
        {
            token_ = token;
        }

        mdToken Get() const
        {
            RuntimeCheck::Verify([&]{ return IsInitialized(); });
            return token_;
        }

        CorTokenType GetType() const
        {
            RuntimeCheck::Verify([&]{ return IsInitialized(); });
            return static_cast<CorTokenType>(token_ & MetadataTokenTypeMask);
        }

        bool IsInitialized() const
        {
            return token_ != InvalidMetadataTokenValue;
        }

        bool IsValid(IMetaDataImport* import) const
        {
            RuntimeCheck::VerifyNotNull(import);
            return import->IsValidToken(token_) == TRUE;
        }

    private:

        mdToken token_;
    };

    template <CorTokenType TokenType>
    class CheckedMetadataToken
    {
    public:

        CheckedMetadataToken()
            : token_(InvalidMetadataTokenValue)
        {
        }

        CheckedMetadataToken(MetadataToken token)
            : token_(token.Get())
        {
            RuntimeCheck::Verify([&]{ return IsStateValid(); });
        }

        CheckedMetadataToken(mdToken token)
            : token_(token)
        {
            RuntimeCheck::Verify([&]{ return IsStateValid(); });
        }

        void Set(mdToken token)
        {
            token_ = token;
            RuntimeCheck::Verify([&]{ return IsStateValid(); });
        }

        mdToken Get() const
        {
            RuntimeCheck::Verify([&]{ return IsInitialized(); });
            return token_;
        }

        CorTokenType GetType() const
        {
            return TokenType;
        }

        bool IsInitialized() const
        {
            return token_ != InvalidMetadataTokenValue;
        }

        bool IsValid(IMetaDataImport* import) const
        {
            RuntimeCheck::VerifyNotNull(import);
            return IsInitialized() && import->IsValidToken(token_);
        }

        friend bool operator==(CheckedMetadataToken const& lhs, CheckedMetadataToken const& rhs)
        {
            return lhs.token_ == rhs.token_;
        }

        friend bool operator<(CheckedMetadataToken const& lhs, CheckedMetadataToken const& rhs)
        {
            return lhs.token_ < rhs.token_;
        }

    private:

        bool IsStateValid() const
        {
            return (token_ & MetadataTokenTypeMask) == TokenType;
        }

        mdToken token_;
    };

    typedef CheckedMetadataToken<mdtModule>                    ModuleToken;
    typedef CheckedMetadataToken<mdtTypeRef>                   TypeRefToken;
    typedef CheckedMetadataToken<mdtTypeDef>                   TypeDefToken;
    typedef CheckedMetadataToken<mdtFieldDef>                  FieldDefToken;
    typedef CheckedMetadataToken<mdtMethodDef>                 MethodDefToken;
    typedef CheckedMetadataToken<mdtParamDef>                  ParamDefToken;
    typedef CheckedMetadataToken<mdtInterfaceImpl>             InterfaceImplToken;
    typedef CheckedMetadataToken<mdtMemberRef>                 MemberRefToken;
    typedef CheckedMetadataToken<mdtCustomAttribute>           CustomAttributeToken;
    typedef CheckedMetadataToken<mdtPermission>                PermissionToken;
    typedef CheckedMetadataToken<mdtSignature>                 SignatureToken;
    typedef CheckedMetadataToken<mdtEvent>                     EventToken;
    typedef CheckedMetadataToken<mdtProperty>                  PropertyToken;
    typedef CheckedMetadataToken<mdtModuleRef>                 ModuleRefToken;
    typedef CheckedMetadataToken<mdtTypeSpec>                  TypeSpecToken;
    typedef CheckedMetadataToken<mdtAssembly>                  AssemblyToken;
    typedef CheckedMetadataToken<mdtAssemblyRef>               AssemblyRefToken;
    typedef CheckedMetadataToken<mdtFile>                      FileToken;
    typedef CheckedMetadataToken<mdtExportedType>              ExportedTypeToken;
    typedef CheckedMetadataToken<mdtManifestResource>          ManifestResourceToken;
    typedef CheckedMetadataToken<mdtGenericParam>              GenericParamToken;
    typedef CheckedMetadataToken<mdtMethodSpec>                MethodSpecToken;
    typedef CheckedMetadataToken<mdtGenericParamConstraint>    GenericParamConstraintToken;
    typedef CheckedMetadataToken<mdtString>                    StringToken;
    typedef CheckedMetadataToken<mdtName>                      NameToken;
    typedef CheckedMetadataToken<mdtBaseType>                  BaseTypeToken;

    #pragma endregion

    #pragma region HCorEnumIterator

    
    #pragma endregion

    CxxReflect::AssemblyName GetAssemblyNameFromToken(IMetaDataAssemblyImport* import, MetadataToken token)
    {
        RuntimeCheck::VerifyNotNull(import);

        const void* publicKeyOrToken(nullptr);
        ULONG publicKeyOrTokenLength(0);

        ULONG hashAlgorithmId(0);

        std::array<wchar_t, 512> nameChars = { };
        ULONG nameLength(0);

        ASSEMBLYMETADATA metadata = { };

        const void* hashValue(nullptr);
        ULONG hashValueLength(0);

        DWORD flags(0);

        if (token.GetType() == mdtAssembly)
        {
            CxxReflect::Detail::ThrowOnFailure(import->GetAssemblyProps(
                token.Get(),
                &publicKeyOrToken,
                &publicKeyOrTokenLength,
                &hashAlgorithmId,
                nameChars.data(),
                nameChars.size(),
                &nameLength,
                &metadata,
                &flags));
        }
        else if (token.GetType() == mdtAssemblyRef)
        {
            CxxReflect::Detail::ThrowOnFailure(import->GetAssemblyRefProps(
                token.Get(),
                &publicKeyOrToken,
                &publicKeyOrTokenLength,
                nameChars.data(),
                nameChars.size(),
                &nameLength,
                &metadata,
                &hashValue,
                &hashValueLength,
                &flags));
        }

        std::wstring name(nameChars.data());

        CxxReflect::Version version(
            metadata.usMajorVersion,
            metadata.usMinorVersion,
            metadata.usBuildNumber,
            metadata.usRevisionNumber);

        return CxxReflect::AssemblyName(name, version);
    }
}

namespace CxxReflect { namespace Detail {

    #pragma region Implementation Class Definitions

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
            : assembly_(assembly), token_(token), realizedTypeDefProps_(false), baseType_(nullptr), resolvedBaseType_(false)
        {
            RealizeTypeDefProps();
        }

        TypeDefToken GetMetadataToken() const
        {
            return token_;
        }

        String GetName() const
        {
            return fullName_; // TODO
        }

        String GetFullName() const
        {
            return fullName_;
        }

        TypeImpl const* GetBaseType() const { ResolveBaseType(); return baseType_; }

        bool IsAbstract()              const { return (flags_ & tdAbstract) != 0;                            }
        bool IsArray()                 const { return false; } // TODO
        bool IsAutoClass()             const { return false; } // TODO
        bool IsAutoLayout()            const { return (flags_ & tdLayoutMask) == tdAutoLayout;               }
        bool IsByRef()                 const { return false; } // TODO
        bool IsClass()                 const { return (flags_ & tdClassSemanticsMask) == tdClass;            }
        bool IsCOMObject()             const { return false; } // TODO
        bool IsContextful()            const { return IsDerivedFromSystemType(L"System.ContextBoundObject"); }
        bool IsEnum()                  const { return IsDerivedFromSystemType(L"System.Enum");               }
        bool IsExplicitLayout()        const { return (flags_ & tdLayoutMask) == tdExplicitLayout;           }
        bool IsGenericParameter()      const { return false; } // TODO
        bool IsGenericType()           const { return false; } // TODO
        bool IsGenericTypeDefinition() const { return false; } // TODO
        bool IsImport()                const { return (flags_ & tdImport) != 0;                              }
        bool IsInterface()             const { return (flags_ & tdClassSemanticsMask) == tdInterface;        }
        bool IsLayoutSequential()      const { return (flags_ & tdLayoutMask) == tdSequentialLayout;         }
        bool IsMarshalByRef()          const { return IsDerivedFromSystemType(L"System.MarshalByRefType");   }
        bool IsNested()                const { return (flags_ & tdVisibilityMask) >= tdNestedPublic;         }
        bool IsNestedAssembly()        const { return (flags_ & tdVisibilityMask) == tdNestedAssembly;       }
        bool IsNestedFamANDAssem()     const { return (flags_ & tdVisibilityMask) == tdNestedFamANDAssem;    }
        bool IsNestedFamily()          const { return (flags_ & tdVisibilityMask) == tdNestedFamily;         }
        bool IsNestedPrivate()         const { return (flags_ & tdVisibilityMask) == tdNestedPrivate;        }
        bool IsNestedPublic()          const { return (flags_ & tdVisibilityMask) == tdNestedPublic;         }
        bool IsNotPublic()             const { return (flags_ & tdVisibilityMask) == tdNotPublic;            }
        bool IsPointer()               const { return false; } // TODO
        bool IsPrimitive()             const { return false; } // TODO Is one of: Boolean, Byte, SByte, Int16, UInt16, Int32, UInt32, Int64, UInt64, IntPtr, UIntPtr, Char, Double, Single
        bool IsPublic()                const { return (flags_ & tdVisibilityMask) == tdPublic;               }
        bool IsSealed()                const { return (flags_ & tdSealed) != 0;                              }
        bool IsSecurityCritical()      const { return false; } // TODO
        bool IsSecuritySafeCritical()  const { return false; } // TODO
        bool IsSecurityTransparent()   const { return false; } // TODO
        bool IsSerializable()          const { return (flags_ & tdSerializable) != 0;                        }
        bool IsSpecialName()           const { return (flags_ & tdRTSpecialName) != 0;                       }
        bool IsUnicodeClass()          const { return (flags_ & tdStringFormatMask) == tdUnicodeClass;       }
        bool IsValueType()             const { return IsDerivedFromSystemType(L"System.ValueType");          }
        bool IsVisible()               const { return false; } // TODO

        AssemblyImpl GetAssembly() const;

    private:

        bool IsDerivedFromSystemType(String const& typeName) const;

        void RealizeTypeDefProps() const;

        void ResolveBaseType() const;

        AssemblyImpl const* assembly_;
        mutable TypeDefToken token_;

        mutable bool realizedTypeDefProps_;
        mutable String fullName_;
        mutable unsigned flags_;
        mutable MetadataToken baseToken_;

        mutable bool resolvedBaseType_;
        mutable TypeImpl const* baseType_;

        enum RealizationFlags
        {
            EventsRealized,
            FieldsRealized,
            MethodsRealized,
            PropertiesRealized
        };

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

        bool IsSystemAssembly() const
        {
            RealizeReferencedAssemblies();
            return referencedAssemblies_.empty();
        }
            
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

    #pragma endregion

    #pragma region Opaque Iterator Implementation

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

    #pragma endregion

    #pragma region MetadataReaderImpl

    MetadataReaderImpl::MetadataReaderImpl(std::unique_ptr<IReferenceResolver> referenceResolver)
        : referenceResolver_(std::move(referenceResolver))
    {
        ThrowOnFailure(CoCreateInstance(
            CLSID_CorMetaDataDispenser,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IMetaDataDispenserEx,
            reinterpret_cast<void**>(&dispenser_)));
    }

    Assembly MetadataReaderImpl::GetAssembly(String const& path) const
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

    Assembly MetadataReaderImpl::GetAssemblyByName(AssemblyName const& name) const
    {
        return GetAssembly(referenceResolver_->ResolveAssembly(name));
    }

    #pragma endregion

    #pragma region AssemblyImpl

    AssemblyImpl::AssemblyImpl(MetadataReaderImpl const* metadataReader,
                               String const& path,
                               IMetaDataImport2* import)
        : path_(path), metadataReader_(metadataReader), import_(import)
    {
    }

    TypeSequence AssemblyImpl::GetTypes() const
    {
        RealizeTypes();

        TypeSequence result;
        std::transform(types_.begin(), types_.end(), std::back_inserter(result), [&](TypeImpl const& impl)
        {
            return Type(&impl);
        });
        return result;
    }

    Type AssemblyImpl::GetType(Detail::String const& name, bool throwOnError, bool ignoreCase) const
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

    void AssemblyImpl::RealizeName() const
    {
        if (state_.IsSet(NameRealized)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(import_);
        RuntimeCheck::VerifyNotNull(assemblyImport);

        mdAssembly assemblyToken;
        ThrowOnFailure(assemblyImport->GetAssemblyFromScope(&assemblyToken));
        name_ = GetAssemblyNameFromToken(assemblyImport, assemblyToken);

        state_.Set(NameRealized);
    }

    void AssemblyImpl::RealizeReferencedAssemblies() const
    {
        if (state_.IsSet(ReferencedAssembliesRealized)) { return; }

        CComQIPtr<IMetaDataAssemblyImport, &IID_IMetaDataAssemblyImport> assemblyImport(import_);

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

    void AssemblyImpl::RealizeTypes() const
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

        state_.Set(TypesRealized);
    }

    #pragma endregion

    #pragma region TypeImpl

    void TypeImpl::RealizeTypeDefProps() const
    {
        if (realizedTypeDefProps_)
            return;

        IMetaDataImport2* import(assembly_->UnsafeGetImport());
        std::array<wchar_t, 512> nameBuffer;
        ULONG count;
        DWORD flags;
        mdToken extends;
        ThrowOnFailure(import->GetTypeDefProps(token_.Get(), nameBuffer.data(), nameBuffer.size(), &count, &flags, &extends));
        
        fullName_ = String(nameBuffer.begin(), nameBuffer.begin() + count - 1);
        flags_ = flags;
        baseToken_ = extends;
        realizedTypeDefProps_ = true;
    }

    bool TypeImpl::IsDerivedFromSystemType(String const& typeName) const
    {
        UNREFERENCED_PARAMETER(typeName);
        return false; // TODO
    }

    void TypeImpl::ResolveBaseType() const
    {
        if (resolvedBaseType_)
            return;

        RealizeTypeDefProps();

        if (baseToken_.Get() == 0)
            return;

        if (baseToken_.GetType() == mdtTypeDef)
        {
            baseType_ = assembly_->ResolveTypeDef(baseToken_);
        }
        else if (baseToken_.GetType() == mdtTypeRef)
        {
            throw std::logic_error("wtf");
        }
        else
        {
            throw std::logic_error("wtf");
        }

        resolvedBaseType_ = true;
    }

    #pragma endregion

} }

namespace CxxReflect {

    #pragma region MetadataReader

    MetadataReader::MetadataReader(std::unique_ptr<IReferenceResolver> referenceResolver)
        : impl_(new Detail::MetadataReaderImpl(std::move(referenceResolver)))
    {
    }

    Assembly MetadataReader::GetAssembly(Detail::String const& path)
    {
        return impl_->GetAssembly(path);
    }

    Assembly MetadataReader::GetAssemblyByName(AssemblyName const& name)
    {
        return impl_->GetAssemblyByName(name);
    }

    IMetaDataDispenserEx* MetadataReader::UnsafeGetDispenser() const
    {
        return impl_->UnsafeGetDispenser();
    }

    #pragma endregion

    #pragma region Assembly

    Assembly::Assembly(Detail::AssemblyImpl const* impl)
        : impl_(impl)
    {
    }

    AssemblySequence Assembly::GetReferencedAssemblies() const
    {
        return impl_->GetReferencedAssemblies();
    }

    TypeSequence Assembly::GetTypes() const
    {
        return impl_->GetTypes();
    }

    Type Assembly::GetType(Detail::String const& name, bool throwOnError, bool ignoreCase) const
    {
        return impl_->GetType(name, throwOnError, ignoreCase);
    }

    IMetaDataImport2* Assembly::UnsafeGetImport() const
    {
        return impl_->UnsafeGetImport();
    }

    #pragma endregion

    #pragma region Type

    Type::Type(Detail::TypeImpl const* impl)
        : impl_(impl)
    {
    }

    unsigned long Type::GetMetadataToken() const
    {
        return impl_->GetMetadataToken().Get();
    }

    Detail::String Type::GetName() const
    {
        return impl_->GetName();
    }

    bool Type::IsAbstract()              const { return impl_->IsAbstract();              }
    bool Type::IsArray()                 const { return impl_->IsArray();                 }
    bool Type::IsAutoClass()             const { return impl_->IsAutoClass();             }
    bool Type::IsAutoLayout()            const { return impl_->IsAutoLayout();            }
    bool Type::IsByRef()                 const { return impl_->IsByRef();                 }
    bool Type::IsClass()                 const { return impl_->IsClass();                 }
    bool Type::IsCOMObject()             const { return impl_->IsCOMObject();             }
    bool Type::IsContextful()            const { return impl_->IsContextful();            }
    bool Type::IsEnum()                  const { return impl_->IsEnum();                  }
    bool Type::IsExplicitLayout()        const { return impl_->IsExplicitLayout();        }
    bool Type::IsGenericParameter()      const { return impl_->IsGenericParameter();      }
    bool Type::IsGenericType()           const { return impl_->IsGenericType();           }
    bool Type::IsGenericTypeDefinition() const { return impl_->IsGenericTypeDefinition(); }
    bool Type::IsImport()                const { return impl_->IsImport();                }
    bool Type::IsInterface()             const { return impl_->IsInterface();             }
    bool Type::IsLayoutSequential()      const { return impl_->IsLayoutSequential();      }
    bool Type::IsMarshalByRef()          const { return impl_->IsMarshalByRef();          }
    bool Type::IsNested()                const { return impl_->IsNested();                }
    bool Type::IsNestedAssembly()        const { return impl_->IsNestedAssembly();        }
    bool Type::IsNestedFamANDAssem()     const { return impl_->IsNestedFamANDAssem();     }
    bool Type::IsNestedFamily()          const { return impl_->IsNestedFamily();          }
    bool Type::IsNestedPrivate()         const { return impl_->IsNestedPrivate();         }
    bool Type::IsNestedPublic()          const { return impl_->IsNestedPublic();          }
    bool Type::IsNotPublic()             const { return impl_->IsNotPublic();             }
    bool Type::IsPointer()               const { return impl_->IsPointer();               }
    bool Type::IsPrimitive()             const { return impl_->IsPrimitive();             }
    bool Type::IsPublic()                const { return impl_->IsPublic();                }
    bool Type::IsSealed()                const { return impl_->IsSealed();                }
    bool Type::IsSecurityCritical()      const { return impl_->IsSecurityCritical();      }
    bool Type::IsSecuritySafeCritical()  const { return impl_->IsSecuritySafeCritical();  }
    bool Type::IsSecurityTransparent()   const { return impl_->IsSecurityTransparent();   }
    bool Type::IsSerializable()          const { return impl_->IsSerializable();          }
    bool Type::IsSpecialName()           const { return impl_->IsSpecialName();           }
    bool Type::IsUnicodeClass()          const { return impl_->IsUnicodeClass();          }
    bool Type::IsValueType()             const { return impl_->IsValueType();             }
    bool Type::IsVisible()               const { return impl_->IsVisible();               }

    Type Type::GetBaseType() const
    {
        return impl_->GetBaseType();
    }
    #pragma endregion

}

namespace { namespace SignatureParser {

    using namespace CxxReflect;
    using namespace CxxReflect::Detail;

    typedef std::uint8_t const* ByteIterator;

    #pragma region Primitives

    std::int8_t ReadInt8(ByteIterator& it, ByteIterator last)
    {
        if (it == last)
            throw std::runtime_error("wtf");

        return *reinterpret_cast<std::int8_t const*>(it++);
    }

    std::uint8_t ReadUInt8(ByteIterator& it, ByteIterator last)
    {
        if (it == last)
            throw std::runtime_error("wtf");

        return *it++;
    }

    std::int32_t ReadCompressedInt32(ByteIterator& it, ByteIterator last)
    {
        UNREFERENCED_PARAMETER(it);
        UNREFERENCED_PARAMETER(last);
        return 0; // TODO
    }

    std::uint32_t ReadCompressedUInt32(ByteIterator& it, ByteIterator last)
    {
        std::array<std::uint8_t, 4> bytes = { 0 };

        bytes[0] = ReadUInt8(it, last);
        std::uint32_t length(0);
        if ((bytes[0] & 0x80) == 0)      { length = 1; }
        else if ((bytes[0] & 0x40) == 0) { length = 2; bytes[0] ^= 0x80; }
        else if ((bytes[0] & 0x20) == 0) { length = 4; bytes[0] ^= 0xC0; }
        else { throw std::logic_error("wtf"); }

        for (unsigned i(1); i < length; ++i)
            bytes[i] = ReadUInt8(it, last);

        switch (length)
        {
        case 1: return *reinterpret_cast<std::uint8_t*>(&bytes[0]);
        case 2: return *reinterpret_cast<std::uint16_t*>(&bytes[0]);
        case 4: return *reinterpret_cast<std::uint32_t*>(&bytes[0]);
        default: throw std::logic_error("It is impossible to get here.");
        }
    }

    std::uint32_t ReadTypeDefOrRefOrSpecEncoded(ByteIterator& it, ByteIterator last)
    {
        std::array<std::uint8_t, 4> bytes = { 0 };

        bytes[0] = ReadUInt8(it, last);
        std::uint8_t encodedTokenLength(bytes[0] >> 6);
        RuntimeCheck::Verify(encodedTokenLength < 4);

        for (unsigned i(1); i < encodedTokenLength; ++i)
            bytes[i] = ReadUInt8(it, last);

        std::uint32_t tokenValue(bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24));
        std::uint32_t tokenType(tokenValue & 0x03);
        tokenValue >>= 2;

        switch (tokenType)
        {
        case 0x01:  tokenValue |= mdtTypeRef;  break;
        case 0x02:  tokenValue |= mdtTypeDef;  break;
        case 0x03:  tokenValue |= mdtTypeSpec; break;
        default: throw std::logic_error("wtf");
        }

        return tokenValue;
    }

    CorElementType ReadCorElementType(ByteIterator& it, ByteIterator last)
    {
        std::uint8_t value(ReadUInt8(it, last));
        if (value > ELEMENT_TYPE_MAX && value != 0x41 && value != 0x45 && value != 0x46 && value != 0x47)
            throw std::logic_error("wtf");
        
        return static_cast<CorElementType>(value);
    }

    std::int8_t PeekInt8(ByteIterator it, ByteIterator last)
    {
        return ReadInt8(it, last);
    }

    std::uint8_t PeekUInt8(ByteIterator it, ByteIterator last)
    {
        return ReadUInt8(it, last);
    }

    std::int32_t PeekCompressedInt32(ByteIterator it, ByteIterator last)
    {
        return ReadCompressedInt32(it, last);
    }

    std::uint32_t PeekCompressedUInt32(ByteIterator it, ByteIterator last)
    {
        return ReadCompressedUInt32(it, last);
    }

    std::uint32_t PeekTypeDefOrRefOrSpecEncoded(ByteIterator it, ByteIterator last)
    {
        return ReadTypeDefOrRefOrSpecEncoded(it, last);
    }

    CorElementType PeekCorElementType(ByteIterator it, ByteIterator last)
    {
        return ReadCorElementType(it, last);
    }

    #pragma endregion

    // These are the core signature types
    class RawArrayShape;
    class RawConstraint;
    class RawCustomMod;
    class RawFieldSig;
    class RawLocalVarSig;
    class RawMethodDefSig;
    class RawMethodRefSig;
    class RawMethodSpec;
    class RawParam;
    class RawPropertySig;
    class RawRetType;
    class RawStandAloneMethodSig;
    class RawType;
    class RawTypeSpec;

    // These are subtypes of the RawType signature type
    class RawTypeArray;
    class RawTypeClassOrValueType;
    class RawTypeFunctionPointer;
    class RawTypeGenericInstance;
    class RawTypePointer;
    class RawTypeSzArray;
    class RawTypeTypeVariable;

    RawArrayShape                ReadArrayShape               (ByteIterator& it, ByteIterator last);
    RawConstraint                ReadConstraint               (ByteIterator& it, ByteIterator last);
    RawCustomMod                 ReadCustomMod                (ByteIterator& it, ByteIterator last);
    RawFieldSig                  ReadFieldSig                 (ByteIterator& it, ByteIterator last);
    RawLocalVarSig               ReadLocalVarSig              (ByteIterator& it, ByteIterator last);
    RawMethodDefSig              ReadMethodDefSig             (ByteIterator& it, ByteIterator last);
    RawMethodRefSig              ReadMethodRefSig             (ByteIterator& it, ByteIterator last);
    RawMethodSpec                ReadMethodSpec               (ByteIterator& it, ByteIterator last);
    RawParam                     ReadParam                    (ByteIterator& it, ByteIterator last);
    RawPropertySig               ReadPropertySig              (ByteIterator& it, ByteIterator last);
    RawRetType                   ReadRetType                  (ByteIterator& it, ByteIterator last);
    RawStandAloneMethodSig       ReadStandAloneMethodSig      (ByteIterator& it, ByteIterator last);
    RawType                      ReadType                     (ByteIterator& it, ByteIterator last);
    RawTypeSpec                  ReadTypeSpec                 (ByteIterator& it, ByteIterator last);
    
    RawTypeArray            ReadTypeArray           (CorElementType type, ByteIterator& it, ByteIterator last);
    RawTypeClassOrValueType ReadTypeClassOrValueType(CorElementType type, ByteIterator& it, ByteIterator last);
    RawTypeFunctionPointer  ReadTypeFunctionPointer (CorElementType type, ByteIterator& it, ByteIterator last);
    RawTypeGenericInstance  ReadTypeGenericInstance (CorElementType type, ByteIterator& it, ByteIterator last);
    RawTypePointer          ReadTypePointer         (CorElementType type, ByteIterator& it, ByteIterator last);
    RawTypeSzArray          ReadTypeSzArray         (CorElementType type, ByteIterator& it, ByteIterator last);
    RawTypeTypeVariable     ReadTypeTypeVariable    (CorElementType type, ByteIterator& it, ByteIterator last);
    
    class RawArrayShape
    {
    public:

        std::uint32_t                    Rank;
        std::uint32_t                    SizeCount;
        std::unique_ptr<std::uint32_t[]> SizeSequence;
        std::uint32_t                    LowBoundCount;
        std::unique_ptr<std::uint32_t[]> LowBoundSequence;

        RawArrayShape()
            : Rank(), SizeCount(), LowBoundCount()
        {
        }

        RawArrayShape(RawArrayShape&& other)
            : Rank(other.Rank),
              SizeCount(other.SizeCount),
              SizeSequence(std::move(other.SizeSequence)),
              LowBoundCount(other.LowBoundCount),
              LowBoundSequence(std::move(other.LowBoundSequence))
        {
        }

        // TODO move=

    private:

        RawArrayShape(RawArrayShape const&);
        RawArrayShape& operator=(RawArrayShape const&);
    };

    class RawConstraint
    {
    public:

        CorElementType ElementType; // Not really needed; it's always PINNED
    };

    class RawCustomMod
    {
    public:

        bool          IsOptional;
        std::uint32_t TypeToken;
    };

    class RawFieldSig
    {
    public:

    };

    class RawType
    {
    public:

        CorElementType ElementType;
        std::unique_ptr<RawTypeArray> ArrayType;
        std::unique_ptr<RawTypeClassOrValueType> ClassOrValueType;
        std::unique_ptr<RawTypeFunctionPointer> FunctionPointer;
        std::unique_ptr<RawTypeGenericInstance> GenericInstance;
        std::unique_ptr<RawTypeTypeVariable> TypeVariable;
        std::unique_ptr<RawTypePointer> Pointer;
        std::unique_ptr<RawTypeSzArray> SzArray;

        RawType()
            : ElementType()
        {
        }

        RawType(RawType&& other)
            : ElementType(std::move(other.ElementType)),
              ArrayType(std::move(other.ArrayType)),
              ClassOrValueType(std::move(other.ClassOrValueType)),
              FunctionPointer(std::move(other.FunctionPointer)),
              GenericInstance(std::move(other.GenericInstance)),
              TypeVariable(std::move(other.TypeVariable)),
              Pointer(std::move(other.Pointer)),
              SzArray(std::move(other.SzArray))
        {
        }

        RawType& operator=(RawType&& other)
        {
            // TODO fix this
            ElementType = other.ElementType;
            ArrayType = std::move(other.ArrayType);
            return *this;
        }

    private:

        RawType(RawType const&);
        RawType& operator=(RawType const&);
    };

    RawCustomMod ReadCustomMod(ByteIterator& it, ByteIterator last)
    {
        RawCustomMod x;
        x.IsOptional = ReadCorElementType(it, last) == ELEMENT_TYPE_CMOD_OPT;
        x.TypeToken = ReadTypeDefOrRefOrSpecEncoded(it, last);
        return x;
    }

    class RawMethodDefSig
    {
    public:

        std::uint8_t                Flags;
        std::uint32_t               GenericParameterCount;
        std::uint32_t               ParameterCount;
        std::unique_ptr<RawRetType> ReturnType;
        std::unique_ptr<RawParam[]> Parameters;

        RawMethodDefSig()
            : Flags(), GenericParameterCount(), ParameterCount()
        {
        }

        RawMethodDefSig(RawMethodDefSig&& other)
            : Flags(other.Flags),
              GenericParameterCount(other.GenericParameterCount),
              ParameterCount(other.ParameterCount),
              ReturnType(std::move(other.ReturnType)),
              Parameters(std::move(other.Parameters))
        {
        }

    private:

        RawMethodDefSig(RawMethodDefSig const&);
        RawMethodDefSig& operator=(RawMethodDefSig const&);
    };

    class RawMethodRefSig
    {
    public:

        // TODO
    };

    class RawParam
    {
    public:

        // TODO
    };

    class RawRetType
    {
    public:

        std::uint32_t                   CustomModCount;
        std::unique_ptr<RawCustomMod[]> CustomModSequence;

        std::uint32_t                   Flags;
        std::unique_ptr<RawType>        Type;

        RawRetType()
            : CustomModCount(), Flags()
        {
        }

        RawRetType(RawRetType&& other)
            : CustomModCount(other.CustomModCount),
              CustomModSequence(std::move(other.CustomModSequence)),
              Flags(other.Flags),
              Type(std::move(other.Type))
        {
        }

    private:

        RawRetType(RawRetType const&);
        RawRetType& operator=(RawRetType const&);

    };

    // A Type declared with an ELEMENT_TYPE_ARRAY prefix
    class RawTypeArray
    {
    public:

        std::unique_ptr<RawType>       Type;
        std::unique_ptr<RawArrayShape> Shape;

        RawTypeArray()
        {
        }

        RawTypeArray(RawTypeArray&& other)
            : Type(std::move(other.Type)),
              Shape(std::move(other.Shape))
        {
        }

        RawTypeArray& operator=(RawTypeArray&& other)
        {
            // TODO fix this
            Type = std::move(other.Type);
            Shape = std::move(other.Shape);
        }

    private:

        RawTypeArray(RawTypeArray const&);
        RawTypeArray& operator=(RawTypeArray const&);
    };

    class RawTypeClassOrValueType
    {
    public:

        bool IsClassType; // If false, value type
        std::uint32_t Type;
    };

    // A Type declared with an ELEMENT_TYPE_FNPTR prefix
    class RawTypeFunctionPointer
    {
    public:

        // Exactly one of these will be set
        std::unique_ptr<RawMethodDefSig> MethodDefSig;
        std::unique_ptr<RawMethodRefSig> MethodRefSig;

        RawTypeFunctionPointer()
        {
        }

        RawTypeFunctionPointer(RawTypeFunctionPointer&& other)
            : MethodDefSig(std::move(other.MethodDefSig)),
              MethodRefSig(std::move(other.MethodRefSig))
        {
        }

        // TODO move=

    private:

        RawTypeFunctionPointer(RawTypeFunctionPointer const&);
        RawTypeFunctionPointer& operator=(RawTypeFunctionPointer const&);
    };

    class RawTypeGenericInstance
    {
    public:

        // If false, it was declared as a reference type
        bool IsValueType;

        std::uint32_t TypeToken;
        std::uint32_t GenericArgumentCount;
        std::unique_ptr<RawType[]> GenericArguments;

        RawTypeGenericInstance()
            : TypeToken(), GenericArgumentCount()
        {
        }

        RawTypeGenericInstance(RawTypeGenericInstance&& other)
            : TypeToken(other.TypeToken),
              GenericArgumentCount(other.GenericArgumentCount),
              GenericArguments(std::move(other.GenericArguments))
        {
        }

        // TODO move=

    private:

        RawTypeGenericInstance(RawTypeGenericInstance const&);
        RawTypeGenericInstance& operator=(RawTypeGenericInstance const&);
    };

    class RawTypeTypeVariable
    {
    public:

        bool IsClassVariable; // if false, method variable
        std::uint32_t Number;
    };

    // A Type declared with an ELEMENT_TYPE_PTR prefix
    class RawTypePointer
    {
    public:

        std::uint32_t CustomModCount;

        // If CustomModSequence is null, there are zero CustomMods
        std::unique_ptr<RawCustomMod[]> CustomModSequence;
        
        // If Type is null, it implies VOID
        std::unique_ptr<RawType> Type;

        RawTypePointer()
            : CustomModCount()
        {
        }

        RawTypePointer(RawTypePointer&& other)
            : CustomModCount(other.CustomModCount),
            CustomModSequence(std::move(other.CustomModSequence)),
            Type(std::move(other.Type))
        {
        }

        // TODO move=

    private:

        RawTypePointer(RawTypePointer const&);
        RawTypePointer& operator=(RawTypePointer const&);
    };

    class RawTypeSzArray
    {
    public:

        std::uint32_t                   CustomModCount;
        std::unique_ptr<RawCustomMod[]> CustomModSequence;
        std::unique_ptr<RawType>        Type;

        RawTypeSzArray()
            : CustomModCount()
        {
        }

        RawTypeSzArray(RawTypeSzArray&& other)
            : CustomModCount(other.CustomModCount),
            CustomModSequence(std::move(other.CustomModSequence)),
            Type(std::move(other.Type))
        {
        }

        // TODO move=

    private:

        RawTypeSzArray(RawTypeSzArray const&);
        RawTypeSzArray& operator=(RawTypeSzArray const&);
    };

    RawArrayShape ReadArrayShape(ByteIterator& it, ByteIterator last)
    {
        RawArrayShape x;
        x.Rank = ReadCompressedUInt32(it, last);

        x.SizeCount = ReadCompressedUInt32(it, last);
        if (x.SizeCount > 0)
        {
            x.SizeSequence.reset(new std::uint32_t[x.SizeCount]);
            for (unsigned i(0); i < x.SizeCount; ++i)
                x.SizeSequence[i] = ReadCompressedUInt32(it, last);
        }

        x.LowBoundCount = ReadCompressedUInt32(it, last);
        if (x.LowBoundCount > 0)
        {
            x.LowBoundSequence.reset(new std::uint32_t[x.LowBoundCount]);
            for (unsigned i(0); i < x.LowBoundCount; ++i)
                x.LowBoundSequence[i] = ReadCompressedInt32(it, last);
        }

        return x;
    }

    RawConstraint ReadConstraint(ByteIterator& it, ByteIterator last)
    {
        RawConstraint x;
        x.ElementType = ReadCorElementType(it, last);
        if (x.ElementType != ELEMENT_TYPE_PINNED)
            throw std::logic_error("wtf");
        return x;
    }

    RawRetType ReadRetType(ByteIterator&, ByteIterator)
    {
        RawRetType x;

        // TODO

        return x;
    }

    RawType ReadType(ByteIterator& it, ByteIterator last)
    {
        RawType x;
        x.ElementType = ReadCorElementType(it, last);
        switch (x.ElementType)
        {
        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_R8:
        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_U:
        case ELEMENT_TYPE_OBJECT:
        case ELEMENT_TYPE_STRING:
            return x;

        case ELEMENT_TYPE_ARRAY:
            x.ArrayType.reset(new RawTypeArray(ReadTypeArray(x.ElementType, it, last)));
            return x;

        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_VALUETYPE:
            x.ClassOrValueType.reset(new RawTypeClassOrValueType(ReadTypeClassOrValueType(x.ElementType, it, last)));
            return x;

        case ELEMENT_TYPE_FNPTR:
            x.FunctionPointer.reset(new RawTypeFunctionPointer(ReadTypeFunctionPointer(x.ElementType, it, last)));
            return x;

        case ELEMENT_TYPE_GENERICINST:
            x.GenericInstance.reset(new RawTypeGenericInstance(ReadTypeGenericInstance(x.ElementType, it, last)));
            return x;

        case ELEMENT_TYPE_MVAR:
        case ELEMENT_TYPE_VAR:
            x.TypeVariable.reset(new RawTypeTypeVariable(ReadTypeTypeVariable(x.ElementType, it, last)));
            return x;

        case ELEMENT_TYPE_PTR:
            x.Pointer.reset(new RawTypePointer(ReadTypePointer(x.ElementType, it, last)));
            return x;

        case ELEMENT_TYPE_SZARRAY:
            x.SzArray.reset(new RawTypeSzArray(ReadTypeSzArray(x.ElementType, it, last)));
            return x;

        default:
            throw std::logic_error("wtf");
        }
    }

    RawTypeArray ReadTypeArray(CorElementType, ByteIterator& it, ByteIterator last)
    {
        RawTypeArray x;
        x.Type.reset(new RawType(ReadType(it, last)));
        x.Shape.reset(new RawArrayShape(ReadArrayShape(it, last)));
        return x;
    }

    RawTypeClassOrValueType ReadTypeClassOrValueType(CorElementType type, ByteIterator& it, ByteIterator last)
    {
        RawTypeClassOrValueType x;
        x.IsClassType = type == ELEMENT_TYPE_CLASS;
        x.Type = ReadTypeDefOrRefOrSpecEncoded(it, last);
        return x;
    }

    RawTypeFunctionPointer ReadTypeFunctionPointer(CorElementType, ByteIterator& it, ByteIterator last)
    {
        RawTypeFunctionPointer x;
        try
        {
            x.MethodDefSig.reset(new RawMethodDefSig(ReadMethodDefSig(it, last)));
        }
        catch (...) // TODO
        {
            x.MethodRefSig.reset(new RawMethodRefSig(ReadMethodRefSig(it, last)));
        }
        return x;
    }

    RawTypeGenericInstance ReadTypeGenericInstance(CorElementType, ByteIterator& it, ByteIterator last)
    {
        RawTypeGenericInstance x;
        CorElementType type(ReadCorElementType(it, last));
        switch (type)
        {
        case ELEMENT_TYPE_CLASS:     x.IsValueType = false; break;
        case ELEMENT_TYPE_VALUETYPE: x.IsValueType = true;  break;
        default:                     throw std::logic_error("wtf");
        }

        x.TypeToken = ReadTypeDefOrRefOrSpecEncoded(it, last);
        x.GenericArgumentCount = ReadCompressedUInt32(it, last);

        if (x.GenericArgumentCount > 0)
        {
            x.GenericArguments.reset(new RawType[x.GenericArgumentCount]);
            for (unsigned i(0); i < x.GenericArgumentCount; ++i)
            {
                x.GenericArguments[i] = ReadType(it, last);
            }
        }

        return x;
    }

    RawTypeTypeVariable ReadTypeTypeVariable(CorElementType type, ByteIterator& it, ByteIterator last)
    {
        RawTypeTypeVariable x;

        x.IsClassVariable = type == ELEMENT_TYPE_VAR;
        x.Number = ReadCompressedUInt32(it, last);

        return x;
    }

    RawTypePointer ReadTypePointer(CorElementType, ByteIterator& it, ByteIterator last)
    {
        RawTypePointer x;

        std::vector<RawCustomMod> customModifiers;

        CorElementType elementType(PeekCorElementType(it, last));
        while (true)
        {
            if (elementType != ELEMENT_TYPE_CMOD_OPT &&
                elementType != ELEMENT_TYPE_CMOD_REQD)
            {
                break;
            }

            customModifiers.push_back(ReadCustomMod(it, last));

            elementType = PeekCorElementType(it, last);
        }

        x.CustomModCount = customModifiers.size();
        x.CustomModSequence.reset(new RawCustomMod[x.CustomModCount]);
        std::copy(customModifiers.begin(), customModifiers.end(), x.CustomModSequence.get());

        elementType = PeekCorElementType(it, last);
        if (elementType != ELEMENT_TYPE_VOID)
        {
            x.Type.reset(new RawType(ReadType(it, last)));
        }

        return x;
    }

    RawTypeSzArray ReadTypeSzArray(CorElementType, ByteIterator& it, ByteIterator last)
    {
        RawTypeSzArray x;

        std::vector<RawCustomMod> customModifiers;

        CorElementType elementType(PeekCorElementType(it, last));
        while (elementType != ELEMENT_TYPE_CMOD_OPT && elementType != ELEMENT_TYPE_CMOD_REQD)
        {
            customModifiers.push_back(ReadCustomMod(it, last));

            elementType = PeekCorElementType(it, last);
        }

        x.CustomModCount = customModifiers.size();
        x.CustomModSequence.reset(new RawCustomMod[x.CustomModCount]);
        std::copy(customModifiers.begin(), customModifiers.end(), x.CustomModSequence.get());

        x.Type.reset(new RawType(ReadType(it, last)));

        return std::move(x);
    }

    RawMethodDefSig ReadMethodDefSig(ByteIterator& it, ByteIterator last)
    {
        RawMethodDefSig x;

        x.Flags = ReadUInt8(it, last);

        x.GenericParameterCount = (x.Flags & IMAGE_CEE_CS_CALLCONV_GENERIC) != 0
            ? ReadCompressedUInt32(it, last)
            : 0;

        x.ParameterCount = ReadCompressedUInt32(it, last);

        x.ReturnType.reset(new RawRetType(ReadRetType(it, last)));

        if (x.ParameterCount > 0)
        {
            x.Parameters.reset(new RawParam[x.ParameterCount]);
            for (unsigned i(0); i < x.ParameterCount; ++i)
                x.Parameters[i] = ReadParam(it, last);
        }

        return x; // TODO
    }

    RawMethodRefSig ReadMethodRefSig(ByteIterator&, ByteIterator)
    {
        RawMethodRefSig x;
        return x; // TODO
    }

    RawParam ReadParam(ByteIterator&, ByteIterator)
    {
        RawParam x;
        return x; // TODO
    }

    TypeImpl ParseTypeSpecGenericInst(AssemblyImpl const* assembly, TypeSpecToken token, ByteIterator it, ByteIterator last)
    {
        std::uint8_t const typeCode(ReadUInt8(it, last));
        if (typeCode != ELEMENT_TYPE_CLASS && typeCode != ELEMENT_TYPE_VALUETYPE)
            throw std::logic_error("wtf");

        MetadataToken genericTypeToken(ReadTypeDefOrRefOrSpecEncoded(it, last));

        //std::uint32_t genericArgumentCount(ReadCompressedUInt32(it, last));


        return TypeImpl(assembly, token.Get()); // TODO
    }

    TypeImpl ParseTypeSpec(AssemblyImpl const* assembly, TypeSpecToken token)
    {
        RuntimeCheck::VerifyNotNull(assembly);

        IMetaDataImport* import(assembly->UnsafeGetImport());
        RuntimeCheck::VerifyNotNull(import);

        ByteIterator signature(nullptr);
        ULONG length(0);

        ThrowOnFailure(import->GetTypeSpecFromToken(token.Get(), &signature, &length));
        RuntimeCheck::VerifyNotNull(signature);

        ByteIterator it(signature);
        ByteIterator const end(signature + length);

        std::uint8_t const initialElementType(ReadUInt8(it, end));
        switch (initialElementType)
        {
        case ELEMENT_TYPE_PTR:         throw std::logic_error("NYI");
        case ELEMENT_TYPE_FNPTR:       throw std::logic_error("NYI");
        case ELEMENT_TYPE_ARRAY:       throw std::logic_error("NYI");
        case ELEMENT_TYPE_SZARRAY:     throw std::logic_error("NYI");
        case ELEMENT_TYPE_GENERICINST: return ParseTypeSpecGenericInst(assembly, token, it, end);
        default:
            throw std::logic_error("wtf");
        }
    }
} }


int main()
{
    using namespace CxxReflect;

    CoInitializeEx(0, COINIT_APARTMENTTHREADED);

    std::unique_ptr<DirectoryBasedReferenceResolver> referenceResolver(new DirectoryBasedReferenceResolver());
    referenceResolver->AddDirectory(L"C:\\Windows\\Microsoft.NET\\Framework\\v4.0.30319\\");

    MetadataReader mr(std::move(referenceResolver));
    Assembly ass = mr.GetAssemblyByName(AssemblyName(L"System.Core"));
    TypeSequence types = ass.GetTypes();

   /* Type t = ass.GetType(L"system.NullRefeRenceException", false, true);
    Type base = t.GetBaseType();
    Type base2 = base.GetBaseType();
    Type base3 = base2.GetBaseType();*/

    AssemblySequence refAss = ass.GetReferencedAssemblies();
    //std::for_each(refAss.begin(), refAss.end(), [](Assembly const& a) { a.GetTypes(); });

    IMetaDataImport2* import = ass.UnsafeGetImport();

    

    mdTypeSpec token = 0x1b000001;
    //SignatureParser::ParseTypeSpec(ass.impl_, TypeSpecToken(token));

    std::uint8_t const* signature(nullptr);
    ULONG length(0);

    CxxReflect::Detail::ThrowOnFailure(import->GetTypeSpecFromToken(token, &signature, &length));
    RuntimeCheck::VerifyNotNull(signature);

    std::uint8_t const* it(signature);
    std::uint8_t const* const end(signature + length);
    
    CxxReflect::Detail::BlobMetadata::BlobAllocator alloc;
    CxxReflect::Detail::BlobMetadata::TypeSpec* ts = alloc.Allocate<CxxReflect::Detail::BlobMetadata::TypeSpec>(it, end);
    


/*    std::for_each(TypeSpecIterator(import), TypeSpecIterator(), [&](mdTypeSpec token) -> void
    {
        PCCOR_SIGNATURE signature((PCCOR_SIGNATURE()));
        ULONG length((ULONG()));
        ThrowOnFailure(import->GetTypeSpecFromToken(token, &signature, &length));

        std::vector<char> v(signature, signature + length);
    });*/
}