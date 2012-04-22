
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "CxxReflect/PrecompiledHeaders.hpp"

#include "CxxReflect/FundamentalUtilities.hpp"

namespace CxxReflect { namespace Detail {

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

} }