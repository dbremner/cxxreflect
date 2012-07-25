
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINDOWS_RUNTIME_UTILITY_HPP_
#define CXXREFLECT_WINDOWS_RUNTIME_UTILITY_HPP_

// This is a set of standalone Windows Runtime utilities that can be used even without the rest of
// the CxxReflect libraries.

#ifndef CXXREFLECT_WINDOWS_RUNTIME_UTILITY_STANDALONE
#include "cxxreflect/core/diagnostic.hpp"
#endif

#if defined(CXXREFLECT_WINDOWS_RUNTIME_UTILITY_STANDALONE) || defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION)

#include <algorithm>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

#ifndef NOMINMAX
#    define NOMINMAX
#endif

#include <cor.h>
#include <hstring.h>
#include <roapi.h>
#include <rometadataresolution.h>
#include <winstring.h>
#include <wrl/client.h>

namespace cxxreflect { namespace windows_runtime { namespace utility {

    #ifndef CXXREFLECT_WINDOWS_RUNTIME_UTILITY_STANDALONE
    typedef cxxreflect::core::hresult_error hresult_error;
    #else
    class hresult_error : public std::exception
    {
    public:

        hresult_error(HRESULT const hr = E_FAIL)
            : _hr(hr)
        {
        }

        HRESULT error() const { return _hr; }

    private:

        HRESULT _hr;
    };
    #endif

    inline void throw_on_failure(HRESULT const hr)
    {
        if (hr < 0)
            throw hresult_error(hr);
    }

    typedef std::uint32_t size_type;
    typedef std::int32_t  difference_type;

    class guarded_roinitialize
    {
    public:

        guarded_roinitialize()
        {
            HRESULT const hr(::RoInitialize(RO_INIT_MULTITHREADED));
            if (FAILED(hr))
                throw hresult_error(hr);
        }

        ~guarded_roinitialize()
        {
            ::RoUninitialize();
        }

    private:

        guarded_roinitialize(guarded_roinitialize const&);
        auto operator=(guarded_roinitialize const&) -> guarded_roinitialize&;
    };

    extern "C" __declspec(dllimport) int __stdcall AllocConsole();
    extern "C" __declspec(dllimport) int __stdcall FreeConsole();

    /// RAII wrapper for balancing AllocConsole and FreeConsole calls
    ///
    /// AllocConsole and FreeConsole are not on the approved APIs list for Windows Runtime projects,
    /// so this can't be used in an application submitted to the Windows Store.  However, for unit 
    /// tests and for debugging, everything works wonderfully.
    class guarded_console
    {
    public:

        guarded_console()
        {
            if (AllocConsole() == 0)
                throw hresult_error(E_FAIL);
        }

        ~guarded_console()
        {
            FreeConsole();
        }

    private:

        guarded_console(guarded_console const&);
        auto operator=(guarded_console const&) -> guarded_console&;
    };

} } }

namespace cxxreflect { namespace windows_runtime { namespace utility {

    /// A std::wstring-like wrapper around HSTRING
    ///
    /// Useful for Windows Runtime interop code, this class provides most of the const parts of the
    /// std::wstring interface.  For mutation, it is recommended to convert to std::wstring, mutate,
    /// then convert back to SmartHString.
    class smart_hstring
    {
    public:

        typedef wchar_t           value_type;
        typedef size_type         size_type;
        typedef difference_type   difference_type;

        typedef value_type const& reference;
        typedef value_type const& const_reference;
        typedef value_type const* pointer;
        typedef value_type const* const_pointer;

        typedef pointer           iterator;
        typedef const_pointer     const_iterator;

        typedef std::reverse_iterator<iterator>       reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        smart_hstring()
            : _value()
        {
        }

        explicit smart_hstring(const_pointer const s)
        {
            throw_on_failure(WindowsCreateString(s, static_cast<DWORD>(::wcslen(s)), &_value));
        }

        smart_hstring(smart_hstring const& other)
        {
            throw_on_failure(WindowsDuplicateString(other._value, &_value));
        }

        smart_hstring(smart_hstring&& other)
            : _value(other._value)
        {
            other._value = nullptr;
        }

        auto operator=(smart_hstring other) -> smart_hstring&
        {
            swap(other);
            return *this;
        }

        auto operator=(smart_hstring&& other) -> smart_hstring&
        {
            throw_on_failure(WindowsDeleteString(_value));
            _value = other._value;
            other._value = nullptr;
            return *this;
        }

        ~smart_hstring()
        {
            WindowsDeleteString(_value);
        }

