
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_FILE_IO_HPP_
#define CXXREFLECT_CORE_FILE_IO_HPP_

#include "cxxreflect/core/algorithm.hpp"
#include "cxxreflect/core/enumeration.hpp"
#include "cxxreflect/core/external.hpp"
#include "cxxreflect/core/utility.hpp"

namespace cxxreflect { namespace core {

    // We avoid using the <iostream> library for performance reasons. (Really, its performance sucks;
    // this isn't a rant, it really does suck.  The <cstdio> library outperforms <iostream> for one
    // of the main unit test apps by well over 30x.  This wrapper gives us most of the convenience of
    // <iostream> with the awesome performance of <cstdio>.

    // Wrap a number with `hex_format` before inserting the number into the stream. This will cause the
    // number to be written in hexadecimal format.
    class hex_format
    {
    public:

        explicit hex_format(size_type const value)
            : _value(value)
        {
        }

        auto value() const -> size_type { return _value; }

    private:

        size_type _value;
    };

    enum class file_mode : byte
    {
        read_write_append_mask = 0x03,
        read                   = 0x01, // r
        write                  = 0x02, // w
        append                 = 0x03, // a

        update_mask            = 0x04,
        non_update             = 0x00,
        update                 = 0x04, // +

        text_binary_mask       = 0x08,
        text                   = 0x00,
        binary                 = 0x08  // b
    };

    typedef flags<file_mode> file_mode_flags;

    CXXREFLECT_GENERATE_SCOPED_ENUM_OPERATORS(file_mode)





    enum class file_origin : byte
    {
        begin   = SEEK_SET,
        current = SEEK_CUR,
        end     = SEEK_END
    };





    class file_handle
    {
    public:

        // This is the mapping of <cstdio> functions to file_handle member functions:
        // fclose    close
        // feof      is_eof
        // ferror    is_error
        // fflush    flush
        // fgetc     get_char
        // fgetpos   get_position
        // fgets     [not implemented]
        // fopen     [constructor]
        // fprintf   operator<<
        // fputc     put_char
        // fputs     operator<<
        // fread     read
        // freopen   [not implemented]
        // fscanf    [not implemented]
        // fseek     seek
        // fsetpos   set_position
        // ftell     tell
        // fwrite    write
        // getc      get_char
        // putc      put_char
        // puts      operator<<
        // rewind    [not implemented]

        enum origin_type
        {
            Begin   = SEEK_SET,
            Current = SEEK_CUR,
            End     = SEEK_END
        };

        file_handle(wchar_t const* const file_name, file_mode_flags const mode)
            : _mode(mode), _handle(externals::open_file(file_name, translate_mode(mode)))
        {
            if (_handle == nullptr)
                throw io_error();
        }

        file_handle(file_handle&& other)
            : _handle(other._handle)
        {
            other._handle = nullptr;
        }

        auto operator=(file_handle&& other) -> file_handle&
        {
            swap(other);
            return *this;
        }

        ~file_handle()
        {
            if (_handle != nullptr)
                std::fclose(_handle);
        }

        auto swap(file_handle& other) -> void
        {
            std::swap(_handle, other._handle);
        }

        auto close() -> void
        {
            // This is safe to call on a closed stream
            FILE* const local_handle(_handle);
            _handle = nullptr;

            if (local_handle != nullptr && std::fclose(local_handle) == EOF)
                throw io_error();
        }

        auto flush() -> void
        {
            assert_output_stream();

            if (std::fflush(_handle) == EOF)
                throw io_error();
        }

        auto get_char() -> int
        {
            assert_input_stream();

            int const value(std::fgetc(_handle));
            if (value == EOF)
                throw io_error();

            return value;
        }

        auto get_position() const -> std::fpos_t
        {
            assert_initialized(*this);

            fpos_t position((fpos_t()));
            if (std::fgetpos(_handle, &position) != 0)
                throw io_error();

            return position;
        }

        auto eof() const -> bool
        {
            assert_initialized(*this);
            return std::feof(_handle) != 0;
        }

        auto error() const -> bool
        {
            assert_initialized(*this);
            return std::ferror(_handle) != 0;
        }

        auto put_char(unsigned char const character) -> void
        {
            assert_output_stream();
            if (std::fputc(character, _handle))
                throw io_error();
        }

        auto read(void* const buffer, size_type const size, size_type const count) -> void
        {
            assert_input_stream();
            if (std::fread(buffer, size, count, _handle) != count)
                throw io_error();
        }

