//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header contains private implementation details only.  It defines a policy-based iterator
// template that wraps the HCORENUM provided by various unmanaged metadata interface functions.
#ifndef CXXREFLECT_CORENUMITERATOR_HPP_
#define CXXREFLECT_CORENUMITERATOR_HPP_

#include "Utility.hpp"

#include <cor.h>

namespace CxxReflect { namespace Detail {

    template <typename TIteratorPolicy>
    class CorEnumIterator
    {
    public:

        typedef std::input_iterator_tag                    iterator_category;
        typedef typename TIteratorPolicy::ValueType        value_type;
        typedef std::ptrdiff_t                             difference_type;
        typedef void                                       pointer;
        typedef value_type                                 reference;

        typedef typename TIteratorPolicy::InterfaceType    InterfaceType;
        typedef typename TIteratorPolicy::ArgumentType     ArgumentType;

        CorEnumIterator()
            : context_()
        {
        }

        explicit CorEnumIterator(InterfaceType* import, ArgumentType argument = ArgumentType())
            : context_(new Context(import, argument))
        {
        }

        mdTypeDef operator*() const
        {
            return context_->Current();
        }

        CorEnumIterator& operator++()
        {
            context_->Advance();
            return *this;
        }

        CorEnumIterator& operator++(int)
        {
            context_->Advance();
            return *this;
        }

        friend bool operator==(CorEnumIterator const& lhs, CorEnumIterator const& rhs)
        {
            if (!lhs.context_.IsValid() && !rhs.context_.IsValid())
                return true;

            if (!lhs.context_.IsValid() && rhs.context_->IsEnd())
                return true;

            if (lhs.context_->IsEnd() && !rhs.context_.IsValid())
                return true;

            if (lhs.context_.IsValid() && rhs.context_.IsValid() && *lhs.context_ == *rhs.context_)
                return true;

            return false;
        }

        friend bool operator!=(CorEnumIterator const& lhs, CorEnumIterator const& rhs)
        {
            return !(lhs == rhs);
        }

    private:

        class Context : public RefCounted
        {
        public:

            Context(InterfaceType* import, ArgumentType argument)
                : import_(import), e_(), buffer_(), current_(), end_(), argument_(argument)
            {
                Advance();
            }

            ~Context()
            {
                if (e_)
                {
                    import_->CloseEnum(e_);
                }
            }

            mdTypeDef Current() const
            {
                return buffer_[current_];
            }

            bool IsEnd() const
            {
                return end_;
            }

            void Advance()
            {
                if (e_ && current_ != buffer_.size() - 1)
                {
                    ++current_;
                    end_ = Current() == 0;
                }
                else
                {
                    unsigned count(TIteratorPolicy::Advance(import_, e_, buffer_, argument_));
                    end_ = (count == 0);
                    current_ = 0;
                }
            }

            friend bool operator==(Context const& lhs, Context const& rhs)
            {
                return lhs.import_ == rhs.import_ && lhs.Current() == rhs.Current();
            }

            friend bool operator!=(Context const& lhs, Context const& rhs)
            {
                return !(lhs == rhs);
            }

        private:

            typedef typename TIteratorPolicy::BufferType BufferType;

            InterfaceType*    import_;
            HCORENUM          e_;
            BufferType        buffer_;
            unsigned          current_;
            bool              end_;
            ArgumentType      argument_;
        };

        RefPointer<Context> context_;
    };

    template <
        typename TValueType,
        typename TInterface,
        HRESULT (__stdcall TInterface::*Function)(HCORENUM*, TValueType*, ULONG, ULONG*)
    >
    struct NoArgHCorEnumIteratorPolicy
    {
        typedef TInterface                    InterfaceType;
        typedef TValueType                    ValueType;
        typedef std::array<ValueType, 128>    BufferType;
        typedef bool                          ArgumentType;

        static unsigned Advance(InterfaceType* import,
                                HCORENUM& e,
                                BufferType& buffer,
                                ArgumentType)
        {
            ULONG count;
            ThrowOnFailure((import->*Function)(&e, buffer.data(), buffer.size(), &count));
            return count;
        }
    };

    template <
        typename TValueType,
        typename TInterface,
        typename TArgument,
        HRESULT (__stdcall TInterface::*Function)(HCORENUM*, TArgument, TValueType*, ULONG, ULONG*)
    >
    struct OneArgCorEnumIteratorPolicy
    {
        typedef TInterface                    InterfaceType;
        typedef TValueType                    ValueType;
        typedef std::array<ValueType, 128>    BufferType;
        typedef TArgument                     ArgumentType;

        static unsigned Advance(InterfaceType* import,
                                HCORENUM& e,
                                BufferType& buffer,
                                ArgumentType argument)
        {
            ULONG count;
            ThrowOnFailure((import->*Function)(&e, argument, buffer.data(), buffer.size(), &count));
            return count;
        }
    };

    typedef NoArgHCorEnumIteratorPolicy<
        mdAssemblyRef, IMetaDataAssemblyImport, &IMetaDataAssemblyImport::EnumAssemblyRefs
    > AssemblyRefIteratorPolicy;

    typedef CorEnumIterator<AssemblyRefIteratorPolicy> AssemblyRefIterator;

