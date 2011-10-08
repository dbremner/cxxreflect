#ifndef CXXREFLECT_UTILITYDEFINITIONS_HPP_
#define CXXREFLECT_UTILITYDEFINITIONS_HPP_

#include "Utility.hpp"

namespace CxxReflect { namespace Detail {

    template <typename T>
    RefCounted const* RefPointer<T>::GetBase()
    {
        return pointer_;
    }

} }

#endif
