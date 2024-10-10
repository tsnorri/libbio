/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_COMPARE_STRINGS_TRANSPARENT_HH
#define LIBBIO_UTILITY_COMPARE_STRINGS_TRANSPARENT_HH

#include <cstring>
#include <functional>			// std::less, std::equal_to
#include <span>
#include <string>
#include <type_traits>


namespace libbio { namespace detail {
	
	template <typename t_cmp>
	struct compare_char_span
	{
		template <typename t_char, std::size_t t_extent>
		constexpr static inline bool compare(std::string const &lhs, std::span <t_char, t_extent> const &rhs)
		{
			t_cmp cmp;
			return cmp(std::string_view(lhs), std::string_view(rhs.data(), rhs.size()));
		}
		
		template <typename t_char, std::size_t t_extent>
		constexpr static inline bool compare(std::span <t_char, t_extent> const &lhs, std::string const &rhs)
		{
			t_cmp cmp;
			return cmp(std::string_view(lhs.data(), lhs.size()), std::string_view(rhs));
		}
	};
}}


namespace libbio {
	
	template <typename t_cmp>
	struct compare_strings_transparent_tpl
	{
		using is_transparent = std::true_type;
		
		// Strings
		constexpr inline bool operator()(std::string const &lhs, std::string const &rhs) const
		{
			t_cmp cmp;
			return cmp(lhs, rhs);
		}
		
		// String views
		constexpr inline bool operator()(std::string_view const &lhs, std::string const &rhs) const
		{
			t_cmp cmp;
			return cmp(lhs, std::string_view(rhs));
		}
		
		constexpr inline bool operator()(std::string const &lhs, std::string_view const &rhs) const
		{
			t_cmp cmp;
			return cmp(std::string_view(lhs), rhs);
		}
		
		// Character arrays
		template <std::size_t t_n>
		constexpr inline bool operator()(char const (&lhs)[t_n], std::string const &rhs) const
		{
			t_cmp cmp;
			return cmp(std::string_view(lhs, t_n), std::string_view(rhs));
		}
		
		template <std::size_t t_n>
		constexpr inline bool operator()(std::string const &lhs, char const (&rhs)[t_n]) const
		{
			t_cmp cmp;
			return cmp(std::string_view(lhs), std::string_view(rhs, t_n));
		}
		
		// Span
		template <std::size_t t_n>
		constexpr inline bool operator()(std::string const &lhs, std::span <char, t_n> const &rhs) const
		{
			return detail::compare_char_span <t_cmp>::compare(lhs, rhs);
		}
		
		template <std::size_t t_n>
		constexpr inline bool operator()(std::string const &lhs, std::span <char const, t_n> const &rhs) const
		{
			return detail::compare_char_span <t_cmp>::compare(lhs, rhs);
		}
		
		template <std::size_t t_n>
		constexpr inline bool operator()(std::span <char, t_n> const &lhs, std::string const &rhs) const
		{
			return detail::compare_char_span <t_cmp>::compare(lhs, rhs);
		}
		
		template <std::size_t t_n>
		constexpr inline bool operator()(std::span <char const, t_n> const &lhs, std::string const &rhs) const
		{
			return detail::compare_char_span <t_cmp>::compare(lhs, rhs);
		}
		
		// char*
		constexpr inline bool operator()(char const *lhs, std::string const &rhs) const
		{
			t_cmp cmp;
			return cmp(std::string_view(lhs, std::strlen(lhs)), std::string_view(rhs));
		}
		
		constexpr inline bool operator()(std::string const &lhs, char const *rhs) const
		{
			t_cmp cmp;
			return cmp(std::string_view(lhs), std::string_view(rhs, std::strlen(rhs)));
		}
	};
	
	
	typedef compare_strings_transparent_tpl <std::less <void>>		compare_strings_transparent;
	typedef compare_strings_transparent_tpl <std::equal_to <void>>	string_equal_to_transparent;
}

#endif
