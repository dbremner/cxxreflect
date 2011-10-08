//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This is the public header for the CxxReflect library.  It contains (or includes headers that
// contain) all of the symbols that are intended to be used by a consumer of the library.
#ifndef CXXREFLECT_CXXREFLECT_HPP_
#define CXXREFLECT_CXXREFLECT_HPP_

// Be careful not to include any headers that include <cor.h> or any other system header
#include "Exceptions.hpp"
#include "Utility.hpp"

#include <array>
#include <cstdio>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

// Forward-declare the metadata interfaces so we can return them
struct IMetaDataDispenserEx;
struct IMetaDataImport2;

namespace CxxReflect
{
    // Implementation details; not for public use
    namespace Detail
    {
        typedef std::wstring String;

        #pragma region Core Utilities



        #pragma endregion

        #pragma region RefCounted/RefPointer Intrusive Reference Counting
        #pragma endregion

        #pragma region OpaqueIterator
        
        template <typename T>
        class OpaqueIterator
        {
        public:

            typedef std::random_access_iterator_tag    iterator_category;
            typedef T                                  value_type;
            typedef T const&                           reference;
            typedef T const*                           pointer;
            typedef std::ptrdiff_t                     difference_type;

            template <typename U>
            explicit OpaqueIterator(U const* = nullptr);

            pointer   operator->() const;
            reference operator*()  const;

            reference operator[](difference_type) const;

            OpaqueIterator& operator++();
            OpaqueIterator  operator++(int);

            OpaqueIterator& operator--();
            OpaqueIterator  operator--(int);

            OpaqueIterator& operator+=(difference_type);
            OpaqueIterator& operator-=(difference_type);

        private:

            void const* element_;
        };

        template <typename T>
        OpaqueIterator<T> operator+(OpaqueIterator<T> lhs, std::ptrdiff_t rhs)
        {
            return lhs += rhs;
        }

        template <typename T>
        OpaqueIterator<T> operator+(std::ptrdiff_t lhs, OpaqueIterator<T> rhs)
        {
            return rhs += lhs;
        }

        template <typename T>
        OpaqueIterator<T> operator-(OpaqueIterator<T> lhs, std::ptrdiff_t rhs)
        {
            return lhs -= rhs;
        }

        // TODO Implemnent in source file
        template <typename T>
        std::ptrdiff_t operator-(OpaqueIterator<T> lhs, OpaqueIterator<T> rhs);

        template <typename T>
        bool operator==(OpaqueIterator<T> lhs, OpaqueIterator<T> rhs)
        {
            return lhs.operator->() == rhs.operator->();
        }

        template <typename T>
        bool operator<(OpaqueIterator<T> const& lhs, OpaqueIterator<T> const& rhs)
        {
            return std::less<void const*>()(lhs.operator->(), rhs.operator->());
        }

        #pragma endregion
        
        class MetadataReaderImpl;
        class AssemblyImpl;
        class ModuleImpl;
        class TypeImpl;
        class EventImpl;
        class FieldImpl;
        class MethodImpl;
        class PropertyImpl;
        class ParameterImpl;

        using namespace std::rel_ops;
    }

    #pragma region Version

    class Version
    {
    public:

        Version()
            : major_(), minor_(), build_(), revision_()
        {
        }

        Version(std::uint16_t major,
                std::uint16_t minor,
                std::uint16_t build = 0,
                std::uint16_t revision = 0)
            : major_(major), minor_(minor), build_(build), revision_(revision)
        {
        }

        std::uint16_t GetMajor()    const { return major_;    }
        std::uint16_t GetMinor()    const { return minor_;    }
        std::uint16_t GetBuild()    const { return build_;    }
        std::uint16_t GetRevision() const { return revision_; }

    private:

        std::uint16_t major_;
        std::uint16_t minor_;
        std::uint16_t build_;
        std::uint16_t revision_;
    };

    inline bool operator==(Version const& lhs, Version const& rhs)
    {
        return lhs.GetMajor()    == rhs.GetMajor()
            && lhs.GetMinor()    == rhs.GetMinor()
            && lhs.GetBuild()    == rhs.GetBuild()
            && lhs.GetRevision() == rhs.GetRevision();
    }

