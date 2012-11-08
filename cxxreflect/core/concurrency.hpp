
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_CONCURRENCY_HPP_
#define CXXREFLECT_CORE_CONCURRENCY_HPP_

#include "cxxreflect/core/utility.hpp"

namespace cxxreflect { namespace core {

    #if CXXREFLECT_COMPILER == CXXREFLECT_COMPILER_VISUALCPP

    CXXREFLECT_STATIC_ASSERT(sizeof(long) == 4);
    CXXREFLECT_STATIC_ASSERT(sizeof(long long) == 8);

    template <typename T>
    struct is_supported_atomic_type
    {
        enum { value = std::is_pointer<T>::value || std::is_integral<T>::value || std::is_enum<T>::value };
    };

    template <typename T>
    auto atomic_load(T volatile& value)
        -> typename std::enable_if<sizeof(T) == 4, T>::type
    {
        CXXREFLECT_STATIC_ASSERT(is_supported_atomic_type<T>::value);

        std::int32_t const result(_InterlockedOr(reinterpret_cast<long volatile*>(&value), 0));
        return *reinterpret_cast<T const*>(&result);
    }

    template <typename T>
    auto atomic_load(T volatile& value)
        -> typename std::enable_if<sizeof(T) == 8, T>::type
    {
        CXXREFLECT_STATIC_ASSERT(is_supported_atomic_type<T>::value);

        std::int64_t const result(_InterlockedOr64(reinterpret_cast<long long volatile*>(&value), 0));
        return *reinterpret_cast<T const*>(&result);
    }

    template <typename T>
    auto atomic_store(T volatile& value, T new_value)
        -> typename std::enable_if<sizeof(T) == 4>::type
    {
        CXXREFLECT_STATIC_ASSERT(is_supported_atomic_type<T>::value);

        _InterlockedExchange(reinterpret_cast<long volatile*>(&value), *reinterpret_cast<long*>(&new_value));
    }

    template <typename T>
    auto atomic_store(T volatile& value, T new_value)
        -> typename std::enable_if<sizeof(T) == 8>::type
    {
        CXXREFLECT_STATIC_ASSERT(is_supported_atomic_type<T>::value);

        _InterlockedExchange64(reinterpret_cast<long long volatile*>(&value), *reinterpret_cast<long long*>(&new_value));
    }

    #endif

    /// A simple wrapper type that atomicifies reads and writes to a 32-bit or 64-bit object
    ///
    /// Note:  This is not intended for genreal purpose use; it is designed to work correctly for
    /// one particular use case.  Ideally we would use `std::atomic<T>`, but the Visual C++ 
    /// Standard Library concurrency headers cannot be included in C++/CLI translation units, which
    /// is a big problem.
    template <typename T>
    class atomic
    {
    public:

        typedef T value_type;

        atomic(value_type const value = value_type())
            : _value(value)
        {
        }

        atomic(atomic const& other)
            : _value(other.load())
        {
        }

        auto operator=(atomic const& other) -> atomic&
        {
            store(other.load());
        }

        auto load() const -> value_type
        {
            return core::atomic_load(const_cast<value_type volatile&>(_value));
        }

        auto store(value_type const new_value) -> void
        {
            return core::atomic_store(_value, new_value);
        }

    private:

        value_type volatile _value;
    };





    class recursive_mutex;
    class recursive_mutex_context;

    /// An RAII container that owns a lock on a `recursive_mutex`
    ///
    /// This type is moveable but not copyable
    class recursive_mutex_lock
    {
    public:

        recursive_mutex_lock(recursive_mutex&);

        recursive_mutex_lock(recursive_mutex_lock&&);
        auto operator=(recursive_mutex_lock&&) -> recursive_mutex_lock&;

        /// Releases the held lock; the destructor also calls this member function
        ///
        /// If the lock has already been released, this function is a no-op.
        auto release() -> void;

        /// Destroys the lock object and unlocks the mutex if it is currently locked by this thread
        ~recursive_mutex_lock();

    private:

        recursive_mutex_lock(recursive_mutex_lock const&);
        auto operator=(recursive_mutex_lock const&) -> recursive_mutex_lock&;

        value_initialized<recursive_mutex*> _mutex;
    };

    /// A recursive mutex that can be locked multiple times by a single thread
    ///
    /// The behavior of this class is roughly equivalent to that of `std::recursive_mutex`.  Note
    /// that this class dynamically allocates the underlying mutex, so one should not expect to be
    /// able to create these rapidly.
    ///
    /// This type is neither copyable nor moveable.
    class recursive_mutex
    {
    public:

        recursive_mutex();
        ~recursive_mutex();

        /// Causes the calling thread to acquire the mutex, or block and wait for it to be available
        ///
        /// To unlock the mutex, destroy the returned lock or call its `Release()` member function.
        auto lock() -> recursive_mutex_lock;

    private:

        friend recursive_mutex_lock;

        recursive_mutex(recursive_mutex const&);
        auto operator=(recursive_mutex const&) -> recursive_mutex&;

        auto private_lock()   -> void;
        auto private_unlock() -> void;

        std::unique_ptr<recursive_mutex_context> _mutex;
    };

} }

#endif
