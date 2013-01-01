
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/metadata/metadata.hpp"

// Metadata Signature Builder
//
// This library is for test purposes only, hence it is not part of the acutal CxxReflect Metadata
// library.  It defines a set of utilities for constructing metadata signatures in a more convenient
// fashion than writing out byte arrays.  The logic here is slow and dynamic allocation heavy, but
// it is good enough to enable building some unit and performance tests for the signature parsing
// and instantiation logic in the Metadata library.

namespace cxxreflect { namespace metadata { namespace signature_builder {

    /// Gets the invalid, fake scope used to identify tokens to be encoded without a scope.
    ///
    /// The token types all include the scope in which the token is to be resolved.  In a metadata
    /// signature, however, not all encoded token values are scoped (only signatures fabricated
    /// during instantiation are scoped; signatures defined in a database are never scoped), so
    /// when we construct signatures for testing we need a fake scope that we can use to identify
    /// "unscoped" tokens, so that we do not encode the scope.  This scope is used for that
    /// purpose:  it is a pointer at the end of the address range, which can never be a valid 
    /// database pointer.
    inline auto unscoped() -> database const*
    {
        return reinterpret_cast<database const*>(static_cast<std::uintptr_t>(-1));
    }





    /// The type of buffer into which signatures are encoded; this is a sequence container of bytes
    typedef std::vector<core::byte> buffer_type;





    /// Represents a composable node for use during signature composition
    ///
    /// This is the value type wrapper that we use to compose signatures.  The size_type template
    /// parameter is unused; it simply allows us to have different kinds of nodes, for better type
    /// checking later.
    template <core::size_type>
    class signature_node
    {
    private:

        class base_signature_node_data
        {
        public:

            virtual auto copy() const -> std::unique_ptr<base_signature_node_data> = 0;
            virtual auto emit(buffer_type& buffer) const -> void = 0;

            virtual ~base_signature_node_data()
            {
            }
        };

        template <typename T>
        class derived_signature_node_data : public base_signature_node_data
        {
        public:

            derived_signature_node_data(T x)
                : _x(std::move(x))
            {
            }

            virtual auto copy() const -> std::unique_ptr<base_signature_node_data> override
            {
                return core::make_unique<derived_signature_node_data<T>>(_x);
            }

            virtual auto emit(buffer_type& buffer) const -> void
            {
                return _x.emit(buffer);
            }

        private:

            T _x;
        };

    public:

        signature_node()
        {
        }

        template <typename T>
        explicit signature_node(T x)
            : _x(core::make_unique<derived_signature_node_data<T>>(std::move(x)))
        {
        }

        signature_node(signature_node const& other)
            : _x(other._x ? other._x->copy() : nullptr)
        {
        }

        signature_node(signature_node&& other)
            : _x(std::move(other._x))
        {
        }

        auto operator=(signature_node const& other) -> signature_node&
        {
            _x = other._x ? other._x->copy() : nullptr;
            return *this;
        }

        auto operator=(signature_node&& other) -> signature_node&
        {
            _x = std::move(other._x);
            return *this;
        }

        auto emit(buffer_type& buffer) const -> void
        {
            core::assert_not_null(_x.get());
            _x->emit(buffer);
        }

        auto is_initialized() const -> bool
        {
            return _x.get() != nullptr;
        }

    private:

        std::unique_ptr<base_signature_node_data> _x;
    };

    // Specializations of signature_node used throughout this library
    typedef signature_node<0> method_def_node;
    typedef signature_node<1> field_node;
    typedef signature_node<2> property_node;
    typedef signature_node<3> custom_mod_node;
    typedef signature_node<4> param_node;
    typedef signature_node<5> ret_type_node;
    typedef signature_node<6> type_node;
    typedef signature_node<7> array_shape_node;





    /// Represents an "owned" signature
    ///
    /// When an owned_signature is constructed, it encodes the signature from the node tree into a
    /// buffer.  It takes ownership of that buffer and provides access to it as a 'Signature'.
    template <typename Signature>
    class owned_signature
    {
    public:

        template <typename Node>
        owned_signature(database const* const scope, Node const& n)
            : _scope(scope)
        {
            n.emit(_bytes);
        }