    inline bool operator<(Version const& lhs, Version const& rhs)
    {
        if (lhs.GetMajor()    < rhs.GetMajor())    { return true;  }
        if (lhs.GetMajor()    > rhs.GetMajor())    { return false; }
        if (lhs.GetMinor()    < rhs.GetMinor())    { return true;  }
        if (lhs.GetMinor()    > rhs.GetMinor())    { return false; }
        if (lhs.GetBuild()    < rhs.GetBuild())    { return true;  }
        if (lhs.GetBuild()    > rhs.GetBuild())    { return false; }
        if (lhs.GetRevision() < rhs.GetRevision()) { return true;  }
        return false;
    }

    using namespace std::rel_ops;

    inline std::wostream& operator<<(std::wostream& os, Version const& v)
    {
        return os << v.GetMajor() << L'.'
                  << v.GetMinor() << L'.'
                  << v.GetBuild() << L'.'
                  << v.GetRevision();
    }

    #pragma endregion

    #pragma region AssemblyName

    typedef std::array<unsigned char, 8> PublicKeyToken;

    // TODO Lots of missing information here
    class AssemblyName
    {
    public:

        //typedef std::vector<unsigned char>   PublicKey;
        //typedef std::array<unsigned char, 8> PublicKeyToken;

        explicit AssemblyName(Detail::String const& name = Detail::String(),
                              Version const& version = Version(),
                              Detail::String const& culture = Detail::String(),
                              PublicKeyToken const& publicKeyToken = PublicKeyToken(),
                              Detail::String const& path = Detail::String())
            : name_(name), version_(version), culture_(culture), publicKeyToken_(publicKeyToken), path_(path)
        {
        }
        
        // URI formatting?
        Detail::String GetCodeBase() const { return path_; }

        Detail::String GetCulture() const { return culture_; }
        // EscapedCodeBase?

        //unsigned long GetFlags() const { return flags_; }

        Detail::String GetFullName() const
        {
            Detail::String result(name_);

            if (GetVersion() != Version())
                result += L", Version=" + Detail::ToString(version_);

            if (GetCulture().size() > 0)
                result += L", Culture=" + GetCulture();

            // TODO HAS PUBLIC KEY TOKEN???
            result += L", PublicKeyToken=";
            std::for_each(publicKeyToken_.begin(), publicKeyToken_.end(), [&](std::uint8_t c)
            {
                std::array<wchar_t, 3> buffer = { 0 };
                std::swprintf(buffer.data(), buffer.size(), L"%x", c);
                result += buffer.data();
            });

            return result;
        }

        Detail::String GetName() const { return name_; }

        Version GetVersion() const { return version_; }

        //PublicKeyToken GetPublicKeyToken() const { return publicKeyToken_; }

    private:

        Detail::String name_;
        Detail::String path_;
        Detail::String culture_;
        //unsigned long  flags_;
        Version        version_;
        PublicKeyToken publicKeyToken_;
    };

    #pragma endregion

    #pragma region Enumerations

    struct AssemblyNameFlags
    {
        enum Type
        {
            None,
            PublicKey,
            EnableJITCompileOptimizer,
            EnableJITCompileTracking,
            Retargetable
        };
    };

    /*
    struct BindingFlags
    {
        enum Type
        {
            Default,
            IgnoreCase,
            DeclaredOnly,
            Instance,
            Static,
            Public,
            NonPublic,
            FlattenHierarchy,
            InvokeMethod,
            CreateInstance,
            GetField,
            SetField,
            GetProperty,
            SetProperty,
            PutDispProperty,
            PutRefDispProperty,
            ExactBinding,
            SuppressChangeType,
            OptionalParamBinding,
            IgnoreReturn
        };
    };
    */

    // TODO Add values to these
    struct FieldAttributes
    {
        enum Type
        {
            FieldAccessMask,
            PrivateScope,
            Private,
            FamilyAndAssembly,
            Assembly,
            Family,
            FamilyOrAssembly,
            Public,
            Static,
            InitOnly,
            Literal,
            NotSerialized,
            SpecialName,
            PinvokeImpl,
            ReservedMask,
            RTSpecialName,
            HasFieldMarshal,
            HasDefault,
            HasFieldRVA
        };
    };


    #pragma endregion

    class MetadataReader;
    class Assembly;
    class Module;
    class Type;
    class Event;
    class Field;
    class Method;
    class Property;
    class Parameter;

