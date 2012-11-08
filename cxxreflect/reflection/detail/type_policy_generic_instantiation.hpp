
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_GENERIC_INSTANCE_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_POLICY_GENERIC_INSTANCE_HPP_

#include "cxxreflect/reflection/detail/type_policy_specialization.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    /// A base type policy for generic type instantiations type specializations (signatures)
    class generic_instantiation_type_policy final : public specialization_type_policy
    {
    public:

        virtual auto is_generic_type_instantiation(unresolved_type_context const&) const -> bool override;



        virtual auto is_generic_type(resolved_type_context const&) const -> bool override;
        virtual auto is_visible     (resolved_type_context const&) const -> bool override;

        virtual auto metadata_token(resolved_type_context const&) const -> core::size_type override;
    };

} } }

#endif
