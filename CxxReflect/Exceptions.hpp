//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header defines exceptions that are part of the public interface.
#ifndef CXXREFLECT_EXCEPTIONS_HPP_
#define CXXREFLECT_EXCEPTIONS_HPP_

#include <stdexcept>

namespace CxxReflect {

    // This exception is thrown when an HRESULT failure is encountered during an operation but we
    // are unable to translate the HRESULT into some more user-friendly exception.
    class HResultException : public std::runtime_error
    {
    public:

        HResultException(long hresult)
            : std::runtime_error(""), hresult_(hresult)
        {
        }

        long GetHResult() const { return hresult_; }

    private:

        long hresult_;
    };

    // VerificationFailure is thrown when a debug check fails.
    struct VerificationFailure : std::logic_error
    {
        VerificationFailure(char const* const message) : std::logic_error(message) { }
    };

}

#endif