    // TODO Let's remove these and stick with the iterators
    typedef std::vector<Assembly>               AssemblySequence;
    typedef std::vector<Module>                 ModuleSequence;
    typedef std::vector<Type>                   TypeSequence;
    typedef std::vector<Event>                  EventSequence;
    typedef std::vector<Field>                  FieldSequence;
    typedef std::vector<Method>                 MethodSequence;
    typedef std::vector<Property>               PropertySequence;
    typedef std::vector<Parameter>              ParameterSequence;

    typedef Detail::OpaqueIterator<Assembly>    AssemblyIterator;
    typedef Detail::OpaqueIterator<Module>      ModuleIterator;
    typedef Detail::OpaqueIterator<Type>        TypeIterator;
    typedef Detail::OpaqueIterator<Event>       EventIterator;
    typedef Detail::OpaqueIterator<Field>       FieldIterator;
    typedef Detail::OpaqueIterator<Method>      MethodIterator;
    typedef Detail::OpaqueIterator<Property>    PropertyIterator;
    typedef Detail::OpaqueIterator<Parameter>   ParameterIterator;

    #pragma region IReferenceResolver and default implementations

    class IReferenceResolver
    {
    public:

        virtual Detail::String ResolveAssembly(AssemblyName const&) = 0;

        virtual ~IReferenceResolver() { }
    };

    class PathBasedReferenceResolver : public IReferenceResolver
    {
        // TODO
    };

    class DirectoryBasedReferenceResolver : public IReferenceResolver
    {
    public:

        virtual Detail::String ResolveAssembly(AssemblyName const& name)
        {
            return Detail::String(directories_[0] + name.GetName() + L".dll");
        }

        // TODO

        void AddDirectory(Detail::String const& directory)
        {
            directories_.push_back(directory);
        }

    private:

        std::vector<Detail::String> directories_;
    };

    class WindowsRuntimeReferenceResolver : public IReferenceResolver
    {
        // TODO
    };

    #pragma endregion

    class MetadataReader
    {
    public:

        MetadataReader(std::unique_ptr<IReferenceResolver> referenceResolver);

        Assembly GetAssembly(Detail::String const& path);

        Assembly GetAssemblyByName(AssemblyName const& name);

        IMetaDataDispenserEx* UnsafeGetDispenser() const;

    private:

        MetadataReader(MetadataReader const&);
        void operator=(MetadataReader const&);

        Detail::RefPointer<Detail::MetadataReaderImpl> impl_;
    };

    class Assembly
    {
    public:

        Detail::String GetFullName() const;

        Module GetModule(Detail::String const& name) const;

        ModuleSequence GetModules() const; // TODO Remove

        ModuleIterator BeginModules() const;
        ModuleIterator EndModules() const;

        AssemblyName GetName() const;

        AssemblySequence GetReferencedAssemblies() const;

        Type GetType(Detail::String const& name, bool throwOnError = false, bool ignoreCase = false) const;

        TypeIterator BeginTypes() const;
        TypeIterator EndTypes() const;

        TypeSequence GetTypes() const; // TODO Remove

        bool IsDefined(Type const& type);

        Detail::String ToString() const;

        IMetaDataImport2* UnsafeGetImport() const;

    private:

        friend MetadataReader;
        friend Detail::MetadataReaderImpl;
        friend Detail::AssemblyImpl;

        Assembly(Detail::AssemblyImpl const* impl);

    public: // TODO

        Detail::AssemblyImpl const* impl_;
    };

    bool operator==(Assembly const&, Assembly const&);
    bool operator< (Assembly const&, Assembly const&);

    std::wostream& operator<<(std::wostream&, Assembly const&);

    class TypeAttributes;

    class Type
    {
    public:

        Assembly GetAssembly() const;

        Detail::String GetAssemblyQualifiedName() const;

        TypeAttributes GetAttributes() const;

        bool HasBaseType() const;
        Type GetBaseType() const;

        bool ContainsGenericParameters() const;

        // DeclaringMethod
        // DeclaringType
        // DefaultBinder
        
        Detail::String GetFullName() const;

        // GenericParameterAttributes
        // GenericParameterPosition

        // GUID

        // HasElementType

