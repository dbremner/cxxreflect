
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/AssemblyName.hpp"
#include "CxxReflect/CoreComponents.hpp"
#include "CxxReflect/Event.hpp"
#include "CxxReflect/Field.hpp"
#include "CxxReflect/Loader.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Module.hpp"
#include "CxxReflect/Parameter.hpp"
#include "CxxReflect/Property.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace { namespace Private {

    class DefaultLoaderConfiguration : public ILoaderConfiguration
    {
    public:

        virtual StringReference GetSystemNamespace() const
        {
            return L"System";
        }
    };

} } }

namespace CxxReflect {

    ModuleLocation::ModuleLocation()
        : _kind(Kind::Uninitialized)
    {
    }

    ModuleLocation::ModuleLocation(ConstByteRange const& memoryRange)
        : _kind(Kind::Memory), _memoryRange(memoryRange)
    {
        Detail::Assert([&]{ return memoryRange.IsInitialized(); });
    }

    ModuleLocation::ModuleLocation(String const& filePath)
        : _kind(Kind::File), _filePath(filePath.c_str())
    {
        Detail::Assert([&]{ return !filePath.empty(); });
    }

    ModuleLocation::Kind ModuleLocation::GetKind() const
    {
        return _kind.Get();
    }

    bool ModuleLocation::IsFile() const
    {
        return GetKind() == Kind::File;
    }

    bool ModuleLocation::IsMemory() const
    {
        return GetKind() == Kind::Memory;
    }

    bool ModuleLocation::IsInitialized() const
    {
        return GetKind() != Kind::Uninitialized;
    }

    ConstByteRange const& ModuleLocation::GetMemoryRange() const
    {
        Detail::Assert([&]{ return IsMemory(); });
        return _memoryRange;
    }

    String const& ModuleLocation::GetFilePath() const
    {
        Detail::Assert([&]{ return IsFile(); });
        return _filePath;
    }

    String ModuleLocation::ToString() const
    {
        Detail::Assert([&]{ return IsInitialized(); });

        if (IsFile())
            return GetFilePath();

        if (IsMemory())
            return L"<Memory:" + Detail::ToString(GetMemoryRange().Begin()) + L">";

        Detail::AssertFail(L"Unreachable code");
        return String();
    }

    bool operator==(ModuleLocation const& lhs, ModuleLocation const& rhs)
    {
        if (lhs._kind.Get() != rhs._kind.Get())
            return false;

        if (lhs._kind.Get() == ModuleLocation::Kind::Uninitialized)
            return true;

        if (lhs._kind.Get() == ModuleLocation::Kind::File)
            return lhs._filePath == rhs._filePath;

        if (lhs._kind.Get() == ModuleLocation::Kind::Memory)
            return std::equal_to<ConstByteIterator>()(lhs._memoryRange.Begin(), rhs._memoryRange.Begin());

        Detail::AssertFail(L"Unreachable code");
        return false;
    }

    bool operator<(ModuleLocation const& lhs, ModuleLocation const& rhs)
    {
        // Provide an arbitrary but consistent ordering of kinds:
        if (lhs._kind.Get() < rhs._kind.Get())
            return true;

        if (lhs._kind.Get() > rhs._kind.Get())
            return false;

        Detail::Assert([&]{ return lhs._kind.Get() == rhs._kind.Get(); });

        // All uninitialized objects compare equal:
        if (lhs._kind.Get() == ModuleLocation::Kind::Uninitialized)
            return false;

        if (lhs._kind.Get() == ModuleLocation::Kind::File)
            return lhs._filePath < rhs._filePath;

        if (lhs._kind.Get() == ModuleLocation::Kind::Memory)
            return std::less<ConstByteIterator>()(lhs._memoryRange.Begin(), rhs._memoryRange.Begin());

        Detail::AssertFail(L"Unreachable code");
        return false;
    }





    IModuleLocator::~IModuleLocator()
    {
    }

    ILoaderConfiguration::~ILoaderConfiguration()
    {
    }

}

