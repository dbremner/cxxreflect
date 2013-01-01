
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //





// A utility that encodes a binary file as an array of bytes in an executable.
//
// This program is used to encode PE files (notably, CLI manifest-bearing PE files) in an array of
// bytes to be linked into an executable.  This serves two purposes:
//
// (1) It allows us to utilize ilasm to assemble metadata-only assemblies but not have to rely on
//     these assemblies existing on disk at runtime.
//
// (2) WACK does not like us having extraneous PE files in an App Package, so we can hide them as
//     data in the binary.  Shhh, don't tell anyone!  :-D





#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    typedef std::uint8_t              byte;
    typedef std::vector<std::uint8_t> byte_vector;
    typedef std::uint32_t             size_type;

    auto print_usage() -> void
    {
        std::cout << "Creates a .cpp file that defines an array of bytes.\n"
                  << "\n"
                  << "embed_binary_in_cpp {0} {1} {2}\n"
                  << "  {0}: The path to the source file.\n"
                  << "  {1}: The path to the .cpp file to create.\n"
                  << "  {2}: The name of the array to create in the file." << std::endl;
    }

    auto parse_qualified_name(std::string name) -> std::vector<std::string>
    {
        std::transform(begin(name), end(name), begin(name), [](char const c)
        {
            return c == ':' ? ' ' : c;
        });

        std::istringstream iss(name);

        std::vector<std::string> const parts((std::istream_iterator<std::string>(iss)),
                                              std::istream_iterator<std::string>());

        return parts;
    }

    auto read_file(std::string const& file_name) -> byte_vector
    {
        std::ifstream file(file_name.c_str(), std::ios::in | std::ios::binary);
        if (!file)
             throw std::runtime_error("unable to open file for reading");

        file.seekg(0, std::ios::end);
        std::streampos const end_position(file.tellg());
        if (end_position > std::numeric_limits<size_type>::max())
            throw std::runtime_error("the provided file is way too big");

        file.seekg(0, std::ios::beg);
        
        byte_vector data(static_cast<size_type>(end_position));
        if (!file.read(reinterpret_cast<char*>(data.data()), data.size()))
            throw std::runtime_error("failed to read from file");

        return data;
    }

    auto write_file(std::string const& file_name,
                    std::string const& array_name,
                    byte_vector const& data) -> void
    {
        std::ofstream file(file_name.c_str(), std::ios::out | std::ios::trunc);
        if (!file)
            throw std::runtime_error("unable to open file for writing");

        std::vector<std::string> const parts(parse_qualified_name(array_name));
        if (parts.size() == 0)
            throw std::runtime_error("failed to parse array name");

        file << "#include <cstdint>\n";
        file << "\n";
        

        // First write the data:
        file << "namespace {\n\n";
        file << "    std::uint8_t const " << parts.back() << "_raw_data[] = \n";
        file << "    {\n        ";

        unsigned count(0);
        std::for_each(begin(data), end(data), [&](byte const x)
        {
            file << "0x" << std::setfill('0') << std::setw(2) << std::hex << (unsigned)x << ", ";
            ++count;
            if (count % 32 == 0)
                file << "\n        ";
        });

        file << "\n    };\n";
        file << "}\n\n";

        // Then write the pointer accessors:
        std::for_each(begin(parts), std::prev(end(parts)), [&](std::string const& part)
        {
            file << "namespace " << part << " { ";
        });
        file << "\n";
        file << "\n";

        file << "    std::uint8_t const* begin_" << parts.back() << "()\n";
        file << "    {\n";
        file << "        return " << parts.back() << "_raw_data;\n";
        file << "    }\n\n";

        file << "    std::uint8_t const* end_" << parts.back() << "()\n";
        file << "    {\n";
        file << "        return " << parts.back() << "_raw_data + sizeof " << parts.back() << "_raw_data;\n";
        file << "    }\n\n";

        std::for_each(begin(parts), std::prev(end(parts)), [&](std::string const&)
        {
            file << "} ";
        });
        file << "\n\n";
    }
}

auto main(int argc, char** argv) -> int
{
    try
    {
        std::vector<std::string> const arguments(argv, argv + argc);
        if (arguments.size() != 4)
            return print_usage(), EXIT_FAILURE;

        std::string const& source_file_name(arguments[1]);
        std::string const& target_file_name(arguments[2]);
        std::string const& target_data_name(arguments[3]);

        byte_vector const data(read_file(source_file_name));

        write_file(target_file_name, target_data_name, data);
    }
    catch (std::exception const& e)
    {
        std::cout << "Uh oh.  An exception occurred during execution :'(\n"
                  << e.what();
    }
    catch (...)
    {
        std::cout << "Uh oh.  An unknown exception occurred during execution :'(\n";
    }

    // Note:  We return success even if an exception is thrown, to ensure that the build does not
    // fail when multiple configurations are built in parallel.  We'll still get the error text,
    // which is sufficient for debugging purposes.
    return EXIT_SUCCESS;
}
