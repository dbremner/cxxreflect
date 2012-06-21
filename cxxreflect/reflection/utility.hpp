
//                            Copyright James P. McNellis 2011 - 2012.                            //
//                   Distributed under the Boost Software License, Version 1.0.                   //
//     (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)    //

#ifndef CXXREFLECT_REFLECTION_UTILITY_HPP_
#define CXXREFLECT_REFLECTION_UTILITY_HPP_

#include "cxxreflect/metadata/metadata.hpp"

namespace cxxreflect { namespace reflection { namespace detail {

    class metadata_token_default_getter
    {
    public:

        template <typename Member>
        auto operator()(Member const& member) const -> core::size_type
        {
            return member.metadata_token();
        }
    };

    template <typename TokenGetter, typename Comparer>
    class metadata_token_comparer_impl
    {
    public:

        metadata_token_comparer_impl(TokenGetter const& get_token = TokenGetter())
            : _get_token(get_token)
        {
        }

        template <typename Member>
        auto operator()(Member const& lhs, Member const& rhs) const -> bool
        {
            return Comparer()(_get_token(lhs), _get_token(rhs));
        }

    private:

        TokenGetter _get_token;
    };

} } }

namespace cxxreflect { namespace reflection {

    inline auto metadata_token_equal_comparer()
        -> detail::metadata_token_comparer_impl<
            detail::metadata_token_default_getter,
            std::equal_to<core::size_type>
        >
    {
        return detail::metadata_token_comparer_impl<
            detail::metadata_token_default_getter,
            std::equal_to<core::size_type>
        >();
    }

    template <typename TokenGetter>
    inline auto metadata_token_equal_comparer(TokenGetter const& get_token)
        -> detail::metadata_token_comparer_impl<TokenGetter, std::equal_to<core::size_type>>
    {
        return detail::metadata_token_comparer_impl<
            TokenGetter,
            std::equal_to<core::size_type>
        >(get_token);
    }

    inline auto metadata_token_less_than_comparer()
        -> detail::metadata_token_comparer_impl<detail::metadata_token_default_getter, std::less<core::size_type>>
    {
        return detail::metadata_token_comparer_impl<
            detail::metadata_token_default_getter,
            std::less<core::size_type>
        >();
    }

    template <typename TokenGetter>
    inline auto metadata_token_less_than_comparer(TokenGetter const& get_token)
        -> detail::metadata_token_comparer_impl<TokenGetter, std::less<core::size_type>>
    {
        return detail::metadata_token_comparer_impl<
            TokenGetter,
            std::less<core::size_type>
        >(get_token);
    }

} }

#endif 

// AMDG //
