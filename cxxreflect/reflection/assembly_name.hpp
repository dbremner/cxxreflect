
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_ASSEMBLY_NAME_HPP_
#define CXXREFLECT_REFLECTION_ASSEMBLY_NAME_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection {

    /// A four-component version number (of the form "0.0.0.0")
    ///
    /// Among other things, a four component version number is used to represent the version of an
    /// assembly.
    class version
    {
    public:

        typedef std::uint16_t component;

        version();
        version(component major, component minor, component build = 0, component revision = 0);

        auto major()    const -> component;
        auto minor()    const -> component;
        auto build()    const -> component;
        auto revision() const -> component;

        friend auto operator==(version const&, version const&) -> bool;
        friend auto operator< (version const&, version const&) -> bool;

        friend auto operator<<(core::output_stream&, version const&) -> core::output_stream&;
        friend auto operator>>(core::input_stream&,  version      &) -> core::input_stream&;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(version)

    private:

        core::value_initialized<component> _major;
        core::value_initialized<component> _minor;
        core::value_initialized<component> _build;
        core::value_initialized<component> _revision;
    };





    /// An assembly public key token, which is the last eight bytes of the SHA1 hash of a public key
    typedef std::array<std::uint8_t, 8> public_key_token;





    /// An assembly name, including its simple name, version, public key, flags, and optionally path
    class assembly_name
    {
    public:

        typedef version          version_type;
        typedef public_key_token public_key_token_type;

        assembly_name();

        assembly_name(core::string_reference simple_name,
                      version const&         assembly_version,
                      core::string_reference path = L"");

        assembly_name(core::string_reference   simple_name,
                      version const&           assembly_version,
                      core::string_reference   culture_info,
                      public_key_token const&  token,
                      metadata::assembly_flags flags,
                      core::string_reference   path = L"");

        auto simple_name()      const -> core::string const&;
        auto version()          const -> version_type const&;
        auto culture_info()     const -> core::string const&;
        auto public_key_token() const -> public_key_token_type const&;
        auto flags()            const -> metadata::assembly_flags;
        auto path()             const -> core::string const&;
        auto full_name()        const -> core::string const&;

        friend auto operator==(assembly_name const&, assembly_name const&) -> bool;
        friend auto operator< (assembly_name const&, assembly_name const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(assembly_name)

    public: // internal members

        assembly_name(std::nullptr_t, metadata::assembly_or_assembly_ref_token token, core::internal_key);
        assembly_name(metadata::assembly_or_assembly_ref_token token, core::string_reference path, core::internal_key);

    private:
        
        core::string                                   _simple_name;
        version_type                                   _version;
        core::string                                   _culture_info;
        core::value_initialized<public_key_token_type> _public_key_token;
        metadata::assembly_flags                       _flags;
        core::string                                   _path;
        core::string mutable                           _full_name;
    };

} }

#endif
