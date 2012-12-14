
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECTTEST_UNITTESTS_WINDOWS_RUNTIME_PRECOMPILED_HEADERS_HPP_
#define CXXREFLECTTEST_UNITTESTS_WINDOWS_RUNTIME_PRECOMPILED_HEADERS_HPP_

#include "cxxreflect/cxxreflect.hpp"
#include "tests/unit_tests/infrastructure/test_driver.hpp"

#ifndef CXXREFLECT_USE_TEST_FRAMEWORK
#include <CppUnitTest.h>
#endif

namespace win {

    using namespace Windows::Foundation;

    /// Synchronizes an asynchronous call
    ///
    /// This takes a Windows Runtime IAsyncOperation, waits for it to complete, then returns the
    /// result.  Ideally we'd just call GetResults() and block, but PPL prohibits blocking on a
    /// task on a RoInitialize'd STA.  So, we spin-wait on the asynchronous operation.  Woo hoo!
    ///
    /// (We could yield, or sleep for a while, but at the moment we only synchronize operations
    /// that are expected to complete very quickly during unit testing, so this isn't much of a
    /// concern yet.)
    template <typename AsyncOperation>
    auto sync(AsyncOperation&& op) -> decltype(op->GetResults())
    {
        while (op->Status != ::Windows::Foundation::AsyncStatus::Completed) { }
        return op->GetResults();
    }

}

#endif
