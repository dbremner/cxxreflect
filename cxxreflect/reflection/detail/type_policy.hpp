
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/detail/independent_handles.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    class type_policy_handle;





    /// \defgroup cxxreflect_reflection_type_policies Reflection -> Details -> Type Policies
    ///
    /// Policy classes that implement the fundamental logic used by the `type` class
    ///
    /// @{





    /// A pair containing a module handle and some sort of type token and/or signature
    /// 
    /// This is provided for convenience, to make passing around a type with its module easier.
    template <typename Type>
    class type_with_module
    {
    public:

        typedef Type type_type;

        type_with_module() { }

        type_with_module(module_handle const& module, type_type const& type)
            : _module(module), _type(type)
        {
            core::assert_initialized(module);
            core::assert_initialized(type);
        }

        template <typename OtherType>
        type_with_module(type_with_module<OtherType> const& other)
        {
            if (other.is_initialized())
            {
                _module = other.module();
                _type   = other.type();
            }
        }

        auto module() const -> module_handle const&
        {
            core::assert_initialized(*this);
            return _module;
        }

        auto type() const -> type_type const&
        {
            core::assert_initialized(*this);
            return _type;
        }

        auto is_initialized() const -> bool
        {
            return _module.is_initialized() && _type.is_initialized();
        }

    private:

        module_handle _module;
        type_type     _type;
    };

    typedef type_with_module<metadata::type_def_token>                 type_def_with_module;
    typedef type_with_module<metadata::type_def_or_signature>          type_def_or_signature_with_module;
    typedef type_with_module<metadata::type_def_ref_spec_or_signature> type_def_ref_spec_or_signature_with_module;





    /// The layout attributes from `metadata::type_attribute`
    enum class type_attribute_layout
    {
        unknown,
        auto_layout,
        explicit_layout,
        sequential_layout
    };

    /// The string format attributes from `metadata::type_attribute`
    enum class type_attribute_string_format
    {
        unknown,
        ansi_string_format,
        auto_string_format,
        unicode_string_format
    };

    /// The visibility attributes from `metadata::type_attribute`
    enum class type_attribute_visibility
    {
        unknown,
        not_public,
        public_,
        nested_public,
        nested_private,
        nested_family,
        nested_assembly,
        nested_family_and_assembly,
        nested_family_or_assembly
    };





    /// Resolves a type for construction of a `type` object
    ///
    /// This function is used for many other purposes now, but its original use was to resolve types
    /// consistently for the `type` constructor.  It does much more than the `type_resolver` type
    /// resolution, because it attempts to find a normalized form of each type.  The normalizations
    /// that occur are as follows:
    ///
    /// * A TypeRef is resolved to the TypeDef or TypeSpec to which it refers
    /// * A TypeSpec token is resolved to its signature
    /// * A type signature representing a primitive type or class type is resolved to the primitive
    ///   type's TypeDef token (if and only if the type signature does not have the ByRef flag or
    ///   any ModOpt/ModReq signature elements).
    ///
    /// These rules are applied recursively, so a TypeRef referring to a TypeSpec whose signature
    /// represents a primitive type will collapse to be a TypeDef representing the primitive type.
    /// The intent is to make it easier to compare types because we have one canonical view of every
    /// type definition (we still need to compare signatures recursively, though, since they may
    /// have all sorts of representations). 
    ///
    /// The returned type will always be initialized.
    auto resolve_type_for_construction(type_def_ref_spec_or_signature_with_module const&)
        -> type_def_or_signature_with_module;





    /// The abstract base type policy that roots the type policies
    ///
    /// Type policies are stateless classes that implement the core functionality of the `type`
    /// class in the reflection library.  There are many different kinds of types (e.g. type
    /// definitions, generic type instances, arrays, and pointers), so to keep from having lots of
    /// unwieldy conditional logic or repetitive switches in the `type` class and its member
    /// functions, we encapsulate all of the logic into policy classes.
    ///
    /// When a `type` is constructed, the constructor calls `type_policy::get_for` to get a handle
    /// to the correct policy instance to be used for it.  There are different policy classes for
    /// each of the different kinds of types.  All (most!) of the member functions of `type` then
    /// call into that policy to perform the right actions based on the kind of type.
    ///
    /// There are two policies derived directly from `type_policy`:  `definition_type_policy`, which
    /// represents type definitions (TypeDefs), and `specialization_type_policy`, which represents
    /// type specializations (TypeSpecs).  The specialization policy has other policies derived from
    /// it.
    ///
    /// We use stateless classes so that we can get polymorphic behavior without dynamic allocation,
    /// by having the `get_for` factory return a pointer to different kinds of `type_policy`
    /// instances.  We encapsulate this logic into policy classes for the same reason:  we don't
    /// want to make `type` itself polymorphic, because value semantics are The Best Thing Ever.
    class type_policy
    {
    public:

        /// Gets the `base_type_functions` to be used for the provided type
        ///
        /// This function returns a handle to the static instance of the type policy that best
        /// matches the kind of type it is passed.  This should be called to get a handle for use
        /// in the `type` class or any other time that you have some unknown kind of type and you
        /// need to query it for information.
        static auto get_for(type_def_or_signature_with_module const&) -> type_policy_handle;





        virtual auto attributes    (type_def_or_signature_with_module const&) const -> metadata::type_flags              = 0;
        virtual auto base_type     (type_def_or_signature_with_module const&) const -> type_def_or_signature_with_module = 0;
        virtual auto declaring_type(type_def_or_signature_with_module const&) const -> type_def_or_signature_with_module = 0;

        virtual auto is_abstract               (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_array                  (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_by_ref                 (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_com_object             (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_contextful             (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_enum                   (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_generic_parameter      (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_generic_type           (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_generic_type_definition(type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_import                 (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_interface              (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_marshal_by_ref         (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_nested                 (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_pointer                (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_primitive              (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_sealed                 (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_serializable           (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_special_name           (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_value_type             (type_def_or_signature_with_module const&) const -> bool = 0;
        virtual auto is_visible                (type_def_or_signature_with_module const&) const -> bool = 0;

        virtual auto layout        (type_def_or_signature_with_module const&) const -> type_attribute_layout        = 0;
        virtual auto metadata_token(type_def_or_signature_with_module const&) const -> core::size_type              = 0;
        virtual auto namespace_name(type_def_or_signature_with_module const&) const -> core::string_reference       = 0;
        virtual auto string_format (type_def_or_signature_with_module const&) const -> type_attribute_string_format = 0;
        virtual auto visibility    (type_def_or_signature_with_module const&) const -> type_attribute_visibility    = 0;

        virtual ~type_policy();
    };





    /// A handle that points to a `type_policy` implementation
    ///
    /// We only use static instances of the `type_policy` class because the class is stateless.  To
    /// make resource ownership clearer, we use this handle type to encapsulate the fact that we're
    /// just passing around pointers to static instances.  This also makes it easier to verify that
    /// we only attempt to dereference non-null pointers.
    class type_policy_handle
    {
    public:

        type_policy_handle();
        type_policy_handle(type_policy const* policy);

        auto operator->() const -> type_policy const*;

        auto is_initialized() const -> bool;

    private:

        core::value_initialized<type_policy const*> _policy;
    };





    /// Resolves the primary TypeDef from a type token (TypeDef or TypeSpec)
    ///
    /// If the provided type is a TypeDef, it is returned unmodified.  If it is a TypeSpec, then
    /// this function attempts to compute the primary TypeDef from the TypeSpec.  For example,
    /// given a generic instance, this will return the TypeDef for the generic type definition from
    /// which the generic instance was instantiated.
    ///
    /// Note that this may return an uninitialized type.  For example, if the provided type is a
    /// generic type variable, it has no corresponding TypeDef and thus an uninitialized type is
    /// returned.
    auto resolve_type_def(type_def_or_signature_with_module const&) -> type_def_with_module;

    /// Resolves a type to its primary type definition and calls a function on its type policy
    template <typename MemberFunction>
    auto resolve_type_def_and_call(type_def_or_signature_with_module const& t, MemberFunction const f)
        -> decltype((std::declval<type_policy>().*f)(t))
    {
        type_def_or_signature_with_module const type_def(resolve_type_def(t));
        if (!type_def.is_initialized())
            return decltype((std::declval<type_policy>().*f)(t))();

        return (type_policy::get_for(type_def).operator->()->*f)(type_def);
    }

    /// Resolves the element type of a TypeSpec
    ///
    /// If the provided type is a TypeDef, the return value is uninitialized.  If the provided type
    /// has no element type, the return value is uninitialized.  Otherwise, this function returns
    /// the nested element type in the TypeSpec
    auto resolve_element_type(type_def_or_signature_with_module const&) -> type_def_or_signature_with_module;

    /// Resolves a type to its next nested element type and calls a function on its type policy
    template <typename MemberFunction>
    auto resolve_element_type_and_call(type_def_or_signature_with_module const& t, MemberFunction const f)
        -> decltype((std::declval<type_policy>().*f)(t))
    {
        type_def_or_signature_with_module const element_type(resolve_element_type(t));
        if (!element_type.is_initialized())
            return decltype((std::declval<type_policy>().*f)(t))();

        return (type_policy::get_for(element_type).operator->()->*f)(element_type);
    }





    /// @}

} } }

#endif
