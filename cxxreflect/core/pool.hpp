
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

// NOTE:  This header file contains incomplete implementation details.  They are not yet used in the
// library and are entirely untested.

#ifndef CXXREFLECT_CORE_POOL_HPP_
#define CXXREFLECT_CORE_POOL_HPP_

#include "cxxreflect/core/concurrency.hpp"
#include "cxxreflect/core/diagnostic.hpp"
#include "cxxreflect/core/standard_library.hpp"
#include "cxxreflect/core/utility.hpp"

namespace cxxreflect { namespace core {

    template <typename T>
    class object_pool;

} }

namespace cxxreflect { namespace core { namespace detail {

    template <typename T>
    class object_pool_node
    {
    public:

        typedef T                                              element_type;
        typedef object_pool<T>                                 pool_type;
        typedef typename std::aligned_storage<sizeof(T)>::type storage_type;

        object_pool_node()
            : _next_free_node(nullptr)
        {
        }

    private:

        friend class object_pool<T>;

        alignment_type _storage;

        union
        {
            object_pool_node* _next_free_node;
            pool_type*        _owner_pool;
        };
    };

} } }

namespace cxxreflect { namespace core {

    template <typename T>
    class object_pool
    {
    public:

        typedef T                           element_type;
        typedef detail::object_pool_node<T> node_type;

        object_pool(size_type const slab_size)
            : _slab_size(slab_size)
        {
        }

        

    private:

        typedef std::unique_ptr<node_type[]> slab_type;
        typedef std::vector<slab_type>       slab_sequence;

        auto allocate_node() -> node_type*
        {
            recursive_mutex_lock const lock(_sync.lock());

            if (_head == nullptr)
                allocate_slab();

            assert_not_null(_head);

            node_type* const node(_head);
            _head = node->_next_free_node;

            node->_owner_pool = this;
            return node;
        }

        auto deallocate_node(node_type* const node) -> void
        {
            assert_not_null(node);

            recursive_mutex_lock const lock(_sync.lock());

            node->_next_free_node = _head;
            _head = node;
        }

        auto allocate_slab() -> void
        {
            assert_true([&]{ return _head == nullptr; });

            slab_type new_slab(new node_type[_slab_size.get()]);
            for (unsigned i(0); i != _slab_size.get() - 1; ++i)
            {
                new_slab[i]._next_free_node = &new_slab[i + 1];
            }

            new_slab[_slab_size.get() - 1]._next_free_node = _head;
            _head = new_slab.get();
        }
        
        value_initialized<size_type> _slab_size;
        slab_sequence                _slabs;
        
        node_type*                   _head;
        recursive_mutex              _sync;
    };

} }

#endif
