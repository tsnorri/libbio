/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_UTILITY_HH
#define LIBBIO_GENERIC_PARSER_UTILITY_HH

#include <type_traits> // std::false_type, std::true_type


namespace libbio::parsing::detail {
	
	template <typename t_value>
	bool is_printable(t_value const cc)
	{
		return ' ' < cc && cc < '~';
	}
	
	
	template <typename t_type> struct is_character_type		: public std::false_type {};
	template <> struct is_character_type <char>				: public std::true_type {};
	template <> struct is_character_type <signed char>		: public std::true_type {};
	template <> struct is_character_type <unsigned char>	: public std::true_type {};
	
	template <typename t_type>
	constexpr static inline auto const is_character_type_v{is_character_type <t_type>::value};
}

#endif
