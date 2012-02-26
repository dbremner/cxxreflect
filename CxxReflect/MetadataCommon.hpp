//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Common Metadata library definitions, used in both MetadataDatabase and MetadataSignature.
#ifndef CXXREFLECT_METADATACOMMON_HPP_
#define CXXREFLECT_METADATACOMMON_HPP_

#include "CxxReflect/Fundamentals.hpp"

namespace CxxReflect {

    enum class AssemblyAttribute : std::uint32_t
    {
        PublicKey                  = 0x0001,
        Retargetable               = 0x0100,
        DisableJitCompileOptimizer = 0x4000,
        EnableJitCompileTracking   = 0x8000,

        // Note: These are not in ECMA-335; they can be found in the Windows SDK 8.0 header <cor.h>.
        DefaultContentType         = 0x0000,
        WindowsRuntimeContentType  = 0x0200,
        ContentTypeMask            = 0x0E00
    };

    enum class AssemblyHashAlgorithm : std::uint32_t
    {
        None     = 0x0000,
        MD5      = 0x8003,
        SHA1     = 0x8004
    };

    // The subset of System.Reflection.BindingFlags that are useful for reflection-only
    enum class BindingAttribute : std::uint32_t
    {
        Default                     = 0x00000000,
        IgnoreCase                  = 0x00000001,
        DeclaredOnly                = 0x00000002,
        Instance                    = 0x00000004,
        Static                      = 0x00000008,
        Public                      = 0x00000010,
        NonPublic                   = 0x00000020,
        FlattenHierarchy            = 0x00000040,

        InternalUseOnlyMask         = 0x10000000,
        InternalUseOnlyConstructor  = 0x10000001
    };

    enum class CallingConvention : std::uint8_t
    {
        Standard     = 0x00,
        VarArgs      = 0x05,
        HasThis      = 0x20,
        ExplicitThis = 0x40
    };

    enum class EventAttribute : std::uint16_t
    {
        SpecialName        = 0x0200,
        RuntimeSpecialName = 0x0400
    };

    enum class FieldAttribute : std::uint16_t
    {
        FieldAccessMask    = 0x0007,
        MemberAccessMask   = 0x0007,

        CompilerControlled = 0x0000,
        Private            = 0x0001,
        FamilyAndAssembly  = 0x0002,
        Assembly           = 0x0003,
        Family             = 0x0004,
        FamilyOrAssembly   = 0x0005,
        Public             = 0x0006,

        Static             = 0x0010,
        InitOnly           = 0x0020,
        Literal            = 0x0040,
        NotSerialized      = 0x0080,
        SpecialName        = 0x0200,

        PInvokeImpl        = 0x2000,

        RuntimeSpecialName = 0x0400,
        HasFieldMarshal    = 0x1000,
        HasDefault         = 0x8000,
        HasFieldRva        = 0x0100
    };

    enum class FileAttribute : std::uint32_t
    {
        ContainsMetadata   = 0x0000,
        ContainsNoMetadata = 0x0001
    };

    enum class GenericParameterAttribute : std::uint16_t
    {
        VarianceMask                   = 0x0003,
        None                           = 0x0000,
        Covariant                      = 0x0001,
        Contravariant                  = 0x0002,

        SpecialConstraintMask          = 0x001c,
        ReferenceTypeConstraint        = 0x0004,
        NotNullableValueTypeConstraint = 0x0008,
        DefaultConstructorConstraint   = 0x0010
    };

    enum class ManifestResourceAttribute : std::uint32_t
    {
        VisibilityMask = 0x0007,
        Public         = 0x0001,
        Private        = 0x0002
    };

    enum class MethodAttribute : std::uint16_t
    {
        MemberAccessMask      = 0x0007,
        CompilerControlled    = 0x0000,
        Private               = 0x0001,
        FamilyAndAssembly     = 0x0002,
        Assembly              = 0x0003,
        Family                = 0x0004,
        FamilyOrAssembly      = 0x0005,
        Public                = 0x0006,

