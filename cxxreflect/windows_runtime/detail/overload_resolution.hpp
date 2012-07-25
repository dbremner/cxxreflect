
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_DETAIL_OVERLOAD_RESOLUTION_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_DETAIL_OVERLOAD_RESOLUTION_HPP_

#include "cxxreflect/reflection/reflection.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/detail/argument_handling.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    class overload_resolver
    {
    public:

        enum class conversion_rank : std::uint32_t
        {
            no_match                        = 0xffffffff,

            exact_match                     = 0x00000000,
            integral_promotion              = 0x00010000,
            real_conversion                 = 0x00020000,
            derived_to_base_conversion      = 0x00040000,
            derived_to_interface_conversion = 0x00080000
        };

        enum class comparative_rank : std::uint8_t
        {
            no_match,
            better_match,
            same_match,
            worse_match
        };

        enum class state
        {
            not_evaluated,
            evaluated
        };

        template <typename InputIterator>
        overload_resolver(InputIterator const first, InputIterator const last, variant_argument_pack const arguments)
            : _candidates(first, last), _arguments(arguments)
        {
        }

        auto succeeded() const -> bool;
        auto result()    const -> reflection::method;

    private:

        static auto compute_conversion_rank(reflection::type const& parameter_type,
                                            reflection::type const& argument_type) -> conversion_rank;

        static auto compute_class_conversion_rank(reflection::type const& parameter_type,
                                                  reflection::type const& argument_type) -> conversion_rank;

        static auto compute_numeric_conversion_rank(metadata::element_type parameter_type,
                                                    metadata::element_type argument_type) -> conversion_rank;

        /// Performs overload resolution and sets the state and result
        auto evaluate() const -> void;

        core::value_initialized<state> mutable _state;
        reflection::method             mutable _result;

        std::vector<reflection::method>        _candidates;
        variant_argument_pack                  _arguments;
    };

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(overload_resolver::conversion_rank);





    /// Computes the `element_type` for use in overload resolution
    ///
    /// This is not necessarily the actual `element_type` that best fits `type`.  For example,
    /// `Object^` has its own `element_type`, but this will return `class_type` because that is
    /// how the `Object^` type is considered during overload resolution.
    auto compute_overload_element_type(reflection::type const&) -> metadata::element_type;

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 
