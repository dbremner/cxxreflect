//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/Assembly.hpp"
#include "CxxReflect/CustomAttribute.hpp"
#include "CxxReflect/Method.hpp"
#include "CxxReflect/Type.hpp"

namespace CxxReflect { namespace { namespace Private {

    typedef std::pair<Metadata::RowReference, Metadata::RowReference> RowReferencePair;

    SizeType TableIdToCustomAttributeFlag(Metadata::TableId tableId)
    {
        switch (tableId)
        {
        case Metadata::TableId::MethodDef:               return  0;
        case Metadata::TableId::Field:                   return  1;
        case Metadata::TableId::TypeRef:                 return  2;
        case Metadata::TableId::TypeDef:                 return  3;
        case Metadata::TableId::Param:                   return  4;
        case Metadata::TableId::InterfaceImpl:           return  5;
        case Metadata::TableId::MemberRef:               return  6;
        case Metadata::TableId::Module:                  return  7;
        // TODO WTF is the 'Permission' table? 
        // case Metadata::TableId::Permission:           return  8;
        case Metadata::TableId::Property:                return  9;
        case Metadata::TableId::Event:                   return 10;
        case Metadata::TableId::StandaloneSig:           return 11;
        case Metadata::TableId::ModuleRef:               return 12;
        case Metadata::TableId::TypeSpec:                return 13;
        case Metadata::TableId::Assembly:                return 14;
        case Metadata::TableId::AssemblyRef:             return 15;
        case Metadata::TableId::File:                    return 16;
        case Metadata::TableId::ExportedType:            return 17;
        case Metadata::TableId::ManifestResource:        return 18;
        case Metadata::TableId::GenericParam:            return 19;
        case Metadata::TableId::GenericParamConstraint:  return 20;
        case Metadata::TableId::MethodSpec:              return 21;
        default: Detail::VerifyFail("Invalid table id"); return  0;
        }
    }

    struct CustomAttributeStrictWeakOrdering
    {
        typedef Metadata::CustomAttributeRow CustomAttributeRow;
        typedef Metadata::RowReference       RowReference;

        bool operator()(CustomAttributeRow const& lhs, CustomAttributeRow const& rhs) const volatile
        {
            return (*this)(lhs.GetParent(), rhs.GetParent());
        }

        bool operator()(CustomAttributeRow const& lhs, RowReference const& rhs) const volatile
        {
            return (*this)(lhs.GetParent(), rhs);
        }

        bool operator()(RowReference const& lhs, CustomAttributeRow const& rhs) const volatile
        {
            return (*this)(lhs, rhs.GetParent());
        }

        bool operator()(RowReference const& lhs, RowReference const& rhs) const volatile
        {
            if (lhs.GetIndex() < rhs.GetIndex())
                return true;

            if (lhs.GetIndex() > rhs.GetIndex())
                return false;

            if (TableIdToCustomAttributeFlag(lhs.GetTable()) < TableIdToCustomAttributeFlag(rhs.GetTable()))
                return true;

            return false;
        }
    };

    RowReferencePair GetCustomAttributesRange(Metadata::Database const& database, Metadata::RowReference const& parent)
    {
        auto const range(std::equal_range(database.Begin<Metadata::TableId::CustomAttribute>(),
                                          database.End<Metadata::TableId::CustomAttribute>(),
                                          parent,
                                          CustomAttributeStrictWeakOrdering()));;

        return std::make_pair(range.first.GetReference(), range.second.GetReference());
    }

} } }

namespace CxxReflect {

    CustomAttribute::CustomAttribute()
    {
    }

    CustomAttribute::CustomAttribute(Assembly               const& assembly,
                                     Metadata::RowReference const& customAttribute,
                                     InternalKey)
    {
        Detail::Verify([&]{ return assembly.IsInitialized();        });
        Detail::Verify([&]{ return customAttribute.IsInitialized(); });

        Metadata::Database const& parentDatabase(assembly.GetContext(InternalKey()).GetDatabase());
        Metadata::CustomAttributeRow const customAttributeRow(parentDatabase
            .GetRow<Metadata::TableId::CustomAttribute>(customAttribute));

        _parent = Metadata::FullReference(&parentDatabase, customAttributeRow.GetParent());
        _attribute = customAttribute;

        if (customAttributeRow.GetType().GetTable() == Metadata::TableId::MethodDef)
        {
            // TODO Do we really need to do a linear search to map from method => type? This is
            // embarrasing and horrible and it would be great if we could optimize this somehow.

            Metadata::RowReference const methodDefReference(customAttributeRow.GetType());

            // Find the owning TypeDef:
            auto const typeDefIt(std::find_if(parentDatabase.Begin<Metadata::TableId::TypeDef>(),
                                              parentDatabase.End<Metadata::TableId::TypeDef>(),
                                              [&](Metadata::TypeDefRow const& typeDef) -> bool
            {
                return typeDef.GetFirstMethod().GetIndex() <= methodDefReference.GetIndex()
                    && typeDef.GetLastMethod().GetIndex()  >  methodDefReference.GetIndex();
            }));

            Detail::Verify([&]{ return typeDefIt != parentDatabase.End<Metadata::TableId::TypeDef>(); });

            Type const type(assembly, typeDefIt->GetSelfReference(), InternalKey());

            BindingFlags const flags(
                BindingAttribute::Public    |
                BindingAttribute::NonPublic |
                BindingAttribute::Instance);

            auto const constructorIt(std::find_if(type.BeginConstructors(flags),
                                                  type.EndConstructors(),
                                                  [&](Method const& constructor) -> bool
            {
                return constructor.GetMetadataToken() == methodDefReference.GetToken();
            }));

            Detail::Verify([&]{ return constructorIt != type.EndConstructors(); });

            _constructor = Detail::MethodHandle(*constructorIt);
        }
        else if (customAttributeRow.GetType().GetTable() == Metadata::TableId::MemberRef)
        {
            // TODO
            Detail::VerifyFail("NYI");
        }
        else
        {
            // We should never get here; the Database layer should throw if the type is invalid.
            Detail::VerifyFail("Invalid custom attribute type");
        }
    }

    SizeType CustomAttribute::GetMetadataToken() const
    {
        return _attribute.GetToken();
    }

    CustomAttributeIterator CustomAttribute::BeginFor(Assembly               const& assembly,
                                                      Metadata::RowReference const& parent,
                                                      InternalKey)
    {
        return CustomAttributeIterator(
            assembly,
            Private::GetCustomAttributesRange(assembly.GetContext(InternalKey()).GetDatabase(), parent).first);
    }

    CustomAttributeIterator CustomAttribute::EndFor(Assembly               const& assembly,
                                                    Metadata::RowReference const& parent,
                                                    InternalKey)
    {
        return CustomAttributeIterator(
            assembly,
            Private::GetCustomAttributesRange(assembly.GetContext(InternalKey()).GetDatabase(), parent).second);
    }

    bool CustomAttribute::IsInitialized() const
    {
        return _parent.IsInitialized() && _constructor.IsInitialized();
    }

    void CustomAttribute::VerifyInitialized() const
    {
        Detail::Verify([&]{ return IsInitialized(); });
    }

    Method CustomAttribute::GetConstructor() const
    {
        VerifyInitialized();

        return _constructor.Realize();
    }

}