namespace CxxReflect { namespace Detail {

    ModuleContext::ModuleContext(AssemblyContext const* const assembly, ModuleLocation const& location)
        : _assembly(assembly), _location(location), _database(CreateDatabase(location))
    {
        AssertNotNull(_assembly.Get());
        Assert([&]{ return _location.IsInitialized(); });
        Assert([&]{ return _database.IsInitialized(); });

        // Register the newly-loaded module with the Loader so that it can do fast reverse lookups:
        assembly->GetLoader().RegisterModule(this);
    }

    AssemblyContext const& ModuleContext::GetAssembly() const
    {
        return *_assembly.Get();
    }

    ModuleLocation const& ModuleContext::GetLocation() const
    {
        return _location;
    }

    Metadata::Database const& ModuleContext::GetDatabase() const
    {
        return _database;
    }

    Metadata::RowReference ModuleContext::GetTypeDefByName(StringReference const namespaceName,
                                                           StringReference const typeName) const
    {
        auto const it(std::find_if(_database.Begin<Metadata::TableId::TypeDef>(),
                                   _database.End<Metadata::TableId::TypeDef>(),
                                   [&](Metadata::TypeDefRow const& typeDef)
        {
            return typeDef.GetNamespace() == namespaceName && typeDef.GetName() == typeName;
        }));

        return it == _database.End<Metadata::TableId::TypeDef>()
            ? Metadata::RowReference()
            : it->GetSelfReference();
    }

    Metadata::Database ModuleContext::CreateDatabase(ModuleLocation const& location)
    {
        Assert([&]{ return location.IsInitialized(); });

        if (location.IsFile())
        {
            return Metadata::Database::CreateFromFile(location.GetFilePath().c_str());
        }
        else if (!location.IsMemory())
        {
            AssertFail(L"Unreachable code");
        }

        return Metadata::Database(
            Detail::FileRange(
                location.GetMemoryRange().Begin(),
                location.GetMemoryRange().End(),
                nullptr));
    }





    AssemblyContext::AssemblyContext(LoaderContext const* const loader, ModuleLocation const& location)
        : _loader(loader)
    {
        AssertNotNull(_loader.Get());

        UniqueModuleContext module(new ModuleContext(this, std::move(location)));
        _modules.push_back(std::move(module));

        if (_modules[0]->GetDatabase().GetTables().GetTable(Metadata::TableId::Assembly).GetRowCount() != 1)
            throw RuntimeError(L"The module at the specified location has no manifest and is not an assembly");
    }

    LoaderContext const& AssemblyContext::GetLoader() const
    {
        return *_loader.Get();
    }

    ModuleContext const& AssemblyContext::GetManifestModule() const
    {
        return *_modules.front();
    }

    AssemblyContext::ModuleContextSequence const& AssemblyContext::GetModules() const
    {
        RealizeModules();
        return _modules;
    }

    AssemblyName const& AssemblyContext::GetAssemblyName() const
    {
        RealizeName();
        return *_name;
    }

    void AssemblyContext::RealizeName() const
    {
        if (_state.IsSet(RealizationState::Name))
            return;

        _name.reset(new AssemblyName(
            Assembly(this, InternalKey()),
            Metadata::RowReference(Metadata::TableId::Assembly, 0),
            InternalKey()));

        _state.Set(RealizationState::Name);
    }

    void AssemblyContext::RealizeModules() const
    {
        if (_state.IsSet(RealizationState::Modules))
            return;

        Metadata::Database const& manifestDatabase(GetManifestModule().GetDatabase());

        std::for_each(manifestDatabase.Begin<Metadata::TableId::File>(),
                      manifestDatabase.End<Metadata::TableId::File>(),
                      [&](Metadata::FileRow const& file)
        {
            if (file.GetFlags().IsSet(FileAttribute::ContainsNoMetadata))
                return;

            ModuleLocation const location(_loader.Get()->GetLocator().LocateModule(
                GetAssemblyName(),
                file.GetName().c_str()));

            if (!location.IsInitialized())
                throw RuntimeError(L"Failed to locate module");

            UniqueModuleContext module(new ModuleContext(this, location));
            _modules.push_back(std::move(module));
        });

        _state.Set(RealizationState::Modules);
    }





