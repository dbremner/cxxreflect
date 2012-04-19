#ifndef CXXREFLECT_WINDOWSRUNTIMEUTILITY_HPP_
#define CXXREFLECT_WINDOWSRUNTIMEUTILITY_HPP_

//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// Utilities used by our Windows Runtime library components.  DO NOT INCLUDE THIS HEADER IN ANY
// PUBLIC INTERFACE HEADERS.  It includes lots of platform headers that we don't necessarily want to
// push upon our users.

#include "CxxReflect/WindowsRuntimeCommon.hpp"

#include <guiddef.h>
#include <hstring.h>
#include <winstring.h>

namespace CxxReflect { namespace WindowsRuntime { namespace Internal {

    // A smart, std::string-like wrapper around HSTRING for use in Windows Runtime interop code.
    // Most of the const std::wstring interface is provided; for mutability, convert to String
    // (std::wstring) and back.
    class SmartHString
    {
    public:

        typedef wchar_t           value_type;
        typedef std::size_t       size_type;
        typedef std::ptrdiff_t    difference_type;

        typedef value_type const& reference;
        typedef value_type const& const_reference;
        typedef value_type const* pointer;
        typedef value_type const* const_pointer;

        typedef pointer                               iterator;
        typedef const_pointer                         const_iterator;
        typedef std::reverse_iterator<iterator>       reverse_iterator;
        typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

        SmartHString();

        SmartHString(const_pointer          s);
        SmartHString(StringReference        s);
        SmartHString(String          const& s);

        SmartHString(SmartHString const&  other);
        SmartHString(SmartHString      && other);

        SmartHString& operator=(SmartHString   other);
        SmartHString& operator=(SmartHString&& other);

        ~SmartHString();

        void swap(SmartHString& other);

        const_iterator begin()  const;
        const_iterator end()    const;
        const_iterator cbegin() const;
        const_iterator cend()   const;

        const_reverse_iterator rbegin()  const;
        const_reverse_iterator rend()    const;
        const_reverse_iterator crbegin() const;
        const_reverse_iterator crend()   const;

        size_type size()     const;
        size_type length()   const;
        size_type max_size() const;
        size_type capacity() const;
        bool      empty()    const;

        const_reference operator[](size_type n) const;
        const_reference at        (size_type n) const;

        const_reference front() const;
        const_reference back()  const;

        const_pointer c_str() const;
        const_pointer data()  const;

        // A reference proxy, returned by proxy(), that can be passed into a function expecting an
        // HSTRING*.  When the reference proxy is destroyed, it sets the value of the SmartHString
        // from which it was created.
        class ReferenceProxy
        {
        public:

            ReferenceProxy(SmartHString*);

            ~ReferenceProxy();

            operator HSTRING*();

        private:

            // Note that this type is copyable though it is not intended to be copied, aside from
            // when it is returned from SmartHString::proxy().
            ReferenceProxy& operator=(ReferenceProxy const&);

            Detail::ValueInitialized<HSTRING>       _proxy;
            Detail::ValueInitialized<SmartHString*> _value;
        };

        ReferenceProxy proxy();
        HSTRING        value() const;

        friend bool operator==(SmartHString const&, SmartHString const&);
        friend bool operator< (SmartHString const&, SmartHString const&);

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(SmartHString)

    private:

        // These yield pointers that are equivalent to begin() and end(), which return iterators.
        const_pointer get_buffer_begin() const;
        const_pointer get_buffer_end()   const;

        static int compare(SmartHString const& lhs, SmartHString const& rhs);

        Detail::ValueInitialized<HSTRING> _value;
    };





    // Converts an HSTRING into a String (std::wstring).
    String ToString(HSTRING hstring);





    // An RAII wrapper for an array of HSTRINGs, useful e.g. when calling RoResolveNamespace().
    class RaiiHStringArray
    {
    public:

        RaiiHStringArray();
        ~RaiiHStringArray();

        DWORD&    GetCount();
        HSTRING*& GetArray();

        HSTRING* begin() const;
        HSTRING* end()   const;

    private:

        RaiiHStringArray(RaiiHStringArray const&);
        RaiiHStringArray& operator=(RaiiHStringArray const&);

        Detail::ValueInitialized<DWORD>    _count;
        Detail::ValueInitialized<HSTRING*> _array;
    };





    // Gets the root directory of the app package from which the current executable is executing.
    // This should not fail if called from within an app package.  If it does fail, it will return
    // an empty string.  The returned path will include a trailing backslash.
    String GetCurrentPackageRoot();





    // These functions can be used to enumerate the metadata files resolvable in the current app
    // package.  They will not work correctly if we are not executing in an app package.  The
    // 'packageRoot' parameter is optional; if provided, it should point to the root of the app
    // package (i.e., where the manifest and native DLL files are located).
    std::vector<String> EnumeratePackageMetadataFiles(StringReference packageRoot);
    void EnumeratePackageMetadataFilesRecursive(SmartHString rootNamespace, std::vector<String>& result);





    // Utility functions for converting between our 'Guid' type and the COM 'GUID' type.
    GUID ToComGuid(Guid const& cxxGuid);
    Guid ToCxxGuid(GUID const& comGuid);





    // Removes the rightmost component of a type name.  So, 'A.B.C' becomes 'A.B' and 'A' becomes an
    // empty string.  If the input is an empty string, the function returns without modifying it.
    void RemoveRightmostTypeNameComponent(String& typeName);
    




    UniqueInspectable GetActivationFactoryInterface(String const& typeFullName, Guid const& interfaceGuid);
    UniqueInspectable QueryInterface(IInspectable* instance, Type const& interfaceType);

} } }

#endif
