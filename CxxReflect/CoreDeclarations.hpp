//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_COREDECLARATIONS_HPP_
#define CXXREFLECT_COREDECLARATIONS_HPP_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// From <cor.h>
struct IMetaDataDispenserEx;
struct IMetaDataAssemblyImport;
struct IMetaDataImport;
struct IMetaDataImport2;

namespace CxxReflect {

    typedef std::wstring  String;

    // This mirrors the definition of CorTokenType in <cor.h>
    enum class MetadataTokenType : std::uint32_t
    {
        Module                 = 0x00000000,
        TypeRef                = 0x01000000,
        TypeDef                = 0x02000000,
        FieldDef               = 0x04000000,
        MethodDef              = 0x06000000,
        ParamDef               = 0x08000000,
        InterfaceImpl          = 0x09000000,
        MemberRef              = 0x0a000000,
        CustomAttribute        = 0x0c000000,
        Permission             = 0x0e000000,
        Signature              = 0x11000000,
        Event                  = 0x14000000,
        Property               = 0x17000000,
        ModuleRef              = 0x1a000000,
        TypeSpec               = 0x1b000000,
        Assembly               = 0x20000000,
        AssemblyRef            = 0x23000000,
        File                   = 0x26000000,
        ExportedType           = 0x27000000,
        ManifestResource       = 0x28000000,
        GenericParam           = 0x2a000000,
        MethodSpec             = 0x2b000000,
        GenericParamConstraint = 0x2c000000,

        String                 = 0x70000000,
        Name                   = 0x71000000
    };

    class MetadataToken
    {
    public:

        MetadataToken(std::uint32_t value = 0)
            : _value(value)
        {
        }

        MetadataTokenType GetType() const
        {
            return static_cast<MetadataTokenType>(_value & 0xFF000000);
        }

        std::uint32_t Get()                    const { return _value;      }
        bool          IsValid()                const { return _value != 0; }
        void          Set(std::uint32_t value)       { _value = value;     }

    private:

        std::uint32_t _value;
    };

    inline bool operator==(MetadataToken const& lhs, MetadataToken const& rhs) { return lhs.Get() == rhs.Get(); }
    inline bool operator< (MetadataToken const& lhs, MetadataToken const& rhs) { return lhs.Get() <  rhs.Get(); }

    inline bool operator!=(MetadataToken const& lhs, MetadataToken const& rhs) { return !(lhs == rhs); }
    inline bool operator> (MetadataToken const& lhs, MetadataToken const& rhs) { return  (rhs <  lhs); }
    inline bool operator>=(MetadataToken const& lhs, MetadataToken const& rhs) { return !(lhs <  rhs); }
    inline bool operator<=(MetadataToken const& lhs, MetadataToken const& rhs) { return !(rhs <  lhs); }

    class Assembly;
    class AssemblyName;
    class Field;
    class MetadataReader;
    class Method;
    class Property;
    class Type;

    typedef std::vector<Assembly>                AssemblySequence;
    typedef std::vector<AssemblyName>            AssemblyNameSequence;
    typedef std::vector<Type>                    TypeSequence;

    typedef AssemblyNameSequence::iterator AssemblyNameIterator;
    typedef TypeSequence::iterator         TypeIterator;

}

namespace CxxReflect { namespace Detail {

    // When we use a "noncopyable" base class and accidentally use one of the copy operations, 
    // Visual C++ fails to report the location where the invalid use occurs (it points to the
    // derive class, which is not helpful).  Instead, we use a macro.
    #define CXXREFLECT_MAKE_NONCOPYABLE(c) \
        c(c const&);                       \
        c& operator=(c const&) // We intentionally omit the semicolon to force users to add it.

    template <typename T, typename TAllocator = std::allocator<T>>
    class AllocatorBasedArray
    {
    public:

        AllocatorBasedArray()
            : _data(nullptr), _capacity(0), _size(0)
        {
        }

        ~AllocatorBasedArray()
        {
            if (_data == nullptr) { return; }

            for (std::size_t i(_size - 1); i != static_cast<std::size_t>(-1); --i)
            {
                TAllocator().destroy(_data + i);
            }

            TAllocator().deallocate(_data, _capacity);
        }

        T*          Get()         const { return _data;     }
        std::size_t GetCapacity() const { return _capacity; }
        std::size_t GetSize()     const { return _size;     }

        void Allocate(std::size_t capacity)
        {
            if (_data != nullptr)
                throw std::logic_error("The array has already been allocated.");

            _data = TAllocator().allocate(capacity);
            _capacity = capacity;
        }

        void EmplaceBack()
        {
            VerifyAvailable();
            TAllocator().construct(_data + _size);
            ++_size;
        }

        template <typename A0>
        void EmplaceBack(A0&& a0)
        {
            VerifyAvailable();
            TAllocator().construct(_data + _size, std::move(a0));
            ++_size;
        }

        template <typename A0, typename A1>
        void EmplaceBack(A0&& a0, A1&& a1)
        {
            VerifyAvailable();
            TAllocator().construct(_data + _size, std::move(a0), std::move(a1));
            ++_size;
        }

        template <typename A0, typename A1, typename A2>
        void EmplaceBack(A0&& a0, A1&& a1, A2&& a2)
        {
            VerifyAvailable();
            TAllocator().construct(_data + _size, std::move(a0), std::move(a1), std::move(a2));
            ++_size;
        }

    private:

        CXXREFLECT_MAKE_NONCOPYABLE(AllocatorBasedArray);

        void VerifyAvailable() const
        {
            if (_data == nullptr || _capacity - _size == 0)
                throw std::logic_error("There is insufficient space avalilable in the array.");
        }

        T*                 _data;
        std::size_t        _capacity;
        std::size_t        _size;
    };

    template <typename T>
    class FlagSet
    {
    public:

        FlagSet()
            : value_()
        {
        }

        void Set(T x)         { value_ = static_cast<T>(value_ | x); }
        void Unset(T x)       { value_ = static_cast<T>(value_ ^ x); }
        bool IsSet(T x) const { return (value_ & x) != 0;            }

    private:

        T value_;
    };

} }

#endif