    class LoaderContextSynchronizer
    {
    public:

        std::unique_lock<std::recursive_mutex> Lock() const
        {
            return std::unique_lock<std::recursive_mutex>(_lock);
        }

    private:

        std::recursive_mutex mutable _lock;
    };





    // Disable the "don't use this in initializer list" warning; we know what we're doing! ;-)
    #pragma warning(push)
    #pragma warning(disable: 4355)
    LoaderContext::LoaderContext(UniqueModuleLocator locator, UniqueLoaderConfiguration configuration)
        : _locator(std::move(locator)),
          _configuration(std::move(configuration)),

          _contextStorage(CreateElementContextTableStorage()),
          _events    (this, _contextStorage.get()),
          _fields    (this, _contextStorage.get()),
          _interfaces(this, _contextStorage.get()),
          _methods   (this, _contextStorage.get()),
          _properties(this, _contextStorage.get()),

          _sync(new LoaderContextSynchronizer())
    {
        if (_configuration == nullptr)
            _configuration.reset(new Private::DefaultLoaderConfiguration());
    }
    #pragma warning(pop)

    LoaderContext::~LoaderContext()
    {
        // Destructor required for std::unique_ptr completeness
    }

    IModuleLocator const& LoaderContext::GetLocator() const
    {
        return *_locator;
    }

    AssemblyContext const& LoaderContext::GetOrLoadAssembly(ModuleLocation const& location) const
    {
        Assert([&]{ return location.IsInitialized(); });

        String const canonicalUri(location.IsFile()
            ? Externals::ComputeCanonicalUri(location.GetFilePath().c_str())
            : L"memory://" + ToString(location.GetMemoryRange().Begin()));

        auto const lock(_sync->Lock());
        
        auto const it0(_assemblies.find(canonicalUri));
        if (it0 != end(_assemblies))
            return *it0->second;

        UniqueAssemblyContext assembly(new AssemblyContext(this, location));
        auto const it1(_assemblies.insert(std::make_pair(canonicalUri, std::move(assembly))));

        return *it1.first->second;
    }

    AssemblyContext const& LoaderContext::GetOrLoadAssembly(AssemblyName const& name) const
    {
        return GetOrLoadAssembly(_locator->LocateAssembly(name));
    }

