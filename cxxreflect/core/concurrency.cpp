
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/core/precompiled_headers.hpp"
#include "cxxreflect/core/concurrency.hpp"

#if CXXREFLECT_THREADING == CXXREFLECT_THREADING_STDCPPSYNCHRONIZED
#    include <mutex>
#endif

namespace cxxreflect { namespace core {

    #if CXXREFLECT_THREADING == CXXREFLECT_THREADING_STDCPPSYNCHRONIZED
    class recursive_mutex_context
    {
    public:

        auto lock()   -> void { _mutex.lock();   }
        auto unlock() -> void { _mutex.unlock(); }

    private:

        std::recursive_mutex _mutex;
    };
    #elif CXXREFLECT_THREADING == CXXREFLECT_THREADING_SINGLETHREADED
    class recursive_mutex_context
    {
    public:

        auto lock()   -> void { }
        auto unlock() -> void { }
    };
    #else
    #    error Unknown threading model
    #endif





    recursive_mutex::recursive_mutex()
        : _mutex(make_unique<recursive_mutex_context>())
    {
    }

    recursive_mutex::~recursive_mutex()
    {
        // For completeness
    }

    void recursive_mutex::private_lock()
    {
        _mutex->lock();
    }

    void recursive_mutex::private_unlock()
    {
        _mutex->unlock();
    }

    auto recursive_mutex::lock() -> recursive_mutex_lock
    {
        return recursive_mutex_lock(*this);
    }

    recursive_mutex_lock::recursive_mutex_lock(recursive_mutex& mutex)
        : _mutex(&mutex)
    {
        _mutex.get()->private_lock();
    }

    recursive_mutex_lock::recursive_mutex_lock(recursive_mutex_lock&& other)
        : _mutex(std::move(other._mutex.get()))
    {
        other._mutex.get() = nullptr;
    }

    auto recursive_mutex_lock::operator=(recursive_mutex_lock&& other) -> recursive_mutex_lock&
    {
        _mutex.get() = other._mutex.get();
        other._mutex.get() = nullptr;
        return *this;
    }

    auto recursive_mutex_lock::release() -> void
    {
        if (_mutex.get() != nullptr)
        {
            _mutex.get()->private_unlock();
            _mutex.get() = nullptr;
        }
    }

    recursive_mutex_lock::~recursive_mutex_lock()
    {
        release();
    }

} }

// AMDG //
