#include "CxxReflect.hpp"

#include "BlobMetadata.hpp"
#include "CorEnumIterator.hpp"
#include "Utility.hpp"

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