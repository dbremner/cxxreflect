
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_CONSTANT_HPP_
#define CXXREFLECT_REFLECTION_CONSTANT_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"

namespace cxxreflect { namespace reflection {

    /// A constant value from metadata, usually associated with a field, property, or parameter
    class constant
    {
    public:

        enum class kind
        {
            /// Indicates the constant has an unknown kind and attempts to get its value will fail
            unknown,

            boolean,
            character,
            int8,
            uint8,
            int16,
            uint16,
            int32,
            uint32,
            int64,
            uint64,
            single_precision,
            double_precision,
            string,

            /// Indicates the constant has class type, which means its value is a nullptr
            class_type
        };

        constant();

        auto get_kind() const -> kind;

        auto as_boolean()   const -> bool;
        auto as_character() const -> wchar_t;
        auto as_int8()      const -> std::int8_t;
        auto as_uint8()     const -> std::uint8_t;
        auto as_int16()     const -> std::int16_t;
        auto as_uint16()    const -> std::uint16_t;
        auto as_int32()     const -> std::int32_t;
        auto as_uint32()    const -> std::uint32_t;
        auto as_int64()     const -> std::int64_t;
        auto as_uint64()    const -> std::uint64_t;
        auto as_float()     const -> float;
        auto as_double()    const -> double;
        // TODO auto as_string()    const -> core::string_reference;
        
        auto is_initialized() const -> bool;

    public: // internal members

        constant(metadata::constant_token const& element, core::internal_key);

        static auto create_for(metadata::has_constant_token const& parent, core::internal_key) -> constant;

    private:

        auto row() const -> metadata::constant_row;

        metadata::constant_token _constant;
    };

} }

#endif 
