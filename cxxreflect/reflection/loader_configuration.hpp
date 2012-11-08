
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_LOADER_CONFIGURATION_HPP_
#define CXXREFLECT_REFLECTION_LOADER_CONFIGURATION_HPP_

#include "cxxreflect/reflection/detail/forward_declarations.hpp"





namespace cxxreflect { namespace reflection { namespace detail {

    typedef std::unique_ptr<class base_loader_configuration> unique_base_loader_configuration;

    class base_loader_configuration
    {
    public:

        virtual auto is_filtered_type(metadata::type_def_token const& token) const -> bool = 0;

        virtual auto system_namespace() const -> core::string_reference = 0;

        virtual auto copy() const -> unique_base_loader_configuration = 0;

        virtual ~base_loader_configuration() { }
    };

    template <typename T>
    class derived_loader_configuration : public base_loader_configuration
    {
    public:

        template <typename U>
        derived_loader_configuration(U&& x)
            : _x(std::forward<U>(x))
        {
        }

        virtual auto is_filtered_type(metadata::type_def_token const& token) const -> bool
        {
            return _x.is_filtered_type(token);
        }

        virtual auto system_namespace() const -> core::string_reference
        {
            return _x.system_namespace();
        }

        virtual auto copy() const -> unique_base_loader_configuration
        {
            return core::make_unique<derived_loader_configuration>(_x);
        }

    private:

        T _x;
    };

} } }





namespace cxxreflect { namespace reflection {

    class loader_configuration
    {
    public:

        loader_configuration();

        template <typename T>
        loader_configuration(T x)
            : _x(core::make_unique<detail::derived_loader_configuration<T>>(x))
        {
        }

        loader_configuration(loader_configuration const&);
        loader_configuration(loader_configuration&&);

        auto operator=(loader_configuration const&) -> loader_configuration&;
        auto operator=(loader_configuration&&)      -> loader_configuration&;

        auto is_filtered_type(metadata::type_def_token const& token) const -> bool;
        auto system_namespace() const -> core::string_reference;

        auto is_initialized() const -> bool;

    private:

        detail::unique_base_loader_configuration _x;
    };





    class loader_configuration_all_types_filter_policy
    {
    public:

        auto is_filtered_type(metadata::type_def_token const& token) const -> bool;
    };

    class loader_configuration_public_types_filter_policy
    {
    public:

        auto is_filtered_type(metadata::type_def_token const& token) const -> bool;
    };

    class loader_configuration_system_system_namespace_policy
    {
    public:

        auto system_namespace() const -> core::string_reference;
    };





    class default_loader_configuration
        : public loader_configuration_all_types_filter_policy,
          public loader_configuration_system_system_namespace_policy
    {
    };

} }

#endif