        template <typename T>
        auto read(T* const buffer, size_type const count) -> void
        {
            assert_not_null(buffer);
            assert_true([&](){ return count > 0; });

            return this->read(buffer, sizeof *buffer, count);
        }

        auto seek(difference_type const position, origin_type const origin) -> void
        {
            assert_initialized(*this);
            if (::fseek(_handle, position, origin) != 0)
                throw io_error();
        }

        auto set_position(fpos_t const position) -> void
        {
            assert_initialized(*this);
            if (std::fsetpos(_handle, &position) != 0)
                throw io_error();
        }

        auto tell() const -> difference_type
        {
            assert_initialized(*this);
            return ::ftell(_handle);
        }

        auto unget_char(unsigned char character) -> void
        {
            assert_input_stream();
            // No errors are specified for ungetc, so if an error occurs, we don't know what it is:
            if (std::ungetc(character, _handle) == EOF)
                throw io_error(L"an unknown error occurred when ungetting");
        }

        auto write(void const* const data, size_type const size, size_type const count) -> void
        {
            assert_output_stream();
            if (std::fwrite(data, size, count, _handle) != count)
                throw io_error();
        }

        #define CXXREFLECT_GENERATE(t, f)        \
            auto operator<<(t x) -> file_handle& \
            {                                    \
                std::fprintf(_handle, f, x);     \
                return *this;                    \
            }

        CXXREFLECT_GENERATE(char const* const,    "%s" )
        CXXREFLECT_GENERATE(wchar_t const* const, "%ls")
        CXXREFLECT_GENERATE(int const,            "%i" )
        CXXREFLECT_GENERATE(unsigned int const,   "%u" )
        CXXREFLECT_GENERATE(double const,         "%g" )

        #undef CXXREFLECT_GENERATE

        auto operator<<(hex_format const x) -> file_handle&
        {
            std::fprintf(_handle, "%08x", x.value());
            return *this;
        }

        auto handle() const -> FILE*
        {
            return _handle;
        }

        auto is_initialized() const -> bool
        {
            return _handle != nullptr;
        }

    private:

        file_handle(file_handle const&);
        auto operator=(file_handle const&) -> file_handle&;

        static auto translate_mode(file_mode_flags const mode) -> const_character_iterator
        {
            #define CXXREFLECT_GENERATE(x, y, z)                                 \
                static_cast<underlying_type<file_mode>::type>(file_mode::x) | \
                static_cast<underlying_type<file_mode>::type>(file_mode::y) | \
                static_cast<underlying_type<file_mode>::type>(file_mode::z)

            switch (mode.integer())
            {
            case CXXREFLECT_GENERATE(read,   non_update, text)   : return L"r"  ;
            case CXXREFLECT_GENERATE(write,  non_update, text)   : return L"w"  ;
            case CXXREFLECT_GENERATE(append, non_update, text)   : return L"a"  ;
            case CXXREFLECT_GENERATE(read,   update,     text)   : return L"r+" ;
            case CXXREFLECT_GENERATE(write,  update,     text)   : return L"w+" ;
            case CXXREFLECT_GENERATE(append, update,     text)   : return L"a+" ;

            case CXXREFLECT_GENERATE(read,   non_update, binary) : return L"rb" ;
            case CXXREFLECT_GENERATE(write,  non_update, binary) : return L"wb" ;
            case CXXREFLECT_GENERATE(append, non_update, binary) : return L"ab" ;
            case CXXREFLECT_GENERATE(read,   update,     binary) : return L"rb+";
            case CXXREFLECT_GENERATE(write,  update,     binary) : return L"wb+";
            case CXXREFLECT_GENERATE(append, update,     binary) : return L"ab+";

            default: throw io_error(L"invalid file mode");
            }

            #undef CXXREFLECT_GENERATE
        }

        auto assert_input_stream() const -> void
        {
            assert_initialized(*this);
            assert_true([&]
            {
                return _mode.is_set(file_mode::update)
                    || _mode.with_mask(file_mode::read_write_append_mask) != file_mode::write;
            });
        }

        auto assert_output_stream() const -> void
        {
            assert_initialized(*this);
            assert_true([&]
            {
                return _mode.is_set(file_mode::update)
                    || _mode.with_mask(file_mode::read_write_append_mask) != file_mode::read;
            });
        }

        file_mode_flags _mode;
        FILE*           _handle;
    };





