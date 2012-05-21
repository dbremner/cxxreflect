
//                            Copyright James P. McNellis 2011 - 2012.                            //
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
//     data in the binary.  Shhh, don't tell anyone!





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
    typedef std::uint8_t              Byte;
    typedef std::vector<std::uint8_t> ByteVector;
    typedef std::uint32_t             SizeType;

    void PrintUsage()
    {
        std::cout << "Creates a .cpp file that defines an array of bytes.\n"
                  << "\n"
                  << "CreateFileInCpp {0} {1} {2}\n"
                  << "  {1}: The path to the source file.\n"
                  << "  {2}: The path to the .cpp file to create.\n"
                  << "  {3}: The name of the array to create in the file." << std::endl;
    }

    std::vector<std::string> ParseQualifiedName(std::string name)
    {
        std::transform(begin(name), end(name), begin(name), [](char c)
        {
            return c == ':' ? ' ' : c;
        });

        std::istringstream iss(name);

        std::vector<std::string> parts((std::istream_iterator<std::string>(iss)),
                                        std::istream_iterator<std::string>());

        return parts;
    }

    ByteVector ReadFile(std::string const& fileName)
    {
        std::ifstream file(fileName.c_str(), std::ios::in | std::ios::binary);
        if (!file)
             throw std::runtime_error("Unable to open file for reading.");

        file.seekg(0, std::ios::end);
        std::streampos const endPosition(file.tellg());
        if (endPosition > std::numeric_limits<SizeType>::max())
            throw std::runtime_error("The provided file is way too big.");

        file.seekg(0, std::ios::beg);
        
        ByteVector data(static_cast<SizeType>(endPosition));
        if (!file.read(reinterpret_cast<char*>(data.data()), data.size()))
            throw std::runtime_error("Failed to read from file.");

        return data;
    }

    void WriteFile(std::string const& fileName,
                   std::string const& arrayName,
                   ByteVector  const& data)
    {
        std::ofstream file(fileName.c_str(), std::ios::out | std::ios::trunc);
        if (!file)
            throw std::runtime_error("Unable to open file for writing.");

        std::vector<std::string> const parts(ParseQualifiedName(arrayName));
        if (parts.size() == 0)
            throw std::runtime_error("Failed to parse array name.");

        file << "#include <cstdint>\n";
        file << "\n";
        

        // First write the data:
        file << "namespace {\n\n";
        file << "    std::uint8_t const " << parts.back() << "RawData[] = \n";
        file << "    {\n        ";

        unsigned count(0);
        std::for_each(begin(data), end(data), [&](Byte const x)
        {
            file << "0x" << std::setfill('0') << std::setw(2) << std::hex << (unsigned)x << ", ";
            ++count;
            if (count % 32 == 0)
                file << "\n        ";
        });

        file << "\n    };\n";
        file << "}\n\n";

        // Then write the pointer accessors:
        std::for_each(begin(parts), prev(end(parts)), [&](std::string const& part)
        {
            file << "namespace " << part << " { ";
        });
        file << "\n";
        file << "\n";

        file << "    std::uint8_t const* Begin" << parts.back() << "()\n";
        file << "    {\n";
        file << "        return " << parts.back() << "RawData;\n";
        file << "    }\n\n";

        file << "    std::uint8_t const* End" << parts.back() << "()\n";
        file << "    {\n";
        file << "        return " << parts.back() << "RawData + sizeof " << parts.back() << "RawData;\n";
        file << "    }\n\n";

        std::for_each(begin(parts), prev(end(parts)), [&](std::string const&)
        {
            file << "} ";
        });
        file << "\n\n";
    }
}

int main(int argc, char** argv)
{
    try
    {
        std::vector<std::string> const arguments(argv, argv + argc);
        if (arguments.size() != 4)
            return PrintUsage(), EXIT_FAILURE;

        std::string const& sourceFileName(arguments[1]);
        std::string const& targetFileName(arguments[2]);
        std::string const& targetDataName(arguments[3]);

        ByteVector const data(ReadFile(sourceFileName));

        WriteFile(targetFileName, targetDataName, data);
    }
    catch (std::exception const& e)
    {
        std::cout << "Uh oh.  An exception occurred during execution :'(\n"
                  << e.what();

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