        Static                = 0x0010,
        Final                 = 0x0020,
        Virtual               = 0x0040,
        HideBySig             = 0x0080,

        VTableLayoutMask      = 0x0100,
        ReuseSlot             = 0x0000,
        NewSlot               = 0x0100,

        Strict                = 0x0200,
        Abstract              = 0x0400,
        SpecialName           = 0x0800,

        PInvokeImpl           = 0x2000,
        RuntimeSpecialName    = 0x1000,
        HasSecurity           = 0x4000,
        RequireSecurityObject = 0x8000
    };

    enum class MethodImplementationAttribute : std::uint16_t
    {
        CodeTypeMask   = 0x0003,
        IL             = 0x0000,
        Native         = 0x0001,
        Runtime        = 0x0003,

        ManagedMask    = 0x0004,
        Unmanaged      = 0x0004,
        Managed        = 0x0000,

        ForwardRef     = 0x0010,
        PreserveSig    = 0x0080,
        InternalCall   = 0x1000,
        Synchronized   = 0x0020,
        NoInlining     = 0x0008,
        NoOptimization = 0x0040
    };

    enum class MethodSemanticsAttribute : std::uint16_t
    {
        Setter   = 0x0001,
        Getter   = 0x0002,
        Other    = 0x0004,
        AddOn    = 0x0008,
        RemoveOn = 0x0010,
        Fire     = 0x0020
    };

    enum class ParameterAttribute : std::uint16_t
    {
        In              = 0x0001,
        Out             = 0x0002,
        Optional        = 0x0010,
        HasDefault      = 0x1000,
        HasFieldMarshal = 0x2000
    };

    enum class PInvokeAttribute : std::uint16_t
    {
        NoMangle                     = 0x0001,

        CharacterSetMask             = 0x0006,
        CharacterSetNotSpecified     = 0x0000,
        CharacterSetAnsi             = 0x0002,
        CharacterSetUnicode          = 0x0004,
        CharacterSetAuto             = 0x0006,

        SupportsLastError            = 0x0040,

        CallingConventionMask        = 0x0700,
        CallingConventionPlatformApi = 0x0100,
        CallingConventionCDecl       = 0x0200,
        CallingConventionStdCall     = 0x0300,
        CallingConventionThisCall    = 0x0400,
        CallingConventionFastCall    = 0x0500
    };

    enum class PropertyAttribute : std::uint16_t
    {
        SpecialName        = 0x0200,
        RuntimeSpecialName = 0x0400,
        HasDefault         = 0x1000
    };

    enum class TypeAttribute : std::uint32_t
    {
        VisibilityMask          = 0x00000007,
        NotPublic               = 0x00000000,
        Public                  = 0x00000001,
        NestedPublic            = 0x00000002,
        NestedPrivate           = 0x00000003,
        NestedFamily            = 0x00000004,
        NestedAssembly          = 0x00000005,
        NestedFamilyAndAssembly = 0x00000006,
        NestedFamilyOrAssembly  = 0x00000007,

        LayoutMask              = 0x00000018,
        AutoLayout              = 0x00000000,
        SequentialLayout        = 0x00000008,
        ExplicitLayout          = 0x00000010,

        ClassSemanticsMask      = 0x00000020,
        Class                   = 0x00000000,
        Interface               = 0x00000020,

        Abstract                = 0x00000080,
        Sealed                  = 0x00000100,
        SpecialName             = 0x00000400,

        Import                  = 0x00001000,
        Serializable            = 0x00002000,

        StringFormatMask        = 0x00030000,
        AnsiClass               = 0x00000000,
        UnicodeClass            = 0x00010000,
        AutoClass               = 0x00020000,
        CustomFormatClass       = 0x00030000,
        CustomStringFormatMask  = 0x00c00000,

        BeforeFieldInit         = 0x00100000,

