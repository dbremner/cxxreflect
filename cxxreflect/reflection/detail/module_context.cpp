
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/detail/loader_context.hpp"
#include "cxxreflect/reflection/detail/module_context.hpp"
#include "cxxreflect/reflection/module.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    auto module_type_def_index_iterator_constructor::operator()(metadata::database const*                    const scope,
                                                                std::vector<core::size_type>::const_iterator const it) const
        -> metadata::type_def_token
    {
        return metadata::type_def_token(scope, *it);
    }

    module_type_def_index::module_type_def_index(metadata::database const* scope)
        : _scope(scope)
    {
        core::assert_not_null(scope);

        _index.resize(_scope->tables()[metadata::table_id::type_def].row_count());

        for (core::size_type i(0); i < _index.size(); ++i)
            _index[i] = (core::as_integer(metadata::table_id::type_def) << 24) | (i + 1);

        // Preemptively remove all filtered types from the index:  we use the index both for by-name
        // type lookup and for type enumeration; by eliminating hidden types here, we make it 
        // impossible to (legitimately) get a handle to a filtered type.
        loader_context const& loader(loader_context::from(*scope));
        _index.erase(std::remove_if(_index.begin(), _index.end(), [&](core::size_type const i) -> bool
        {
            core::string_reference xa(row_from(metadata::type_def_token(_scope.get(), i)).name());
            return loader.is_filtered_type(metadata::type_def_token(_scope.get(), i));
        }), _index.end());

        _index.shrink_to_fit();

        std::sort(_index.begin(), _index.end(), comparer(_scope.get()));
    }

    auto module_type_def_index::find(core::string_reference const& namespace_name,
                                     core::string_reference const& name) const -> metadata::type_def_token
    {
        auto const it(core::binary_search(_index.begin(), _index.end(), std::make_pair(namespace_name, name), comparer(_scope.get())));
        if (it == _index.end())
            return metadata::type_def_token();

        return metadata::type_def_token(_scope.get(), *it);
    }

    auto module_type_def_index::find(core::string_reference const& namespace_name) const -> type_def_iterator_range
    {
        auto const range(std::equal_range(_index.begin(), _index.end(), namespace_name, comparer(_scope.get())));

        return type_def_iterator_range(
            type_def_iterator(_scope.get(), range.first),
            type_def_iterator(_scope.get(), range.second));
    }

    auto module_type_def_index::begin() const -> type_def_iterator
    {
        return type_def_iterator(_scope.get(), _index.begin());
    }

    auto module_type_def_index::end() const -> type_def_iterator
    {
        return type_def_iterator(_scope.get(), _index.end());
    }

    module_type_def_index::comparer::comparer(metadata::database const* const scope)
        : _scope(scope)
    {
        core::assert_not_null(scope);
    }

    auto module_type_def_index::comparer::operator()(type_name_pair const& lhs, type_name_pair const& rhs) const -> bool
    {
        return lhs < rhs;
    }

    auto module_type_def_index::comparer::operator()(type_name_pair const& lhs, core::size_type const rhs) const -> bool
    {
        return (*this)(lhs, get_name_of_type_def(rhs));
    }

    auto module_type_def_index::comparer::operator()(core::size_type const lhs, type_name_pair const& rhs) const -> bool
    {
        return (*this)(get_name_of_type_def(lhs), rhs);
    }

    auto module_type_def_index::comparer::operator()(core::size_type const lhs, core::size_type const rhs) const -> bool
    {
        return (*this)(get_name_of_type_def(lhs), get_name_of_type_def(rhs));
    }
    
    auto module_type_def_index::comparer::operator()(core::string_reference const& lhs, core::string_reference const& rhs) const -> bool
    {
        return lhs < rhs;
    }

    auto module_type_def_index::comparer::operator()(core::string_reference const& lhs, core::size_type const rhs) const -> bool
    {
        return (*this)(lhs, get_name_of_type_def(rhs).first);
    }

    auto module_type_def_index::comparer::operator()(core::size_type const lhs, core::string_reference const& rhs) const -> bool
    {
        return (*this)(get_name_of_type_def(lhs).first, rhs);
    }

    auto module_type_def_index::comparer::get_name_of_type_def(core::size_type const index) const -> type_name_pair
    {
        metadata::type_def_row const row(row_from(metadata::type_def_token(_scope.get(), index)));
        return std::make_pair(row.namespace_name(), row.name());
    }





    module_context::module_context(assembly_context const* assembly, module_location const& location)
        : _assembly(assembly),
          _location(location),
          _database(create_database(location)),

          _type_def_index    (&_database),
          _assembly_ref_cache(&_database),
          _module_ref_cache  (&_database),
          _type_ref_cache    (&_database),
          _member_ref_cache  (&_database)
    {
        core::assert_not_null(assembly);
        core::assert_initialized(_location);
        core::assert_initialized(_database);
    }

    auto module_context::assembly() const -> assembly_context const&
    {
        return *_assembly.get();
    }

    auto module_context::location() const -> module_location const&
    {
        return _location;
    }

    auto module_context::database() const -> metadata::database const&
    {
        return _database;
    }

    auto module_context::type_def_index() const -> module_type_def_index const&
    {
        return _type_def_index;
    }

    auto module_context::assembly_ref_cache() const -> module_assembly_ref_cache&
    {
        return _assembly_ref_cache;
    }

    auto module_context::module_ref_cache() const -> module_module_ref_cache&
    {
        return _module_ref_cache;
    }

    auto module_context::type_ref_cache() const -> module_type_ref_cache&
    {
        return _type_ref_cache;
    }

    auto module_context::member_ref_cache() const -> module_member_ref_cache&
    {
        return _member_ref_cache;
    }

    auto module_context::from(metadata::database const& scope) -> module_context const&
    {
        module_context const* const owner(dynamic_cast<module_context const*>(&scope.owner()));
        if (owner == nullptr)
            throw core::logic_error(L"attempted to get module owner of unrelated database");

        return *owner;
    }

    auto module_context::create_database(module_location const& location) -> metadata::database
    {
        core::assert_initialized(location);

        if (location.is_file())
        {
            return metadata::database::create_from_file(location.file_path().c_str(), this);
        }
        else if (location.is_memory())
        {
            core::unique_byte_array range(begin(location.memory_range()), end(location.memory_range()));
            return metadata::database(std::move(range), this);
        }

        core::assert_unreachable();
    }

    CXXREFLECT_DEFINE_INCOMPLETE_DELETE(unique_module_context_delete, module_context)

} } }
