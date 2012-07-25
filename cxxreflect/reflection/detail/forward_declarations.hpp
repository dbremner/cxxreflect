
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_DETAIL_FORWARD_DECLARATIONS_HPP_
#define CXXREFLECT_REFLECTION_DETAIL_FORWARD_DECLARATIONS_HPP_

#include "cxxreflect/metadata/metadata.hpp"

namespace cxxreflect { namespace reflection {

    class assembly;
    class assembly_name;
    class constant;
    class custom_attribute;
    class event;
    class field;
    class file;
    class guid;
    class method;
    class module;
    class module_location;
    class parameter;
    class type;
    class version;

    typedef core::instantiating_iterator
    <
        metadata::token_with_arithmetic<metadata::custom_attribute_token>::type,
        custom_attribute,
        module
    > custom_attribute_iterator;

    using std::begin;
    using std::end;

} }

namespace cxxreflect { namespace reflection { namespace detail {

    class assembly_context;
    class loader_context;
    class module_context;

} } }

#endif 
