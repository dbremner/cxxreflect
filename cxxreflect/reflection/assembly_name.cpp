
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/assembly_name.hpp"

namespace cxxreflect { namespace reflection { namespace {

    auto compute_public_key_token(metadata::blob const key, bool const is_full_public_key) -> public_key_token
    {
        public_key_token result((public_key_token()));

        if (is_full_public_key)
        {
            core::sha1_hash const hash(core::compute_sha1_hash(begin(key), end(key)));
            std::copy(hash.rbegin(), hash.rbegin() + 8, begin(result));
        }
        else if (core::distance(begin(key), end(key)) > 0)
        {
            if (core::distance(begin(key), end(key)) != 8)
                throw core::runtime_error(L"failed to compute public key token");

            std::copy(begin(key), end(key), begin(result));
        }

        return result;
    }

    template <typename Token>
    auto build_assembly_name_internal(assembly_name& name, Token const token, core::string_reference const path) -> void
    {
        auto const row(row_from(token));

        metadata::assembly_flags const flags(row.flags());

        public_key_token const public_key(compute_public_key_token(
            row.public_key(),
            flags.is_set(metadata::assembly_attribute::public_key)));

        version const v(
            row.version().major(),
            row.version().minor(),
            row.version().build(),
            row.version().revision());

        name = assembly_name(row.name().c_str(), v, row.culture().c_str(), public_key, flags, path.c_str());
    }

    auto build_assembly_name(assembly_name&                                 name,
                             metadata::assembly_or_assembly_ref_token const token,
                             core::string_reference                   const path) -> void
    {
        core::assert_initialized(token);

        switch (token.table())
        {
        case metadata::table_id::assembly:
            build_assembly_name_internal(name, token.as<metadata::assembly_token>(), path);
            return;

        case metadata::table_id::assembly_ref:
            build_assembly_name_internal(name, token.as<metadata::assembly_ref_token>(), path);
            return;

        default:
            core::assert_fail(L"unreachable code");
        }
    }
        
} } }

namespace cxxreflect { namespace reflection {

    version::version()
    {
    }

    version::version(component const major, component const minor, component const build, component const revision)
        : _major(major), _minor(minor), _build(build), _revision(revision)
    {
    }

    auto version::major() const -> component
    {
        return _major.get();
    }

    auto version::minor() const -> component
    {
        return _minor.get();
    }

    auto version::build() const -> component
    {
        return _build.get();
    }

    auto version::revision() const -> component
    {
        return _revision.get();
    }

    auto operator==(version const& lhs, version const& rhs) -> bool
    {
        return lhs._major.get()    == rhs._major.get()
            && lhs._minor.get()    == rhs._minor.get()
            && lhs._build.get()    == rhs._build.get()
            && lhs._revision.get() == rhs._revision.get();
    }

    auto operator<(version const& lhs, version const& rhs) -> bool
    {
        if (lhs._major.get() < rhs._major.get())
            return true;

        if (lhs._major.get() > rhs._major.get())
            return false;

        if (lhs._minor.get() < rhs._minor.get())
            return true;

        if (lhs._minor.get() > rhs._minor.get())
            return false;

        if (lhs._build.get() < rhs._build.get())
            return true;

        if (lhs._build.get() > rhs._build.get())
            return false;

        return lhs._revision.get() < rhs._revision.get();
    }

    auto operator<<(core::output_stream& os, version const& v) -> core::output_stream&
    {
        os << v.major() << L'.' << v.minor() << L'.' << v.build() << L'.' << v.revision();
        return os;
    }

    auto operator>>(core::input_stream& is, version& v) -> core::input_stream&
    {
        typedef std::array<version::component, 4> component_array;
        
        component_array components((component_array()));

        unsigned components_read(0);

        bool true_value_to_suppress_c4127(true);
        while (true_value_to_suppress_c4127)
        {
            if (!(is >> components[components_read]))
                return is;

            ++components_read;
            if (components_read == 4 || is.peek() != L'.')
                break;

            is.ignore(1); // ignore the . between numbers
        }

        v = version(components[0], components[1], components[2], components[3]);
        return is;
    }




    
    assembly_name::assembly_name()
    {
    }