        auto swap(smart_hstring& other) -> void
        {
            std::swap(_value, other._value);
        }

        auto begin()  const -> const_iterator { return get_buffer_begin(); }
        auto end()    const -> const_iterator { return get_buffer_end();   }
        auto cbegin() const -> const_iterator { return begin();            }
        auto cend()   const -> const_iterator { return end();              }

        auto rbegin()  const -> const_reverse_iterator { return reverse_iterator(get_buffer_end());   }
        auto rend()    const -> const_reverse_iterator { return reverse_iterator(get_buffer_begin()); }
        auto crbegin() const -> const_reverse_iterator { return rbegin();                             }
        auto crend()   const -> const_reverse_iterator { return rend();                               }

        auto size()     const -> size_type { return static_cast<size_type>(end() - begin()); }
        auto length()   const -> size_type { return size();                                  }
        auto max_size() const -> size_type { return std::numeric_limits<size_type>::max();   }
        auto capacity() const -> size_type { return size();                                  }
        auto empty()    const -> bool      { return size() == 0;                             }

        auto operator[](size_type const n) const -> const_reference
        {
            return get_buffer_begin()[n];
        }

        auto at(size_type const n) const -> const_reference
        {
            if (n >= size())
                throw hresult_error(E_BOUNDS);

            return get_buffer_begin()[n];
        }

        auto front() const -> const_reference { return *get_buffer_begin();     }
        auto back()  const -> const_reference { return *(get_buffer_end() - 1); }

        auto c_str() const -> const_pointer { return get_buffer_begin(); }
        auto data()  const -> const_pointer { return get_buffer_begin(); }

        // A reference proxy, returned by proxy(), that can be passed into a function expecting an
        // HSTRING*.  When the reference proxy is destroyed, it sets the value of the SmartHString
        // from which it was created.
        class reference_proxy
        {
        public:

            reference_proxy(smart_hstring* const value)
                : _value(value), _proxy(value->_value)
            {
            }

            ~reference_proxy()
            {
                if (_value->_value == _proxy)
                    return;

                smart_hstring new_string;
                new_string._value = _proxy;

                _value->swap(new_string);
            }

            operator HSTRING*() { return &_proxy; }

        private:

            // Note that this type is copyable though it is not intended to be copied, aside from
            // when it is returned from SmartHString::proxy().
            auto operator=(reference_proxy const&) -> reference_proxy&;

            HSTRING        _proxy;
            smart_hstring* _value;
        };

        auto proxy()       -> reference_proxy { return reference_proxy(this); }
        auto value() const -> HSTRING         { return _value;                }

        friend auto operator==(smart_hstring const& lhs, smart_hstring const& rhs) -> bool
        {
            return compare(lhs, rhs) ==  0;
        }

        friend auto operator<(smart_hstring const& lhs, smart_hstring const& rhs) -> bool
        {
            return compare(lhs, rhs) == -1;
        }

        friend auto operator!=(smart_hstring const& lhs, smart_hstring const& rhs) -> bool { return !(lhs == rhs); }
        friend auto operator> (smart_hstring const& lhs, smart_hstring const& rhs) -> bool { return   rhs <  lhs ; }
        friend auto operator>=(smart_hstring const& lhs, smart_hstring const& rhs) -> bool { return !(lhs <  rhs); }
        friend auto operator<=(smart_hstring const& lhs, smart_hstring const& rhs) -> bool { return !(rhs <  lhs); }

    private:

        auto get_buffer_begin() const -> const_pointer
        {
            const_pointer const result(WindowsGetStringRawBuffer(_value, nullptr));
            return result == nullptr ? get_empty_string() : result;
        }

        auto get_buffer_end() const -> const_pointer
        {
            std::uint32_t length(0);
            const_pointer const first(WindowsGetStringRawBuffer(_value, &length));
            return first == nullptr ? get_empty_string() : first + length;
        }

        static auto get_empty_string() -> const_pointer
        {
            static const_pointer const value(L"");
            return value;
        }

        static auto compare(smart_hstring const& lhs, smart_hstring const& rhs) -> int
        {
            std::int32_t result(0);
            throw_on_failure(WindowsCompareStringOrdinal(lhs._value, rhs._value, &result));
            return result;
        }

        HSTRING _value;
    };





    /// An RAII wrapper for a callee-allocated, caller-destroyed array of HSTRING
    ///
    /// Several low-level Windows Runtime functions allocate an array of HSTRING and require the
    /// caller to destroy the HSTRINGs and the array.  This RAII container makes that pattern much
    /// more pleasant.
    class smart_hstring_array
    {
    public:

        smart_hstring_array()
            : _count(), _array()
        {
        }

        ~smart_hstring_array()
        {
            std::for_each(begin(), end(), [](HSTRING& s)
            {
                WindowsDeleteString(s);
            });

            CoTaskMemFree(_array);
        }

        DWORD&    count() { return _count; }
        HSTRING*& array() { return _array; }

        HSTRING* begin() const { return _array;          }
        HSTRING* end()   const { return _array + _count; }

    private:

        smart_hstring_array(smart_hstring_array const&);
        auto operator=(smart_hstring_array const&) -> void;

        DWORD    _count;
        HSTRING* _array;
    };





    /// Converts an `HSTRING` to a `std::wstring`
    inline auto to_string(HSTRING const hstring) -> std::wstring
    {
        wchar_t const* const buffer(::WindowsGetStringRawBuffer(hstring, nullptr));
        return buffer == nullptr ? L"" : buffer;
    }

} } }

namespace cxxreflect { namespace windows_runtime { namespace utility {

    struct corenum_iteration_policy
    {
        typedef size_type interface_type;
        typedef size_type value_type;
        typedef size_type buffer_type;
        typedef size_type argument_type;

        static auto advance(interface_type&, HCORENUM&, buffer_type&, argument_type) -> size_type;

        static auto get(buffer_type const&, size_type) -> value_type;
    };

    template <
        typename Interface,
        typename ValueType,
        HRESULT (__stdcall Interface::*Function)(HCORENUM*, ValueType*, ULONG, ULONG*)
    >
    struct base_nullary_corenum_iteration_policy
    {
        typedef Interface                    interface_type;
        typedef ValueType                    value_type;
        typedef std::array<value_type, 128>  buffer_type;
        typedef size_type                    argument_type;

        static auto advance(interface_type     & import,
                            HCORENUM           & e,
                            buffer_type        & buffer,
                            argument_type const argument) -> unsigned
        {
            ULONG count;
            throw_on_failure((import.*Function)(&e, buffer.data(), buffer.size(), &count));
            return count;
        }

        static auto get(buffer_type const& buffer, size_type const index) -> value_type
        {
            return buffer[index];
        }
    };

    template <
        typename Interface,
        typename ValueType,
        typename Argument,
        HRESULT (__stdcall Interface::*Function)(HCORENUM*, Argument, ValueType*, ULONG, ULONG*)
    >
    struct base_unary_corenum_iteration_policy
    {
        typedef Interface                    interface_type;
        typedef ValueType                    value_type;
        typedef std::array<value_type, 128>  buffer_type;
        typedef Argument                     argument_type;

        static auto advance(interface_type     & import,
                            HCORENUM           & e,
                            buffer_type        & buffer,
                            argument_type const  argument) -> unsigned
        {
            ULONG count;
            throw_on_failure((import.*Function)(&e, argument, buffer.data(), buffer.size(), &count));
            return count;
        }

        static auto get(buffer_type const& buffer, size_type const index) -> value_type
        {
            return buffer[index];
        }
    };





    template <typename IterationPolicy>
    class corenum_iteration_context
    {
    public:

        typedef typename IterationPolicy                 policy_type;
        typedef typename IterationPolicy::interface_type interface_type;
        typedef typename IterationPolicy::value_type     value_type;
        typedef typename IterationPolicy::buffer_type    buffer_type;
        typedef typename IterationPolicy::argument_type  argument_type;

        corenum_iteration_context(interface_type* const import, argument_type const argument = argument_type())
            : _import(import), _e(), _buffer(), _current(), _count(), _argument(argument)
        {
            advance();
        }

        ~corenum_iteration_context()
        {
            close();
        }

        auto close() -> void
        {
            if (_e != nullptr)
            {
                _import->CloseEnum(_e);
                _e = nullptr;
            }
        }

        auto reset() -> void
        {
            if (_e != nullptr)
            {
                throw_on_failure(_import->ResetEnum(_e, 0));
            }
        }

        void advance()
        {
            if (_e != nullptr && _current != _count - 1)
            {
                ++_current;
            }
            else
            {
                _count = policy_type::advance(*_import, _e, _buffer, _argument);
                _current = 0;
            }
        }

        value_type current() const
        {
            return policy_type::get(_buffer, _current);
        }

        auto end() const -> bool
        {
            return _current == _count;
        }