        bool IsAbstract() const;
        bool IsArray() const;
        bool IsAutoClass() const;
        bool IsAutoLayout() const;
        bool IsByRef() const;
        bool IsClass() const;
        bool IsCOMObject() const;
        bool IsContextful() const;
        bool IsEnum() const;
        bool IsExplicitLayout() const;
        bool IsGenericParameter() const;
        bool IsGenericType() const;
        bool IsGenericTypeDefinition() const;
        bool IsImport() const;
        bool IsInterface() const;
        bool IsLayoutSequential() const;
        bool IsMarshalByRef() const;
        bool IsNested() const;
        bool IsNestedAssembly() const;
        bool IsNestedFamANDAssem() const;
        bool IsNestedFamily() const;
        bool IsNestedPrivate() const;
        bool IsNestedPublic() const;
        bool IsNotPublic() const;
        bool IsPointer() const;
        bool IsPrimitive() const;
        bool IsPublic() const;
        bool IsSealed() const;
        bool IsSecurityCritical() const;
        bool IsSecuritySafeCritical() const;
        bool IsSecurityTransparent() const;
        bool IsSerializable() const;
        bool IsSpecialName() const;
        bool IsUnicodeClass() const;
        bool IsValueType() const;
        bool IsVisible() const;

        // MemberType

        unsigned long GetMetadataToken() const;

        // Module

        Detail::String GetName() const;
        
        Detail::String GetNamespace() const;

        // ReflectedType
        // StructLayoutAttribute
        // TypeHandle
        // TypeInitializer
        // UnderlyingSystemType

        // ---

        // FindInterfaces
        // FindMembers
        // GetArrayRank
        // GetConstructor
        // GetConstructors
        // GetCustomAttributes
        // GetCustomAttributesData
        // GetDefaultMembers
        // GetElementType
        // GetEnumName
        // GetEnumNames
        // GetEnumUnderlyingType
        // GetEnumValues
        // GetEvent
        // GetEvents
        // GetField
        // GetFields
        // GetGenericArguments
        // GetGenericParameterConstraints
        // GetInterface
        // GetInterfaceMap
        // GetInterfaces
        // GetMember
        // GetMembers
        // GetMethod
        // GetMethods
        // GetNestedType
        // GetNestedTypes
        // GetProperties
        // GetProperty
        // GetTypeCode
        // IsDefined
        // IsEnumDefined
        // IsSubclassOf
        // MakeArrayType
        // MakeByRefType
        // MakeGenericType
        // MakePointerType
        // ToString
        // IsDefined

    private:

        friend Assembly;
        friend Detail::AssemblyImpl;

        Type(Detail::TypeImpl const* impl);

        Detail::TypeImpl const* impl_;
    };

    class CustomAttributeData; // TODO?

    typedef std::vector<CustomAttributeData> CustomAttributeDataSequence;

    class Field
    {
    public:

        FieldAttributes::Type GetAttributes() const;

        Type GetDeclaringType() const;

        Type GetFieldType() const;

        bool IsAssembly() const;
        bool IsFamily() const;
        bool IsFamilyAndAssembly() const;
        bool IsFamilyOrAssembly() const;
        bool IsInitOnly() const;
        bool IsLiteral() const;
        bool IsNotSerialized() const;
        bool IsPinvokeImpl() const;
        bool IsPrivate() const;
        bool IsPublic() const;
        bool IsSecurityCritical() const;
        bool IsSecuritySafeCritical() const;
        bool IsSecurityTransparent() const;
        bool IsSpecialName() const;
        bool IsStatic() const;
        
        long GetMetadataToken() const;

        Module GetModule() const;

        Type GetReflectedType() const;

        CustomAttributeDataSequence GetCustomAttributeData() const;

        TypeSequence GetOptionalCustomModifiers() const;
        // ??? GetRawConstantValue() const;
        TypeSequence GetRequiredCustomModifiers() const;

        // GetValue();
        // GetValueDirect();
        // SetValue();
        // SetValueDirect();

        Detail::String ToString() const;

    private:

        friend bool operator==(Field const& lhs, Field const& rhs);
        friend bool operator< (Field const& lhs, Field const& rhs);

        Detail::FieldImpl const* impl_;
    };

    inline bool operator==(Field const& lhs, Field const& rhs)
    {
        return lhs.impl_ == rhs.impl_;
    }

    inline bool operator< (Field const& lhs, Field const& rhs)
    {
        return std::less<Detail::FieldImpl const*>()(lhs.impl_, rhs.impl_);
    }



    using namespace std::rel_ops;
}

#endif
