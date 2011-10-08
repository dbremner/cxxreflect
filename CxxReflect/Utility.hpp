//                 Copyright (c) 2011 James P. McNellis <james@jamesmcnellis.com>                 //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// This header contains private implementation details only.  It defines a number of usefule utility
// classes, templates, and functions that are used throughout the library.
#ifndef CXXREFLECT_UTILITY_HPP_
#define CXXREFLECT_UTILITY_HPP_

#include "CxxReflect/Exceptions.hpp"

#include <iostream>
#include <string>
#include <utility>

namespace CxxReflect { namespace Detail {

    typedef std::wstring String;

    template <typename T>
    struct Identity
    {
        typedef T Type;
    };

    template <typename Target>
    Target ImplicitCast(typename Identity<Target>::Type x)
    {
        return x;
    }

    template <typename Source>
    String ToString(Source const& x)
    {
        std::wostringstream oss;
        oss << x;
        return String(oss.str());
    }

    inline void ThrowOnFailure(long hr)
    {
        if (hr < 0)
        {
            throw HResultException(hr);
        }
    }
    
    // A simple reference-counting base class.  This implementation is not thread-safe by design; we
    // use it as a high-performance alternative to std::shared_ptr, which must use interlocked
    // operations.
    class RefCounted
    {
    protected:

        RefCounted()
            : references_(0)
        {
        }

        virtual ~RefCounted()
        {
        }

    private:

        template <typename T>
        friend class RefPointer;

        void Increment() const
        {
            ++references_;
        }

        void Decrement() const
        {
            --references_;
            if (references_ == 0)
            {
                delete this;
            }
        }

        mutable unsigned references_;
    };

    // Smart pointer that manages reference counting of an object of a type derived from RefCounted.
    template <typename T>
    class RefPointer
    {
    public:

        typedef T* Pointer;
        typedef T& Reference;

        explicit RefPointer(T* p = 0)
            : pointer_(p)
        {
            if (IsValid())
            {
                GetBase()->Increment();
            }
        }

        RefPointer(RefPointer const& other)
            : pointer_(other.pointer_)
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

        Reference operator*()  const { return *pointer_;     }
        Pointer   operator->() const { return pointer_;      }
        Pointer   Get()        const { return pointer_;      }
        bool      IsValid()    const { return pointer_ != 0; }

        void Reset(T* p = 0)
        {
            RefPointer interim(p);
            Swap(interim);
        }

        void Swap(RefPointer& other)
        {
            std::swap(pointer_, other.pointer_);
        }

    private:

        RefCounted const* GetBase();

        Pointer pointer_;
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

    using namespace std::rel_ops;
} }

#endif
