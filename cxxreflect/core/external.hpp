
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_EXTERNAL_HPP_
#define CXXREFLECT_CORE_EXTERNAL_HPP_

#include "cxxreflect/core/diagnostic.hpp"

namespace cxxreflect { namespace core { namespace detail {

    class base_externals
    {
    public:

        virtual auto compute_sha1_hash(core::const_byte_iterator first, core::const_byte_iterator last) -> core::sha1_hash = 0;

        /// Given a UTF-8 string, computes its length in characters when represented in UTF-16
        virtual auto compute_utf16_length_of_utf8_string(char const* source) const -> unsigned = 0;

        /// Converts a UTF-8 string to UTF-16
        virtual auto convert_utf8_to_utf16(char const* source, wchar_t* target, unsigned length) const -> bool = 0;

        /// Canonicalizes a URI
        virtual auto compute_canonical_uri(wchar_t const* path_or_uri) const -> string = 0;

        /// Opens a file
        virtual auto open_file(wchar_t const* fileName, wchar_t const* mode) const -> FILE* = 0;

        /// Maps a file into memory
        virtual auto map_file(FILE* file) const -> unique_byte_array = 0;

        /// Tests whether a file exists
        virtual auto file_exists(wchar_t const* file_path) const -> bool = 0;

        /// Virtual destructor for interface class
        virtual ~base_externals() { }
    };

    template <typename T>
    class derived_externals : public base_externals
    {
    public:

        typedef T externals_type;

        derived_externals(externals_type instance)
            : _instance(std::move(instance))
        { }

        virtual auto compute_sha1_hash(core::const_byte_iterator first, core::const_byte_iterator last) -> core::sha1_hash
        {
            return _instance.compute_sha1_hash(first, last);
        }

        virtual auto compute_utf16_length_of_utf8_string(char const* source) const -> unsigned
        {
            return _instance.compute_utf16_length_of_utf8_string(source);
        }

        virtual auto convert_utf8_to_utf16(char const* source, wchar_t* target, unsigned length) const -> bool
        {
            return _instance.convert_utf8_to_utf16(source, target, length);
        }

        virtual auto compute_canonical_uri(wchar_t const* path_or_uri) const -> string
        {
            return _instance.compute_canonical_uri(path_or_uri);
        }

        virtual auto open_file(wchar_t const* file_name, wchar_t const* mode) const -> FILE*
        {
            return _instance.open_file(file_name, mode);
        }

        virtual auto map_file(FILE* file) const -> unique_byte_array
        {
            return _instance.map_file(file);
        }

        virtual auto file_exists(wchar_t const* file_path) const -> bool
        {
            return _instance.file_exists(file_path);
        }

    private:

        externals_type _instance;
    };

    class global_externals
    {
    public:

        template <typename T>
        static auto initialize(T&& a) -> void
        {
            std::auto_ptr<base_externals> p(new derived_externals<T>(std::forward<T>(a)));
            instance(p);
        }

        static auto get() -> base_externals&
        {
            return instance(std::auto_ptr<base_externals>(nullptr));
        }

    private:

        static auto instance(std::auto_ptr<base_externals> p) -> base_externals&
        {
            static std::auto_ptr<base_externals> instance;
            if (instance.get() == nullptr && p.get() == nullptr)
                throw logic_error(L"externals not initialized");

            if (instance.get() != nullptr && p.get() != nullptr)
                throw logic_error(L"externals already initialized");

            if (instance.get() == nullptr)
                instance = p;
            
            return *instance.get();
        }
    };

} } }

namespace cxxreflect { namespace core { namespace externals {

    template <typename T>
    auto initialize(T const& x) -> size_type
    {
        detail::global_externals::initialize(x);
        return 0;
    }

    inline auto compute_sha1_hash(const_byte_iterator const first, const_byte_iterator const last) -> sha1_hash
    {
        return detail::global_externals::get().compute_sha1_hash(first, last);
    }

    inline auto compute_utf16_length_of_utf8_string(char const* const source) -> unsigned
    {
        return detail::global_externals::get().compute_utf16_length_of_utf8_string(source);
    }

    inline auto convert_utf8_to_utf16(char const* const source, wchar_t* const target, unsigned const length) -> bool
    {
        return detail::global_externals::get().convert_utf8_to_utf16(source, target, length);
    }

    inline auto compute_canonical_uri(wchar_t const* const path_or_uri) -> string
    {
        return detail::global_externals::get().compute_canonical_uri(path_or_uri);
    }

    inline auto open_file(wchar_t const* const file_name, wchar_t const* const mode) -> FILE*
    {
        return detail::global_externals::get().open_file(file_name, mode);
    }

    inline auto map_file(FILE* const file) -> unique_byte_array
    {
        return detail::global_externals::get().map_file(file);
    }

    inline auto file_exists(wchar_t const* const file_path) -> bool
    {
        return detail::global_externals::get().file_exists(file_path);
    }

} } }

#endif
