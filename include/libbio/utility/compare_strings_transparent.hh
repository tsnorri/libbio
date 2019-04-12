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