    typedef NoArgHCorEnumIteratorPolicy<
        mdExportedType, IMetaDataAssemblyImport, &IMetaDataAssemblyImport::EnumExportedTypes
    > ExportedTypeIteratorPolicy;

    typedef CorEnumIterator<ExportedTypeIteratorPolicy> ExportedTypeIterator;

    typedef NoArgHCorEnumIteratorPolicy<
        mdFile, IMetaDataAssemblyImport, &IMetaDataAssemblyImport::EnumFiles
    > FileIteratorPolicy;

    typedef CorEnumIterator<FileIteratorPolicy> FileIterator;

    typedef NoArgHCorEnumIteratorPolicy<
        mdManifestResource, IMetaDataAssemblyImport, &IMetaDataAssemblyImport::EnumManifestResources
    > ManifestResourceIteratorPolicy;

    typedef CorEnumIterator<ManifestResourceIteratorPolicy> ManifestResourceIterator;

    class CustomAttributeIteratorArgument
    {
    public:

        CustomAttributeIteratorArgument(mdToken scope, mdToken type)
            : scope_(scope), type_(type)
        {
        }

        mdToken GetScope() const { return scope_; }
        mdToken GetType()  const { return type_;  }

    private:

        mdToken scope_;
        mdToken type_;
    };

    struct CustomAttributeIteratorPolicy
    {
        typedef IMetaDataImport                    InterfaceType;
        typedef mdCustomAttribute                  ValueType;
        typedef std::array<ValueType, 128>         BufferType;
        typedef CustomAttributeIteratorArgument    ArgumentType;

        static unsigned Advance(InterfaceType* import,
                                HCORENUM& e,
                                BufferType& buffer,
                                ArgumentType const& argument)
        {
            ULONG count;
            ThrowOnFailure(import->EnumCustomAttributes(
                &e,
                argument.GetScope(),
                argument.GetType(),
                buffer.data(),
                buffer.size(),
                &count));
            return count;
        }
    };

    typedef CorEnumIterator<CustomAttributeIteratorPolicy> CustomAttributeIterator;

    typedef OneArgCorEnumIteratorPolicy<
        mdEvent, IMetaDataImport, mdTypeDef, &IMetaDataImport::EnumEvents
    > EventIteratorPolicy;

    typedef CorEnumIterator<EventIteratorPolicy> EventIterator;

    typedef OneArgCorEnumIteratorPolicy<
        mdFieldDef, IMetaDataImport, mdTypeDef, &IMetaDataImport::EnumFields
    > FieldIteratorPolicy;

    typedef CorEnumIterator<FieldIteratorPolicy> FieldIterator;

    typedef OneArgCorEnumIteratorPolicy<
        mdMethodDef, IMetaDataImport, mdTypeDef, &IMetaDataImport::EnumMethods
    > MethodIteratorPolicy;

    typedef CorEnumIterator<MethodIteratorPolicy> MethodIterator;

    typedef NoArgHCorEnumIteratorPolicy<
        mdModuleRef, IMetaDataImport, &IMetaDataImport::EnumModuleRefs
    > ModuleRefIteratorPolicy;

    typedef CorEnumIterator<ModuleRefIteratorPolicy> ModuleRefIterator;

    typedef OneArgCorEnumIteratorPolicy<
        mdParamDef, IMetaDataImport, mdMethodDef, &IMetaDataImport::EnumParams
    > ParameterIteratorPolicy;

    typedef CorEnumIterator<ParameterIteratorPolicy> ParameterIterator;

    struct PermissionSetIteratorPolicy
    {
        // TODO
    };

    typedef CorEnumIterator<PermissionSetIteratorPolicy> PermissionSetIterator;

    typedef OneArgCorEnumIteratorPolicy<
        mdProperty, IMetaDataImport, mdTypeDef, &IMetaDataImport::EnumProperties
    > PropertyIteratorPolicy;

    typedef CorEnumIterator<PropertyIteratorPolicy> PropertyIterator;

    typedef NoArgHCorEnumIteratorPolicy<
        mdSignature, IMetaDataImport, &IMetaDataImport::EnumSignatures
    > SignatureIteratorPolicy;

    typedef CorEnumIterator<SignatureIteratorPolicy> SignatureIterator;

    typedef NoArgHCorEnumIteratorPolicy<
        mdTypeDef, IMetaDataImport, &IMetaDataImport::EnumTypeDefs
    > TypeDefIteratorPolicy;

    typedef CorEnumIterator<TypeDefIteratorPolicy> TypeDefIterator;

    typedef NoArgHCorEnumIteratorPolicy<
        mdTypeRef, IMetaDataImport, &IMetaDataImport::EnumTypeRefs
    > TypeRefIteratorPolicy;

    typedef CorEnumIterator<TypeRefIteratorPolicy> TypeRefIterator;

    typedef NoArgHCorEnumIteratorPolicy<
        mdTypeSpec, IMetaDataImport, &IMetaDataImport::EnumTypeSpecs
    > TypeSpecIteratorPolicy;

    typedef CorEnumIterator<TypeSpecIteratorPolicy> TypeSpecIterator;


} }

#endif