    Metadata::FullReference LoaderContext::ResolveType(Metadata::FullReference const& typeReference) const
    {
        using namespace CxxReflect::Metadata;

        Assert([&]{ return typeReference.IsInitialized() && typeReference.IsRowReference(); });

        // A TypeDef or TypeSpec is already resolved:
        if (typeReference.AsRowReference().GetTable() == TableId::TypeDef ||
            typeReference.AsRowReference().GetTable() == TableId::TypeSpec)
            return typeReference;

        // If it isn't a TypeDef or TypeSpec, it must be a TypeRef:
        Assert([&]{ return typeReference.AsRowReference().GetTable() == TableId::TypeRef; });

        Database     const& typeRefDatabase(typeReference.GetDatabase());
        SizeType     const  typeRefIndex(typeReference.AsRowReference().GetIndex());
        TypeRefRow   const  typeRef(typeRefDatabase.GetRow<TableId::TypeRef>(typeRefIndex));
        RowReference const  typeRefScope(typeRef.GetResolutionScope());

        StringReference const typeRefNamespace(typeRef.GetNamespace());
        StringReference const typeRefName(typeRef.GetName());

        // If the resolution scope is null, we need to look in the ExportedType table for this type:
        if (!typeRefScope.IsInitialized())
        {
            throw LogicError(L"Not yet implemented");
        }

        switch (typeRefScope.GetTable())
        {
        case TableId::Module:
        {
            // A module resolution scope means the target type is defined in the same module:
            ModuleContext const& definingModule(GetContextForDatabase(typeReference.GetDatabase()));
            Database      const& definingDatabase(definingModule.GetDatabase());

            RowReference const typeDef(definingModule.GetTypeDefByName(typeRefNamespace, typeRefName));
            if (!typeDef.IsInitialized())
                throw RuntimeError(L"Failed to resolve type in assembly");

            return Metadata::FullReference(&definingDatabase, typeDef);
        }
        case TableId::ModuleRef:
        {
            throw LogicError(L"Not yet implemented");
        }
        case TableId::AssemblyRef:
        {
            AssemblyName const definingAssemblyName(
                Assembly(&GetContextForDatabase(typeRefDatabase).GetAssembly(), InternalKey()),
                typeRefScope,
                InternalKey());

            String const typeRefFullName(typeRefNamespace.empty()
                ? String(typeRefName.c_str())
                : String(typeRefNamespace.c_str()) + L"." + typeRefName.c_str());

            ModuleLocation const& location(_locator->LocateAssembly(definingAssemblyName, typeRefFullName));

            AssemblyContext const& definingAssembly(GetOrLoadAssembly(location));
            ModuleContext   const& definingModule(definingAssembly.GetManifestModule());
            Database        const& definingDatabase(definingModule.GetDatabase());

            RowReference const typeDef(definingModule.GetTypeDefByName(typeRefNamespace, typeRefName));
            if (!typeDef.IsInitialized())
                throw RuntimeError(L"Failed to resolve type in assembly");

            return Metadata::FullReference(&definingDatabase, typeDef);
        }
        case TableId::TypeRef:
        {
            throw LogicError(L"Not yet implemented");
        }
        default:
        {
            // The resolution scope must be from one of the four tables in the switch; if we get
            // here, something is broken in the Database code.
            Detail::AssertFail(L"Unreachable code");
            return Metadata::FullReference();
        }
        }
    }

    Metadata::FullReference LoaderContext::ResolveFundamentalType(Metadata::ElementType const elementType) const
    {
        Assert([&]{ return elementType < Metadata::ElementType::ConcreteElementTypeMax; });

        auto const lock(_sync->Lock());

        if (_fundamentalTypes[Detail::AsInteger(elementType)].IsInitialized())
            return _fundamentalTypes[Detail::AsInteger(elementType)];

        StringReference primitiveTypeName;
        switch (elementType)
        {
        case Metadata::ElementType::Boolean:    primitiveTypeName = L"Boolean";        break;
        case Metadata::ElementType::Char:       primitiveTypeName = L"Char";           break;
        case Metadata::ElementType::I1:         primitiveTypeName = L"SByte";          break;
        case Metadata::ElementType::U1:         primitiveTypeName = L"Byte";           break;
        case Metadata::ElementType::I2:         primitiveTypeName = L"Int16";          break;
        case Metadata::ElementType::U2:         primitiveTypeName = L"UInt16";         break;
        case Metadata::ElementType::I4:         primitiveTypeName = L"Int32";          break;
        case Metadata::ElementType::U4:         primitiveTypeName = L"UInt32";         break;
        case Metadata::ElementType::I8:         primitiveTypeName = L"Int64";          break;
        case Metadata::ElementType::U8:         primitiveTypeName = L"UInt64";         break;
        case Metadata::ElementType::R4:         primitiveTypeName = L"Single";         break;
        case Metadata::ElementType::R8:         primitiveTypeName = L"Double";         break;
        case Metadata::ElementType::I:          primitiveTypeName = L"IntPtr";         break;
        case Metadata::ElementType::U:          primitiveTypeName = L"UIntPtr";        break;
        case Metadata::ElementType::Object:     primitiveTypeName = L"Object";         break;
        case Metadata::ElementType::String:     primitiveTypeName = L"String";         break;
        case Metadata::ElementType::Array:      primitiveTypeName = L"Array";          break;
        case Metadata::ElementType::SzArray:    primitiveTypeName = L"Array";          break;
        case Metadata::ElementType::ValueType:  primitiveTypeName = L"ValueType";      break;
        case Metadata::ElementType::Void:       primitiveTypeName = L"Void";           break;
        case Metadata::ElementType::TypedByRef: primitiveTypeName = L"TypedReference"; break;
        default:
            Detail::AssertFail(L"Unknown primitive type");
            break;
        }

        ModuleContext const& systemModule(GetSystemModule());

        Metadata::RowReference const typeDef(systemModule.GetTypeDefByName(GetSystemNamespace(), primitiveTypeName));
        if (!typeDef.IsInitialized())
            throw RuntimeError(L"Failed to find expected type in system assembly");
                          
        return Metadata::FullReference(&systemModule.GetDatabase(), typeDef);
    }

