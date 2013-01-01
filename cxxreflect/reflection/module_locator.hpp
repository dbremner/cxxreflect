
//                            Copyright James P. McNellis 2011 - 2013.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_MODULE_LOCATOR_HPP_
#define CXXREFLECT_REFLECTION_MODULE_LOCATOR_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection {

    /// Represents the location of a module, either on disk (path) or in memory (byte range)
    class module_location
    {
    public:

        enum class kind
        {
            uninitialized,
            file,
            memory
        };

        module_location();
        explicit module_location(core::const_byte_range const& memory_range);
        explicit module_location(core::string_reference const& file_path);

        auto get_kind()       const -> kind;
        auto is_file()        const -> bool;
        auto is_memory()      const -> bool;
        auto is_initialized() const -> bool;

        auto memory_range() const -> core::const_byte_range;
        auto file_path()    const -> core::string_reference;

        auto to_string() const -> core::string;

        friend auto operator==(module_location const&, module_location const&) -> bool;
        friend auto operator< (module_location const&, module_location const&) -> bool;

        CXXREFLECT_GENERATE_COMPARISON_OPERATORS(module_location)

    private:

        core::const_byte_range        _memory_range;
        core::string                  _file_path;
        core::value_initialized<kind> _kind;
    };

} }





namespace cxxreflect { namespace reflection { namespace detail {

    typedef std::unique_ptr<class base_module_locator> unique_base_module_locator;

    class base_module_locator
    {
    public:

        virtual auto locate_assembly(assembly_name const& target_assembly) const -> module_location = 0;

        virtual auto locate_namespace(core::string_reference const& namespace_name) const -> module_location = 0;

        virtual auto locate_module(assembly_name          const& requesting_assembly,
                                   core::string_reference const& module_name) const -> module_location = 0;

        virtual auto copy() const -> unique_base_module_locator = 0;

        virtual ~base_module_locator() { }
    };

    template <typename T>
    class derived_module_locator : public base_module_locator
    {
    public:

        template <typename U>
        derived_module_locator(U&& x)
            : _x(std::forward<U>(x))
        {
        }

        virtual auto locate_assembly(assembly_name const& target_assembly) const -> module_location override
        {
            return _x.locate_assembly(target_assembly);
        }

        virtual auto locate_namespace(core::string_reference const& namespace_name) const -> module_location override
        {
            return _x.locate_namespace(namespace_name);
        }

        virtual auto locate_module(assembly_name          const& requesting_assembly,
                                   core::string_reference const& module_name) const -> module_location override
        {
            return _x.locate_module(requesting_assembly, module_name);
        }

        virtual auto copy() const -> unique_base_module_locator
        {
            return core::make_unique<derived_module_locator>(_x);
        }

    private:

        T _x;
    };

} } }





namespace cxxreflect { namespace reflection {

    class module_locator
    {
    public:

        module_locator();

        template <typename T>
        module_locator(T x)
            : _x(core::make_unique<detail::derived_module_locator<T>>(x))
        {
        }

        module_locator(module_locator const&);
        module_locator(module_locator&&);

        auto operator=(module_locator const&) -> module_locator&;
        auto operator=(module_locator&&)      -> module_locator&;

        auto locate_assembly(assembly_name const& target_assembly) const -> module_location;

        auto locate_namespace(core::string_reference const& namespace_name) const -> module_location;

        auto locate_module(assembly_name          const& requesting_assembly,
                           core::string_reference const& module_name) const -> module_location;

        auto is_initialized() const -> bool;

    private:

        detail::unique_base_module_locator _x;
    };





    class search_path_module_locator
    {
    public:

        typedef std::vector<core::string> search_path_sequence;

        search_path_module_locator(search_path_sequence const& sequence);

        auto locate_assembly(assembly_name const& target_assembly) const -> module_location;

        auto locate_namespace(core::string_reference const& namespace_name) const -> module_location;

        auto locate_module(assembly_name          const& requesting_assembly,
                           core::string_reference const& module_name) const -> module_location;

    private:

        search_path_sequence _sequence;
    };

} }

#endif
