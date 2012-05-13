
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Module.hpp"

namespace CxxReflect {

    Module::Module()
    {
    }

    Module::Module(Detail::ModuleContext const* const context, InternalKey)
        : _context(context)
    {
        Detail::AssertNotNull(context);
    }

    Module::Module(Assembly const& assembly, SizeType const moduleIndex, InternalKey)
    {
        Detail::Assert([&]{ return assembly.IsInitialized(); });

        _context.Get() = assembly.GetContext(InternalKey()).GetModules().at(moduleIndex).get();
        Detail::Assert([&]{ return _context.Get() != nullptr; });
    }

    Assembly Module::GetAssembly() const
    {
        AssertInitialized();

        return Assembly(&_context.Get()->GetAssembly(), InternalKey());
    }

    SizeType Module::GetMetadataToken() const
    {
        AssertInitialized();

        // We can cheat here:  every metadata database contains a Module table with exactly one row:
        return 0x00000001;
    }

    StringReference Module::GetName() const
    {
        AssertInitialized();

        return _context.Get()->GetDatabase().GetRow<Metadata::TableId::Module>(0).GetName();
    }

    StringReference Module::GetPath() const
    {
        AssertInitialized();

        return _context.Get()->GetLocation().ToString().c_str();
    }

    CustomAttributeIterator Module::BeginCustomAttributes() const
    {
        AssertInitialized();

        return CustomAttributeIterator(); // TODO
    }

    CustomAttributeIterator Module::EndCustomAttributes() const
    {
        AssertInitialized();

        return CustomAttributeIterator(); // TODO
    }

    Module::TypeIterator Module::BeginTypes() const
    {
        AssertInitialized();

        // We intentionally skip the type at index 0; this isn't a real type, it's the internal
        // <module> "type" containing module-scope thingies.
        return TypeIterator(*this, Metadata::RowReference(Metadata::TableId::TypeDef, 1));
    }

    Module::TypeIterator Module::EndTypes() const
    {
        AssertInitialized();

        return TypeIterator(*this, Metadata::RowReference(
            Metadata::TableId::TypeDef,
            _context.Get()->GetDatabase().GetTables().GetTable(Metadata::TableId::TypeDef).GetRowCount()));
    }

    Detail::ModuleContext const& Module::GetContext(InternalKey) const
    {
        AssertInitialized();
        return *_context.Get();
    }

    bool Module::IsInitialized() const
    {
        return _context.Get() != nullptr;
    }

    bool Module::operator!() const
    {
        return !IsInitialized();
    }

    void Module::AssertInitialized() const
    {
        Detail::Assert([&]{ return IsInitialized(); });
    }

    bool operator==(Module const& lhs, Module const& rhs)
    {
        return lhs._context.Get() == rhs._context.Get();
    }

    bool operator<(Module const& lhs, Module const& rhs)
    {
        return std::less<Detail::ModuleContext const*>()(lhs._context.Get(), rhs._context.Get());
    }


}
