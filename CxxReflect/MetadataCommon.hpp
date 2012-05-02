#ifndef CXXREFLECT_METADATACOMMON_HPP_
#define CXXREFLECT_METADATACOMMON_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Common Metadata library definitions, used in both MetadataDatabase and MetadataSignature.

#include "CxxReflect/FundamentalUtilities.hpp"

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

        AllInstance                 = Instance | Public | NonPublic,
        AllStatic                   = Static   | Public | NonPublic,

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

    enum class ElementType : std::uint8_t
    {
        End                        = 0x00,
        Void                       = 0x01,
        Boolean                    = 0x02,
        Char                       = 0x03,
        I1                         = 0x04,
        U1                         = 0x05,
        I2                         = 0x06,
        U2                         = 0x07,
        I4                         = 0x08,
        U4                         = 0x09,
        I8                         = 0x0a,
        U8                         = 0x0b,
        R4                         = 0x0c,
        R8                         = 0x0d,
        String                     = 0x0e,
        Ptr                        = 0x0f,
        ByRef                      = 0x10,
        ValueType                  = 0x11,
        Class                      = 0x12,
        Var                        = 0x13,
        Array                      = 0x14,
        GenericInst                = 0x15,
        TypedByRef                 = 0x16,

        I                          = 0x18,
        U                          = 0x19,
        FnPtr                      = 0x1b,
        Object                     = 0x1c,

        ConcreteElementTypeMax     = 0x1d,

        SzArray                    = 0x1d,
        MVar                       = 0x1e,

        CustomModifierRequired     = 0x1f,
        CustomModifierOptional     = 0x20,

        Internal                   = 0x21,
        Modifier                   = 0x40,
        Sentinel                   = 0x41,
        Pinned                     = 0x45,

        Type                       = 0x50,
        CustomAttributeBoxedObject = 0x51,
        CustomAttributeField       = 0x53,
        CustomAttributeProperty    = 0x54,
        CustomAttributeEnum        = 0x55,

        /// For internal use only.
        ///
        /// This is not a real ElementType and it will never be found in metadata read from a
        /// database.  This faux ElementType is used when a signature is instantiated with types
        /// that are defined in or referenced from a database other than the database in which the
        /// uninstantiated signature is located.
        ///
        /// The cross-module type reference is composed of both a TypeDefOrSpec and a pointer to the
        /// database in which it is to be resolved.
        CrossModuleTypeReference   = 0x5f
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(ElementType);

    /// Resolves types.
    ///
    /// A Type Resolver is used to resolve type references (TypeRef tokens) and fundamental types.
    /// This interface is provided because the Metadata library itself is incapable of resolving a
    /// TypeRef to its definition if it is defined in another database (the Metadata library only
    /// deals with individual assemblies, not with type universes containing multiple assemblies).
    class ITypeResolver
    {
    public:

        /// Resolves a TypeRef to the TypeDef or TypeSpec to which it refers.
        ///
        /// This function should be used to resolve type references, potentially across assembly
        /// boundaries.  The provided `type` must be a `RowReference` and must refer to a row in
        /// the TypeDef, TypeRef, or TypeSpec table of a database.  If the row is a TypeDef or a
        /// TypeSpec, it is returned immediately as no resolution is required.
        ///
        /// If the row is a TypeRef, its resolution scope is located and the TypeRef is resolved
        /// in that scope.  This resolution process may cause a new assembly to be loaded into the
        /// type universe.
        ///
        /// \param type The TypeDef, TypeRef, or TypeSpec `RowReference` to be resolved.
        ///
        /// \returns The resolved TypeDef or TypeSpec `RowReference`, with its resolution scope.
        ///
        /// \throws LogicError If `type` is not initialized or if `type` is not a RowReference.
        ///
        /// \todo Other throws?
        virtual FullReference ResolveType(FullReference const& type) const = 0;

        /// Resolves a fundamental type.
        ///
        /// This function should be used to resolve a fundamental ElementType to the TypeDef that
        /// represents the fundamental type.  The resolved TypeDef must be found in the System
        /// assembly (that is, the assembly that references no other assemblies).
        ///
        /// \param elementType The fundamental type to be resolved.
        ///
        /// \returns The TypeDef representing the fundamental type.
        ///
        /// \todo Throws?
        virtual FullReference ResolveFundamentalType(ElementType elementType) const = 0;

        /// Resolves an internal-use-only replacement type.
        ///
        /// This interface is for internal support and allows CxxReflect to emulate the behavior of
        /// different reflection APIs and type systems.  For example, in the CLI type system, a `T[]`
        /// implements `IEnumerable<T>`, `IList<T>`, and `ICollection<T>`.  To allow us to correctly
        /// include these (and their related methods and properties) when we are queried about the
        /// array type, we can use this `ResolveReplacementType()` to transform `T[]` into a faux
        /// `Array<T>` type (name for exposition only) that implements the interfaces and provides
        /// the runtime capabilities.
        ///
        /// \param type The type for which to get its replacement.
        ///
        /// \returns The replacement type.  If there is no replacement type, it returns `type`.
        ///
        /// \throws LogicError If `type` is not initialized.
        ///
        /// \todo Throws?
        virtual FullReference ResolveReplacementType(FullReference const& type) const = 0;

        /// Virtual destructor required for interface.
        virtual ~ITypeResolver();
    };

} }

#endif
