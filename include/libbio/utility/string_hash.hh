/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_STRING_HASH_HH
#define LIBBIO_UTILITY_STRING_HASH_HH

#include <cstring>
#include <libbio/cxxcompat.hh>
#include <string>
#include <type_traits>


namespace libbio {
	
	// Calculate the hash value using an underlying function template.
	template <template <typename> typename t_hash>
	struct string_hash_transparent_tpl
	{
		using is_transparent = std::true_type;
		
		// Strings
		constexpr inline auto operator()(std::string const &str) const
		{
			// The standard guarantees that since C++17, given a string S and a string view SV,
			// their hash values retrieved from std::hash equal iff. S == SV.
			// See https://en.cppreference.com/w/cpp/string/basic_string/hash
			t_hash <std::string> hash;
			return hash(str);
		}
		
		// String views
		constexpr inline auto operator()(std::string_view const &str) const
		{
			t_hash <std::string_view> hash;
			return hash(str);
		}
		
		// Character arrays
		template <std::size_t t_n>
		constexpr inline auto operator()(char const (&str)[t_n]) const
		{
			t_hash <std::string_view> hash;
			return hash(str);
		}
		
		// Span
		template <std::size_t t_n>
		constexpr inline auto operator()(std::span <char, t_n> const &str) const
		{
			t_hash <std::string_view> hash;
			return hash(std::string_view(str.data(), str.size()));
		}
		
		template <std::size_t t_n>
		constexpr inline auto operator()(std::span <char const, t_n> const &str) const
		{
			t_hash <std::string_view> hash;
			return hash(std::string_view(str.data(), str.size()));
		}
		
		// char*
		constexpr inline auto operator()(char const *str) const
		{
			t_hash <std::string_view> hash;
			return hash(std::string_view(str, std::strlen(str)));
		}
	};
	
	
	typedef string_hash_transparent_tpl <std::hash>	string_hash_transparent;
}

#endif
