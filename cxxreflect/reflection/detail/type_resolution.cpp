
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/type_resolution.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    auto compute_type(metadata::type_def_ref_spec_or_signature const t) -> metadata::type_def_ref_or_signature
    {
        core::assert_initialized(t);

        // If the type represents a type signature, we make a few simple attempts to resolve it to
        // a TypeDef, because it's cheap to do so and it generally makes it easier to deal with
        // primitive and class type signatures.
        if (t.is_blob())
        {
            metadata::type_signature const signature(t.as_blob().as<metadata::type_signature>());

            // We can only resolve a signature to a TypeDef if there is no signature information
            // that precedes the type code.  Required and optional custom modifiers and the ByRef
            // tag can both appear before the type code.
            if (signature.seek_to(metadata::type_signature::part::cross_module_type_reference) != signature.begin_bytes())
                return t.as_blob();

            // If we have a primitive type, we resolve it to the TypeDef that represents it and we
            // return that type.  This is the only resolution that we perform in this function; this
            // should be sufficiently fast that we're okay with this.
            if (signature.get_kind() == metadata::type_signature::kind::primitive)
            {
                metadata::type_resolver  const& root(loader_context::from(t.scope()));
                metadata::element_type   const  primitive_type_tag(signature.primitive_type());
                metadata::type_def_token const  primitive_type(root.resolve_fundamental_type(primitive_type_tag));

                return primitive_type;
            }

            // If we have a class type, recurse with the named class type and recompute (this allows
            // us to re-transform a TypeSpec token to its signature).
            if (signature.get_kind() == metadata::type_signature::kind::class_type)
                return compute_type(signature.class_type());

            // Otherwise, we have one of the other signature types, which are not resolved here:
            return t.as_blob();
        }
        // Otherwise, the type represents a type token of some kind.  TypeDef tokens are never
        // modified and we do not resolve TypeRef tokens in this function, so we only need to
        // consider TypeSpec tokens:
        else
        {
            metadata::type_def_ref_spec_token const token(t.as_token());

            // If the token is a TypeSpec, we recompute with the TypeSpec's signature:
            if (token.is<metadata::type_spec_token>())
                return compute_type(row_from(token.as<metadata::type_spec_token>()).signature());

            // Otherwise, we simply return the token unmodified:
            return token.as<metadata::type_def_ref_token>();
        }
    }

    auto resolve_type(metadata::type_def_ref_spec_or_signature const t) -> metadata::type_def_or_signature
    {
        core::assert_initialized(t);

        // First, perform trivial collapsing of the original type using logic common to this
        // function and the compute_type function:
        metadata::type_def_ref_or_signature const computed_type(compute_type(t));

        // If computation resulted in a signature, then there's nothing left to resolve:  it's not
        // a class or primitive signature, so we cannot reduce it further:
        if (computed_type.is_blob())
            return computed_type.as_blob();

        metadata::type_def_ref_token const tr_token(computed_type.as_token());
        
        metadata::type_resolver       const& root(loader_context::from(t.scope()));
        metadata::type_def_spec_token const tds_token(root.resolve_type(tr_token));

        // Since we never pass a TypeSpec to resolve_type, we should never get a TypeSpec back:
        core::assert_true([&]{ return tds_token.is<metadata::type_def_token>(); });

        return tds_token.as<metadata::type_def_token>();
    }

    auto compute_element_type(metadata::type_def_ref_or_signature const t) -> metadata::type_def_ref_or_signature
    {
        core::assert_initialized(t);

        if (!t.is_initialized() || t.is_token())
            return metadata::type_def_or_signature();

        metadata::type_signature const signature(t.as_blob().as<metadata::type_signature>());

        // If the signature is ByRef, we need to fabricate a new signature that skips the ByRef tag
        // and represents the element type.  Note that this also removes any custom modifiers, which
        // appear before the ByRef tag.
        if (signature.is_by_ref())
        {
            // The cross-module type reference tag is the first part that may appear after the ByRef
            // tag; by seeking to it, we skip past the ByRef tag:
            metadata::blob const new_signature(
                &signature.scope(),
                signature.seek_to(metadata::type_signature::part::cross_module_type_reference),
                signature.end_bytes());

            return compute_type(new_signature);
        }

        metadata::type_def_ref_spec_or_signature const next_type([&]() -> metadata::type_def_ref_spec_or_signature
        {
            switch (signature.get_kind())
            {
            case metadata::type_signature::kind::general_array:
                return metadata::blob(signature.array_type());

            case metadata::type_signature::kind::class_type:
                return signature.class_type();

            case metadata::type_signature::kind::function_pointer:
                core::assert_not_yet_implemented();

            case metadata::type_signature::kind::generic_instance:
                return signature.generic_type();

            case metadata::type_signature::kind::primitive:
                return loader_context::from(t.scope()).resolve_fundamental_type(signature.primitive_type());

            case metadata::type_signature::kind::pointer:
                return metadata::blob(signature.pointer_type());

            case metadata::type_signature::kind::simple_array:
                return metadata::blob(signature.array_type());

            case metadata::type_signature::kind::variable:
                // A Class or Method Variable is never itself a TypeDef:
                return metadata::type_def_ref_spec_or_signature();

            default:
                throw core::runtime_error(L"unknown signature type");
            }
        }());

        if (!next_type.is_initialized())
            return metadata::type_def_or_signature();

        return compute_type(next_type);
    }

    auto resolve_element_type(metadata::type_def_ref_or_signature const t) -> metadata::type_def_or_signature
    {
        core::assert_initialized(t);

        return resolve_type(compute_element_type(t));
    }

    auto compute_primary_type(metadata::type_def_ref_or_signature const t) -> metadata::type_def_ref_token
    {
        core::assert_initialized(t);

        if (t.is_token())
            return t.as_token();

        metadata::type_def_ref_or_signature current(t);
        while (current.is_initialized() && current.is_blob())
        {
            metadata::type_def_ref_or_signature const next(compute_element_type(current));
            if (!next.is_initialized())
                break;

            current = next;
        }

        if (!current.is_initialized() || !current.is_token())
            return metadata::type_def_token();

        return current.as_token();
    }

    auto resolve_primary_type(metadata::type_def_ref_or_signature const t) -> metadata::type_def_token
    {
        core::assert_initialized(t);

        metadata::type_def_or_signature const resolved_type(resolve_type(compute_primary_type(t)));

        // Since we never pass a signature to resolve_type, we should never get a signature back:
        core::assert_true([&]{ return resolved_type.is_token(); });

        return resolved_type.as_token();
    }

} } }
