//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Primary implementation file

#include "CxxReflect/CxxReflect.hpp"

#include "CxxReflect/InternalBlobMetadata.hpp"
#include "CxxReflect/InternalCorEnumIterator.hpp"
#include "CxxReflect/InternalCxxReflectImpl.hpp"
#include "CxxReflect/InternalUtility.hpp"

#include <atlbase.h>
#include <cor.h>

#include <array>
#include <exception>
#include <functional>
#include <iostream>
#include <map>
#include <type_traits>

namespace CxxReflect {

    MetadataReader::MetadataReader(std::unique_ptr<IReferenceResolver> referenceResolver)
        : impl_(new Detail::MetadataReaderImpl(std::move(referenceResolver)))
    {
    }

    Assembly MetadataReader::GetAssembly(Detail::String const& path)
    {
        return impl_->GetAssembly(path);
    }

    Assembly MetadataReader::GetAssemblyByName(AssemblyName const& name)
    {
        return impl_->GetAssemblyByName(name);
    }

    IMetaDataDispenserEx* MetadataReader::UnsafeGetDispenser() const
    {
        return impl_->UnsafeGetDispenser();
    }

    Assembly::Assembly(Detail::AssemblyImpl const* impl)
        : impl_(impl)
    {
    }

    Detail::String Assembly::GetFullName() const
    {
        return impl_->GetName().GetFullName();
    }

    AssemblySequence Assembly::GetReferencedAssemblies() const
    {
        return impl_->GetReferencedAssemblies();
    }

    TypeSequence Assembly::GetTypes() const
    {
        return impl_->GetTypes();
    }

    Type Assembly::GetType(Detail::String const& name, bool throwOnError, bool ignoreCase) const
    {
        return impl_->GetType(name, throwOnError, ignoreCase);
    }

    IMetaDataImport2* Assembly::UnsafeGetImport() const
    {
        return impl_->UnsafeGetImport();
    }

    Type::Type(Detail::TypeImpl const* impl)
        : impl_(impl)
    {
    }

    Detail::String Type::GetAssemblyQualifiedName() const { return impl_->GetAssemblyQualifiedName(); }

    Detail::String Type::GetFullName() const
    {
        return impl_->GetFullName();
    }

    unsigned long Type::GetMetadataToken() const
    {
        return impl_->GetMetadataToken().Get();
    }

    Detail::String Type::GetName()      const { return impl_->GetName();      }
    Detail::String Type::GetNamespace() const { return impl_->GetNamespace(); }

    bool Type::IsAbstract()              const { return impl_->IsAbstract();              }
    bool Type::IsArray()                 const { return impl_->IsArray();                 }
    bool Type::IsAutoClass()             const { return impl_->IsAutoClass();             }
    bool Type::IsAutoLayout()            const { return impl_->IsAutoLayout();            }
    bool Type::IsByRef()                 const { return impl_->IsByRef();                 }
    bool Type::IsClass()                 const { return impl_->IsClass();                 }
    bool Type::IsCOMObject()             const { return impl_->IsCOMObject();             }
    bool Type::IsContextful()            const { return impl_->IsContextful();            }
    bool Type::IsEnum()                  const { return impl_->IsEnum();                  }
    bool Type::IsExplicitLayout()        const { return impl_->IsExplicitLayout();        }
    bool Type::IsGenericParameter()      const { return impl_->IsGenericParameter();      }
    bool Type::IsGenericType()           const { return impl_->IsGenericType();           }
    bool Type::IsGenericTypeDefinition() const { return impl_->IsGenericTypeDefinition(); }
    bool Type::IsImport()                const { return impl_->IsImport();                }
    bool Type::IsInterface()             const { return impl_->IsInterface();             }
    bool Type::IsLayoutSequential()      const { return impl_->IsLayoutSequential();      }
    bool Type::IsMarshalByRef()          const { return impl_->IsMarshalByRef();          }
    bool Type::IsNested()                const { return impl_->IsNested();                }
    bool Type::IsNestedAssembly()        const { return impl_->IsNestedAssembly();        }
    bool Type::IsNestedFamANDAssem()     const { return impl_->IsNestedFamANDAssem();     }
    bool Type::IsNestedFamily()          const { return impl_->IsNestedFamily();          }
    bool Type::IsNestedPrivate()         const { return impl_->IsNestedPrivate();         }
    bool Type::IsNestedPublic()          const { return impl_->IsNestedPublic();          }
    bool Type::IsNotPublic()             const { return impl_->IsNotPublic();             }
    bool Type::IsPointer()               const { return impl_->IsPointer();               }
    bool Type::IsPrimitive()             const { return impl_->IsPrimitive();             }
    bool Type::IsPublic()                const { return impl_->IsPublic();                }
    bool Type::IsSealed()                const { return impl_->IsSealed();                }
    bool Type::IsSecurityCritical()      const { return impl_->IsSecurityCritical();      }
    bool Type::IsSecuritySafeCritical()  const { return impl_->IsSecuritySafeCritical();  }
    bool Type::IsSecurityTransparent()   const { return impl_->IsSecurityTransparent();   }
    bool Type::IsSerializable()          const { return impl_->IsSerializable();          }
    bool Type::IsSpecialName()           const { return impl_->IsSpecialName();           }
    bool Type::IsUnicodeClass()          const { return impl_->IsUnicodeClass();          }
    bool Type::IsValueType()             const { return impl_->IsValueType();             }
    bool Type::IsVisible()               const { return impl_->IsVisible();               }

    bool Type::HasBaseType() const { return impl_->GetBaseType() != nullptr; }
    Type Type::GetBaseType() const { return impl_->GetBaseType();            }
}