    Metadata::FullReference LoaderContext::ResolveReplacementType(Metadata::FullReference const& type) const
    {
        return type; // TODO
    }

    ModuleContext const& LoaderContext::GetContextForDatabase(Metadata::Database const& database) const
    {
        auto const it(_moduleMap.find(&database));
        if (it == end(_moduleMap))
            throw RuntimeError(L"Database is not owned by this loader");

        return *it->second;
    }

    void LoaderContext::RegisterModule(ModuleContext const* module) const
    {
        AssertNotNull(module);

        _moduleMap.insert(std::make_pair(&module->GetDatabase(), module));
    }

    ModuleContext const& LoaderContext::GetSystemModule() const
    {
        {
            auto const lock(_sync->Lock());
            if (_systemModule.Get() != nullptr)
                return *_systemModule.Get();
        }

        if (_assemblies.empty())
            throw RuntimeError(L"No assemblies have been loaded; cannot determine system assembly");

        // First check to see if the system assembly has already been loaded:
        auto const it0(std::find_if(begin(_assemblies), end(_assemblies), [&](AssemblyMapEntry const& assembly)
        {
            return (*assembly.second)
                .GetManifestModule()
                .GetDatabase()
                .GetTables()
                .GetTable(Metadata::TableId::AssemblyRef)
                .GetRowCount() == 0;
        }));

        if (it0 != end(_assemblies))
        {
            auto const lock(_sync->Lock());
            _systemModule.Get() = &it0->second->GetManifestModule();
            return *_systemModule.Get();
        }

        // Ok, we haven't loaded the system assembly yet.  Pick an arbitrary type from a loaded
        // assembly and resolve the Object base type from it.  First we need to find an assembly
        // that defines types:
        auto const it1(std::find_if(begin(_assemblies), end(_assemblies), [&](AssemblyMapEntry const& assembly)
        {
            // Note that we need more than one row in the TypeDef table because the row at index 0
            // is the faux global entry:
            return (*assembly.second)
                .GetManifestModule()
                .GetDatabase()
                .GetTables()
                .GetTable(Metadata::TableId::TypeDef)
                .GetRowCount() > 1;
        }));

        if (it1 == end(_assemblies))
            throw RuntimeError(L"No loaded assemblies define types; cannot determine system assembly");

        // Resolve Object by recursively walking the type's base classes.  TODO We should be able to
        // do this without creating all of the public interface context.
        Assembly const referenceAssembly(&*it1->second, InternalKey());

        Assert([&]{ return referenceAssembly.BeginTypes() != referenceAssembly.EndTypes(); });

        Type referenceType(*referenceAssembly.BeginTypes());
        while (referenceType.GetBaseType().IsInitialized())
            referenceType = referenceType.GetBaseType();

        Assert([&]{ return referenceType.GetName() == L"Object"; });

        auto const lock(_sync->Lock());
        _systemModule.Get() = &referenceType.GetModule().GetContext(InternalKey());
        return *_systemModule.Get();
    }

    StringReference LoaderContext::GetSystemNamespace() const
    {
        return _configuration->GetSystemNamespace();
    }

    EventContextTable LoaderContext::GetOrCreateEventTable(Metadata::FullReference const& type) const
    {
        Assert([&]{ return type.IsInitialized(); });

        return _events.GetOrCreateTable(type);
    }

