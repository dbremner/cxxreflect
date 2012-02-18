//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/File.hpp"
#include "CxxReflect/Loader.hpp"

namespace CxxReflect {

    File::File()
    {
    }

    File::File(Assembly const assembly, Metadata::RowReference const file, InternalKey)
        : _assembly(assembly), _file(file)
    {
        AssertInitialized();
    }

    FileFlags File::GetAttributes() const
    {
        return GetFileRow().GetFlags();
    }

    StringReference File::GetName() const
    {
        return GetFileRow().GetName();
    }

    Assembly File::GetAssembly() const
    {
        AssertInitialized();
        return _assembly.Realize();
    }

    Metadata::FileRow File::GetFileRow() const
    {
        AssertInitialized();
        return _assembly.Realize()
            .GetContext(InternalKey())
            .GetDatabase()
            .GetRow<Metadata::TableId::File>(_file);
    }

    bool File::IsInitialized() const
    {
        return _assembly.IsInitialized() && _file.IsInitialized();
    }

    bool File::operator!() const
    {
        return !IsInitialized();
    }

    void File::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    bool operator==(File const& lhs, File const& rhs)
    {
        return lhs._assembly == rhs._assembly
            && lhs._file == rhs._file;
    }

    bool operator<(File const& lhs, File const& rhs)
    {
        if (lhs._assembly < rhs._assembly)
            return true;

        return lhs._assembly == rhs._assembly && lhs._file < rhs._file;
    }

}
