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

    /// The types of Platform implementations with which Externals can be initialized.
    enum class Platform : SizeType
    {
        /// Basic platform dependencies making use of the Win32 API.
        Win32,

        /// Platform dependencies for WinRT that will pass WACK compliance.
        WinRT
    };

    /// Represents a 20-byte SHA1 hash.
    typedef std::array<Byte, 20> Sha1Hash;

    /// An interface used to isolate platform dependencies.
    ///
    /// All platform dependencies are isolated through this interface.
    class IExternalFunctions
    {
    public:

        /// Computes the SHA1 hash of the data in the range [first, last).
        ///
        /// \param    first, last The range of bytes for which to compute a SHA1 hash.
        /// \returns  The SHA1 hash.
        /// \nothrows
        virtual Sha1Hash ComputeSha1Hash(ConstByteIterator first, ConstByteIterator last) const = 0;

        /// Given a UTF-8 string, computes its length in characters when represented in UTF-16.
        ///
        /// \param    source The source string, encoded as a null-terminated UTF-8 string.
        /// \returns  The length of the string in 16-bit characters, including the null terminator.
        /// \nothrows 
        virtual unsigned ComputeUtf16LengthOfUtf8String(char const* source) const = 0;

        /// Converts a UTF-8 string to UTF-16.
        ///
        /// \param    source       The source string, encoded as a null-terminated UTF-8 string.
        /// \param    target       The target buffer, in which the UTF-16 string will be written.
        /// \param    targetLength The length of the target buffer, in elements.
        /// \returns  `true` if the conversion succeeds; `false` otherwise.
        /// \nothrows
        virtual bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength) const = 0;

        /// Canonicalizes a URI.
        ///
        /// \param    pathOrUri An absolute path or URI.
        /// \returns  The canonical URI form of the provided path.
        /// \nothrows
        virtual std::wstring ComputeCanonicalUri(wchar_t const* pathOrUri) const = 0;

        /// Opens a file.
        ///
        /// \param    fileName The path to the file to be opened.
        /// \param    mode     The mode with which to open the file.
        /// \returns  The file handle to the opened file, or a nullptr if opening of the file failed.
        /// \throws   FileIOError If the file cannot be opened.
        /// \throws   LogicError  If the underlying platform returns an invalid file handle.
        virtual FILE* OpenFile(wchar_t const* fileName, wchar_t const* mode) const = 0;

        /// Maps a file into memory
        ///
        /// If mapping of the file fails, an empty FileRange is returned.
        ///
        /// \param    file The handle of the file to be mapped into memory.
        /// \returns  A unique-ownership FileRange that owns the memory-mapped I/O range.
        /// \nothrows
        virtual Detail::FileRange MapFile(FILE* file) const = 0;

        /// Tests whether a file exists.
        ///
        /// \param    filePath The path to the file
        /// \returns  `true` if the file exists and can be opened; `false` otherwise.
        /// \nothrows
        virtual bool FileExists(wchar_t const* filePath) const = 0;

        /// Virtual destructor for interface class.
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
        virtual Detail::FileRange MapFile(FILE* file) const;
        virtual bool FileExists(ConstCharacterIterator filePath) const;

        virtual ~Win32ExternalFunctions();
    };

    class WinRTExternalFunctions : public IExternalFunctions
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
        virtual Detail::FileRange MapFile(FILE* file) const;
        virtual bool FileExists(ConstCharacterIterator filePath) const;

        virtual ~WinRTExternalFunctions();
    };

    template <Platform> struct PlatformToExternalFunctions;
    template <> struct PlatformToExternalFunctions<Platform::Win32> { typedef Win32ExternalFunctions Type; };
    template <> struct PlatformToExternalFunctions<Platform::WinRT> { typedef WinRTExternalFunctions Type; };

} }

namespace CxxReflect {

    /// Encapsulates platform-specific functionality
    ///
    /// Any functionality that is not provided directly by the C++ Standard Library is wrapped in
    /// this class, to make it easier to port the library to other platforms.
    ///
    /// (Note that Windows Runtime integration functionality is not included here; everything
    /// required for those parts of the library are platform-dependent; that's just how it goes.)
    class Externals
    {
    public:

        /// Initializes the global Externals instance for a specific Platform implementation.
        ///
        /// We tread carefully with this implementation.  Our goal is to be able to include several
        /// Platform implementations (e.g. Win32 and WinRT) in the CxxReflect static library, but
        /// at the same time ensure that when we link it into an application, we only pick up the
        /// Platform that we actually use.
        ///
        /// This is useful so that we do not violate WACK guidelines in WinRT projects by linking in
        /// references to unapproved Windows API functions.
        ///
        /// The trick is that we only call Platform-specific API functions in member functions of
        /// IExternalFunctions implementations.  These member function definitions will only be
        /// linked into a binary if they are actually called.  By only instantiating the external
        /// functions implementation in a function template here, we ensure that only one gets 
        /// linked in.
        template <Platform P>
        static void Initialize()
        {
            typedef typename Detail::PlatformToExternalFunctions<P>::Type ExternalFunctionsType;

            std::auto_ptr<IExternalFunctions> instance(new ExternalFunctionsType());
            Initialize(std::auto_ptr<IExternalFunctions>(instance));
        }

        /// \copydoc IExternalFunctions::ComputeSha1Hash()
        static Sha1Hash ComputeSha1Hash(ConstByteIterator first, ConstByteIterator last);

        /// \copydoc IExternalFunctions::ComputeUtf16LengthOfUtf8String()
        static unsigned ComputeUtf16LengthOfUtf8String(char const* source);

        /// \copydoc IExternalFunctions::ConvertUtf8ToUtf16()
        static bool ConvertUtf8ToUtf16(char const* source, wchar_t* target, unsigned targetLength);

        /// \copydoc IExternalFunctions::ComputeCanonicalUri()
        static String ComputeCanonicalUri(ConstCharacterIterator pathOrUri);

        /// \copydoc IExternalFunctions::OpenFile()
        static FILE* OpenFile(ConstCharacterIterator fileName, ConstCharacterIterator mode);
        
        /// \copydoc IExternalFunctions::MapFile()
        static Detail::FileRange MapFile(FILE* file);

        /// \copydoc IExternalFunctions::FileExists()
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
