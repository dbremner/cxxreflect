
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_RESOLUTION_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_RESOLUTION_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    /// Type Resolution Routines
    ///
    /// These functions facilitate various forms of type resolution.  These complement the type
    /// resolver interface defined by the metadata library and implemented by the reflection loader.
    /// That interface provides resolution of type references (i.e., TypeRef -> TypeDef resolution),
    /// and that functionality is provided by the loader because it is closely associated with the
    /// ability to load referenced assemblies.
    ///
    /// These functions complement that resolution process by enabling (a) normalization of type
    /// representations and (b) computation of associated types.  The former is enabled through
    /// `resolve_type`.
    ///
    /// Two definitions are of importance.  The element type is a property of a type specialization.
    /// It is the next nested type in the specialization.  For example, given T[][], the element
    /// type is T[].  Given T[], the element type is T.  Given T, there is no element type.  In a
    /// somewhat similar way, the element type of a generic type instantiation is the generic type
    /// definition, e.g., the element type of G<int> is G<>.
    ///
    /// The primary type is the most-nested element type.  For example, the primary type for both
    /// T[] and T[][] (and T[][][] even) is T.
    ///
    /// The compute functions perform simple type translation, normalization, and (for the element
    /// and primary types) discovery.  They do not perform any cross-module type resolution.  The
    /// resolve functions perform cross-module type resolution.
    ///
    /// DOCS

    auto compute_type(metadata::type_def_ref_spec_or_signature) -> metadata::type_def_ref_or_signature;
    auto resolve_type(metadata::type_def_ref_spec_or_signature) -> metadata::type_def_or_signature;

    auto compute_element_type(metadata::type_def_ref_or_signature) -> metadata::type_def_ref_or_signature;
    auto resolve_element_type(metadata::type_def_ref_or_signature) -> metadata::type_def_or_signature;

    auto compute_primary_type(metadata::type_def_ref_or_signature) -> metadata::type_def_ref_token;
    auto resolve_primary_type(metadata::type_def_ref_or_signature) -> metadata::type_def_token;





    #define CXXREFLECT_GENERATE decltype(std::declval<Callback>()(std::declval<metadata::type_def_ref_token>()))

    template <typename Callback>
    auto compute_primary_type_and_call(
        metadata::type_def_or_signature const& type,
        Callback callback,
        CXXREFLECT_GENERATE default_result = typename core::identity<CXXREFLECT_GENERATE>::type()
    ) -> CXXREFLECT_GENERATE
    {
        core::assert_initialized(type);

        metadata::type_def_ref_token const token(compute_primary_type(type));
        if (!token.is_initialized())
            return default_result;

        return callback(token);
    }

    #undef CXXREFLECT_GENERATE

    #define CXXREFLECT_GENERATE decltype(std::declval<Callback>()(std::declval<metadata::type_def_token>()))

    template <typename Callback>
    auto resolve_primary_type_and_call(
        metadata::type_def_or_signature const& type,
        Callback callback,
        CXXREFLECT_GENERATE default_result = typename core::identity<CXXREFLECT_GENERATE>::type()
    ) -> CXXREFLECT_GENERATE
    {
        core::assert_initialized(type);

        metadata::type_def_token const token(resolve_primary_type(type));
        if (!token.is_initialized())
            return default_result;

        return callback(token);
    }

    #undef CXXREFLECT_GENERATE

} } }

#endif
