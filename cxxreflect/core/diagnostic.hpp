
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_DIAGNOSTIC_HPP_
#define CXXREFLECT_CORE_DIAGNOSTIC_HPP_

#include "cxxreflect/core/standard_library.hpp"

namespace cxxreflect { namespace core {

    /// \defgroup cxxreflect_core_diagnostic Core :: Exceptions and Diagnostics
    ///
    /// @{

    class exception : public std::exception
    {
    public:

        virtual ~exception() throw() { }

        auto message() const -> string const& { return _message; }

    protected:

        explicit exception(string const& message = string())
            : _message(message)
        { }

    private:

        string _message;
    };

    class logic_error : public exception
    {
    public:

        explicit logic_error(string const& message = string())
            : exception(message)
        { }
    };

    class assertion_error : public logic_error
    {
    public:

        explicit assertion_error(string const& message = string())
            : logic_error(message)
        { }
    };

    class runtime_error : public exception
    {
    public:

        explicit runtime_error(string const& message = string())
            : exception(message)
        { }
    };

    class hresult_error : public runtime_error
    {
    public:

        explicit hresult_error(hresult const hr = 0x80004005 /* E_FAIL */)
            : _hr(hr)
        { }

    private:

        hresult _hr;
    };

    class io_error : public runtime_error
    {
    public:

        explicit io_error(string const& message = string())
            : runtime_error(message)
        { }
    };

    class metadata_error : public runtime_error
    {
    public:

        explicit metadata_error(string const& message = string())
            : runtime_error(message)
        { }
    };





    #ifdef CXXREFLECT_ENABLE_DEBUG_ASSERTIONS

    inline auto assert_fail(const_character_iterator const message = L"") -> void
    {
        throw assertion_error(message);
    }

    inline auto assert_not_null(void const* const p) -> void
    {
        if (p == nullptr)
            throw assertion_error(L"unexpected null pointer");
    }

    template <typename Callable>
    auto assert_true(Callable&& callable, const_character_iterator const message = L"") -> void
    {
        if (!callable())
            throw assertion_error(message);
    }
    
    template <typename T>
    auto assert_initialized(T const& object) -> void
    {
        if (!object.is_initialized())
            throw assertion_error(L"object is not initialized");
    }

    #else

    inline auto assert_fail(const_character_iterator = L"") -> void
    {
    }

    inline auto assert_not_null(void const*) -> void
    {
    }

    template <typename Callable>
    auto assert_true(Callable&&, const_character_iterator = L"") -> void
    {
    }
    
    template <typename T>
    auto assert_initialized(T const&) -> void
    {
    }

    #endif

    /// @}

} }

#endif

// AMDG //
