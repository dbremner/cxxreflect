
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_CORE_CONCURRENCY_HPP_
#define CXXREFLECT_CORE_CONCURRENCY_HPP_

#include "cxxreflect/core/utility.hpp"

namespace cxxreflect { namespace core {

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
