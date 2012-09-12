
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_DETAIL_MODULE_TYPE_INDEX_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_DETAIL_MODULE_TYPE_INDEX_HPP_

#include "cxxreflect/windows_runtime/common.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace cxxreflect { namespace windows_runtime { namespace detail {

    /// An index that provides for fast (log N) lookup of a type definition by qualified name
    ///
    /// The Windows Runtime type system is type name centric:  each type has a unique namespace-
    /// qualified name, and the name can be used to determine the metadata file in which the type
    /// is defined.  However, in a metadata file, type definitions are not sorted by name (they are
    /// not sorted at all).
    ///
    /// This type takes a database and creates an index that allows fast lookup of type definitions
    /// by namespace-qualified name.  We expect that this will substantially improve the performance
    /// of some scenarios, especially those that are important for Windows Runtime interoperation.
    class module_type_index
    {
    public:

        typedef std::pair<core::string_reference, core::string_reference> type_name_pair;

        /// The `scope` must be non-null and must point to a valid, initialized `database`.  The
        /// caller is responsible for the lifetime of the scope.  This builds the index and has
        /// N log N average time complexity, where N is the number of type definitions in the
        /// database.
        module_type_index(metadata::database const* scope);

        /// Finds a type by name; returns the token identifying the type on success and a null token
        /// on failure.  The index is built during construction, so this uses a binary search and
        /// has log N time complexity, where N is the number of type definitions in the database.
        auto find(core::string_reference const& namespace_name, core::string_reference const& name) const -> metadata::type_def_token;

        /// The `operator()` overloads define a strict weak ordering over type definitions by type
        /// name (it is a less-than ordering).  For overloads that take a `size_type`, the value
        /// must be a valid token that refers to a `type_def` row in the target database.  External
        /// callers should not call these directly:  instead, call `find`.  These are public so that
        /// they may be called by both `std::sort` and `std::lower_bound`, which are used to sort
        /// and search in the index.

        auto operator()(type_name_pair  const& lhs, type_name_pair  const& rhs) const -> bool;
        auto operator()(type_name_pair  const& lhs, core::size_type const  rhs) const -> bool;
        auto operator()(core::size_type const  lhs, type_name_pair  const& rhs) const -> bool;
        auto operator()(core::size_type const  lhs, core::size_type const  rhs) const -> bool;

    private:

        auto get_name_of_type_def(core::size_type const index) const -> type_name_pair;

        core::checked_pointer<metadata::database const> _scope;

        // Note that we only index a single database, so all tokens have the same database value.
        // For compactness, we store only the integer token value in the index.  When we need to
        // get a full token with scope, we compose the token value with `_scope`.
        std::vector<core::size_type>                    _index;
    };

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 
