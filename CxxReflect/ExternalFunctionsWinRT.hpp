#ifndef CXXREFLECT_EXTERNALFUNCTIONSWINRT_HPP_
#define CXXREFLECT_EXTERNALFUNCTIONSWINRT_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/ExternalFunctions.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace CxxReflect { namespace Detail {

    class WinRTExternalFunctions : IExternalFunctions
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

        virtual ~WinRTExternalFunctions();
    };

} }

#endif

#endif
