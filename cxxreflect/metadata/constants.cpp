
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/precompiled_headers.hpp"
#include "cxxreflect/metadata/constants.hpp"

namespace cxxreflect { namespace metadata {

    auto is_valid_table_id(core::size_type const table) -> bool
    {
        static std::array<core::byte, 0x40> const mask =
        { {
            1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        } };

        return table < mask.size() && mask[table] == 1;
    }

    auto is_valid_table_id(table_id const table) -> bool
    {
        return is_valid_table_id(core::as_integer(table));
    }

    auto table_mask_for(table_id const table) -> table_mask
    {
        core::assert_true([&]{ return is_valid_table_id(table); });
        return static_cast<table_mask>(1ull << static_cast<integer_table_mask>(table));
    }

    auto table_id_for(table_mask const mask) -> table_id
    {
        core::assert_true([&]{ return core::pop_count(core::as_integer(mask)) == 1; });

        core::size_type const value(core::log_base_2(core::as_integer(mask)));

        core::assert_true([&]{ return is_valid_table_id(value); });

        return static_cast<table_id>(value);
    }

    auto table_id_for(composite_index_key const key, composite_index const index) -> table_id
    {
        switch (index)
        {
        case composite_index::custom_attribute_type:
            switch (key)
            {
            case 2:  return table_id::method_def;
            case 3:  return table_id::member_ref;
            default: return invalid_table_id;
            }

        case composite_index::has_constant:
            switch (key)
            {
            case 0:  return table_id::field;
            case 1:  return table_id::param;
            case 2:  return table_id::property;
            default: return invalid_table_id;
            }

        case composite_index::has_custom_attribute:
            switch (key)
            {
            case  0:  return table_id::method_def;
            case  1:  return table_id::field;
            case  2:  return table_id::type_ref;
            case  3:  return table_id::type_def;
            case  4:  return table_id::param;
            case  5:  return table_id::interface_impl;
            case  6:  return table_id::member_ref;
            case  7:  return table_id::module;
            case  8:  return table_id::decl_security;
            case  9:  return table_id::property;
            case 10:  return table_id::event;
            case 11:  return table_id::standalone_sig;
            case 12:  return table_id::module_ref;
            case 13:  return table_id::type_spec;
            case 14:  return table_id::assembly;
            case 15:  return table_id::assembly_ref;
            case 16:  return table_id::file;
            case 17:  return table_id::exported_type;
            case 18:  return table_id::manifest_resource;
            case 19:  return table_id::generic_param;
            case 20:  return table_id::generic_param_constraint;
            case 21:  return table_id::method_spec;
            default:  return invalid_table_id;
            }

        case composite_index::has_decl_security:
            switch (key)
            {
            case 0:  return table_id::type_def;
            case 1:  return table_id::method_def;
            case 2:  return table_id::assembly;
            default: return invalid_table_id;
            }

        case composite_index::has_field_marshal:
            switch (key)
            {
            case 0:  return table_id::field;
            case 1:  return table_id::param;
            default: return invalid_table_id;
            }

        case composite_index::has_semantics:
            switch (key)
            {
            case 0:  return table_id::event;
            case 1:  return table_id::property;
            default: return invalid_table_id;
            }

        case composite_index::implementation:
            switch (key)
            {
            case 0:  return table_id::file;
            case 1:  return table_id::assembly_ref;
            case 2:  return table_id::exported_type;
            default: return invalid_table_id;
            }

        case composite_index::member_forwarded:
            switch (key)
            {
            case 0:  return table_id::field;
            case 1:  return table_id::method_def;
            default: return invalid_table_id;
            }

        case composite_index::member_ref_parent:
            switch (key)
            {
            case 0:  return table_id::type_def;
            case 1:  return table_id::type_ref;
            case 2:  return table_id::module_ref;
            case 3:  return table_id::method_def;
            case 4:  return table_id::type_spec;
            default: return invalid_table_id;
            }

        case composite_index::method_def_or_ref:
            switch (key)
            {
            case 0:  return table_id::method_def;
            case 1:  return table_id::member_ref;
            default: return static_cast<table_id>(-1);
            }

        case composite_index::resolution_scope:
            switch (key)
            {
            case 0:  return table_id::module;
            case 1:  return table_id::module_ref;
            case 2:  return table_id::assembly_ref;
            case 3:  return table_id::type_ref;
            default: return invalid_table_id;
            }

        case composite_index::type_def_ref_spec:
            switch (key)
            {
            case 0:  return table_id::type_def;
            case 1:  return table_id::type_ref;
            case 2:  return table_id::type_spec;
            default: return invalid_table_id;
            }

        case composite_index::type_or_method_def:
            switch (key)
            {
            case 0:  return table_id::type_def;
            case 1:  return table_id::method_def;
            default: return invalid_table_id;
            }

        default:
            return invalid_table_id;
        }
    }
    
