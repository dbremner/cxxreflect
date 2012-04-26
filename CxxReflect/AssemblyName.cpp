
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/Loader.hpp"

namespace CxxReflect { namespace { namespace Private {

    using namespace CxxReflect;

    PublicKeyToken ComputePublicKeyToken(Metadata::BlobReference const blob, bool const isFullPublicKey)
    {
        PublicKeyToken result((PublicKeyToken()));

        if (isFullPublicKey)
        {
            Sha1Hash const hash(Externals::ComputeSha1Hash(blob.Begin(), blob.End()));
            std::copy(hash.rbegin(), hash.rbegin() + 8, result.begin());
        }
        else if (Detail::Distance(blob.Begin(), blob.End()) > 0) // TODO Why would this be zero?
        {
            if (Detail::Distance(blob.Begin(), blob.End()) != 8)
                throw RuntimeError(L"Failed to compute public key token");

            std::copy(blob.Begin(), blob.End(), result.begin());
        }

        return result;
    }

    template <Metadata::TableId TTableKind>
    void BuildAssemblyName(AssemblyName& name, Metadata::Database const& database, SizeType index)
    {
        auto const row(database.GetRow<TTableKind>(index));

        AssemblyFlags const flags(row.GetFlags());

        PublicKeyToken const publicKeyToken(ComputePublicKeyToken(
            row.GetPublicKey(),
            flags.IsSet(AssemblyAttribute::PublicKey)));

        Version const version(
            row.GetVersion().GetMajor(),
            row.GetVersion().GetMinor(),
            row.GetVersion().GetBuild(),
            row.GetVersion().GetRevision());

        name = CxxReflect::AssemblyName(
            row.GetName().c_str(),
            version,
            row.GetCulture().c_str(),
            publicKeyToken,
            flags);
    }

} } }

namespace CxxReflect {

    Version::Version(String const& version)
    {
        std::wistringstream iss(version.c_str());
        if (!(iss >> *this >> std::ws) || !iss.eof())
            throw RuntimeError(L"Failed to parse version");
    }

    std::wostream& operator<<(std::wostream& os, Version const& v)
    {
        os << v.GetMajor() << L'.' << v.GetMinor() << L'.' << v.GetBuild() << L'.' << v.GetRevision();
        return os;
    }

    std::wistream& operator>>(std::wistream& is, Version& v)
    {
        std::array<std::int16_t, 4> components = { 0 };
        unsigned componentsRead(0);
        bool TrueValueToSuppressC4127(true);
        while (TrueValueToSuppressC4127)
        {
            if (!(is >> components[componentsRead]))
                return is;

            ++componentsRead;
            if (componentsRead == 4 || is.peek() != L'.')
                break;

            is.ignore(1); // Ignore the . between numbers
        }

        v = Version(components[0], components[1], components[2], components[3]);
        return is;
    }

    AssemblyName::AssemblyName(Assembly const& assembly, Metadata::RowReference const& reference, InternalKey)
    {
        Metadata::Database const& database(assembly.GetContext(InternalKey()).GetDatabase());
        switch (reference.GetTable())
        {
        case Metadata::TableId::Assembly:
            Private::BuildAssemblyName<Metadata::TableId::Assembly>(*this, database, reference.GetIndex());
            // TODO _path = assembly.GetPath();
            break;

        case Metadata::TableId::AssemblyRef:
            Private::BuildAssemblyName<Metadata::TableId::AssemblyRef>(*this, database, reference.GetIndex());
            break;

        default:
            Detail::AssertFail(L"RowReference references unsupported table");
        }
    }

    AssemblyName::AssemblyName(String const& fullName)
    {
        std::wistringstream iss(fullName.c_str());
        if (!(iss >> *this >> std::ws) || !iss.eof())
            throw RuntimeError(L"Failed to parse AssemblyName");
    }

    String const& AssemblyName::GetFullName() const
    {
        if (_fullName.size() > 0)
            return _fullName; // TODO CHECK FOR VALIDITY OF NAME FIRST

        // TODO MAKE SURE THIS WORKS FOR NULL AND NONEXISTENT COMPONENTS
        std::wostringstream buffer;
        buffer << _simpleName << L", Version=" << _version;

        buffer << L", Culture=";
        if (!_cultureInfo.empty())
            buffer << _cultureInfo;
        else
            buffer << L"neutral";

        buffer << L", PublicKeyToken=";
        bool const publicKeyIsNull(
            std::find_if(begin(_publicKeyToken.Get()), end(_publicKeyToken.Get()), [](Byte x)
            {
                return x != 0;
            }) ==  end(_publicKeyToken.Get()));

        if (!publicKeyIsNull)
        {
            std::array<Character, 17> publicKeyString = { 0 };
            for (SizeType n(0); n < _publicKeyToken.Get().size(); n += 1)
            {
                std::swprintf(publicKeyString.data() + (n * 2), 3, L"%02x", _publicKeyToken.Get()[n]);
            }
            buffer << publicKeyString.data();
        }
        else
        {
            buffer << L"null";
        }

        _fullName = buffer.str();
        return _fullName;
    }

    std::wostream& operator<<(std::wostream& os, AssemblyName const& an)
    {
        return os << an.GetFullName();
    }

    std::wistream& operator>>(std::wistream& is, AssemblyName& an)
    {
        enum class Kind { Label, Name, Version, Culture, PublicKeyToken };

        std::map<Kind, String> components;

        Kind kind = Kind::Name;
        String term;

        wchar_t current;
        while (is >> current)
        {
            if (current == L',')
            {
                if (kind == Kind::Label)
                    throw RuntimeError(L"Failed to parse AssemblyName");

                if (!components.insert(std::make_pair(kind, term)).second)
                    throw RuntimeError(L"Failed to parse AssemblyName");

                term.clear();
                kind = Kind::Label;
                continue;
            }
            else if (current == L'=')
            {
                if (kind != Kind::Label)
                    throw RuntimeError(L"Failed to parse AssemblyName");

                if      (term == L"Version")        { kind = Kind::Version;        }
                else if (term == L"Culture")        { kind = Kind::Culture;        }
                else if (term == L"PublicKeyToken") { kind = Kind::PublicKeyToken; }
                else throw RuntimeError(L"Failed to parse AssemblyName");

                continue;
            }
            else if (current == L' ')
            {
                // TODO IS THIS ACTUALLY CORRECT TO IGNORE WHITESPACE ALL THE TIME?
                continue;
            }
            else
            {
                term.push_back(current);
            }
        }

        an = AssemblyName(
            components[Kind::Name],
            Version(components[Kind::Version]),
            components[Kind::Culture],
            PublicKeyToken(),
            AssemblyFlags()); // TODO THIS IS NOT CORRECT


        // TODO BUILD THIS FUNCTION
        return is;
    }

}
