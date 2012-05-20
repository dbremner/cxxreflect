
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/File.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Module.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect {

    Assembly::InnerTypeIterator Assembly::BeginModuleTypes(Module const& module)
    {
        Detail::Assert([&]{ return module.IsInitialized(); });

        return module.BeginTypes();
    }

    Assembly::InnerTypeIterator Assembly::EndModuleTypes(Module const& module)
    {
        Detail::Assert([&]{ return module.IsInitialized(); });

        return module.EndTypes();
    }

    Assembly::Assembly()
    {
    }

    Assembly::Assembly(Detail::AssemblyContext const* const context, InternalKey)
        : _context(context)
    {
        Detail::AssertNotNull(context);
    }

    AssemblyName const& Assembly::GetName() const
    {
        AssertInitialized();
        return _context.Get()->GetAssemblyName();
    }

    String Assembly::GetLocation() const
    {
        AssertInitialized();

        return _context.Get()->GetManifestModule().GetLocation().ToString();
    }

    SizeType Assembly::GetReferencedAssemblyCount() const
    {
        return _context.Get()->GetManifestModule()
            .GetDatabase()
            .GetTables()
            .GetTable(Metadata::TableId::AssemblyRef)
            .GetRowCount();
    }

    Assembly::AssemblyNameIterator Assembly::BeginReferencedAssemblyNames() const
    {
        return AssemblyNameIterator(*this, Metadata::RowReference(Metadata::TableId::AssemblyRef, 0));
    }

    Assembly::AssemblyNameIterator Assembly::EndReferencedAssemblyNames() const
    {
        return AssemblyNameIterator(*this, Metadata::RowReference(
            Metadata::TableId::AssemblyRef,
            _context.Get()->GetManifestModule()
                .GetDatabase()
                .GetTables()
                .GetTable(Metadata::TableId::AssemblyRef)
                .GetRowCount()));
    }

    Assembly::FileIterator Assembly::BeginFiles() const
    {
        AssertInitialized();

        return FileIterator(*this, Metadata::RowReference(Metadata::TableId::File, 0));
    }

    Assembly::FileIterator Assembly::EndFiles() const
    {
        AssertInitialized();

        return FileIterator(*this, Metadata::RowReference(
            Metadata::TableId::File,
            _context.Get()->GetManifestModule()
                .GetDatabase()
                .GetTables()
                .GetTable(Metadata::TableId::File)
                .GetRowCount()));
    }

    File Assembly::GetFile(StringReference const name) const
    {
        AssertInitialized();

        FileIterator const it(std::find_if(BeginFiles(), EndFiles(), [&](File const& file)
        {
            return file.GetName() == name;
        }));

        return it != EndFiles() ? *it : File();
    }

    Assembly::ModuleIterator Assembly::BeginModules() const
    {
        AssertInitialized();

        return ModuleIterator(*this, 0);
    }

    Assembly::ModuleIterator Assembly::EndModules() const
    {
        AssertInitialized();

        return ModuleIterator(*this, Detail::ConvertInteger(_context.Get()->GetModules().size()));
    }

    Module Assembly::GetModule(StringReference const name) const
    {
        AssertInitialized();

        ModuleIterator const it(std::find_if(BeginModules(), EndModules(), [&](Module const& module)
        {
            return module.GetName() == name;
        }));

        return it != EndModules() ? *it : Module();
    }

    Assembly::TypeIterator Assembly::BeginTypes() const
    {
        AssertInitialized();
        return TypeIterator(BeginModules(), EndModules());
    }

    Assembly::TypeIterator Assembly::EndTypes() const
    {
        AssertInitialized();
        return TypeIterator(EndModules());
    }

    Type Assembly::GetType(StringReference const fullTypeName) const
    {
        auto const it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& t)
        {
            return StringReference(t.GetFullName().c_str()) == fullTypeName;
        }));

        return it != EndTypes() ? *it : Type();
    }

    Type Assembly::GetType(StringReference const namespaceName, StringReference const unqualifiedTypeName) const
    {
        auto const it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& t)
        {
            return t.GetNamespace() == namespaceName
                && t.GetName()      == unqualifiedTypeName;
        }));

        return it != EndTypes() ? *it : Type();
    }

    bool Assembly::IsInitialized() const
    {
        return _context.Get() != nullptr;
    }

    bool Assembly::operator!() const
    {
        return !IsInitialized();
    }

    void Assembly::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    Metadata::AssemblyRow Assembly::GetAssemblyRow() const
    {
        AssertInitialized();

        Metadata::Database const& database(_context.Get()->GetManifestModule().GetDatabase());

        if (database.GetTables().GetTable(Metadata::TableId::Assembly).GetRowCount() == 0)
            throw RuntimeError(L"Metadata for assembly is invalid:  no Assembly record");

        return database.GetRow<Metadata::TableId::Assembly>(0);
    }

    Detail::AssemblyContext const& Assembly::GetContext(InternalKey) const
    {
        AssertInitialized();
        return *_context.Get();
    }

    bool operator==(Assembly const& lhs, Assembly const& rhs)
    {
        return lhs._context.Get() == rhs._context.Get();
    }

    bool operator<(Assembly const& lhs, Assembly const& rhs)
    {
        return std::less<Detail::AssemblyContext const*>()(lhs._context.Get(), rhs._context.Get());
    }

}
