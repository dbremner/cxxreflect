
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_DETAIL_CALL_INVOKER_X64_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_DETAIL_CALL_INVOKER_X64_HPP_

#include "cxxreflect/reflection/reflection.hpp"

#if defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION) && CXXREFLECT_ARCHITECTURE == CXXREFLECT_ARCHITECTURE_X64

#include "cxxreflect/windows_runtime/detail/argument_handling.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    extern "C"
    {
        /// Thunk for dynamically invoking an x64 fastcall function
        ///
        /// __fastcall is the only x64 calling convention, so this thunk supports invoking any
        /// arbitrary function dynamically.  Its declaration must exactly match the declaration
        /// described in the .asm file that defines the procedure.
        ///
        /// Be very, very careful when calling this.  :-O
        int cxxreflect_windows_runtime_x64_fastcall_thunk(void const* fp, void const* args, void const* types, std::uint64_t count);
    }





    /// Flags for the 'types' array passed to the fastcall thunk
    ///
    /// The enumerator values must match those described in the documentation of the thunk procedure
    enum class x64_argument_type : std::uint64_t
    {
        /// Tag for any integer type or pointer type, or any struct eight bytes or fewer in size
        integer               = 0,

        /// Tag for a double-precision (eight byte) floating point value
        double_precision_real = 1,

        /// Tag for a single-precision (four byte) floating point value
        single_precision_real = 2
    };





    /// Frame builder that constructs an argument frame in the form required by the fastcall thunk
    class x64_argument_frame
    {
    public:

        auto arguments() const -> void const*   { return _arguments.data(); }
        auto types()     const -> void const*   { return _types.data();     }
        auto count()     const -> std::uint64_t { return _arguments.size(); }

        auto push(float const x) -> void
        {
            _arguments.insert(_arguments.end(), core::begin_bytes(x), core::end_bytes(x));
            _arguments.resize(_arguments.size() + (8 - sizeof(float)));
            _types.push_back(x64_argument_type::single_precision_real);
        }

        auto push(double const x) -> void
        {
            _arguments.insert(_arguments.end(), core::begin_bytes(x), core::end_bytes(x));
            _types.push_back(x64_argument_type::double_precision_real);
        }

        template <typename T>
        auto push(T const& x) -> typename std::enable_if<sizeof(T) <= 8>::type
        {
            _arguments.insert(_arguments.end(), core::begin_bytes(x), core::end_bytes(x));
            _arguments.resize(_arguments.size() + (8 - sizeof(T)));
            _types.push_back(x64_argument_type::integer);
        }
        
    private:

        std::vector<core::byte>        _arguments;
        std::vector<x64_argument_type> _types;

    };





    /// Call invoker for x64 fastcall functions
    class x64_fastcall_invoker
    {
    public:

        static auto invoke(reflection::method    const& method,
                           IInspectable               * instance,
                           void                       * result,
                           variant_argument_pack const& arguments) -> core::hresult;

    private:

        static auto convert_and_insert(reflection::type          const& parameter_type,
                                       resolved_variant_argument const& argument,
                                       x64_argument_frame             & frame) -> void;

    };






    typedef x64_argument_frame   argument_frame;
    typedef x64_fastcall_invoker call_invoker;

} } }

#endif // #if defined(ENABLE_WINDOWS_RUNTIME_INTEGRATION) && ARCHITECTURE == ARCHITECTURE_X64
#endif 

// AMDG //
