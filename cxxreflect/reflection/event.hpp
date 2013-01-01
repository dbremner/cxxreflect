
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_EVENT_HPP_
#define CXXREFLECT_REFLECTION_EVENT_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"

namespace cxxreflect { namespace reflection {

    class event
    {
    public:

        event();
        event(type const& reflected_type, detail::event_table_entry const* context, core::internal_key);

        auto context(core::internal_key) const -> detail::event_table_entry const&;

        auto declaring_type() const -> type;
        auto reflected_type() const -> type;

        auto is_initialized() const -> bool;
        auto operator!()      const -> bool;

        // Attributes
        // DeclaringType
        // EventHandlerType
        // IsMulticast
        // IsSpecialName
        // MemberType
        // MetadataToken
        // Module
        // Name
        // ReflectedType

        // AddEventHandler
        // GetAddMethod
        // GetCustomAttributes
        // GetOtherMethods
        // GetRaiseMethod
        // GetRemoveMethod
        // IsDefined
        // RemoveEventHandler

        friend auto operator==(event const&, event const&) -> bool;
        friend auto operator< (event const&, event const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(event)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(event)

    private:

        auto row() const -> metadata::event_row;

        metadata::type_def_or_signature                        _reflected_type;
        core::checked_pointer<detail::event_table_entry const> _context;
    };

} }

#endif 
