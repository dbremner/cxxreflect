//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_FILE_HPP_
#define CXXREFLECT_FILE_HPP_

#include "CxxReflect/CoreInternals.hpp"

namespace CxxReflect {

    class File
    {
    public:

        File();

        FileFlags       GetAttributes()    const;
        StringReference GetName()          const;
        Assembly        GetAssembly()      const;
        bool            ContainsMetadata() const;

        // TODO GetHashValue()?
        // TODO Consider providing interface to locate and open the file

        bool IsInitialized() const;
        bool operator!()     const;

        friend bool operator==(File const&, File const&);
        friend bool operator< (File const&, File const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(File)
        CXXREFLECT_GENERATE_SAFE_BOOL_CONVERSION(File)

    public: // Internal Members

        File(Assembly assembly, Metadata::RowReference file, InternalKey);

    private:

        void VerifyInitialized() const;

        Metadata::FileRow GetFileRow() const;

        Detail::AssemblyHandle _assembly;
        Metadata::RowReference _file;
    };

}

#endif