    assembly_name::assembly_name(core::string_reference const  simple_name,
                                 reflection::version    const& assembly_version,
                                 core::string_reference const  path)
        : _simple_name(simple_name.c_str()), _version(assembly_version), _path(path.c_str())
    {
    }

    assembly_name::assembly_name(core::string_reference       const  simple_name,
                                 reflection::version          const& assembly_version,
                                 core::string_reference       const  culture_info,
                                 reflection::public_key_token const& token,
                                 metadata::assembly_flags     const  flags,
                                 core::string_reference       const  path)
        : _simple_name(simple_name.c_str()),
          _version(assembly_version),
          _culture_info(culture_info.c_str()),
          _public_key_token(token),
          _flags(flags),
          _path(path.c_str())
    {
    }

    assembly_name::assembly_name(assembly                                 const&,
                                 metadata::assembly_or_assembly_ref_token const  token,
                                 core::internal_key)
    {
        build_assembly_name(*this, token, core::string_reference());
    }

    assembly_name::assembly_name(metadata::assembly_or_assembly_ref_token const  token,
                                 core::internal_key)
    {
        build_assembly_name(*this, token, core::string_reference());
    }

    assembly_name::assembly_name(metadata::assembly_or_assembly_ref_token const token,
                                 core::string_reference                   const path,
                                 core::internal_key)
    {
        build_assembly_name(*this, token, path);
    }

    auto assembly_name::simple_name() const -> core::string const&
    {
        return _simple_name;
    }

    auto assembly_name::version() const -> reflection::version const&
    {
        return _version;
    }

    auto assembly_name::culture_info() const -> core::string const&
    {
        return _culture_info;
    }

    auto assembly_name::public_key_token() const -> reflection::public_key_token const&
    {
        return _public_key_token.get();
    }

    auto assembly_name::flags() const -> metadata::assembly_flags
    {
        return _flags;
    }

    auto assembly_name::path() const -> core::string const&
    {
        return _path;
    }

    auto assembly_name::full_name() const -> core::string const&
    {
        if (_full_name.size() > 0)
            return _full_name;

        std::wostringstream buffer;
        buffer << _simple_name << L", Version=" << _version;

        buffer << L", Culture=";
        if (!_culture_info.empty())
            buffer << _culture_info;
        else
            buffer << L"neutral";

        buffer << L", PublicKeyToken=";
        bool const public_key_is_null(
            std::find_if(begin(_public_key_token.get()), end(_public_key_token.get()), [](core::byte const x)
            {
                return x != 0;
            }) ==  end(_public_key_token.get()));

        if (!public_key_is_null)
        {
            std::array<core::character, 17> public_key_string = { { 0 } };
            for (core::size_type n(0); n < _public_key_token.get().size(); n += 1)
            {
                std::swprintf(public_key_string.data() + (n * 2), 3, L"%02x", _public_key_token.get()[n]);
            }
            buffer << public_key_string.data();
        }
        else
        {
            buffer << L"null";
        }

        if (_flags.with_mask(metadata::assembly_attribute::content_type_mask) ==
            metadata::assembly_attribute::windows_runtime_content_type)
        {
            buffer << L", ContentType=WindowsRuntime";
        }

        _full_name = buffer.str();
        return _full_name;
    }

    auto operator==(assembly_name const& lhs, assembly_name const& rhs) -> bool
    {
        return lhs.simple_name()      == rhs.simple_name()
            && lhs.version()          == rhs.version()
            && lhs.culture_info()     == rhs.culture_info()
            && lhs.public_key_token() == rhs.public_key_token();
    }

    auto operator<(assembly_name const& lhs, assembly_name const& rhs) -> bool
    {
        if (lhs._simple_name < rhs._simple_name)
            return true;

        if (lhs._simple_name > rhs._simple_name)
            return false;

        if (lhs._version < rhs._version)
            return true;

        if (lhs._version > rhs._version)
            return false;

        if (lhs._culture_info < rhs._culture_info)
            return true;

        if (lhs._culture_info > rhs._culture_info)
            return false;

        return lhs._public_key_token.get() < rhs._public_key_token.get();
    }

    

} }

// AMDG //
