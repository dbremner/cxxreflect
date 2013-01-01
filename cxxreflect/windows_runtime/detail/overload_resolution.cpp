
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/detail/overload_resolution.hpp"
#include "cxxreflect/reflection/detail/type_hierarchy.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    auto overload_resolver::succeeded() const -> bool
    {
        evaluate();

        return _result.is_initialized();
    }

    auto overload_resolver::result() const -> reflection::method
    {
        evaluate();

        if (!_result.is_initialized())
            throw core::logic_error(L"overload resolution failed; call 'succeeded' to test result first");

        return _result;
    }

    auto overload_resolver::compute_conversion_rank(reflection::type const& parameter_type,
                                                    reflection::type const& argument_type) -> conversion_rank
    {
        core::assert_initialized(parameter_type);
        core::assert_initialized(argument_type);

        metadata::element_type const p_kind(compute_overload_element_type(parameter_type));
        metadata::element_type const a_kind(compute_overload_element_type(argument_type));

        // If both logical types are the same, this is an exact match and we can go home early:
        if (parameter_type == argument_type)
        {
            return conversion_rank::exact_match;
        }

        // Value types, boolean, char, and string only match exactly.  We do not allow any
        // conversions to or from these types:
        if (p_kind == metadata::element_type::value_type || a_kind == metadata::element_type::value_type || 
            p_kind == metadata::element_type::boolean    || a_kind == metadata::element_type::boolean    || 
            p_kind == metadata::element_type::character  || a_kind == metadata::element_type::character  || 
            p_kind == metadata::element_type::string     || a_kind == metadata::element_type::string)
        {
            return p_kind == a_kind ? conversion_rank::exact_match : conversion_rank::no_match;
        }

        // A class type may match another class type via derived-to-base or interface conversions:
        if (p_kind == metadata::element_type::class_type && a_kind == metadata::element_type::class_type)
        {
            return compute_class_conversion_rank(parameter_type, argument_type);
        }
        
        // But, a class type is not convertible to any type other than another class type:
        if (p_kind == metadata::element_type::class_type || a_kind == metadata::element_type::class_type)
        {
            return conversion_rank::no_match;
        }

        // Many numeric types allow conversions:
        if (is_numeric_element_type(p_kind) && is_numeric_element_type(a_kind))
        {
            return compute_numeric_conversion_rank(p_kind, a_kind);
        }

        // Hmmm, what other types can we have?  :-)
        core::assert_not_yet_implemented();
    }

    auto overload_resolver::compute_class_conversion_rank(reflection::type const& parameter_type,
                                                          reflection::type const& argument_type) -> conversion_rank
    {
        core::assert_true([&]{ return parameter_type.is_initialized() && !parameter_type.is_value_type(); });
        core::assert_true([&]{ return argument_type.is_initialized()  && !argument_type.is_value_type();  });
        core::assert_true([&]{ return parameter_type != argument_type;                                    });

        // If the parameter is a class type, we'll need a derived-to-base conversion.  To compute
        // the distance, we'll walk the base class hierarchy of the argument to try to find the
        // parameter type:
        if (parameter_type.is_class())
        {
            unsigned base_distance(1);

            // TODO Write a type hierarchy iterator
            reflection::type base_type(argument_type.base_type());
            while (base_type)
            {
                if (base_type == parameter_type)
                    return conversion_rank::derived_to_base_conversion | static_cast<conversion_rank>(base_distance);

                base_type = base_type.base_type();
                ++base_distance;
            }

            // Well, we tried...
            return conversion_rank::no_match;
        }

        // If the parameter type is an interface, we'll need to see if the argument type implements
        // the interface:
        if (parameter_type.is_interface())
        {
            auto const it(std::find(begin(argument_type.interfaces()), end(argument_type.interfaces()), parameter_type));
            return it != end(argument_type.interfaces())
                ? conversion_rank::derived_to_interface_conversion
                : conversion_rank::no_match;
        }

        core::assert_unreachable();
    }

    auto overload_resolver::compute_numeric_conversion_rank(metadata::element_type parameter_type,
                                                            metadata::element_type argument_type) -> conversion_rank
    {
        core::assert_true([&]{ return is_numeric_element_type(parameter_type); });
        core::assert_true([&]{ return is_numeric_element_type(argument_type);  });
        core::assert_true([&]{ return parameter_type != argument_type;         });

        if (is_integer_element_type(parameter_type) && is_integer_element_type(argument_type))
        {
            // Signed -> unsigned and unsigned -> signed conversions are not permitted:
            if (is_signed_integer_element_type(parameter_type) != is_signed_integer_element_type(argument_type))
                return conversion_rank::no_match;

            // Narrowing conversions are not permitted:
            if (parameter_type < argument_type)
                return conversion_rank::no_match;

            unsigned const raw_distance(core::as_integer(parameter_type) - core::as_integer(argument_type));
            core::assert_true([&]{ return raw_distance % 2 == 0; });

            unsigned const conversion_distance(raw_distance / 2);
            return conversion_rank::integral_promotion | static_cast<conversion_rank>(conversion_distance);
        }

        // Real -> Integral conversions are not permitted:
        if (is_integer_element_type(parameter_type))
            return conversion_rank::no_match;

        // Integral -> Real conversions are permitted:
        if (is_integer_element_type(argument_type))
            return conversion_rank::real_conversion;

        core::assert_true([&]{ return is_real_element_type(parameter_type) && is_real_element_type(argument_type); });

        // Double precision -> Single precision conversion is not permitted:
        if (parameter_type == metadata::element_type::r4 && argument_type  == metadata::element_type::r8)
            return conversion_rank::no_match;

        // Single precision -> Double precision conversion is permitted:
        return conversion_rank::real_conversion;
    }

    auto overload_resolver::evaluate() const -> void
    {
        if (_state.get() != state::not_evaluated)
            return;

        _state.get() = state::evaluated;

        // Resolve and accumulate the argument types once, for performance:
        std::vector<reflection::type> argument_types(_arguments.arity());
        std::transform(begin(_arguments), end(_arguments), begin(argument_types),
                       [&](unresolved_variant_argument const& a) -> reflection::type
        {
            return _arguments.resolve(a).logical_type();
        });

        // best_match is an iterator to the current best matching method.  Until a callable method
        // is found, it points to a non-element.  best_match_rank stores the conversion rank for
        // each parameter of the candidate.  If best_match points to a candidate method, then all of
        // the ranks must necessarily not be no_match, and vice-versa.
        reflection::method           best_match;
        std::vector<conversion_rank> best_match_rank(_arguments.arity(), conversion_rank::no_match);

        // Iterate over the candidates to find the best match.  Note that we will check every method
        // because we must find the unique best match, so we must check for ambiguity.
        std::for_each(begin(_candidates), end(_candidates), [&](reflection::method const& current)
        {
            // First check to see that the arity matches.  If the arity doesn't match, this method
            // is not a viable candidate:
            if (core::distance(begin(current.parameters()), end(current.parameters())) != argument_types.size())
                return;

            // Compute the conversion rank of this method by computing the conversion rank to each
            // parameter from its corresponding argument.
            std::vector<conversion_rank> current_rank(argument_types.size(), conversion_rank::no_match);
            core::transform_all(current.parameters(), argument_types, begin(current_rank),
                           [&](reflection::parameter const& p, reflection::type const& a)
            {
                return compute_conversion_rank(p.parameter_type(), a);
            });

            // If any argument was not a match for the corresponding parameter, this method is not
            // a viable candidate, so we skip the further validation and state changes:
            if (core::contains(current_rank, conversion_rank::no_match))
                return;

            // Compute the relative goodness of this method to the best matching method to allow us
            // to order the methods:
            std::vector<comparative_rank> comparison(_arguments.arity(), comparative_rank::no_match);
            std::transform(begin(current_rank), end(current_rank), begin(best_match_rank), begin(comparison),
                           [&](conversion_rank const current, conversion_rank const best) -> comparative_rank
            {
                if (current < best)
                    return comparative_rank::better_match;

                if (best < current)
                    return comparative_rank::worse_match;

                return comparative_rank::same_match;
            });

            // Compute whether any arguments were a better match for this method than for the best
            // matching method, and whether any arguments are a worse match:
            bool const better_match(core::contains(comparison, comparative_rank::better_match));
            bool const worse_match (core::contains(comparison, comparative_rank::worse_match));

            // If some parameters are a better match for the arguments and no parameters are a worse
            // match, this method is unambiguously better than the current best match:
            if (better_match && !worse_match)
            {
                best_match      = current;
                best_match_rank = current_rank;
                return;
            }

            // If some parameters are a worse match for the arguments and no parameters are a better
            // match, this method is unambiguously worse than the current best match:
            if (!better_match && worse_match)
            {
                // The best match state already reflects the correct state:
                return;
            }

            // If some parameters are a better match and some are a worse match, or if both methods
            // have parameters that match equally well, there is an ambiguity between this match and
            // the current best match.  We will continue searching methods at this point, because
            // there may exist another method that is unambiguously better than both of these.  We
            // update the best match state to reflect the best parameters from each of the methods:
            best_match = reflection::method();
            std::transform(begin(current_rank), end(current_rank), begin(best_match_rank), begin(best_match_rank),
                           [&](conversion_rank const current, conversion_rank const best) -> conversion_rank
            {
                return std::min(current, best);
            });

            // And that's it; on to the next method!
        });

        // Whew!  That was a lot of work!
        _result = best_match;
    }

    auto compute_overload_element_type(reflection::type const& t) -> metadata::element_type
    {
        core::assert_initialized(t);

        // Shortcut:  If the type isn't from the system assembly, it isn't one of the system types:
        if (!reflection::detail::is_system_assembly(t.defining_assembly().context(core::internal_key())))
            return t.is_value_type() ? metadata::element_type::value_type : metadata::element_type::class_type;

        reflection::detail::loader_context const& root(reflection::detail::loader_context::from(t.context(core::internal_key()).scope()));

        #define CXXREFLECT_GENERATE(A)                                    \
            reflection::type const A ## _type(                            \
                root.resolve_fundamental_type(metadata::element_type::A), \
                core::internal_key());                                    \
                                                                          \
            if (t == A ## _type)                                          \
            {                                                             \
                return metadata::element_type::A;                         \
            }

        CXXREFLECT_GENERATE(boolean)
        CXXREFLECT_GENERATE(character)
        CXXREFLECT_GENERATE(i1)
        CXXREFLECT_GENERATE(u1)
        CXXREFLECT_GENERATE(i2)
        CXXREFLECT_GENERATE(u2)
        CXXREFLECT_GENERATE(i4)
        CXXREFLECT_GENERATE(u4)
        CXXREFLECT_GENERATE(i8)
        CXXREFLECT_GENERATE(u8)
        CXXREFLECT_GENERATE(r4)
        CXXREFLECT_GENERATE(r8)

        #undef CXXREFLECT_GENERATE

        return t.is_value_type() ? metadata::element_type::value_type : metadata::element_type::class_type;
    }

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
