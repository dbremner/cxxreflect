
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

    Assembly::Assembly()
    {
    }

    Assembly::Assembly(Detail::AssemblyContext const* const context, InternalKey)
        : _context(context)
    {
        AssertInitialized();
    }

    AssemblyName const& Assembly::GetName() const
    {
        AssertInitialized();
        return _context.Get()->GetAssemblyName();
    }

    String const& Assembly::GetLocation() const
    {
        AssertInitialized();
        return _context.Get()->GetLocation();
    }

    SizeType Assembly::GetReferencedAssemblyCount() const
    {
        return _context.Get()->GetDatabase().GetTables().GetTable(Metadata::TableId::AssemblyRef).GetRowCount();
    }

    Assembly::AssemblyNameIterator Assembly::BeginReferencedAssemblyNames() const
    {
        return AssemblyNameIterator(*this, Metadata::RowReference(Metadata::TableId::AssemblyRef, 0));
    }

    Assembly::AssemblyNameIterator Assembly::EndReferencedAssemblyNames() const
    {
        return AssemblyNameIterator(*this, Metadata::RowReference(
            Metadata::TableId::AssemblyRef,
            _context.Get()->GetDatabase().GetTables().GetTable(Metadata::TableId::AssemblyRef).GetRowCount()));
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
            _context.Get()->GetDatabase().GetTables().GetTable(Metadata::TableId::File).GetRowCount()));
    }

    File Assembly::GetFile(StringReference const name) const
    {
        FileIterator const it(std::find_if(BeginFiles(), EndFiles(), [&](File const& file)
        {
            return file.GetName() == name;
        }));

        return it != EndFiles() ? *it : File();
    }

    Assembly::ModuleIterator Assembly::BeginModules() const
    {
        Detail::AssertFail(L"NYI");
        return ModuleIterator();
    }

    Assembly::ModuleIterator Assembly::EndModules() const
    {
        Detail::AssertFail(L"NYI");
        return ModuleIterator();
    }

    Module Assembly::GetModule(StringReference const /*name*/) const
    {
        Detail::AssertFail(L"NYI");
        return Module();
    }

    Assembly::TypeIterator Assembly::BeginTypes() const
    {
        AssertInitialized();
        // We intentionally skip the type at index 0; this isn't a real type, it's the internal
        // <module> "type" containing module-scope thingies.
        return TypeIterator(*this, Metadata::RowReference(Metadata::TableId::TypeDef, 1));
    }

    Assembly::TypeIterator Assembly::EndTypes() const
    {
        AssertInitialized();
        return TypeIterator(*this, Metadata::RowReference(
            Metadata::TableId::TypeDef,
            _context.Get()->GetDatabase().GetTables().GetTable(Metadata::TableId::TypeDef).GetRowCount()));
    }

    Assembly::TypeSequence Assembly::GetTypes() const
    {
        AssertInitialized();
        return TypeSequence(BeginTypes(), EndTypes());
    }

    Type Assembly::GetType(StringReference const namespaceQualifiedTypeName) const
    {
        auto const it(std::find_if(BeginTypes(), EndTypes(), [&](Type const& t)
        {
            return StringReference(t.GetFullName().c_str()) == namespaceQualifiedTypeName;
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

        Metadata::Database const& database(_context.Get()->GetDatabase());

        if (database.GetTables().GetTable(Metadata::TableId::Assembly).GetRowCount() == 0)
            throw std::runtime_error("wtf");

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
