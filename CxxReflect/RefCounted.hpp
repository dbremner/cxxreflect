//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFCOUNTED_HPP_
#define CXXREFLECT_REFCOUNTED_HPP_

#include "CxxReflect/CoreDeclarations.hpp"

namespace CxxReflect { namespace Detail {

    // A simple, non-thread-safe, intrusive reference-counted base class.
    class RefCounted
    {
    protected:

        RefCounted()
            : _references(0)
        {
        }

        virtual ~RefCounted()
        {
        }

    private:

        CXXREFLECT_NONCOPYABLE(RefCounted);

        template <typename T>
        friend class RefPointer;

        void Increment() const
        {
            ++_references;
        }

        void Decrement() const
        {
            --_references;
            if (_references == 0)
            {
                delete this;
            }
        }

        mutable unsigned _references;
    };

    // Smart pointer that manages reference counting of an object of a type derived from RefCounted.
    template <typename T>
    class RefPointer
    {
    public:

        // Lowercase versions for easier interop with C++ Standard Library
        typedef T  value_type;
        typedef T* pointer;
        typedef T& reference;

        typedef T  ValueType;
        typedef T* Pointer;
        typedef T& Reference;

        explicit RefPointer(T* p = 0)
            : _pointer(p)
        {
            if (IsValid())
            {
                GetBase()->Increment();
            }
        }

        RefPointer(RefPointer const& other)
            : _pointer(other._pointer)
        {
            if (IsValid())
            {
                GetBase()->Increment();
            }
        }

        RefPointer& operator=(RefPointer other)
        {
            Swap(other);
            return *this;
        }

        ~RefPointer()
        {
            if (IsValid())
            {
                GetBase()->Decrement();
            }
        }

        Reference operator*()  const { return *_pointer;     }
        Pointer   operator->() const { return _pointer;      }
        Pointer   Get()        const { return _pointer;      }
        bool      IsValid()    const { return _pointer != 0; }

        void Reset(T* p = 0)
        {
            RefPointer interim(p);
            Swap(interim);
        }

        void Swap(RefPointer& other)
        {
            std::swap(_pointer, other._pointer);
        }

    private:

        RefCounted const* GetBase()
        {
            return _pointer;
        }

        Pointer _pointer;
    };

    template <typename T>
    inline bool operator==(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return lhs.Get() == rhs.Get();
    }

    template <typename T>
    inline bool operator<(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return std::less<T*>()(lhs.Get(), rhs.Get());
    }

    template <typename T>
    inline bool operator!=(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return !(lhs == rhs);
    }

    template <typename T>
    inline bool operator> (RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return (rhs < lhs);
    }

    template <typename T>
    inline bool operator>=(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return !(lhs < rhs);
    }

    template <typename T>
    inline bool operator<=(RefPointer<T> const& lhs, RefPointer<T> const& rhs)
    {
        return !(rhs < lhs);
    }

} }

#endif
