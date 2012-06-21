
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_ENUMERATION_HPP_
#define CXXREFLECT_CORE_ENUMERATION_HPP_

#include "cxxreflect/core/utility.hpp"

namespace cxxreflect { namespace core { namespace internal {

    template <size_type N> struct underlying_type_impl;
    template <> struct underlying_type_impl<1> { typedef std::uint8_t  type; };
    template <> struct underlying_type_impl<2> { typedef std::uint16_t type; };
    template <> struct underlying_type_impl<4> { typedef std::uint32_t type; };
    template <> struct underlying_type_impl<8> { typedef std::uint64_t type; };

} } }

namespace cxxreflect { namespace core {

    /// A basic `underlying_type` metafunction for enumeration types
    ///
    /// MinGW does not yet support `underlying_type`, so for compatibility we provide our own when
    /// we cannot find one in the Standard Library.  This assumes that the enumeration uses an
    /// unsigned underlying type, which is true for all CxxReflect enumerations.
    template <typename Enumeration>
    struct underlying_type
    {
        #ifdef CXXREFLECT_CONFIG_STDLIB_HAS_UNDERLYING_TYPE
        typedef typename std::underlying_type<Enumeration>::type type;
        #else
        typedef typename internal::underlying_type_impl<sizeof(Enumeration)>::type type;
        #endif
    };





    /// A function that converts an enumeration value to its underlying integer representation
    ///
    /// This is simply a cast to the enumeration's underlying type, but allows us to perform the 
    /// cast without explicitly stating the type and without writing out the ugly cast all over the
    /// place.
    template <typename Enumeration>
    auto as_integer(Enumeration const value) -> typename underlying_type<Enumeration>::type
    {
        return static_cast<typename underlying_type<Enumeration>::type>(value);
    }





    #define CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EVAL_EVAL(E, Op)                                   \
        inline auto operator Op(E const lhs, E const rhs) -> E                                                  \
        {                                                                                                       \
            return static_cast<E>(::cxxreflect::core::as_integer(lhs)                                           \
                               Op ::cxxreflect::core::as_integer(rhs));                                         \
        }

    #define CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EREF_EVAL(E, Op)                                   \
        inline auto operator Op##=(E& lhs, E const rhs) -> E&                                                   \
        {                                                                                                       \
            lhs = static_cast<E>(::cxxreflect::core::as_integer(lhs)                                            \
                              Op ::cxxreflect::core::as_integer(rhs));                                          \
            return lhs;                                                                                         \
        }

    #define CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE(E, Op)                                     \
        inline auto operator Op(E const lhs, ::cxxreflect::core::underlying_type<E>::type rhs) -> bool          \
        {                                                                                                       \
            return ::cxxreflect::core::as_integer(lhs) Op rhs;                                                  \
        }                                                                                                       \
                                                                                                                \
        inline auto operator Op(::cxxreflect::core::underlying_type<E>::type const lhs, E const rhs) -> bool    \
        {                                                                                                       \
            return lhs Op ::cxxreflect::core::as_integer(rhs);                                                  \
        }

    #define CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(E)                                                        \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EVAL_EVAL(E, |)                                        \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EVAL_EVAL(E, &)                                        \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EVAL_EVAL(E, ^)                                        \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EREF_EVAL(E, |)                                        \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EREF_EVAL(E, &)                                        \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_EREF_EVAL(E, ^)                                        \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, ==)                                       \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, !=)                                       \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, < )                                       \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, > )                                       \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, <=)                                       \
        CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS_BINARY_COMPARE  (E, >=)





    /// A bitflags helper
    template <typename Enumeration>
    class flags
    {
    public:

        typedef Enumeration                                 enumeration_type;
        typedef typename underlying_type<Enumeration>::type integer_type;

        static_assert(std::is_enum<enumeration_type>::value, "T must be an enumeration type");
        static_assert(std::is_unsigned<integer_type>::value, "T's underlying type must be unsigned");

        flags()                             : _value(0)                 { }
        flags(enumeration_type const value) : _value(as_integer(value)) { }
        flags(integer_type     const value) : _value(value)             { }

        auto enumerator() const -> enumeration_type { return static_cast<enumeration_type>(_value); }
        auto integer()    const -> integer_type     { return _value;                                }

        auto set(enumeration_type const mask) -> void { _value |= as_integer(mask); }
        auto set(integer_type     const mask) -> void { _value |= mask;             }

        auto unset(enumeration_type const mask) -> void { _value &= ~as_integer(mask); }
        auto unset(integer_type     const mask) -> void { _value &= ~mask;             }

        auto reset() -> void { _value = 0; }

        auto is_set(enumeration_type const mask) const -> bool { return with_mask(mask) != 0; }
        auto is_set(integer_type     const mask) const -> bool { return with_mask(mask) != 0; }

        auto with_mask(enumeration_type const mask) const -> flags { return _value & as_integer(mask); }
        auto with_mask(integer_type     const mask) const -> flags { return _value & mask;             }

        friend auto operator==(flags const& lhs, flags const& rhs) -> bool { return lhs._value == rhs._value; }
        friend auto operator< (flags const& lhs, flags const& rhs) -> bool { return lhs._value <  rhs._value; }

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(flags)

    private:
        
        integer_type _value;
    };

} }

#endif

// AMDG //
