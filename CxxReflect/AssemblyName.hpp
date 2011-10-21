//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_ASSEMBLYNAME_HPP_
#define CXXREFLECT_ASSEMBLYNAME_HPP_

#include "CxxReflect/Core.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <iosfwd>

namespace CxxReflect {

    class Version
    {
    public:

        Version()
            : _major(0), _minor(0), _build(0), _revision(0)
        {
        }

        explicit Version(String const& version);

        Version(std::uint16_t major, std::uint16_t minor, std::uint16_t build = 0, std::uint16_t revision = 0)
            : _major(major), _minor(minor), _build(build), _revision(revision)
        {
        }

        std::int16_t GetMajor()    const { return _major;    }
        std::int16_t GetMinor()    const { return _minor;    }
        std::int16_t GetBuild()    const { return _build;    }
        std::int16_t GetRevision() const { return _revision; }

    private:

        std::uint16_t _major;
        std::uint16_t _minor;
        std::uint16_t _build;
        std::uint16_t _revision;
    };

    inline bool operator==(Version const& lhs, Version const& rhs)
    {
        return lhs.GetMajor()    == rhs.GetMajor()
            && lhs.GetMinor()    == rhs.GetMinor()
            && lhs.GetBuild()    == rhs.GetBuild()
            && lhs.GetRevision() == rhs.GetRevision();
    }

    inline bool operator<(Version const& lhs, Version const& rhs)
    {
        if (lhs.GetMajor()    < rhs.GetMajor())    { return true;  }
        if (lhs.GetMajor()    > rhs.GetMajor())    { return false; }
        if (lhs.GetMinor()    < rhs.GetMinor())    { return true;  }
        if (lhs.GetMinor()    > rhs.GetMinor())    { return false; }
        if (lhs.GetBuild()    < rhs.GetBuild())    { return true;  }
        if (lhs.GetBuild()    > rhs.GetBuild())    { return false; }
        if (lhs.GetRevision() < rhs.GetRevision()) { return true;  }
        return false;
    }

    inline bool operator!=(Version const& lhs, Version const& rhs) { return !(lhs == rhs); }
    inline bool operator> (Version const& lhs, Version const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(Version const& lhs, Version const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(Version const& lhs, Version const& rhs) { return !(rhs <  lhs); }

    std::wostream& operator<<(std::wostream& os, Version const& v);
    std::wistream& operator>>(std::wistream& is, Version& v);

    enum struct AssemblyNameFlags : std::uint16_t
    {
        None                       = 0x0000,

        // PublicKey is not supported; we always hash the public key before storing it.

        EnableJITCompileTracking   = 0x8000,
        DisableJITCompileOptimizer = 0x4000,

        Retargetable               = 0x0100,

        DefaultContentType         = 0x0000,
        WindowsRuntimeContentType  = 0x0200,
        ContentTypeMask            = 0x0E00
    };

    typedef std::array<std::uint8_t, 8> PublicKeyToken;

    class AssemblyName
    {
    public:

        AssemblyName()
            : _version(), _publicKeyToken(), _flags()
        {
        }

        AssemblyName(String const& name); // TODO

        AssemblyName(String const& simpleName, Version const& version, String const& path = L"")
            : _simpleName(simpleName), _version(version), _path(path)
        {
        }

        AssemblyName(String            const& simpleName,
                     Version           const& version,
                     String            const& cultureInfo,
                     PublicKeyToken    const& publicKeyToken,
                     AssemblyNameFlags const& flags,
                     String            const& path = L"")
            : _simpleName(simpleName), _version(version), _cultureInfo(cultureInfo),
              _publicKeyToken(publicKeyToken), _flags(flags), _path(path)
        {
        }

        String            const& GetName()           const { return _simpleName;     }
        Version           const& GetVersion()        const { return _version;        }
        String            const& GetCultureInfo()    const { return _cultureInfo;    }
        PublicKeyToken    const& GetPublicKeyToken() const { return _publicKeyToken; }    
        AssemblyNameFlags        GetFlags()          const { return _flags;          }
        String            const& GetPath()           const { return _path;           }

        String GetFullName() const;

    private:

        String            _simpleName;
        Version           _version;
        String            _cultureInfo;
        PublicKeyToken    _publicKeyToken;
        AssemblyNameFlags _flags;
        String            _path;
    };

    inline bool operator==(AssemblyName const& lhs, AssemblyName const& rhs)
    {
        return lhs.GetName()           == rhs.GetName()
            && lhs.GetVersion()        == rhs.GetVersion()
            && lhs.GetCultureInfo()    == rhs.GetCultureInfo()
            && lhs.GetPublicKeyToken() == rhs.GetPublicKeyToken();
    }

    inline bool operator<(AssemblyName const& lhs, AssemblyName const& rhs)
    {
        if (lhs.GetName()           < rhs.GetName())           { return true;  }
        if (rhs.GetName()           < lhs.GetName())           { return false; }
        if (lhs.GetVersion()        < rhs.GetVersion())        { return true;  }
        if (rhs.GetVersion()        < lhs.GetVersion())        { return false; }
        if (lhs.GetCultureInfo()    < rhs.GetCultureInfo())    { return true;  }
        if (rhs.GetCultureInfo()    < lhs.GetCultureInfo())    { return false; }
        if (lhs.GetPublicKeyToken() < rhs.GetPublicKeyToken()) { return true;  }
        return false;
    }

    inline bool operator!=(AssemblyName const& lhs, AssemblyName const& rhs) { return !(lhs == rhs); }
    inline bool operator> (AssemblyName const& lhs, AssemblyName const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(AssemblyName const& lhs, AssemblyName const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(AssemblyName const& lhs, AssemblyName const& rhs) { return !(rhs <  lhs); }

    std::wostream& operator<<(std::wostream& os, AssemblyName const& an);
    std::wistream& operator>>(std::wistream& os, AssemblyName & an);      // TODO
}

#endif
