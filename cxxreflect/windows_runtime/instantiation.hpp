
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_INSTANTIATION_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_INSTANTIATION_HPP_

#include "cxxreflect/windows_runtime/common.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/detail/argument_handling.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    /// The instantiator:  all of the instantiation functions defer to this one
    auto create_inspectable_instance(reflection::type      const& type,
                                     variant_argument_pack const& arguments) -> unique_inspectable;

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW
    inline auto create_object_instance(reflection::type      const& type,
                                       variant_argument_pack const& arguments) -> ::Platform::Object^ 
    {
        return reinterpret_cast<::Platform::Object^>(create_inspectable_instance(type, arguments).get());
    }

    template <typename Target>
    auto create_instance(reflection::type      const& type,
                         variant_argument_pack const& arguments) -> Target^
    {
        return dynamic_cast<Target^>(create_object_instance(type, arguments));
    }
    #endif

} } }

namespace cxxreflect { namespace windows_runtime {

    auto create_inspectable_instance(reflection::type const& type) -> unique_inspectable;

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW
    inline auto create_object_instance(reflection::type const& type) -> Platform::Object^
    {
        return reinterpret_cast<Platform::Object^>(create_inspectable_instance(type).release());
    }

    template <typename T>
    auto create_instance(reflection::type const& type) -> T^
    {
        return dynamic_cast<T^>(create_object_instance(type));
    }
    #endif




    template <typename P0>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0)));
    }

    template <typename P0, typename P1>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1)));
    }

    template <typename P0, typename P1, typename P2>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1, P2&& a2) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2)));
    }

    template <typename P0, typename P1, typename P2, typename P3>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1, P2&& a2, P3&& a3) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                     P5&& a5) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                     P5&& a5, P6&& a6) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                     P5&& a5, P6&& a6, P7&& a7) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7, typename P8>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                     P5&& a5, P6&& a6, P7&& a7, P8&& a8) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7),
            std::forward<P8>(a8)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7, typename P8, typename P9>
    auto create_inspectable_instance(reflection::type const& type,
                                     P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                     P5&& a5, P6&& a6, P7&& a7, P8&& a8, P9&& a9) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7),
            std::forward<P8>(a8),
            std::forward<P9>(a9)));
    }





    template <typename ForwardIterator>
    auto create_inspectable_instance_from_arguments(reflection::type const& type,
                                                    ForwardIterator  const  first_argument,
                                                    ForwardIterator  const  last_argument) -> unique_inspectable
    {
        return detail::create_inspectable_instance(type, detail::pack_argument_range(first_argument, last_argument));
    }





    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_ZW
    template <typename P0>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0)));
    }

    template <typename P0, typename P1>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1)));
    }

    template <typename P0, typename P1, typename P2>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1, P2&& a2) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2)));
    }

    template <typename P0, typename P1, typename P2, typename P3>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1, P2&& a2, P3&& a3) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                P5&& a5) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                P5&& a5, P6&& a6) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                P5&& a5, P6&& a6, P7&& a7) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7, typename P8>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                P5&& a5, P6&& a6, P7&& a7, P8&& a8) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7),
            std::forward<P8>(a8)));
    }

    template <typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7, typename P8, typename P9>
    auto create_object_instance(reflection::type const& type,
                                P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                                P5&& a5, P6&& a6, P7&& a7, P8&& a8, P9&& a9) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7),
            std::forward<P8>(a8),
            std::forward<P9>(a9)));
    }





    template <typename ForwardIterator>
    auto create_object_instance_from_arguments(reflection::type const& type,
                                               ForwardIterator  const  first_argument,
                                               ForwardIterator  const  last_argument) -> Platform::Object^
    {
        return detail::create_object_instance(type, detail::pack_argument_range(first_argument, last_argument));
    }





    template <typename Target,
              typename P0>
    auto create_instance(reflection::type const& type,
                         P0&& a0) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0)));
    }

    template <typename Target,
              typename P0, typename P1>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1)));
    }

    template <typename Target,
              typename P0, typename P1, typename P2>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1, P2&& a2) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2)));
    }

    template <typename Target,
              typename P0, typename P1, typename P2, typename P3>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1, P2&& a2, P3&& a3) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3)));
    }

    template <typename Target,
              typename P0, typename P1, typename P2, typename P3, typename P4>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4)));
    }

    template <typename Target,
              typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                         P5&& a5) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5)));
    }

    template <typename Target,
              typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                         P5&& a5, P6&& a6) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6)));
    }

    template <typename Target,
              typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                         P5&& a5, P6&& a6, P7&& a7) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7)));
    }

    template <typename Target,
              typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7, typename P8>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                         P5&& a5, P6&& a6, P7&& a7, P8&& a8) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7),
            std::forward<P8>(a8)));
    }

    template <typename Target,
              typename P0, typename P1, typename P2, typename P3, typename P4,
              typename P5, typename P6, typename P7, typename P8, typename P9>
    auto create_instance(reflection::type const& type,
                         P0&& a0, P1&& a1, P2&& a2, P3&& a3, P4&& a4,
                         P5&& a5, P6&& a6, P7&& a7, P8&& a8, P9&& a9) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_arguments(
            std::forward<P0>(a0),
            std::forward<P1>(a1),
            std::forward<P2>(a2),
            std::forward<P3>(a3),
            std::forward<P4>(a4),
            std::forward<P5>(a5),
            std::forward<P6>(a6),
            std::forward<P7>(a7),
            std::forward<P8>(a8),
            std::forward<P9>(a9)));
    }





    template <typename Target, typename ForwardIterator>
    auto create_instance_from_arguments(reflection::type const& type,
                                        ForwardIterator  const  first_argument,
                                        ForwardIterator  const  last_argument) -> Target^
    {
        return detail::create_instance<Target>(type, detail::pack_argument_range(first_argument, last_argument));
    }
    #endif

} }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif 

// AMDG //