    /// A `file_handle`-like interface for use with an array of bytes
    ///
    /// This class is provided as a stopgap for migrating the metadata database class to exclusively
    /// use memory mapped I/O.  This class has a pointer that serves as a current pointer (or cursor)
    /// and Read and Seek operations advance or retreat the pointer.
    class const_byte_cursor
    {
    public:

        enum origin_type
        {
            begin   = SEEK_SET,
            current = SEEK_CUR,
            end     = SEEK_END
        };

        const_byte_cursor(const_byte_iterator const first, const_byte_iterator const last)
            : _first(first), _last(last), _current(first)
        {
        }

        auto get_current() const -> const_byte_iterator
        {
            assert_initialized(*this);
            return _current.get();
        }

        auto get_position() const -> size_type
        {
            assert_initialized(*this);
            return core::distance(_first.get(), _current.get());
        }

        auto eof() const -> bool
        {
            assert_initialized(*this);
            return _current.get() == _last.get();
        }

        auto read(void* const buffer, size_type const size, size_type const count) -> void
        {
            assert_initialized(*this);
            verify_available(size * count);
            
            byte_iterator buffer_it(static_cast<byte_iterator>(buffer));
            range_checked_copy(_current.get(), _current.get() + size * count,
                               buffer_it, buffer_it + size * count);

            _current.get() += size * count;
        }

        template <typename T>
        auto read(T* const buffer, size_type const count) -> void
        {
            assert_true([&](){ return count > 0; });

            return this->read(buffer, sizeof *buffer, count);
        }

        auto can_read(difference_type const size) const -> bool
        {
            assert_initialized(*this);
            return std::distance(_current.get(), _last.get()) >= size;
        }

        auto seek(difference_type const position, origin_type const origin) -> void
        {
            assert_initialized(*this);
            if (origin == origin_type::begin)
            {
                _current.get() = _first.get();
            }
            else if (origin == origin_type::end)
            {
                _current.get() = _last.get();
            }
            
            verify_available(position);
            _current.get() += position;
        }

        auto can_seek(difference_type const position, origin_type const origin) -> bool
        {
            assert_initialized(*this);
            if (origin == origin_type::begin)
            {
                return std::distance(_first.get(), _last.get()) >= position;
            }
            else if (origin == origin_type::end)
            {
                return -std::distance(_first.get(), _last.get()) <= position;
            }
            else
            {
                return std::distance(_current.get(), _last.get()) >= position;
            }
        }

        auto verify_available(difference_type const size) const -> void
        {
            if (!can_read(size))
                throw io_error(0);
        }

        auto is_initialized() const -> bool
        {
            return _first.get() != nullptr && _last.get() != nullptr && _current.get() != nullptr;
        }

    private:

        value_initialized<const_byte_iterator> _first;
        value_initialized<const_byte_iterator> _last;
        value_initialized<const_byte_iterator> _current;
    };





    /// Abstract base class for the `wostream_wrapper` type to allow for pimpl'ed formatting code
    ///
    /// The only derived classes are specializations of `wostream_wrapper`.  Each implementes the
    /// `write` member function to write a wide character string to the stream.
    class base_wostream_wrapper
    {
    public:

        virtual auto write(core::const_character_iterator) -> void = 0;

        auto write(core::size_type const value) -> void { write_size_type(value,         L"%u"  ); }
        auto write(hex_format      const value) -> void { write_size_type(value.value(), L"%08x"); }

    protected:

        ~base_wostream_wrapper() { }

    private:

        auto write_size_type(core::size_type const value, wchar_t const* const format) -> void
        {
            size_type_buffer buffer((size_type_buffer()));
            std::swprintf(buffer.data(), buffer.size(), format, value);
            write(buffer.data());
        }

        typedef std::array<core::character, 15> size_type_buffer;
    };

    /// Concrete `wostream_wrapper` implementation template (`T` is the conrete stream type)
    ///
    /// The only requirement on `T` is that given an object `x` of type `T` and a `wchar_t const*`
    /// named `s`, the expression `x << s` is valid and inserts the string `s` into the stream.
    template <typename T>
    class wostream_wrapper : public base_wostream_wrapper
    {
    public:

        wostream_wrapper(T* stream)
            : _stream(stream)
        {
            core::assert_not_null(stream);
        }

        virtual auto write(core::const_character_iterator s) -> void
        {
            core::assert_not_null(s);
            *_stream << s;
        }

    private:

        core::checked_pointer<T> _stream;
    };

} }

#endif