    auto index_key_for(table_id const table, composite_index const index) -> composite_index_key
    {
        switch (index)
        {
        case composite_index::custom_attribute_type:
            switch (table)
            {
            case table_id::method_def: return 2;
            case table_id::member_ref: return 3;
            default:                   return core::max_size_type;
            }

        case composite_index::has_constant:
            switch (table)
            {
            case table_id::field:    return 0;
            case table_id::param:    return 1;
            case table_id::property: return 2;
            default:                 return core::max_size_type;
            }

        case composite_index::has_custom_attribute:
            switch (table)
            {
            case table_id::method_def:               return  0;
            case table_id::field:                    return  1;
            case table_id::type_ref:                 return  2;
            case table_id::type_def:                 return  3;
            case table_id::param:                    return  4;
            case table_id::interface_impl:           return  5;
            case table_id::member_ref:               return  6;
            case table_id::module:                   return  7;
            case table_id::decl_security:            return  8;
            case table_id::property:                 return  9;
            case table_id::event:                    return 10;
            case table_id::standalone_sig:           return 11;
            case table_id::module_ref:               return 12;
            case table_id::type_spec:                return 13;
            case table_id::assembly:                 return 14;
            case table_id::assembly_ref:             return 15;
            case table_id::file:                     return 16;
            case table_id::exported_type:            return 17;
            case table_id::manifest_resource:        return 18;
            case table_id::generic_param:            return 19;
            case table_id::generic_param_constraint: return 20;
            case table_id::method_spec:              return 21;
            default:                                 return core::max_size_type;
            }

        case composite_index::has_decl_security:
            switch (table)
            {
            case table_id::type_def:   return 0;
            case table_id::method_def: return 1;
            case table_id::assembly:   return 2;
            default:                   return core::max_size_type;
            }

        case composite_index::has_field_marshal:
            switch (table)
            {
            case table_id::field: return 0;
            case table_id::param: return 1;
            default:              return core::max_size_type;
            }

        case composite_index::has_semantics:
            switch (table)
            {
            case table_id::event:    return 0;
            case table_id::property: return 1;
            default:                  return core::max_size_type;
            }

        case composite_index::implementation:
            switch (table)
            {
            case table_id::file:          return 0;
            case table_id::assembly_ref:  return 1;
            case table_id::exported_type: return 2;
            default:                      return core::max_size_type;
            }

        case composite_index::member_forwarded:
            switch (table)
            {
            case table_id::field:      return 0;
            case table_id::method_def: return 1;
            default:                   return core::max_size_type;
            }

        case composite_index::member_ref_parent:
            switch (table)
            {
            case table_id::type_def:   return 0;
            case table_id::type_ref:   return 1;
            case table_id::module_ref: return 2;
            case table_id::method_def: return 3;
            case table_id::type_spec:  return 4;
            default:                   return core::max_size_type;
            }

        case composite_index::method_def_or_ref:
            switch (table)
            {
            case table_id::method_def: return 0;
            case table_id::member_ref: return 1;
            default:                   return core::max_size_type;
            }

        case composite_index::resolution_scope:
            switch (table)
            {
            case table_id::module:       return 0;
            case table_id::module_ref:   return 1;
            case table_id::assembly_ref: return 2;
            case table_id::type_ref:     return 3;
            default:                     return core::max_size_type;
            }

        case composite_index::type_def_ref_spec:
            switch (table)
            {
            case table_id::type_def:  return 0;
            case table_id::type_ref:  return 1;
            case table_id::type_spec: return 2;
            default:                  return core::max_size_type;
            }

        case composite_index::type_or_method_def:
            switch (table)
            {
            case table_id::type_def:   return 0;
            case table_id::method_def: return 1;
            default:                   return core::max_size_type;
            }

        default:
            return core::max_size_type;
        }
    }

    auto is_valid_composite_index(core::size_type index) -> bool
    {
        return index < composite_index_count;
    }

    auto is_valid_composite_index(composite_index index) -> bool
    {
        return core::as_integer(index) < composite_index_count;
    }

    auto is_valid_element_type(core::byte const id) -> bool
    {
        static std::array<core::byte, 0x60> const mask =
        { {
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 0, 1, 1, 1, 1, 1,
            1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1
        } };

        return id < mask.size() && mask[id] == 1;
    }

    auto is_type_element_type(core::byte const id) -> bool
    {
        static std::array<core::byte, 0x20> const mask =
        { {
            0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            0, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 0
        } };

        return id < mask.size() && mask[id] == 1;
    }

    auto is_custom_modifier_element_type(core::byte const id) -> bool
    {
        return id == element_type::custom_modifier_optional
            || id == element_type::custom_modifier_required;
    }

    auto is_integer_element_type(element_type const type) -> bool
    {
        switch (type)
        {
        case element_type::i1:
        case element_type::u1:
        case element_type::i2:
        case element_type::u2:
        case element_type::i4:
        case element_type::u4:
        case element_type::i8:
        case element_type::u8:
            return true;

        default:
            return false;
        }
    }

    auto is_signed_integer_element_type(element_type const type) -> bool
    {
        switch (type)
        {
        case element_type::i1:
        case element_type::i2:
        case element_type::i4:
        case element_type::i8:
            return true;

        default:
            return false;
        }
    }

    auto is_unsigned_integer_element_type(element_type const type) -> bool
    {
        switch (type)
        {
        case element_type::u1:
        case element_type::u2:
        case element_type::u4:
        case element_type::u8:
            return true;

        default:
            return false;
        }
    }

    auto is_real_element_type(element_type const type) -> bool
    {
        return type == element_type::r4 || type == element_type::r8;
    }

    auto is_numeric_element_type(element_type const type) -> bool
    {
        return is_integer_element_type(type) || is_real_element_type(type);
    }

} }

// AMDG //
