
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/FundamentalUtilities.hpp"

#if CXXREFLECT_THREADING == CXXREFLECT_THREADING_STDCPPSYNCHRONIZED
#    include <mutex>
#endif

//
//
// SHA1 COMPUTATION:  The following SHA1 implementation was extracted from Boost 1.49.0.  It was
// copied and pasted with the following changes:
//  * BOOST_STATIC_ASSERT replaced by static_assert
//  * std::logic_error replaced by LogicError
//  * Namespace changed to CxxReflect::Detail::<unnamed>::Private
//
//
namespace CxxReflect { namespace Detail { namespace { namespace Private {

    // boost/uuid/sha1.hpp header file  ----------------------------------------------//

    // Copyright 2007 Andy Tompkins.
    // Distributed under the Boost Software License, Version 1.0. (See
    // accompanying file LICENSE_1_0.txt or copy at
    // http://www.boost.org/LICENSE_1_0.txt)

    // Revision History
    //  29 May 2007 - Initial Revision
    //  25 Feb 2008 - moved to namespace boost::uuids::detail
    //  10 Jan 2012 - can now handle the full size of messages (2^64 - 1 bits)

    // This is a byte oriented implementation

    static_assert(sizeof(unsigned char)*8 == 8, "Unexpected char type size");
    static_assert(sizeof(unsigned int)*8 == 32, "Unexpected int type size");

    inline unsigned int left_rotate(unsigned int x, std::size_t n)
    {
        return (x<<n) ^ (x>> (32-n));
    }

    class sha1
    {
    public:
        typedef unsigned int(&digest_type)[5];
    public:
        sha1();

        void reset();

        void process_byte(unsigned char byte);
        void process_block(void const* bytes_begin, void const* bytes_end);
        void process_bytes(void const* buffer, std::size_t byte_count);

        void get_digest(digest_type digest);

    private:
        void process_block();
        void process_byte_impl(unsigned char byte);

    private:
        unsigned int h_[5];

        unsigned char block_[64];

        std::size_t block_byte_index_;
        std::size_t bit_count_low;
        std::size_t bit_count_high;
    };

    inline sha1::sha1()
    {
        reset();
    }

    inline void sha1::reset()
    {
        h_[0] = 0x67452301;
        h_[1] = 0xEFCDAB89;
        h_[2] = 0x98BADCFE;
        h_[3] = 0x10325476;
        h_[4] = 0xC3D2E1F0;

        block_byte_index_ = 0;
        bit_count_low = 0;
        bit_count_high = 0;
    }

    inline void sha1::process_byte(unsigned char byte)
    {
        process_byte_impl(byte);

        bit_count_low += 8;
        if (bit_count_low == 0) {
            ++bit_count_high;
            if (bit_count_high == 0) {
                throw RuntimeError(L"sha1 too many bytes");
            }
        }
    }

    inline void sha1::process_byte_impl(unsigned char byte)
    {
        block_[block_byte_index_++] = byte;

        if (block_byte_index_ == 64) {
            block_byte_index_ = 0;
            process_block();
        }
    }

    inline void sha1::process_block(void const* bytes_begin, void const* bytes_end)
    {
        unsigned char const* begin = static_cast<unsigned char const*>(bytes_begin);
        unsigned char const* end = static_cast<unsigned char const*>(bytes_end);
        for(; begin != end; ++begin) {
            process_byte(*begin);
        }
    }

    inline void sha1::process_bytes(void const* buffer, std::size_t byte_count)
    {
        unsigned char const* b = static_cast<unsigned char const*>(buffer);
        process_block(b, b+byte_count);
    }

    inline void sha1::process_block()
    {
        unsigned int w[80];
        for (std::size_t i=0; i<16; ++i) {
            w[i]  = (block_[i*4 + 0] << 24);
            w[i] |= (block_[i*4 + 1] << 16);
            w[i] |= (block_[i*4 + 2] << 8);
            w[i] |= (block_[i*4 + 3]);
        }
        for (std::size_t i=16; i<80; ++i) {
            w[i] = left_rotate((w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16]), 1);
        }

        unsigned int a = h_[0];
        unsigned int b = h_[1];
        unsigned int c = h_[2];
        unsigned int d = h_[3];
        unsigned int e = h_[4];