        friend auto operator==(corenum_iteration_context const& lhs, corenum_iteration_context const& rhs) -> bool
        {
            if (lhs._e != rhs._e)
                return false;

            if (lhs._current != rhs._current)
                return false;

            ULONG lhsCount(0);
            ULONG rhsCount(0);
            if (lhs._import->CountEnum(lhs._e, &lhsCount) != 0 ||
                rhs._import->CountEnum(rhs._e, &rhsCount) != 0 ||
                lhsCount != rhsCount)
                return false;

            return true;
        }

        friend auto operator!=(corenum_iteration_context const& lhs, corenum_iteration_context const& rhs) -> bool
        {
            return !(lhs == rhs);
        }

    private:

        corenum_iteration_context(corenum_iteration_context const&);
        auto operator=(corenum_iteration_context const&) -> void;

        interface_type* _import;
        HCORENUM        _e;
        buffer_type     _buffer;
        size_type       _count;
        size_type       _current;
        argument_type   _argument;
    };





    /// An STL-compatible input iterator wrapper for HCORENUM
    template <typename IterationPolicy>
    class corenum_iterator
    {
    public:

        typedef typename IterationPolicy                 policy_type;
        typedef corenum_iteration_context<policy_type>   context_type;
        typedef typename IterationPolicy::interface_type interface_type;
        typedef typename IterationPolicy::value_type     value_type;
        typedef typename IterationPolicy::buffer_type    buffer_type;
        typedef typename IterationPolicy::argument_type  argument_type;

        typedef std::input_iterator_tag iterator_category;
        typedef value_type              value_type;
        typedef std::ptrdiff_t          difference_type;
        typedef void                    pointer;
        typedef value_type              reference;

        corenum_iterator(context_type* const context = nullptr)
            : _context(context)
        {
        }

        auto operator*() const -> value_type
        {
            return _context->Current();
        }
        
        auto operator++() -> corenum_iterator&
        {
            _context->advance();
            return *this;
        }

        auto operator++(int) -> corenum_iterator&
        {
            _context->advance();
            return *this;
        }

        friend auto operator==(corenum_iterator const& lhs, corenum_iterator const& rhs) -> bool
        {
            bool const lhs_is_end(lhs._context == nullptr || lhs._context->end());
            bool const rhs_is_end(rhs._context == nullptr || rhs._context->end());

            if (lhs_is_end && rhs_is_end)
                return true;

            if (lhs_is_end || rhs_is_end)
                return false;

            // To be comparable, both iterators must point into the same range.  Since this is an
            // input iterator (and is thus single-pass), if neither iterator is an end iterator,
            // both iterators must point to the same element in the range.
            return true;
        }

        friend auto operator!=(corenum_iterator const& lhs, corenum_iterator const& rhs) -> bool
        {
            return !(lhs == rhs);
        }

    private:

        context_type* _context;
    };





    //
    // IMetaDataImport Iterators
    //

    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdFieldDef, mdTypeDef, &IMetaDataImport::EnumEvents
    > event_corenum_iteration_policy;

    typedef corenum_iteration_context<event_corenum_iteration_policy> event_corenum_iteration_context;
    typedef corenum_iterator<event_corenum_iteration_policy>          event_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdFieldDef, mdTypeDef, &IMetaDataImport::EnumFields
    > field_corenum_iteration_policy;

    typedef corenum_iteration_context<field_corenum_iteration_policy> field_corenum_iteration_context;
    typedef corenum_iterator<field_corenum_iteration_policy>          field_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdInterfaceImpl, mdTypeDef, &IMetaDataImport::EnumInterfaceImpls
    > interface_impl_corenum_iterator_policy;

    typedef corenum_iteration_context<interface_impl_corenum_iterator_policy> interface_impl_corenum_iteration_context;
    typedef corenum_iterator<interface_impl_corenum_iterator_policy>          interface_impl_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdMemberRef, mdToken, &IMetaDataImport::EnumMemberRefs
    > member_ref_corenum_iterator_policy;

    typedef corenum_iteration_context<member_ref_corenum_iterator_policy> member_ref_corenum_iteration_context;
    typedef corenum_iterator<member_ref_corenum_iterator_policy>          member_ref_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdToken, mdTypeDef, &IMetaDataImport::EnumMembers
    > member_corenum_iterator_policy;

    typedef corenum_iteration_context<member_corenum_iterator_policy> member_corenum_iteration_context;
    typedef corenum_iterator<member_corenum_iterator_policy>          member_corenum_iterator;