        auto get() const -> Signature
        {
            return Signature(_scope, _bytes.data(), _bytes.data() + _bytes.size());
        }

    private:

        database const*   _scope;
        std::vector<core::byte> _bytes;
    };





    // These emit functions are based on the logic defined in ECMA 335 and the functions defined in
    // <cor.h> in the Windows SDK.  The metadata library contains the functions to undo these
    // transformations.

    enum
    {
        sign_mask_one  = 0xffffffc0,
        sign_mask_two  = 0xffffe000,
        sign_mask_four = 0xf0000000,
    };

    inline auto emit_compressed_unsigned(buffer_type& buffer, core::size_type n) -> void
    {
        if (n <= 0x7f)
        {
            buffer.push_back(static_cast<core::byte>(n));
            return;
        }
    
        else if (n <= 0x3fff)
        {
            buffer.push_back(static_cast<core::byte>((n >> 8) | 0x80));
            buffer.push_back(static_cast<core::byte>((n     ) & 0xff));
            return;
        }
    
        else if (n <= 0x1fffffff)
        {
            buffer.push_back(static_cast<core::byte>((n >> 24) | 0xc0));
            buffer.push_back(static_cast<core::byte>((n >> 16) & 0xff));
            buffer.push_back(static_cast<core::byte>((n >>  8) & 0xff));
            buffer.push_back(static_cast<core::byte>((n      ) & 0xff));
            return;
        }

        core::assert_fail();
    }

    inline auto emit_compressed_signed(buffer_type& buffer, core::difference_type n) -> void
    {
        core::size_type const negative_tag(n < 0);
    
        if ((n & sign_mask_one) == 0 || (n & sign_mask_one) == sign_mask_one)
        {
            n = static_cast<core::difference_type>((n & ~sign_mask_one) << 1 | negative_tag);
            buffer.push_back(static_cast<core::byte>(n));
            return;
        }

        else if ((n & sign_mask_two) == 0 || (n & sign_mask_two) == sign_mask_two)
        {
            n = static_cast<core::difference_type>((n & ~sign_mask_two) << 1 | negative_tag);
            buffer.push_back(static_cast<core::byte>((n >> 8) | 0x80));
            buffer.push_back(static_cast<core::byte>((n     ) & 0xff));
            return;
        }

        else if ((n & sign_mask_four) == 0 || (n & sign_mask_four) == sign_mask_four)
        {
            n = static_cast<core::difference_type>((n & ~sign_mask_four) << 1 | negative_tag);
            buffer.push_back(static_cast<core::byte>((n >> 24) | 0xc0));
            buffer.push_back(static_cast<core::byte>((n >> 16) & 0xff));
            buffer.push_back(static_cast<core::byte>((n >>  8) & 0xff));
            buffer.push_back(static_cast<core::byte>((n      ) & 0xff));
            return;
        }

        core::assert_fail();
    }

    inline auto emit_compressed_element_type(buffer_type& buffer, element_type const e) -> void
    {
        buffer.push_back(static_cast<core::byte>(e));
    }

    inline auto emit_compressed_token(buffer_type& buffer, type_def_ref_spec_token const t) -> void
    {
        core::size_type const tag([&]() -> core::size_type
        {
            switch (t.table())
            {
            case table_id::type_def:  return 0x00;
            case table_id::type_ref:  return 0x01;
            case table_id::type_spec: return 0x02;
            default:                  core::assert_unreachable();
            }
        }());

        core::size_type const value(((t.value() & 0x00ffffff) << 2) | tag);
        emit_compressed_unsigned(buffer, value);

        if (&t.scope() != unscoped())
        {
            database const* const scope(&t.scope());
            std::copy(core::begin_bytes(scope), core::end_bytes(scope), std::back_inserter(buffer));
        }
    }