        for (std::size_t i=0; i<80; ++i) {
            unsigned int f;
            unsigned int k;

            if (i<20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (i<40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i<60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            unsigned temp = left_rotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = left_rotate(b, 30);
            b = a;
            a = temp;
        }

        h_[0] += a;
        h_[1] += b;
        h_[2] += c;
        h_[3] += d;
        h_[4] += e;
    }

    inline void sha1::get_digest(digest_type digest)
    {
        // append the bit '1' to the message
        process_byte_impl(0x80);

        // append k bits '0', where k is the minimum number >= 0
        // such that the resulting message length is congruent to 56 (mod 64)
        // check if there is enough space for padding and bit_count
        if (block_byte_index_ > 56) {
            // finish this block
            while (block_byte_index_ != 0) {
                process_byte_impl(0);
            }

            // one more block
            while (block_byte_index_ < 56) {
                process_byte_impl(0);
            }
        } else {
            while (block_byte_index_ < 56) {
                process_byte_impl(0);
            }
        }

        // append length of message (before pre-processing) 
        // as a 64-bit big-endian integer
        process_byte_impl( static_cast<unsigned char>((bit_count_high>>24) & 0xFF) );
        process_byte_impl( static_cast<unsigned char>((bit_count_high>>16) & 0xFF) );
        process_byte_impl( static_cast<unsigned char>((bit_count_high>>8 ) & 0xFF) );
        process_byte_impl( static_cast<unsigned char>((bit_count_high)     & 0xFF) );
        process_byte_impl( static_cast<unsigned char>((bit_count_low>>24) & 0xFF) );
        process_byte_impl( static_cast<unsigned char>((bit_count_low>>16) & 0xFF) );
        process_byte_impl( static_cast<unsigned char>((bit_count_low>>8 ) & 0xFF) );
        process_byte_impl( static_cast<unsigned char>((bit_count_low)     & 0xFF) );

        // get final digest
        digest[0] = h_[0];
        digest[1] = h_[1];
        digest[2] = h_[2];
        digest[3] = h_[3];
        digest[4] = h_[4];
    }

} } } }

namespace CxxReflect { namespace Detail {

    Sha1Hash ComputeSha1Hash(ConstByteIterator const first, ConstByteIterator const last)
    {
        Private::sha1 instance;
        instance.process_block(first, last);

        Sha1Hash hash;
        instance.get_digest(reinterpret_cast<Private::sha1::digest_type>(*hash.data()));
        return hash;
    }






    IDestructible::~IDestructible()
    {
        // Virtual destructor required definition; no cleanup is required
    }





    FileRange::FileRange()
    {
    }

    FileRange::FileRange(ConstByteIterator const first, ConstByteIterator const last, UniqueDestructible release)
        : _first(first), _last(last), _release(std::move(release))
    {
        VerifyNotNull(first);
        VerifyNotNull(last);
    }

    FileRange::FileRange(FileRange&& other)
        : _first  (other._first             ),
          _last   (other._last              ),
          _release(std::move(other._release))
    {
        other._first.Reset();
        other._last.Reset();
    }

    FileRange& FileRange::operator=(FileRange&& other)
    {
        _first.Get() = other._first.Get();
        _last.Get() = other._last.Get();
        _release = std::move(other._release);

        other._first.Reset();
        other._last.Reset();

        return *this;
    }

    ConstByteIterator FileRange::Begin() const
    {
        return _first.Get();
    }

    ConstByteIterator FileRange::End() const
    {
        return _last.Get();
    }

    bool FileRange::IsInitialized() const
    {
        return _first.Get() != nullptr && _last.Get() != nullptr;
    }





    #if CXXREFLECT_THREADING == CXXREFLECT_THREADING_STDCPPSYNCHRONIZED
    class RecursiveMutexContext
    {
    public:

        void Lock()   { _mutex.lock();   }
        void Unlock() { _mutex.unlock(); }

    private:

        std::recursive_mutex _mutex;
    };
    #elif CXXREFLECT_THREADING == CXXREFLECT_THREADING_SINGLETHREADED
    class RecursiveMutexContext
    {
    public:

        void Lock()   { }
        void Unlock() { }
    };
    #else
    #    error Unknown threading model
    #endif





    RecursiveMutex::RecursiveMutex()
        : _mutex(MakeUnique<RecursiveMutexContext>())
    {
    }

    RecursiveMutex::~RecursiveMutex()
    {
        // For completeness
    }

    void RecursiveMutex::PrivateLock()
    {
        _mutex->Lock();
    }

    void RecursiveMutex::PrivateUnlock()
    {
        _mutex->Unlock();
    }

    RecursiveMutexLock RecursiveMutex::Lock()
    {
        return RecursiveMutexLock(*this);
    }

    RecursiveMutexLock::RecursiveMutexLock(RecursiveMutex& mutex)
        : _mutex(&mutex)
    {
        _mutex.Get()->PrivateLock();
    }

    RecursiveMutexLock::RecursiveMutexLock(RecursiveMutexLock&& other)
        : _mutex(std::move(other._mutex.Get()))
    {
        other._mutex.Get() = nullptr;
    }

    RecursiveMutexLock& RecursiveMutexLock::operator=(RecursiveMutexLock&& other)
    {
        _mutex.Get() = other._mutex.Get();
        other._mutex.Get() = nullptr;
        return *this;
    }

    void RecursiveMutexLock::Release()
    {
        if (_mutex.Get() != nullptr)
        {
            _mutex.Get()->PrivateUnlock();
            _mutex.Get() = nullptr;
        }
    }

    RecursiveMutexLock::~RecursiveMutexLock()
    {
        Release();
    }

} }