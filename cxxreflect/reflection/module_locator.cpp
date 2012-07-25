
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#include "cxxreflect/reflection/precompiled_headers.hpp"
#include "cxxreflect/reflection/module_locator.hpp"

namespace cxxreflect { namespace reflection {

    module_location::module_location()
        : _kind(kind::uninitialized)
    {
    }

    module_location::module_location(core::const_byte_range const& memory_range)
        : _memory_range(memory_range), _kind(kind::memory)
    {
        core::assert_initialized(memory_range);
    }

    module_location::module_location(core::string_reference const& file_path)
        : _file_path(file_path.c_str()), _kind(kind::file)
    {
        core::assert_true([&]{ return !file_path.empty(); });
    }

    auto module_location::get_kind() const -> kind
    {
        return _kind.get();
    }

    auto module_location::is_file() const -> bool
    {
        return get_kind() == kind::file;
    }

    auto module_location::is_memory() const -> bool
    {
        return get_kind() == kind::memory;
    }

    auto module_location::is_initialized() const -> bool
    {
        return get_kind() != kind::uninitialized;
    }

    auto module_location::memory_range() const -> core::const_byte_range
    {
        core::assert_true([&]{ return is_memory(); });

        return _memory_range;
    }

    auto module_location::file_path() const -> core::string_reference
    {
        core::assert_true([&]{ return is_file(); });

        return _file_path.c_str();
    }

    auto module_location::to_string() const -> core::string
    {
        core::assert_initialized(*this);

        if (is_file())
            return file_path().c_str();
        
        if (is_memory())
            return L"<memory:" + core::to_string(begin(memory_range())) + L">";

        core::assert_fail(L"unreachable code");
        return core::string();
    }

    auto operator==(module_location const& lhs, module_location const& rhs) -> bool
    {
        if (lhs._kind.get() != rhs._kind.get())
            return false;

        if (lhs._kind.get() == module_location::kind::uninitialized)
            return true;

        if (lhs._kind.get() == module_location::kind::file)
            return lhs._file_path == rhs._file_path;

        if (lhs._kind.get() == module_location::kind::memory)
            return std::equal_to<core::const_byte_iterator>()(begin(lhs._memory_range), begin(rhs._memory_range));

        core::assert_fail(L"unreachable code");
        return false;
    }

    auto operator<(module_location const& lhs, module_location const& rhs) -> bool
    {
        // Provide an arbitrary but consistent ordering of kinds:
        if (lhs._kind.get() < rhs._kind.get())
            return true;

        if (lhs._kind.get() > rhs._kind.get())
            return false;

        core::assert_true([&]{ return lhs._kind.get() == rhs._kind.get(); });

        // All uninitialized objects compare equal:
        if (lhs._kind.get() == module_location::kind::uninitialized)
            return false;

        if (lhs._kind.get() == module_location::kind::file)
            return lhs._file_path < rhs._file_path;

        if (lhs._kind.get() == module_location::kind::memory)
            return std::less<core::const_byte_iterator>()(begin(lhs._memory_range), begin(rhs._memory_range));

        core::assert_fail(L"unreachable code");
        return false;
    }





    module_locator::module_locator()
    {
    }

    module_locator::module_locator(module_locator const& other)
        : _x(other.is_initialized() ? other._x->copy() : nullptr)
    {
    }

    module_locator::module_locator(module_locator&& other)
        : _x(std::move(other._x))
    {
    }

    auto module_locator::operator=(module_locator const& other) -> module_locator&
    {
        _x = other.is_initialized() ? other._x->copy() : nullptr;
        return *this;
    }

    auto module_locator::operator=(module_locator&& other) -> module_locator&
    {
        _x = std::move(other._x);
        return *this;
    }

    auto module_locator::locate_assembly(assembly_name const& target_assembly) const -> module_location
    {
        core::assert_initialized(*this);
        return _x->locate_assembly(target_assembly);
    }

    auto module_locator::locate_assembly(assembly_name const& target_assembly,
                                         core::string  const& full_type_name) const -> module_location
    {
        core::assert_initialized(*this);
        return _x->locate_assembly(target_assembly, full_type_name);
    }

    auto module_locator::locate_module(assembly_name const& requesting_assembly,
                                       core::string  const& module_name) const -> module_location
    {
        core::assert_initialized(*this);
        return _x->locate_module(requesting_assembly, module_name);
    }

    auto module_locator::is_initialized() const -> bool
    {
        return _x != nullptr;
    }





    directory_based_module_locator::directory_based_module_locator(directory_set const& directories)
        : _directories(directories)
    {
        core::assert_true([&]{ return !directories.empty(); });
    }

    auto directory_based_module_locator::locate_assembly(assembly_name const& target_assembly) const 
        -> module_location
    {
        wchar_t const* const extensions[] = { L".dll", L".exe" };
        for (auto dir_it(begin(_directories)); dir_it != end(_directories); ++dir_it)
        {
            for (auto ext_it(begin(extensions)); ext_it != end(extensions); ++ext_it)
            {
                core::string path(*dir_it + L"\\" + target_assembly.simple_name() + *ext_it);
                if (core::externals::file_exists(path.c_str()))
                    return module_location(path.c_str());
            }
        }

        return module_location();
    }

    auto directory_based_module_locator::locate_assembly(assembly_name const& target_assembly,
                                                         core::string  const& full_type_name) const
        -> module_location
    {
        // This implementation does not utilize namespace-based resolution
        return locate_assembly(target_assembly);
    }

    auto directory_based_module_locator::locate_module(assembly_name const& requesting_assembly,
                                                       core::string  const& module_name) const
        -> module_location
    {
        core::string const& requesting_path(requesting_assembly.path());
        auto const it(std::find(requesting_path.rbegin(), requesting_path.rend(), L'\\'));
        if (it == requesting_path.rend())
            return module_location();

        core::string path(begin(requesting_path), it.base());
        path += module_name;

        if (core::externals::file_exists(path.c_str()))
            return module_location(path.c_str());

        return module_location();
    }

} }
