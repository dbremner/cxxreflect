
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_TYPE_NAME_BUILDER_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_TYPE_NAME_BUILDER_HPP_

#include "cxxreflect/metadata/metadata.hpp"

#include "cxxreflect/reflection/assembly_name.hpp"
#include "cxxreflect/reflection/detail/element_contexts.hpp"
#include "cxxreflect/reflection/detail/forward_declarations.hpp"
#include "cxxreflect/reflection/loader_configuration.hpp"
#include "cxxreflect/reflection/module_locator.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    class type_name_builder
    {
    public:
        
        enum class mode
        {
            simple_name,
            full_name,
            assembly_qualified_name
        };

        static auto build_type_name(type const& t, mode m) -> core::string;

    private:

        type_name_builder(type const& t, mode m);

        type_name_builder(type_name_builder const&);
        auto operator=(type_name_builder const&) -> type_name_builder&;

        operator core::string();

        auto accumulate_type_name                      (type const& t, mode m) -> bool;

        auto accumulate_type_def_name                  (type const& t, mode m) -> bool;
        auto accumulate_type_spec_name                 (type const& t, mode m) -> bool;
        
        auto accumulate_class_type_spec_name           (type const& t, mode m) -> bool;
        auto accumulate_method_signature_spec_name(type const& t, mode m) -> bool;
        auto accumulate_general_array_type_spec_name   (type const& t, mode m) -> bool;
        auto accumulate_generic_instance_type_spec_name(type const& t, mode m) -> bool;
        auto accumulate_pointer_type_spec_name         (type const& t, mode m) -> bool;
        auto accumulate_primitive_type_spec_name       (type const& t, mode m) -> bool;
        auto accumulate_simple_array_type_spec_name    (type const& t, mode m) -> bool;
        auto accumulate_variable_type_spec_name        (type const& t, mode m) -> bool;

        auto accumulate_assembly_qualification_if_required(type const& t, mode m) -> void;

        static auto without_assembly_qualification(mode m) -> mode;

        core::string _buffer;
    };

} } }

#endif

// AMDG //