        RuntimeSpecialName      = 0x00000800,
        HasSecurity             = 0x00040000,
        IsTypeForwarder         = 0x00200000
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(AssemblyAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(BindingAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(EventAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(FieldAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(FileAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(GenericParameterAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(ManifestResourceAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(MethodAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(MethodImplementationAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(MethodSemanticsAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(ParameterAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(PInvokeAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(PropertyAttribute)
    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(TypeAttribute)

    typedef Detail::FlagSet<AssemblyAttribute>             AssemblyFlags;
    typedef Detail::FlagSet<BindingAttribute>              BindingFlags;
    typedef Detail::FlagSet<EventAttribute>                EventFlags;
    typedef Detail::FlagSet<FieldAttribute>                FieldFlags;
    typedef Detail::FlagSet<FileAttribute>                 FileFlags;
    typedef Detail::FlagSet<GenericParameterAttribute>     GenericParameterFlags;
    typedef Detail::FlagSet<ManifestResourceAttribute>     ManifestResourceFlags;
    typedef Detail::FlagSet<MethodAttribute>               MethodFlags;
    typedef Detail::FlagSet<MethodImplementationAttribute> MethodImplementationFlags;
    typedef Detail::FlagSet<MethodSemanticsAttribute>      MethodSemanticsFlags;
    typedef Detail::FlagSet<ParameterAttribute>            ParameterFlags;
    typedef Detail::FlagSet<PInvokeAttribute>              PInvokeFlags;
    typedef Detail::FlagSet<PropertyAttribute>             PropertyFlags;
    typedef Detail::FlagSet<TypeAttribute>                 TypeFlags;

}

namespace CxxReflect { namespace Metadata {

    class Database;
    class Stream;
    class StringCollection;
    class BlobReference;
    class RowReference;
    class FullReference;
    class TableCollection;
    class Table;

    class AssemblyRow;
    class AssemblyOsRow;
    class AssemblyProcessorRow;
    class AssemblyRefRow;
    class AssemblyRefOsRow;
    class AssemblyRefProcessorRow;
    class ClassLayoutRow;
    class ConstantRow;
    class CustomAttributeRow;
    class DeclSecurityRow;
    class EventMapRow;
    class EventRow;
    class ExportedTypeRow;
    class FieldRow;
    class FieldLayoutRow;
    class FieldMarshalRow;
    class FieldRvaRow;
    class FileRow;
    class GenericParamRow;
    class GenericParamConstraintRow;
    class ImplMapRow;
    class InterfaceImplRow;
    class ManifestResourceRow;
    class MemberRefRow;
    class MethodDefRow;
    class MethodImplRow;
    class MethodSemanticsRow;
    class MethodSpecRow;
    class ModuleRow;
    class ModuleRefRow;
    class NestedClassRow;
    class ParamRow;
    class PropertyRow;
    class PropertyMapRow;
    class StandaloneSigRow;
    class TypeDefRow;
    class TypeRefRow;
    class TypeSpecRow;

    class ArrayShape;
    class CustomModifier;
    class FieldSignature;
    class MethodSignature;
    class PropertySignature;
    class TypeSignature;
    class SignatureComparer;

    // A type resolver usable for resolving type references (TypeRef tokens).  This interface is
    // provided because the low-level Metadata library itself is incapable of resolving a TypeRef
    // to its definition.  The Metadata library classes only have knowledge of a single assembly,
    // but TypeRef resolution typically requires knowledge of the entire metadata universe.
    class ITypeResolver
    {
    public:

        // 'type' must be a TypeDef, TypeRef, or TypeSpec.  If it is a TypeDef or TypeSpec, it is
        // returned unmodified. If it is a TypeRef, it is resolved to its definition and the TypeDef
        // is returned with the defining resolution scope.
        virtual FullReference ResolveType(FullReference const& type) const = 0;

        virtual ~ITypeResolver();
    };

} }

#endif