    /// Type used by the accumulate_sequence macro to generate a vector<T>
    ///
    /// The idea here is that when we have a function that accepts a vector<T>, we'd like to be able
    /// to construct that vector<T> inline when calling that function.  If we had variadic templates
    /// or initializer lists, this would be easy.  But no, we are not so fortunate.
    ///
    /// The vector_accumulator is a type that overloads the comma operator to allow expressions like
    ///
    ///     vector_accumulator<T>(), 1, 2, 3
    ///
    /// to generate a vector containing the contents { 1, 2, 3 }.  The vector_accumulator<T> itself
    /// is implicitly convertible to a vector<T>.  The accumulate_sequence macro then makes use of
    /// this type to transform its variadic arguments into a vector<T>.
    ///
    /// It's kind of ugly, but it's type-checked and makes the tests easier to write.
    template <typename T>
    class vector_accumulator
    {
    public:

        template <typename U>
        operator std::vector<U>() const
        {
            return std::vector<U>(begin(_data), end(_data));
        }

        template <typename U>
        auto operator,(U&& x) -> vector_accumulator&
        {
            _data.push_back(x);
            return *this;
        }

    private:

        std::vector<T> _data;
    };

    #define CXXREFLECT_ACCUMULATE_SEQUENCE_HEAD(head, ...) head

    #define accumulate_sequence(...)                                           \
        (                                                                      \
            ::sb::vector_accumulator<                                          \
                typename ::std::remove_reference<                              \
                    decltype(CXXREFLECT_ACCUMULATE_SEQUENCE_HEAD(__VA_ARGS__)) \
                >::type                                                        \
            >(),                                                               \
            __VA_ARGS__                                                        \
        )





    // Signature Policies and Construction Functions
    //
    // Do not use the policies directly--instead, call the make_{k} functions, which delegate to the
    // appropriate policy constructor and construct the right kind of node on the fly.





    //
    // MethodDefSig
    //

    class method_def_policy
    {
    public:

        method_def_policy(signature_flags flags, core::size_type n, ret_type_node ret_type, std::vector<param_node> params)
            : _flags(std::move(flags)), _gen_param_count(std::move(n)), _ret_type(std::move(ret_type)), _params(std::move(params))
        {
        }

        auto emit(buffer_type& buffer) const -> void
        {
            buffer.push_back(_flags.integer());

            if (_flags.is_set(signature_attribute::generic_))
                emit_compressed_unsigned(buffer, _gen_param_count);

            emit_compressed_unsigned(buffer, static_cast<core::size_type>(_params.size()));
            _ret_type.emit(buffer);
            core::for_all(_params, [&](param_node const& p) { p.emit(buffer); });
        }

    private:

        signature_flags          _flags;
        core::size_type         _gen_param_count;
        ret_type_node           _ret_type;
        std::vector<param_node> _params;
    };

    inline auto make_method_def(signature_flags flags, ret_type_node ret_type, std::vector<param_node> params) -> method_def_node
    {
        return method_def_node(method_def_policy(std::move(flags), 0, std::move(ret_type), std::move(params)));
    }

    inline auto make_generic_method_def(signature_flags         flags,
                                        core::size_type         gen_param_count,
                                        ret_type_node           ret_type,
                                        std::vector<param_node> params) -> method_def_node
    {
        flags.set(signature_attribute::generic_);
        return method_def_node(method_def_policy(
            std::move(flags),
            std::move(gen_param_count),
            std::move(ret_type),
            std::move(params)));
    }




    //
    // FieldSig
    //

    class field_policy
    {
    public:

        field_policy(type_node t, std::vector<custom_mod_node> mods)
            : _t(std::move(t)), _mods(std::move(mods))
        {
        }

        auto emit(buffer_type& buffer) const -> void
        {
            buffer.push_back(static_cast<core::byte>(signature_attribute::field));
            core::for_all(_mods, [&](custom_mod_node const& n) { n.emit(buffer); });
            _t.emit(buffer);
        }

    private:

        type_node                    _t;
        std::vector<custom_mod_node> _mods;
    };

    inline auto make_field(type_node t) -> field_node
    {
        return field_node(field_policy(std::move(t), std::vector<custom_mod_node>()));
    }

    inline auto make_field(type_node t, std::vector<custom_mod_node> mods) -> field_node
    {
        return field_node(field_policy(std::move(t), std::move(mods)));
    }





    //
    // PropertySig
    //

    class property_policy
    {
    public:

        property_policy(signature_flags flags, type_node t, std::vector<param_node> params, std::vector<custom_mod_node> mods)
            : _flags(std::move(flags)), _t(std::move(t)), _params(std::move(params)), _mods(std::move(mods))
        {
            _flags.set(signature_attribute::property_);
        }

        auto emit(buffer_type& buffer) const -> void
        {
            buffer.push_back(_flags.integer());
            emit_compressed_unsigned(buffer, static_cast<core::size_type>(_params.size()));
            core::for_all(_mods, [&](custom_mod_node const& n) { n.emit(buffer); });
            _t.emit(buffer);
            core::for_all(_params, [&](param_node const& n) { n.emit(buffer); });
        }

    private:

        signature_flags              _flags;
        type_node                    _t;
        std::vector<param_node>      _params;
        std::vector<custom_mod_node> _mods;
    };

    auto make_property(signature_flags flags, type_node t) -> property_node
    {
        return property_node(property_policy(std::move(flags), std::move(t), std::vector<param_node>(), std::vector<custom_mod_node>()));
    }

    auto make_property(signature_flags flags, type_node t, std::vector<param_node> p) -> property_node
    {
        return property_node(property_policy(std::move(flags), std::move(t), std::move(p), std::vector<custom_mod_node>()));
    }

    auto make_property(signature_flags flags, type_node t, std::vector<custom_mod_node> mods) -> property_node
    {
        return property_node(property_policy(std::move(flags), std::move(t), std::vector<param_node>(), std::move(mods)));
    }

    auto make_property(signature_flags flags, type_node t, std::vector<param_node> p, std::vector<custom_mod_node> mods) -> property_node
    {
        return property_node(property_policy(std::move(flags), std::move(t), std::move(p), std::move(mods)));
    }





    //
    // CustomMod
    //

    class custom_mod_policy
    {
    public:

        custom_mod_policy(element_type e, type_def_ref_spec_token t)
            : _e(std::move(e)), _t(std::move(t))
        {
            switch (_e)
            {
            case element_type::custom_modifier_optional:
            case element_type::custom_modifier_required:
                break;

            default:
                core::assert_fail();
            }
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, _e);
            emit_compressed_token       (buffer, _t);
        }

    private:

        element_type            _e;
        type_def_ref_spec_token _t;
    };

    inline auto make_optional_custom_modifier(type_def_ref_spec_token t) -> custom_mod_node
    {
        return custom_mod_node(custom_mod_policy(element_type::custom_modifier_optional, std::move(t)));
    }

    inline auto make_required_custom_modifier(type_def_ref_spec_token t) -> custom_mod_node
    {
        return custom_mod_node(custom_mod_policy(element_type::custom_modifier_required, std::move(t)));
    }





    //
    // Param
    //

    class param_policy
    {
    public:

        param_policy(element_type type_tag, bool is_by_ref, type_node t, std::vector<custom_mod_node> mods)
            : _type_tag(std::move(type_tag)), _is_by_ref(std::move(is_by_ref)), _t(std::move(t)), _mods(std::move(mods))
        {
            core::assert_true([&]{ return (_type_tag == element_type::end) == _t.is_initialized(); });

            switch (_type_tag)
            {
            case element_type::end:
            case element_type::typed_by_ref:
                break;

            default:
                core::assert_fail();
            }
        }

        auto emit(buffer_type& buffer) const -> void
        {
            core::for_all(_mods, [&](custom_mod_node const& n) { n.emit(buffer); });
            if (_is_by_ref)
                emit_compressed_element_type(buffer, element_type::by_ref);

            _type_tag == element_type::end
                ? _t.emit(buffer)
                : emit_compressed_element_type(buffer, _type_tag);
        }

    private:

        type_node                    _t;
        element_type                 _type_tag;
        bool                         _is_by_ref;
        std::vector<custom_mod_node> _mods;
    };

    inline auto make_param(type_node t) -> param_node
    {
        return param_node(param_policy(element_type::end, false, std::move(t), std::vector<custom_mod_node>()));
    }

    inline auto make_param(type_node t, std::vector<custom_mod_node> mods) -> param_node
    {
        return param_node(param_policy(element_type::end, false, std::move(t), std::move(mods)));
    }

