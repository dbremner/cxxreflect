
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_INDEPENDENT_HANDLES_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_INDEPENDENT_HANDLES_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    class parameter_data
    {
    public:

        parameter_data();

        // Note:  This constructor takes an InternalKey only so that it matches other constructors
        // of types with which the parameter iterator is instantiated.
        parameter_data(metadata::param_token                          const& token,
                       metadata::method_signature::parameter_iterator const& signature,
                       core::internal_key);

        auto token()     const -> metadata::param_token    const&;
        auto signature() const -> metadata::type_signature const&;

        auto is_initialized() const -> bool;

        auto operator++()    -> parameter_data&;
        auto operator++(int) -> parameter_data;

        friend auto operator==(parameter_data const&, parameter_data const&) -> bool;
        friend auto operator< (parameter_data const&, parameter_data const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(parameter_data)

    private:

        metadata::param_token                          _token;
        metadata::method_signature::parameter_iterator _signature;
    };

} } }

#endif