    FieldContextTable LoaderContext::GetOrCreateFieldTable(Metadata::FullReference const& type) const
    {
        Assert([&]{ return type.IsInitialized(); });

        return _fields.GetOrCreateTable(type);
    }

    InterfaceContextTable LoaderContext::GetOrCreateInterfaceTable(Metadata::FullReference const& type) const
    {
        Assert([&]{ return type.IsInitialized(); });

        return _interfaces.GetOrCreateTable(type);
    }

    MethodContextTable LoaderContext::GetOrCreateMethodTable(Metadata::FullReference const& type) const
    {
        Assert([&]{ return type.IsInitialized(); });

        return _methods.GetOrCreateTable(type);
    }

    PropertyContextTable LoaderContext::GetOrCreatePropertyTable(Metadata::FullReference const& type) const
    {
        Assert([&]{ return type.IsInitialized(); });

        return _properties.GetOrCreateTable(type);
    }

    LoaderContext const& LoaderContext::From(AssemblyContext const& o)
    {
        return o.GetLoader();
    }

    LoaderContext const& LoaderContext::From(ModuleContext const& o)
    {
        return o.GetAssembly().GetLoader();
    }

    LoaderContext const& LoaderContext::From(Assembly const& o)
    {
        Assert([&]{ return o.IsInitialized(); });

        return o.GetContext(InternalKey()).GetLoader();
    }

    LoaderContext const& LoaderContext::From(Module const& o)
    {
        Assert([&]{ return o.IsInitialized(); });

        return o.GetContext(InternalKey()).GetAssembly().GetLoader();
    }

    LoaderContext const& LoaderContext::From(Type const& o)
    {
        Assert([&]{ return o.IsInitialized(); });

        return o.GetAssembly().GetContext(InternalKey()).GetLoader();
    }





    AssemblyHandle::AssemblyHandle()
    {
    }

    AssemblyHandle::AssemblyHandle(AssemblyContext const* context)
        : _context(context)
    {
        AssertInitialized();
    }

    AssemblyHandle::AssemblyHandle(Assembly const& assembly)
        : _context(&assembly.GetContext(InternalKey()))
    {
        AssertInitialized();
    }

    Assembly AssemblyHandle::Realize() const
    {
        AssertInitialized();
        return Assembly(_context.Get(), InternalKey());
    }

    bool AssemblyHandle::IsInitialized() const
    {
        return _context.Get() != nullptr;
    }