    inline auto make_param(element_type e) -> param_node
    {
        return param_node(param_policy(std::move(e), false, type_node(), std::vector<custom_mod_node>()));
    }

    inline auto make_param(element_type e, std::vector<custom_mod_node> mods) -> param_node
    {
        return param_node(param_policy(std::move(e), false, type_node(), std::move(mods)));
    }

    inline auto make_by_ref_param(type_node t) -> param_node
    {
        return param_node(param_policy(element_type::end, true, std::move(t), std::vector<custom_mod_node>()));
    }

    inline auto make_by_ref_param(type_node t, std::vector<custom_mod_node> mods) -> param_node
    {
        return param_node(param_policy(element_type::end, true, std::move(t), std::move(mods)));
    }

    inline auto make_by_ref_param(element_type e) -> param_node
    {
        return param_node(param_policy(std::move(e), true, type_node(), std::vector<custom_mod_node>()));
    }

    inline auto make_by_ref_param(element_type e, std::vector<custom_mod_node> mods) -> param_node
    {
        return param_node(param_policy(std::move(e), true, type_node(), std::move(mods)));
    }





    //
    // RetType
    //

    class ret_type_policy
    {
    public:

        ret_type_policy(element_type type_tag, bool is_by_ref, type_node t, std::vector<custom_mod_node> mods)
            : _type_tag(std::move(type_tag)), _is_by_ref(std::move(is_by_ref)), _t(std::move(t)), _mods(std::move(mods))
        {
            core::assert_true([&]{ return (_type_tag == element_type::end) == _t.is_initialized(); });

            switch (_type_tag)
            {
            case element_type::end:
            case element_type::typed_by_ref:
            case element_type::void_type:
                break;

            default:
                core::assert_fail();
            }
        }

        auto emit(buffer_type& buffer) const -> void
        {
            core::for_all(_mods, [&](custom_mod_node const& n) { n.emit(buffer); });
            if (_is_by_ref)
                emit_compressed_element_type(buffer, element_type::by_ref);

            _type_tag == element_type::end
                ? _t.emit(buffer)
                : emit_compressed_element_type(buffer, _type_tag);
        }

    private:

        type_node                    _t;
        element_type                 _type_tag;
        bool                         _is_by_ref;
        std::vector<custom_mod_node> _mods;
    };

    inline auto make_ret_type(type_node t) -> ret_type_node
    {
        return ret_type_node(ret_type_policy(element_type::end, false, std::move(t), std::vector<custom_mod_node>()));
    }

    inline auto make_ret_type(type_node t, std::vector<custom_mod_node> mods) -> ret_type_node
    {
        return ret_type_node(ret_type_policy(element_type::end, false, std::move(t), std::move(mods)));
    }

    inline auto make_ret_type(element_type e) -> ret_type_node
    {
        return ret_type_node(ret_type_policy(e, false, type_node(), std::vector<custom_mod_node>()));
    }

    inline auto make_ret_type(element_type e, std::vector<custom_mod_node> mods) -> ret_type_node
    {
        return ret_type_node(ret_type_policy(e, false, type_node(), std::move(mods)));
    }

    inline auto make_by_ref_ret_type(type_node t) -> ret_type_node
    {
        return ret_type_node(ret_type_policy(element_type::end, true, std::move(t), std::vector<custom_mod_node>()));
    }

    inline auto make_by_ref_ret_type(type_node t, std::vector<custom_mod_node> mods) -> ret_type_node
    {
        return ret_type_node(ret_type_policy(element_type::end, true, std::move(t), std::move(mods)));
    }

    inline auto make_by_ref_ret_type(element_type e) -> ret_type_node
    {
        return ret_type_node(ret_type_policy(e, true, type_node(), std::vector<custom_mod_node>()));
    }

    inline auto make_by_ref_ret_type(element_type e, std::vector<custom_mod_node> mods) -> ret_type_node
    {
        return ret_type_node(ret_type_policy(e, true, type_node(), std::move(mods)));
    }





    //
    // Type
    //

    class fundamental_type_policy
    {
    public:

        fundamental_type_policy(element_type e)
            : _e(std::move(e))
        {
            switch (_e)
            {
            case element_type::boolean:
            case element_type::character:
            case element_type::i1:
            case element_type::u1:
            case element_type::i2:
            case element_type::u2:
            case element_type::i4:
            case element_type::u4:
            case element_type::i8:
            case element_type::u8:
            case element_type::r4:
            case element_type::r8:
            case element_type::i:
            case element_type::u:
            case element_type::object:
            case element_type::string:
                break;

            default:
                core::assert_fail();
            }
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, _e);
        }

    private:

        element_type _e;
    };

    inline auto make_fundamental_type(element_type e) -> type_node
    {
        return type_node(fundamental_type_policy(std::move(e)));
    }

    class general_array_type_policy
    {
    public:

        general_array_type_policy(type_node t, array_shape_node s)
            : _t(std::move(t)), _s(std::move(s))
        {
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, element_type::array);
            _t.emit(buffer);
            _s.emit(buffer);
        }

    private:

        type_node        _t;
        array_shape_node _s;
    };

    inline auto make_general_array_type(type_node t, array_shape_node s) -> type_node
    {
        return type_node(general_array_type_policy(std::move(t), std::move(s)));
    }

    class class_type_policy
    {
    public:

        class_type_policy(element_type e, type_def_ref_spec_token t)
            : _e(std::move(e)), _t(std::move(t))
        {
            switch (_e)
            {
            case element_type::class_type:
            case element_type::value_type:
                break;

            default:
                core::assert_fail();
            }
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, _e);
            emit_compressed_token       (buffer, _t);
        }

    private:

        element_type            _e;
        type_def_ref_spec_token _t;
    };

    inline auto make_class_type(type_def_ref_spec_token t) -> type_node
    {
        return type_node(class_type_policy(element_type::class_type, std::move(t)));
    }

    inline auto make_value_type(type_def_ref_spec_token t) -> type_node
    {
        return type_node(class_type_policy(element_type::value_type, std::move(t)));
    }

    class fnptr_type_policy
    {
    public:

        fnptr_type_policy(method_def_node s)
            : _s(std::move(s))
        {
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, element_type::fn_ptr);
            _s.emit(buffer);
        }

    private:

        method_def_node _s;
    };

    inline auto make_fnptr_type(method_def_node s) -> type_node
    {
        return type_node(fnptr_type_policy(std::move(s)));
    }

    class generic_inst_type_policy
    {
    public:

        generic_inst_type_policy(element_type e, type_def_ref_spec_token t, std::vector<type_node> a)
            : _e(std::move(e)), _t(std::move(t)), _a(std::move(a))
        {
            switch (_e)
            {
            case element_type::class_type:
            case element_type::value_type:
                break;

            default:
                core::assert_fail();
            }
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, element_type::generic_inst);
            emit_compressed_element_type(buffer, _e);
            emit_compressed_token       (buffer, _t);
            emit_compressed_unsigned    (buffer, static_cast<core::size_type>(_a.size()));
            core::for_all(_a, [&](type_node const& n) { n.emit(buffer); });
        }

    private:

        element_type            _e;
        type_def_ref_spec_token _t;
        std::vector<type_node>       _a;
    };

    inline auto make_generic_inst_class_type(type_def_ref_spec_token t, std::vector<type_node> a) -> type_node
    {
        return type_node(generic_inst_type_policy(element_type::class_type, std::move(t), std::move(a)));
    }

    inline auto make_generic_inst_value_type(type_def_ref_spec_token t, std::vector<type_node> a) -> type_node
    {
        return type_node(generic_inst_type_policy(element_type::value_type, std::move(t), std::move(a)));
    }

    class variable_type_policy
    {
    public:

        variable_type_policy(element_type e, core::size_type n)
            : _e(std::move(e)), _n(std::move(n))
        {
            switch (e)
            {
            case element_type::mvar:
            case element_type::var:
                break;

            default:
                core::assert_fail();
            }
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, _e);
            emit_compressed_unsigned    (buffer, _n);
        }

    private:

        element_type _e;
        core::size_type    _n;
    };

    inline auto make_method_variable(core::size_type n) -> type_node
    {
        return type_node(variable_type_policy(element_type::mvar, std::move(n)));
    }

    inline auto make_class_varibale(core::size_type n) -> type_node
    {
        return type_node(variable_type_policy(element_type::var, std::move(n)));
    }

    class pointer_type_policy
    {
    public:

        pointer_type_policy(type_node t, std::vector<custom_mod_node> mods)
            : _t(std::move(t)), _mods(std::move(mods))
        {
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, element_type::ptr);
            core::for_all(_mods, [&](custom_mod_node const& m) { m.emit(buffer); });
            _t.is_initialized()
                ? _t.emit(buffer)
                : emit_compressed_element_type(buffer, element_type::void_type);
        }

    private:

        type_node                    _t;
        std::vector<custom_mod_node> _mods;
    };

    inline auto make_void_pointer_type() -> type_node
    {
        return type_node(pointer_type_policy(type_node(), std::vector<custom_mod_node>()));
    }

    inline auto make_void_pointer_type(std::vector<custom_mod_node> mods) -> type_node
    {
        return type_node(pointer_type_policy(type_node(), std::move(mods)));
    }

    inline auto make_pointer_type(type_node t) -> type_node
    {
        return type_node(pointer_type_policy(std::move(t), std::vector<custom_mod_node>()));
    }

    inline auto make_pointer_type(type_node t, std::vector<custom_mod_node> mods) -> type_node
    {
        return type_node(pointer_type_policy(std::move(t), std::move(mods)));
    }

    class szarray_type_policy
    {
    public:

        szarray_type_policy(type_node t, std::vector<custom_mod_node> mods)
            : _t(std::move(t)), _mods(std::move(mods))
        {
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_element_type(buffer, element_type::sz_array);
            core::for_all(_mods, [&](custom_mod_node const& m) { m.emit(buffer); });
            _t.emit(buffer);
        }

    private:

        type_node                    _t;
        std::vector<custom_mod_node> _mods;
    };

    inline auto make_simple_array_type(type_node t) -> type_node
    {
        return type_node(szarray_type_policy(std::move(t), std::vector<custom_mod_node>()));
    }

    inline auto make_simple_array_type(type_node t, std::vector<custom_mod_node> mods) -> type_node
    {
        return type_node(szarray_type_policy(std::move(t), std::move(mods)));
    }





    //
    // ArrayShape
    //

    class array_shape_policy
    {
    public:

        array_shape_policy(core::size_type rank, std::vector<core::size_type> sizes, std::vector<core::size_type> lo_bounds)
            : _rank(std::move(rank)), _sizes(std::move(sizes)), _lo_bounds(std::move(lo_bounds))
        {
        }

        auto emit(buffer_type& buffer) const -> void
        {
            emit_compressed_unsigned(buffer, _rank);
            emit_compressed_unsigned(buffer, static_cast<core::size_type>(_sizes.size()));
            core::for_all(_sizes, [&](core::size_type const n) { emit_compressed_unsigned(buffer, n); });
            
            emit_compressed_unsigned(buffer, static_cast<core::size_type>(_lo_bounds.size()));
            core::for_all(_sizes, [&](core::size_type const n) { emit_compressed_signed(buffer, n); });
        }

    private:

        core::size_type              _rank;
        std::vector<core::size_type> _sizes;
        std::vector<core::size_type> _lo_bounds;
    };

    inline auto make_array_shape(core::size_type rank)
        -> array_shape_node
    {
        return array_shape_node(array_shape_policy(std::move(rank), std::vector<core::size_type>(), std::vector<core::size_type>()));
    }

    inline auto make_array_shape(core::size_type rank, std::vector<core::size_type> sizes)
        -> array_shape_node
    {
        return array_shape_node(array_shape_policy(std::move(rank), std::move(sizes), std::vector<core::size_type>()));
    }

    inline auto make_array_shape(core::size_type rank, std::vector<core::size_type> sizes, std::vector<core::size_type> lo_bounds)
        -> array_shape_node
    {
        return array_shape_node(array_shape_policy(std::move(rank), std::move(sizes), std::move(lo_bounds)));
    }

} } }
