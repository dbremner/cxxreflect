//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/ExternalFunctions.hpp"
#include "CxxReflect/Fundamentals.hpp"

namespace CxxReflect { namespace { namespace Private {

    std::unique_ptr<IExternalFunctions> Externals;

} } }

namespace CxxReflect {

    IExternalFunctions::~IExternalFunctions()
    {
        // Virtual destructor requires no definition
    }

    void Externals::Initialize(std::unique_ptr<IExternalFunctions> externals)
    {
        // TODO VERIFY EXTERNALS IS NOT YET INITIALIZED
        Private::Externals.reset(externals.release());
    }

    IExternalFunctions& Externals::Get()
    {
        // TODO VERIFY EXTERNALS IS VALID
        return *Private::Externals.get();
    }

}

namespace CxxReflect {

    Sha1Hash Externals::ComputeSha1Hash(ConstByteIterator first, ConstByteIterator last)
    {
        return Get().ComputeSha1Hash(first, last);
    }

    String Externals::ConvertNarrowStringToWideString(char const* narrowString)
    {
        return Get().ConvertNarrowStringToWideString(narrowString);
    }

    NarrowString Externals::ConvertWideStringToNarrowString(wchar_t const* wideString)
    {
        return Get().ConvertWideStringToNarrowString(wideString);
    }

    unsigned Externals::ComputeUtf16LengthOfUtf8String(char const* source)
    {
        return Get().ComputeUtf16LengthOfUtf8String(source);
    }

    bool Externals::ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength)
    {
        return Get().ConvertUtf8ToUtf16(source, target, targetLength);
    }

    String Externals::ComputeCanonicalUri(ConstCharacterIterator pathOrUri)
    {
        return Get().ComputeCanonicalUri(pathOrUri);
    }

    FILE* Externals::OpenFile(ConstCharacterIterator fileName, ConstCharacterIterator mode)
    {
        return Get().OpenFile(fileName, mode);
    }

    bool Externals::FileExists(ConstCharacterIterator filePath)
    {
        return Get().FileExists(filePath);
    }

}