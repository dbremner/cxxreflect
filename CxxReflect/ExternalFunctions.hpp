#ifndef CXXREFLECT_EXTERNALFUNCTIONS_HPP_
#define CXXREFLECT_EXTERNALFUNCTIONS_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// These functions encapsulate functionality that we rely on the underlying platform (or possibly
// third-party libraries) to provide.  The global IExternalFunctions instance must be initialized by
// calling Externals::Initialize<Platform>() with the current target platform.

#include "CxxReflect/Configuration.hpp"

namespace CxxReflect { namespace Detail {

    class FileRange;

} }

namespace CxxReflect {

    enum class Platform : SizeType
    {
        Win32,
        WinRT
    };

    typedef std::array<std::uint8_t, 20> Sha1Hash;

    // All platform dependencies are isolated through this interface.  Implementations are provided
    // in other files (and may, of course, be provided by library consumers).
    class IExternalFunctions
    {
    public:

        //
        // CRYPTOGRAPHIC SERVICES
        //

        virtual Sha1Hash ComputeSha1Hash(std::uint8_t const* first, std::uint8_t const* last) const = 0;

        //
        // STRING MANIPULATION AND CONVERSION
        //

        virtual unsigned ComputeUtf16LengthOfUtf8String(char const* source) const = 0;
        virtual bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength) const = 0;

        //
        // FILESYSTEM AND LIGHTWEGHT PATH MANIPULATION SERVICES
        //

        virtual std::wstring ComputeCanonicalUri(wchar_t const* pathOrUri) const = 0;
        virtual FILE* OpenFile(wchar_t const* fileName, wchar_t const* mode) const = 0;
        virtual Detail::FileRange MapFileRange(FILE* file, SizeType index, SizeType size) const = 0;
        virtual bool FileExists(wchar_t const* filePath) const = 0;

        virtual ~IExternalFunctions();
    };

}

namespace CxxReflect { namespace Detail {

    class Win32ExternalFunctions : public IExternalFunctions
    {
    public:

        //
        // CRYPTOGRAPHIC SERVICES
        //

        virtual Sha1Hash ComputeSha1Hash(ConstByteIterator first, ConstByteIterator last) const;

        //
        // STRING MANIPULATION AND CONVERSION
        //

        virtual unsigned ComputeUtf16LengthOfUtf8String(char const* source) const;
        virtual bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength) const;

        //
        // FILESYSTEM AND LIGHTWEGHT PATH MANIPULATION SERVICES
        //

        virtual String ComputeCanonicalUri(ConstCharacterIterator pathOrUri) const;
        virtual FILE* OpenFile(ConstCharacterIterator fileName, ConstCharacterIterator mode) const;
        virtual Detail::FileRange MapFileRange(FILE* file, SizeType index, SizeType size) const;
        virtual bool FileExists(ConstCharacterIterator filePath) const;

        virtual ~Win32ExternalFunctions();
    };

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

        virtual unsigned ComputeUtf16LengthOfUtf8String(char const* source) const;
        virtual bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength) const;

        //
        // FILESYSTEM AND LIGHTWEGHT PATH MANIPULATION SERVICES
        //

        virtual String ComputeCanonicalUri(ConstCharacterIterator pathOrUri) const;
        virtual FILE* OpenFile(ConstCharacterIterator fileName, ConstCharacterIterator mode) const;
        virtual Detail::FileRange MapFileRange(FILE* file, SizeType index, SizeType size) const;
        virtual bool FileExists(ConstCharacterIterator filePath) const;

        virtual ~WinRTExternalFunctions();
    };

    template <Platform> struct PlatformToExternalFunctions;
    template <> struct PlatformToExternalFunctions<Platform::Win32> { typedef Win32ExternalFunctions Type; };
    template <> struct PlatformToExternalFunctions<Platform::WinRT> { typedef WinRTExternalFunctions Type; };

} }

namespace CxxReflect {

    class Externals
    {
    public:

        template <Platform P>
        static void Initialize()
        {
            typedef typename Detail::PlatformToExternalFunctions<P>::Type ExternalFunctionsType;

            std::auto_ptr<IExternalFunctions> instance(new ExternalFunctionsType());
            Initialize(std::auto_ptr<IExternalFunctions>(instance));
        }

        static Sha1Hash ComputeSha1Hash(ConstByteIterator first, ConstByteIterator last);

        static unsigned ComputeUtf16LengthOfUtf8String(char const* source);
        static bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength);

        static String ComputeCanonicalUri(ConstCharacterIterator pathOrUri);

        static FILE* OpenFile(ConstCharacterIterator fileName, ConstCharacterIterator mode);
        
        static Detail::FileRange MapFileRange(FILE* file, SizeType index, SizeType size);
        static bool FileExists(ConstCharacterIterator filePath);

    private:

        Externals();
        Externals(Externals const&);
        Externals& operator=(Externals const&);
        ~Externals();

        // Note:  We use auto_ptr for compatibility with C++/CLI, which does not support move-only
        // types.  This is not a publicly accessible interface, so we are not concerned about misuse
        // of the error-prone auto_ptr type.
        static void Initialize(std::auto_ptr<IExternalFunctions> externals);

        static IExternalFunctions& Get();
    };

}

#endif
