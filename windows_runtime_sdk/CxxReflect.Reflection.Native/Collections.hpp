
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_COLLECTIONS_HPP_
#define CXXREFLECT_WINRTCOMPONENTS_REFLECTION_NATIVE_COLLECTIONS_HPP_

#include "Configuration.hpp"

namespace cxxreflect { namespace windows_runtime_sdk {

    template <typename ForwardIterator>
    class RuntimeIterator final
        : public wrl::RuntimeClass<
            win::IIterator<typename ConvertToRuntimeClass<typename std::iterator_traits<ForwardIterator>::value_type>::Type>
        >
    {
        typedef typename std::iterator_traits<ForwardIterator>::value_type AbiValueType;
        typedef typename ConvertToRuntimeClass<AbiValueType>::Type         LogicalValueType;
        typedef win::IIterator<LogicalValueType>                           InterfaceType;

        InspectableClass(InterfaceType::z_get_rc_name_impl(), BaseTrust);

    public:

        RuntimeIterator(ForwardIterator const current, ForwardIterator const last)
            : _current(current), _last(last)
        {
        }

        virtual auto STDMETHODCALLTYPE get_Current(AbiValueType* const current) -> HRESULT override
        {
            return cxr::call_with_runtime_convention([&]() -> HRESULT
            {
                cxr::throw_if_null_and_initialize_out_parameter(current);
                cxr::throw_if_true(_current.get() == _last.get(), E_BOUNDS);

                *current = *_current.get();
                return S_OK;
            });
        }

        virtual auto STDMETHODCALLTYPE get_HasCurrent(boolean* const hasCurrent) -> HRESULT override
        {
            return cxr::call_with_runtime_convention([&]() -> HRESULT
            {
                cxr::throw_if_null_and_initialize_out_parameter(hasCurrent);

                *hasCurrent = _current.get() != _last.get();
                return S_OK;
            });
        }

        virtual auto STDMETHODCALLTYPE MoveNext(boolean* const hasCurrent) -> HRESULT override
        {
            return cxr::call_with_runtime_convention([&]() -> HRESULT
            {
                cxr::throw_if_null_and_initialize_out_parameter(hasCurrent);
                cxr::throw_if_true(_current.get() == _last.get(), E_BOUNDS);

                ++_current.get();

                *hasCurrent = _current.get() != _last.get();
                return S_OK;
            });
        }

    private:

        cxr::value_initialized<ForwardIterator> _current;
        cxr::value_initialized<ForwardIterator> _last;
    };

    /// N.B.:  While we only _require_ a forward iterator, GetAt and Size are O(n) for forward and
    /// bidirectional iterators, so if the range is expected to be large, it would be best either to
    /// expose it as an IIterable (and not an IVectorView) or to realize the range into a temporary
    /// container that provides random access.  (We do not restrict usage to random access iterators
    /// because there are several well-known cases in the library where the number of elements to be
    /// iterated over is known to be small.  For example, a type never has very many generic
    /// arguments--three would be a lot--and it's convenient to be able to treat generic arguments
    /// as an indexable range.)
    template <typename ForwardIterator>
    class RuntimeVectorView final
        : public wrl::RuntimeClass<
            win::IIterable  <typename ConvertToRuntimeClass<typename std::iterator_traits<ForwardIterator>::value_type>::Type>,
            win::IVectorView<typename ConvertToRuntimeClass<typename std::iterator_traits<ForwardIterator>::value_type>::Type>
        >
    {
        typedef typename std::iterator_traits<ForwardIterator>::value_type AbiValueType;
        typedef typename ConvertToRuntimeClass<AbiValueType>::Type         LogicalValueType;
        typedef win::IVectorView<LogicalValueType>                         InterfaceType;

        typedef win::IIterator<LogicalValueType> IteratorInterfaceType;
        typedef RuntimeIterator<ForwardIterator> IteratorRuntimeType;

        InspectableClass(InterfaceType::z_get_rc_name_impl(), BaseTrust);

    public:

        RuntimeVectorView(ForwardIterator const first, ForwardIterator const last)
            : _first(first), _last(last)
        {
        }

        virtual auto STDMETHODCALLTYPE First(IteratorInterfaceType** const first) -> HRESULT override
        {
            return cxr::call_with_runtime_convention([&]() -> HRESULT
            {
                cxr::throw_if_null_and_initialize_out_parameter(first);

                *first = wrl::Make<IteratorRuntimeType>(_first.get(), _last.get()).Detach();
                return *first != nullptr ? S_OK : E_OUTOFMEMORY;
            });
        }

        virtual auto STDMETHODCALLTYPE GetAt(unsigned const index, AbiValueType* const item) -> HRESULT override
        {
            return cxr::call_with_runtime_convention([&]() -> HRESULT
            {
                cxr::throw_if_null_and_initialize_out_parameter(item);
                cxr::throw_if_true(index >= ComputeSize(), E_BOUNDS);

                *item = *std::next(_first.get(), index);
                return S_OK;
            });
        }

        virtual auto STDMETHODCALLTYPE get_Size(unsigned* const size) -> HRESULT override
        {
            return cxr::call_with_runtime_convention([&]() -> HRESULT
            {
                cxr::throw_if_null_and_initialize_out_parameter(size);

                *size = ComputeSize();
                return S_OK;
            });
        }

        virtual auto STDMETHODCALLTYPE IndexOf(AbiValueType const value, unsigned* const index, boolean* const found) -> HRESULT override
        {
            return cxr::call_with_runtime_convention([&]() -> HRESULT
            {
                if (index != nullptr)
                    *index = 0;

                if (found != nullptr)
                    *found = false;

                cxr::throw_if_null(index);
                cxr::throw_if_null(found);

                ForwardIterator const it(std::find(_first.get(), _last.get(), value));
                if (it == _last.get())
                    return S_OK;

                *index = cxr::distance(_first.get(), it);
                *found = true;
                return S_OK;
            });
        }

    private:

        auto ComputeSize() const -> cxr::size_type
        {
            return cxr::distance(_first.get(), _last.get());
        }

        cxr::value_initialized<ForwardIterator> _first;
        cxr::value_initialized<ForwardIterator> _last;
    };

} }

#endif
