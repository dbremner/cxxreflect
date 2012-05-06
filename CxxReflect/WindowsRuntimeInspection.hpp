#ifndef CXXREFLECT_WINDOWSRUNTIMEINSPECTION_HPP_
#define CXXREFLECT_WINDOWSRUNTIMEINSPECTION_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header defines the public interface for type inspection using the Windows Runtime.

#include "CxxReflect/WindowsRuntimeCommon.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

namespace CxxReflect { namespace WindowsRuntime {

    // INTERFACE IMPLEMENTATION QUERIES
    //
    // These functions allow you to get the list of types in the package that implement a given
    // interface.  They grovel the entire set of loaded metadata files for implementers.  If no
    // types implement the interface, an empty sequence is returned.  If the interface cannot
    // be found, a RuntimeError is thrown.

    std::vector<Type> GetImplementers(Type const& interfaceType);
    std::vector<Type> GetImplementers(StringReference interfaceFullName);
    std::vector<Type> GetImplementers(StringReference namespaceName, StringReference interfaceSimpleName);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template<typename TInterface>
    std::vector<Type> GetImplementersOf()
    {
        String const interfaceFullName(TInterface::typeid->FullName->Data());
        return WindowsRuntime::GetImplementers(StringReference(interfaceFullName.c_str()));
    }
    #endif

    // GET TYPE
    //
    // These functions allow you to get a type from the package given its name or given an inspectable
    // object.
    //
    // TODO We do not handle non-activatable runtime types very well, or types that do not have public
    // metadata.  We need to consider how to distinguish between these and how we can give as much
    // functionality as possible.

    /// Gets a Type by name.
    ///
    /// \param typeFullName The namespace-qualified name of the type.
    ///
    /// \todo Finish documentation
    Type GetType(StringReference typeFullName);
    Type GetType(StringReference namespaceName, StringReference typeSimpleName);

    /// Gets the `Type` of a runtime object.
    ///
    /// A runtime object must provide the name of its type via `IInspectable::GetRuntimeClassName`.
    /// This function calls that member function to compute the name of the runtime object's type,
    /// then looks up the named type in the type system and returns a `Type` representing it.
    ///
    /// \param object The object whose type to get.
    ///
    /// \throws LogicError If `object` is a `nullptr`.
    ///
    /// \returns A `Type` representing the type of `object`.  If `object`'s type cannot be determined
    /// an uninitialized `Type` is returned.
    Type GetTypeOf(IInspectable* object);
    
    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    /// Gets the `Type` of a runtime object.
    ///
    /// This function is functionally equivalent to `GetTypeOf(IInspectable*)`.  It is provided for
    /// convenience so that when using C++/CX one needs not `reinterpret_cast` a `T^` to its
    /// `IInspectable*` interface before calling.
    ///
    /// \copydoc GetTypeOf(IInspectable*)
    template <typename T>
    Type GetTypeOf(T^ object)
    {
        return CxxReflect::WindowsRuntime::GetTypeOf(reinterpret_cast<IInspectable*>(object));
    }
    #endif





    // TYPE PROPERTIES
    //
    // These are Windows Runtime-specific properties, or properties that are implemented differently
    // in Windows Runtime from in the basic CLI metadata.

    /// Tests whether a `Type` is default constructible.
    ///
    /// A default constructible type is a type that has a constructor with no parameters.  If this
    /// function returns `true`, the Type may be instantiated by calling `CreateInstance()` with no
    /// constructor arguments.
    ///
    /// \param type The `Type` to test.
    ///
    /// \throws LogicError If `type` is not initialized.
    /// 
    /// \returns `true` if the `Type` is default constructible; `false` otherwise.
    ///
    /// \todo Check whether Windows Runtime allows default construction using a constructor that has
    /// default arguments, e.g. T(int x = 0).  If this is allowed, we should add support for them,
    /// both here and in the `CreateInstance()` functions.
    bool IsDefaultConstructible(Type const& type);

    /// Gets the GUID of a `Type`.
    ///
    /// A `Type`'s GUID is defined using the `Windows::Foundation::Metadata::GuidAttribute` custom
    /// attribute.  A `Type` may have only one `GuidAttribute`.
    ///
    /// \param type The `Type` to test.
    ///
    /// \throws LogicError If `type` is not initialized.
    /// 
    /// \returns The GUID of the `Type`.  If the `Type` has no GUID or has more than one specified
    /// GUID, or if GUID cannot be determined, this function returns the zero GUID (a GUID with all
    /// zeroes).
    Guid GetGuid(Type const& type);





    std::vector<Enumerator> GetEnumerators(Type const& enumerationType);
    std::vector<Enumerator> GetEnumerators(StringReference enumerationFullName);
    std::vector<Enumerator> GetEnumerators(StringReference namespaceName, StringReference enumerationSimpleName);

    #ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_CPPCX
    template<typename TEnumeration>
    std::vector<Enumerator> GetEnumeratorsOf()
    {
        String const enumerationFullName(TInterface::typeid->FullName->Data());
        return WindowsRuntime::GetEnumerators(StringReference(enumerationFullName.c_str()));
    }
    #endif

} }

#endif // CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION
#endif
