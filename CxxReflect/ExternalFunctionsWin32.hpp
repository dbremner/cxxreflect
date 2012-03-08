#ifndef CXXREFLECT_EXTERNALFUNCTIONSWIN32_HPP_
#define CXXREFLECT_EXTERNALFUNCTIONSWIN32_HPP_

//                 Copyright (c) 2012 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/ExternalFunctions.hpp"

namespace CxxReflect { namespace Detail {

    class Win32ExternalFunctions : IExternalFunctions
    {
    public:

        //
        // CRYPTOGRAPHIC SERVICES
        //

        virtual Sha1Hash ComputeSha1Hash(ConstByteIterator first, ConstByteIterator last) const;

        //
        // STRING MANIPULATION AND CONVERSION
        //

        virtual String ConvertNarrowStringToWideString(char const* narrowString) const;
        virtual NarrowString ConvertWideStringToNarrowString(wchar_t const* wideString) const;

        virtual unsigned ComputeUtf16LengthOfUtf8String(char const* source) const;
        virtual bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength) const;

        //
        // FILESYSTEM AND LIGHTWEGHT PATH MANIPULATION SERVICES
        //

        virtual String ComputeCanonicalUri(ConstCharacterIterator pathOrUri) const;
        virtual FILE* OpenFile(ConstCharacterIterator fileName, ConstCharacterIterator mode) const;
        virtual bool FileExists(ConstCharacterIterator filePath) const;

        virtual ~Win32ExternalFunctions();
    };

} }

#endif
