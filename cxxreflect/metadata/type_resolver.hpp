
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_METADATA_TYPE_RESOLVER_HPP_
#define CXXREFLECT_METADATA_TYPE_RESOLVER_HPP_

#include "cxxreflect/metadata/tokens.hpp"

namespace cxxreflect { namespace metadata {

    /// An interface class for resolving TypeRef tokens into TypeDef or TypeSpec tokens
    ///
    /// This is the only extensibility point of the Metadata library.  A particular object graph in
    /// the Metadata library only has a view of a single metadata database and its contents.  A
    /// TypeRef token may refer to an entity in another metadata database.  In order to perform this
    /// cross-module type resolution, we require some other component to implement this interface
    /// and provide an instance of it when TypeRef resolution may need to take place.
    class type_resolver
    {
    public:

        /// Resolves a MemberRef token into the Field or MethodDef token to which it refers
        ///
        /// If the target of the reference cannot be found or if an error occurs, the implementer is
        /// to throw a `metadata_error`.  Note that the referenced member may be a member of a
        /// generic type; if it is, its signature may require instantiation.  The returned member
        /// will be the uninstantiated declaration.  To get the instantiated member, re-resolve it
        /// via its declaring type (from the MemberRef).
        virtual auto resolve_member(member_ref_token) const -> field_or_method_def_token = 0;

        /// Resolves a TypeRef token into the TypeDef or TypeSpec token to which it refers
        ///
        /// The argument is a TypeDef, TypeRef, or TypeSpec token.  If it is a TypeDef or TypeSpec
        /// token, the implementer must return the token unchanged.  If it is a TypeRef token, the
        /// implementer must resolve the token and return the TypeDef or TypeSpec token to which
        /// it refers.
        ///
        /// If the target of the reference cannot be found or if an error occurs, the implementer is
        /// to throw a `metadata_error`.
        virtual auto resolve_type(type_def_ref_spec_token) const -> type_def_spec_token = 0;

        /// Resolves the TypeDef token that represents a fundamental type
        ///
        /// The element type `e` must be one of the concrete element types (i.e., it must have a
        /// value less than `element_type::concrete_element_type_max`) and its value must not be
        /// `end`, `by_ref`, `generic_inst`, or `typed_by_ref`.
        ///
        /// The type resolver is responsible for resolving the type in the type universe's 
        /// system assembly.  If it fails to resolve the type, it must throw a `metadata_error`.
        virtual auto resolve_fundamental_type(element_type) const -> type_def_token = 0;

        virtual ~type_resolver() { }
    };

} }

#endif 