    struct method_impl_corenum_iterator_policy
    {
        typedef IMetaDataImport                                               interface_type;
        typedef std::pair<mdToken, mdToken>                                   value_type;
        typedef std::pair<std::array<mdToken, 128>, std::array<mdToken, 128>> buffer_type;
        typedef mdTypeDef                                                     argument_type;

        static auto advance(interface_type     & import,
                            HCORENUM           & e,
                            buffer_type        & buffer,
                            argument_type const  argument) -> unsigned
        {
            ULONG count;
            throw_on_failure(import.EnumMethodImpls(
                &e,
                argument,
                buffer.first.data(),
                buffer.second.data(),
                static_cast<ULONG>(buffer.first.size()),
                &count));
            return count;
        }

        static auto get(buffer_type const& buffer, size_type const index) -> value_type
        {
            return std::make_pair(buffer.first[index], buffer.second[index]);
        }
    };

    typedef corenum_iteration_context<method_impl_corenum_iterator_policy> method_impl_corenum_iteration_context;
    typedef corenum_iterator<method_impl_corenum_iterator_policy>          method_impl_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdMethodDef, mdTypeDef, &IMetaDataImport::EnumMethods
    > method_corenum_iterator_policy;

    typedef corenum_iteration_context<method_corenum_iterator_policy> method_corenum_iteration_context;
    typedef corenum_iterator<method_corenum_iterator_policy>          method_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdToken, mdMethodDef, &IMetaDataImport::EnumMethodSemantics
    > method_semantics_corenum_iterator_policy;

    typedef corenum_iteration_context<method_semantics_corenum_iterator_policy> method_semantics_corenum_iteration_context;
    typedef corenum_iterator<method_semantics_corenum_iterator_policy>          method_semantics_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataImport, mdModuleRef, &IMetaDataImport::EnumModuleRefs
    > module_ref_corenum_iterator_policy;

    typedef corenum_iteration_context<module_ref_corenum_iterator_policy> module_ref_corenum_iteration_context;
    typedef corenum_iterator<module_ref_corenum_iterator_policy>         module_ref_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdParamDef, mdMethodDef, &IMetaDataImport::EnumParams
    > param_corenum_iterator_policy;

    typedef corenum_iteration_context<param_corenum_iterator_policy> param_corenum_iteration_context;
    typedef corenum_iterator<param_corenum_iterator_policy>          param_corenum_iterator;



    struct permission_set_corenum_iterator_policy
    {
        typedef IMetaDataImport               interface_type;
        typedef mdPermission                  value_type;
        typedef std::array<mdPermission, 128> buffer_type;
        typedef std::pair<mdToken, DWORD>     argument_type;

        static auto advance(interface_type     & import,
                            HCORENUM           & e,
                            buffer_type        & buffer,
                            argument_type const  argument) -> unsigned
        {
            ULONG count;
            throw_on_failure(import.EnumPermissionSets(
                &e,
                argument.first,
                argument.second,
                buffer.data(),
                static_cast<ULONG>(buffer.size()),
                &count));
            return count;
        }

        static auto get(buffer_type const& buffer, size_type const index) -> value_type
        {
            return buffer[index];
        }
    };

    typedef corenum_iteration_context<permission_set_corenum_iterator_policy> permission_set_corenum_iteration_context;
    typedef corenum_iterator<permission_set_corenum_iterator_policy>          permission_set_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport, mdProperty, mdTypeDef, &IMetaDataImport::EnumProperties
    > property_corenum_iterator_policy;

    typedef corenum_iteration_context<property_corenum_iterator_policy> property_corenum_iteration_context;
    typedef corenum_iterator<property_corenum_iterator_policy>          property_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataImport, mdSignature, &IMetaDataImport::EnumSignatures
    > signature_corenum_iterator_policy;

    typedef corenum_iteration_context<signature_corenum_iterator_policy> signature_corenum_iteration_context;
    typedef corenum_iterator<signature_corenum_iterator_policy>          signature_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataImport, mdTypeDef, &IMetaDataImport::EnumTypeDefs
    > type_def_corenum_iterator_policy;

    typedef corenum_iteration_context<type_def_corenum_iterator_policy> type_def_corenum_iteration_context;
    typedef corenum_iterator<type_def_corenum_iterator_policy>          type_def_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataImport, mdTypeRef, &IMetaDataImport::EnumTypeRefs
    > type_ref_corenum_iterator_policy;

