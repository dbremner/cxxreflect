
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/windows_runtime/precompiled_headers.hpp"

#ifdef CXXREFLECT_ENABLE_WINDOWS_RUNTIME_INTEGRATION

#include "cxxreflect/windows_runtime/detail/module_type_index.hpp"

namespace cxxreflect { namespace windows_runtime { namespace detail {

    module_type_index::module_type_index(metadata::database const* scope)
        : _scope(scope)
    {
        core::assert_not_null(scope);

        _index.resize(_scope->tables()[metadata::table_id::type_def].row_count());
        for (core::size_type i(0); i < _index.size(); ++i)
            _index[i] = (core::as_integer(metadata::table_id::type_def) << 24) | (i + 1);
        std::sort(begin(_index), end(_index), *this);
    }

    auto module_type_index::find(core::string_reference const& namespace_name,
                                 core::string_reference const& name) const -> metadata::type_def_token
    {
        auto const it(core::binary_search(begin(_index), end(_index), std::make_pair(namespace_name, name), *this));
        if (it == end(_index))
            return metadata::type_def_token();

        return metadata::type_def_token(_scope.get(), *it);
    }

    auto module_type_index::operator()(type_name_pair const& lhs, type_name_pair const& rhs) const -> bool
    {
        return lhs < rhs;
    }

    auto module_type_index::operator()(type_name_pair const& lhs, core::size_type const rhs) const -> bool
    {
        return (*this)(lhs, get_name_of_type_def(rhs));
    }

    auto module_type_index::operator()(core::size_type const lhs, type_name_pair const& rhs) const -> bool
    {
        return (*this)(get_name_of_type_def(lhs), rhs);
    }

    auto module_type_index::operator()(core::size_type const lhs, core::size_type const rhs) const -> bool
    {
        return (*this)(get_name_of_type_def(lhs), get_name_of_type_def(rhs));
    }

    auto module_type_index::get_name_of_type_def(core::size_type const index) const -> type_name_pair
    {
        metadata::type_def_row const row((*_scope)[metadata::type_def_token(_scope.get(), index)]);
        return std::make_pair(row.namespace_name(), row.name());
    }

} } }

#endif // ENABLE_WINDOWS_RUNTIME_INTEGRATION
