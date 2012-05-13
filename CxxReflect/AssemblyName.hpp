#ifndef CXXREFLECT_ASSEMBLYNAME_HPP_
#define CXXREFLECT_ASSEMBLYNAME_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/CoreComponents.hpp"

namespace CxxReflect {

    /// \ingroup cxxreflect_public_interface
    ///
    /// @{





    /// A four-component version number (of the form "0.0.0.0")
    class Version
    {
    public:

        /// Default-constructs a `Version` with all four components set to zero
        ///
        /// \nothrows
        Version();

        /// Constructs a `Version` by parsing the provided string representation of the version
        ///
        /// This constructor has the same behavior as the `operator>>` overload; see that overload
        /// for an explanation of the behavior of this constructor.  Note that the string must
        /// contain only the version number, possibly with leading or trailing whitespace.  It may
        /// not contain any other text.
        ///
        /// \throws RuntimeError If `version` does not contain a valid version string.
        explicit Version(String const& version);
        
        /// Constructs a `Version` from the provided components
        ///
        /// \param    major    The major component with which to initialize this object
        /// \param    minor    The minor component with which to initialize this object
        /// \param    build    The build component with which to initialize this object
        /// \param    revision The revision component with which to initialize this object
        /// \nothrows
        Version(std::uint16_t major, std::uint16_t minor, std::uint16_t build = 0, std::uint16_t revision = 0);

        /// Gets the major component of this version
        ///
        /// \nothrows
        std::int16_t GetMajor() const;

        /// Gets the minor component of this version
        ///
        /// \nothrows
        std::int16_t GetMinor() const;

        /// Gets the build component of this version
        ///
        /// \nothrows
        std::int16_t GetBuild() const;

        /// Gets the revision component of this version
        ///
        /// \nothrows
        std::int16_t GetRevision() const;

        friend bool operator==(Version const&, Version const&);
        friend bool operator< (Version const&, Version const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(Version)

        /// Emits the version into the output stream
        ///
        /// The version is always emitted with all four components in the form "0.0.0.0"
        ///
        /// \nothrows
        friend std::wostream& operator<<(std::wostream&, Version const&);

        /// Reads a version from the output stream
        ///
        /// The next value in the stream must be a valid version number, of the form "0.0.0.0".  It
        /// needs only have one component and may have up to four components (if there are more
        /// components, they are not read).  "1", "1.0", "1.0.0", and "1.0.0.0" all result in the
        /// same version. 
        ///
        /// \nothrows
        friend std::wistream& operator>>(std::wistream&, Version &);

    private:

        std::uint16_t _major;
        std::uint16_t _minor;
        std::uint16_t _build;
        std::uint16_t _revision;
    };





    /// An assembly public key token, which is the last eight bytes of the SHA1 hash of a public key
    typedef std::array<std::uint8_t, 8> PublicKeyToken;





    /// An assembly name, including its simple name, version, public key, flags, and optionally path
    class AssemblyName
    {
    public:

        /// Default-constructs an `AssemblyName`
        ///
        /// The resulting object is valid and usable, but all of its components are empty or null.
        ///
        /// \nothrows
        AssemblyName();

        /// Constructs an `AssemblyName` from the provided full name
        ///
        /// This constructor has the same behavior as the `operator>>` overload; see that overload
        /// for an explanation of the behavior of this constructor.  Note that the string must
        /// contain only the assembly name, possibly with leading or trailing whitespace.  It may
        /// not contain any other text.
        ///
        /// \throws RuntimeError If `fullName` does not contain a valid full name.
        explicit AssemblyName(String const& fullName);

        /// Constructs an `AssemblyName` with the provided components
        ///
        /// \nothrows
        AssemblyName(String const& simpleName, Version const& version, String const& path = L"");

        /// Constructs an `AssemblyName` with the provided components
        ///
        /// \nothrows
        AssemblyName(String         const& simpleName,
                     Version               version,
                     String         const& cultureInfo,
                     PublicKeyToken        publicKeyToken,
                     AssemblyFlags         flags,
                     String         const& path = L"");

        /// Gets the simple name of the assembly
        ///
        /// \nothrows
        String const& GetName() const;

        /// Gets the version of the assembly
        ///
        /// \nothrows
        Version const& GetVersion() const;

        /// Gets the culture info of the assembly
        ///
        /// This will return an empty string if the named assembly has neutral culture.
        ///
        /// \nothrows
        String const& GetCultureInfo() const;

        /// Gets the public key token of the assembly
        ///
        /// This will be set to all zeroes if the assembly has no public key or token.  If the
        /// assembly has a public key associated with it, we retain only its token; we do not
        /// provide a way to get its public key.
        ///
        /// \nothrows
        PublicKeyToken const& GetPublicKeyToken() const;

        /// Gets the attributes of the assembly's name
        ///
        /// \nothrows
        AssemblyFlags GetFlags() const;
        
        /// Gets the path to the assembly, as a fully-qualified local path
        ///
        /// This may be an empty string if the assembly name does not represent a loaded assembly
        /// and if the name was not initialized with the path
        ///
        /// \nothrows
        String const& GetPath() const;

        /// Gets the full name ("display name") of the assembly
        ///
        /// This includes all of the components of the assembly name, including its simple name,
        /// version, culture, public key token, and content type (if it has one).
        ///
        /// \nothrows
        String const& GetFullName() const;

        friend bool operator==(AssemblyName const&, AssemblyName const&);
        friend bool operator< (AssemblyName const&, AssemblyName const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(AssemblyName)

        /// Emits the assembly name into the output stream, equivalent to `os << an.GetFullName()`
        ///
        /// \nothrows
        friend std::wostream& operator<<(std::wostream&, AssemblyName const&);

        /// Reads an assembly name from the input stream
        ///
        /// \throws RuntimeError If the stream does not contain a valid assembly name
        friend std::wistream& operator>>(std::wistream&, AssemblyName&);

    public: // Internal Members

        /// Constructs a new `AssemblyName` from an Assembly or AssemblyRef row in a database
        ///
        /// \param  assembly  The assembly in which the row is defined
        /// \param  reference A row in the assembly's Assembly or AssemblyRef table
        /// \throws MetadataError 
        AssemblyName(class Assembly const& assembly, class Metadata::RowReference const& reference, InternalKey);

    private:

        String                                   _simpleName;
        Version                                  _version;
        String                                   _cultureInfo;
        Detail::ValueInitialized<PublicKeyToken> _publicKeyToken;
        AssemblyFlags                            _flags;
        String                                   _path;
        mutable String                           _fullName;
    };

    /// @}
}

#endif