    typedef corenum_iteration_context<type_ref_corenum_iterator_policy> type_ref_corenum_iteration_context;
    typedef corenum_iterator<type_ref_corenum_iterator_policy>          type_ref_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataImport, mdTypeSpec, &IMetaDataImport::EnumTypeSpecs
    > type_spec_corenum_iterator_policy;

    typedef corenum_iteration_context<type_spec_corenum_iterator_policy> type_spec_corenum_iteration_context;
    typedef corenum_iterator<type_spec_corenum_iterator_policy>          type_spec_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataImport, mdToken, &IMetaDataImport::EnumUnresolvedMethods
    > unresolved_method_corenum_iterator_policy;

    typedef corenum_iteration_context<unresolved_method_corenum_iterator_policy> unresolved_method_corenum_iteration_context;
    typedef corenum_iterator<unresolved_method_corenum_iterator_policy>          unresolved_method_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataImport, mdToken, &IMetaDataImport::EnumUserStrings
    > user_string_corenum_iterator_policy;

    typedef corenum_iteration_context<user_string_corenum_iterator_policy> user_string_corenum_iteration_context;
    typedef corenum_iterator<user_string_corenum_iterator_policy>          user_string_corenum_iterator;



    /// TODO FINISH UPDATING CASING OF REMAINING ITERATORS



    //
    // IMetaDataImport2 Iterators
    //

    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport2, mdGenericParamConstraint, mdGenericParam, &IMetaDataImport2::EnumGenericParamConstraints
    > generic_param_constraint_corenum_iterator_policy;

    typedef corenum_iteration_context<generic_param_constraint_corenum_iterator_policy> generic_param_constraint_corenum_iteration_context;
    typedef corenum_iterator<generic_param_constraint_corenum_iterator_policy>          generic_param_constraint_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport2, mdGenericParam, mdToken, &IMetaDataImport2::EnumGenericParams
    > generic_param_corenum_iterator_policy;

    typedef corenum_iteration_context<generic_param_corenum_iterator_policy> GenericParam_corenum_iteration_context;
    typedef corenum_iterator<generic_param_corenum_iterator_policy>          GenericParam_corenum_iterator;



    typedef base_unary_corenum_iteration_policy<
        IMetaDataImport2, mdMethodSpec, mdToken, &IMetaDataImport2::EnumMethodSpecs
    > method_spec_corenum_iterator_policy;

    typedef corenum_iteration_context<method_spec_corenum_iterator_policy> method_spec_corenum_iteration_context;
    typedef corenum_iterator<method_spec_corenum_iterator_policy>          method_spec_corenum_iterator;





    //
    // IMetaDataAssemblyImport Iterators
    //

    typedef base_nullary_corenum_iteration_policy<
        IMetaDataAssemblyImport, mdAssemblyRef, &IMetaDataAssemblyImport::EnumAssemblyRefs
    > assembly_ref_corenum_iterator_policy;

    typedef corenum_iteration_context<assembly_ref_corenum_iterator_policy> assembly_ref_corenum_iteration_context;
    typedef corenum_iterator<assembly_ref_corenum_iterator_policy>          assembly_ref_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataAssemblyImport, mdExportedType, &IMetaDataAssemblyImport::EnumExportedTypes
    > exported_type_corenum_iterator_policy;

    typedef corenum_iteration_context<exported_type_corenum_iterator_policy> exported_type_corenum_iteration_context;
    typedef corenum_iterator<exported_type_corenum_iterator_policy>          exported_type_corenum_iterator;



    typedef base_nullary_corenum_iteration_policy<
        IMetaDataAssemblyImport, mdFile, &IMetaDataAssemblyImport::EnumFiles
    > file_corenum_iterator_policy;

    typedef corenum_iteration_context<file_corenum_iterator_policy> file_corenum_iteration_context;
    typedef corenum_iterator<file_corenum_iterator_policy>          file_corenum_iterator;



     typedef base_nullary_corenum_iteration_policy<
        IMetaDataAssemblyImport, mdManifestResource, &IMetaDataAssemblyImport::EnumManifestResources
    > manifest_resource_corenum_iterator_policy;

    typedef corenum_iteration_context<manifest_resource_corenum_iterator_policy> manifest_resource_corenum_iteration_context;
    typedef corenum_iterator<manifest_resource_corenum_iterator_policy>          manifest_resource_corenum_iterator;





    using std::begin;
    using std::end;

} } }

#endif // #if defined(CXXREFLECT_WINDOWSRUNTIME_UTILITIES_STANDALONE) || defined(CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION)

#endif
