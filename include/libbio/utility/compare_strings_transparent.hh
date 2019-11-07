/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_COMPARE_STRINGS_TRANSPARENT_HH
#define LIBBIO_UTILITY_COMPARE_STRINGS_TRANSPARENT_HH

#include <cstring>
#include <libbio/cxxcompat.hh>
#include <string>
#include <type_traits>


namespace libbio { namespace detail {
	struct compare_char_span
	{
		template <typename t_char, std::size_t t_extent>
		static inline bool compare(std::string const &lhs, std::span <t_char, t_extent> const &rhs)
		{
			return std::string_view(lhs) < std::string_view(rhs.data(), rhs.size());
		}
		
		template <typename t_char, std::size_t t_extent>
		static inline bool compare(std::span <t_char, t_extent> const &lhs, std::string const &rhs)
		{
			return std::string_view(lhs.data(), lhs.size()) < std::string_view(rhs);
		}
	};
}}


namespace libbio {
	
	struct compare_strings_transparent
	{
		using is_transparent = std::true_type;
		
		// Strings
		inline bool operator()(std::string const &lhs, std::string const &rhs) const
		{
			return lhs < rhs;
		}
		
		// String views
		inline bool operator()(std::string_view const &lhs, std::string const &rhs) const
		{
			return lhs < std::string_view(rhs);
		}
		
		inline bool operator()(std::string const &lhs, std::string_view const &rhs) const
		{
			return std::string_view(lhs) < rhs;
		}
		
		// Character arrays
		template <std::size_t t_n>
		inline bool operator()(char const (&lhs)[t_n], std::string const &rhs) const
		{
			return std::string_view(lhs, t_n) < std::string_view(rhs);
		}
		
		template <std::size_t t_n>
		inline bool operator()(std::string const &lhs, char const (&rhs)[t_n]) const
		{
			return std::string_view(lhs) < std::string_view(rhs, t_n);
		}
		
		// Span
		template <std::size_t t_n>
		inline bool operator()(std::string const &lhs, std::span <char *, t_n> const &rhs) const
		{
			return detail::compare_char_span::compare(lhs, rhs);
		}
		
		template <std::size_t t_n>
		inline bool operator()(std::string const &lhs, std::span <char const *, t_n> const &rhs) const
		{
			return detail::compare_char_span::compare(lhs, rhs);
		}
		
		template <std::size_t t_n>
		inline bool operator()(std::span <char *, t_n> const &lhs, std::string const &rhs) const
		{
			return detail::compare_char_span::compare(lhs, rhs);
		}
		
		template <std::size_t t_n>
		inline bool operator()(std::span <char const *, t_n> const &lhs, std::string const &rhs) const
		{
			return detail::compare_char_span::compare(lhs, rhs);
		}
		
		// char*
		inline bool operator()(char const *lhs, std::string const &rhs) const
		{
			return std::string_view(lhs, std::strlen(lhs)) < std::string_view(rhs);
		}
		
		inline bool operator()(std::string const &lhs, char const *rhs) const
		{
			return std::string_view(lhs) < std::string_view(rhs, std::strlen(rhs));
		}
	};
}

#endif