    void AssemblyHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(AssemblyHandle const& lhs, AssemblyHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator<(AssemblyHandle const& lhs, AssemblyHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    MethodHandle::MethodHandle()
    {
    }

    MethodHandle::MethodHandle(ModuleContext const*       const  reflectedTypeModule,
                               Metadata::ElementReference const& reflectedType,
                               MethodContext const*       const  context)
        : _reflectedTypeModule(reflectedTypeModule),
          _reflectedType(reflectedType),
          _context(context)
    {
        AssertInitialized();
    }

    MethodHandle::MethodHandle(Method const& method)
        : _reflectedTypeModule(&method.GetReflectedType().GetModule().GetContext(InternalKey())),
          _reflectedType(method.GetReflectedType().GetSelfReference(InternalKey())),
          _context(&method.GetContext(InternalKey()))
    {
        AssertInitialized();
    }

    Method MethodHandle::Realize() const
    {
        AssertInitialized();
        Module const module(_reflectedTypeModule.Get(), InternalKey());

        Type const reflectedType(_reflectedType.IsRowReference()
            ? Type(module, _reflectedType.AsRowReference(),  InternalKey())
            : Type(module, _reflectedType.AsBlobReference(), InternalKey()));

        return Method(reflectedType, _context.Get(), InternalKey());
    }

    bool MethodHandle::IsInitialized() const
    {
        return _reflectedTypeModule.Get() != nullptr
            && _reflectedType.IsInitialized()
            && _context.Get() != nullptr;
    }

    void MethodHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(MethodHandle const& lhs, MethodHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator<(MethodHandle const& lhs, MethodHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    ModuleHandle::ModuleHandle()
    {
    }

    ModuleHandle::ModuleHandle(ModuleContext const* context)
        : _context(context)
    {
        AssertNotNull(context);
    }

    ModuleHandle::ModuleHandle(Module const& module)
        : _context(&module.GetContext(InternalKey()))
    {
        Assert([&]{ return module.IsInitialized(); });
    }

    Module ModuleHandle::Realize() const
    {
        AssertInitialized();
        return Module(_context.Get(), InternalKey());
    }

    ModuleContext const& ModuleHandle::GetContext() const
    {
        AssertInitialized();
        return *_context.Get();
    }

    bool ModuleHandle::IsInitialized() const
    {
        return _context.Get() != nullptr;
    }

    void ModuleHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }





    ParameterHandle::ParameterHandle()
    {
    }

    ParameterHandle::ParameterHandle(ModuleContext              const* reflectedTypeModule,
                                     Metadata::ElementReference const& reflectedType,
                                     MethodContext              const* context,
                                     Metadata::RowReference     const& parameterReference,
                                     Metadata::TypeSignature    const& parameterSignature)
        : _reflectedTypeModule(reflectedTypeModule),
          _reflectedType(reflectedType),
          _context(context),
          _parameterReference(parameterReference),
          _parameterSignature(parameterSignature)
    {
        AssertInitialized();
    }

    ParameterHandle::ParameterHandle(Parameter const& parameter)
        : _reflectedTypeModule(&parameter.GetDeclaringMethod().GetReflectedType().GetModule().GetContext(InternalKey())),
          _reflectedType(parameter.GetDeclaringMethod().GetReflectedType().GetSelfReference(InternalKey())),
          _context(&parameter.GetDeclaringMethod().GetContext(InternalKey())),
          _parameterReference(parameter.GetSelfReference(InternalKey())),
          _parameterSignature(parameter.GetSelfSignature(InternalKey()))
    {
        AssertInitialized();
    }

    Parameter ParameterHandle::Realize() const
    {
        AssertInitialized();
        Module const module(_reflectedTypeModule.Get(), InternalKey());

        Type const reflectedType(_reflectedType.IsRowReference()
            ? Type(module, _reflectedType.AsRowReference(),  InternalKey())
            : Type(module, _reflectedType.AsBlobReference(), InternalKey()));

        Method const declaringMethod(reflectedType, _context.Get(), InternalKey());

        return Parameter(declaringMethod,_parameterReference, _parameterSignature, InternalKey());

        return Parameter();
    }

    bool ParameterHandle::IsInitialized() const
    {
        return _reflectedTypeModule.Get() != nullptr
            && _reflectedType.IsInitialized()
            && _context.Get() != nullptr
            && _parameterReference.IsInitialized()
            && _parameterSignature.IsInitialized();
    }

    void ParameterHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(ParameterHandle const& lhs, ParameterHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator<(ParameterHandle const& lhs, ParameterHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    TypeHandle::TypeHandle()
    {
    }

    TypeHandle::TypeHandle(ModuleContext const* const module, Metadata::ElementReference const& type)
        : _module(module), _type(type)
    {
        AssertInitialized();
    }

    TypeHandle::TypeHandle(Type const& type)
        : _module(&type.GetModule().GetContext(InternalKey())),
          _type(type.GetSelfReference(InternalKey()))
    {
        AssertInitialized();
    }

    Type TypeHandle::Realize() const
    {
        AssertInitialized();
        Module const module(_module.Get(), InternalKey());
        return _type.IsRowReference()
            ? Type(module, _type.AsRowReference(),  InternalKey())
            : Type(module, _type.AsBlobReference(), InternalKey());
    }

    bool TypeHandle::IsInitialized() const
    {
        return _module.Get() != nullptr && _type.IsInitialized();
    }

    void TypeHandle::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    bool operator==(TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() == rhs.Realize();
    }

    bool operator<(TypeHandle const& lhs, TypeHandle const& rhs)
    {
        return lhs.Realize() < rhs.Realize();
    }





    ParameterData::ParameterData()
    {
    }

    ParameterData::ParameterData(Metadata::RowReference                       const& parameter,
                                 Metadata::MethodSignature::ParameterIterator const& signature,
                                 InternalKey)
        : _parameter(parameter), _signature(signature)
    {
        AssertInitialized();
    }

    bool ParameterData::IsInitialized() const
    {
        return _parameter.IsInitialized();
    }

    void ParameterData::AssertInitialized() const
    {
        Assert([&]{ return IsInitialized(); });
    }

    ParameterData& ParameterData::operator++()
    {
        AssertInitialized();

        ++_parameter;
        ++_signature;
        return *this;
    }

    ParameterData ParameterData::operator++(int)
    {
        ParameterData const it(*this);
        ++*this;
        return it;
    }

    bool operator==(ParameterData const& lhs, ParameterData const& rhs)
    {
        lhs.AssertInitialized();
        rhs.AssertInitialized();

        return lhs._parameter == rhs._parameter;
    }

    bool operator< (ParameterData const& lhs, ParameterData const& rhs)
    {
        lhs.AssertInitialized();
        rhs.AssertInitialized();

        return lhs._parameter < rhs._parameter;
    }

    Metadata::RowReference const& ParameterData::GetParameter() const
    {
        AssertInitialized();

        return _parameter;
    }

    Metadata::TypeSignature const& ParameterData::GetSignature() const
    {
        AssertInitialized();

        return *_signature;
    }





    bool IsSystemAssembly(Assembly const& assembly)
    {
        Detail::Assert([&]{ return assembly.IsInitialized(); });

        return assembly.GetReferencedAssemblyCount() == 0;
    }

    bool IsSystemType(Type            const& type,
                      StringReference const& systemTypeNamespace,
                      StringReference const& systemTypeSimpleName)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        String const transformedNamespace(type
            .GetAssembly()
            .GetContext(InternalKey())
            .GetLoader()
            .GetSystemNamespace().c_str());

        return IsSystemAssembly(type.GetAssembly())
            && type.GetNamespace() == transformedNamespace
            && type.GetName()      == systemTypeSimpleName;
    }

    bool IsDerivedFromSystemType(Type const& type, Metadata::ElementType const systemType, bool const includeSelf)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        Type currentType(type);
        if (!includeSelf && currentType.IsInitialized())
            currentType = currentType.GetBaseType();

        LoaderContext const& loader(type.GetModule().GetContext(InternalKey()).GetAssembly().GetLoader());

        Metadata::FullReference const targetTypeReference(loader.ResolveFundamentalType(systemType));

        ModuleContext const& targetTypeModule(loader.GetContextForDatabase(targetTypeReference.GetDatabase()));

        Type const& targetType(targetTypeReference.IsRowReference()
                ? Type(Module(&targetTypeModule, InternalKey()), targetTypeReference.AsRowReference(), InternalKey())
                : Type(Module(&targetTypeModule, InternalKey()), targetTypeReference.AsBlobReference(), InternalKey()));

        while (currentType.IsInitialized())
        {
            if (currentType == targetType)
                return true;

            currentType = currentType.GetBaseType();
        }

        return false;
    }

    bool IsDerivedFromSystemType(Type            const& type,
                                 StringReference const& systemTypeNamespace,
                                 StringReference const& systemTypeSimpleName,
                                 bool                   includeSelf)
    {
        Detail::Assert([&]{ return type.IsInitialized(); });

        Type currentType(type);
        if (!includeSelf && currentType)
            currentType = type.GetBaseType();

        while (currentType)
        {
            if (IsSystemType(currentType, systemTypeNamespace, systemTypeSimpleName))
                return true;

            currentType = currentType.GetBaseType();
        }

        return false;
    }

} }
