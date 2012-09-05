
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_TYPE_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_TYPE_HPP_

#include "Configuration.hpp"

namespace CxxReflect { namespace Reflection { namespace Native { namespace Detail {

    template <typename ForwardIterator>
    class Iterator
        : public wrl::RuntimeClass<
              winabi::IIterator<
                  typename std::iterator_traits<ForwardIterator>::value_type
              >
          >
    {
        typedef typename std::iterator_traits<ForwardIterator>::value_type  ValueType;
        typedef ABI::Windows::Foundation::Collections::IIterator<ValueType> BaseType;

        InspectableClass(BaseType::z_get_rc_name_impl(), BaseTrust)

    public:

        Iterator(ForwardIterator const current, ForwardIterator const last)
            : _current(current), _last(last)
        {
        }

        explicit Iterator(ForwardIterator const last)
            : _current(last), _last(last)
        {
        }

        STDMETHOD(get_Current)(ValueType* const current) override
        {
            *current = *_current;
            return S_OK;
        }

        STDMETHOD(get_HasCurrent)(boolean* const hasCurrent) override
        {
            *hasCurrent = (_current != _last);
            return S_OK;
        }

        STDMETHOD(MoveNext)(boolean* hasCurrent) override
        {
            ++_current;
            *hasCurrent = (_current != _last);
            return S_OK;
        }

    private:

        ForwardIterator _current;
        ForwardIterator _last;
    };


    /*
    template <typename T>
    class Iterable : public wrl::RuntimeClass<winabi::IIterable<T>>
    {
        InspectableClass(winabi::IIterable<T>::z_get_rc_name_impl(), BaseTrust)
            
    public:

        virtual auto STDMETHODCALLTYPE First(winabi::IIterator<T>** first) -> HRESULT override;
    };
    */
} } } }

#endif
