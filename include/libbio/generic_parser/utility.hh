/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_UTILITY_HH
#define LIBBIO_GENERIC_PARSER_UTILITY_HH


namespace libbio::parsing::detail {
	
	template <typename t_value>
	bool is_printable(t_value const cc)
	{
		return ' ' < cc && cc < '~';
	}
	
	
	template <typename t_type> struct is_character_type		{ constexpr static inline bool value{false}; };
	template <> struct is_character_type <char>				{ constexpr static inline bool value{true}; };
	template <> struct is_character_type <signed char>		{ constexpr static inline bool value{true}; };
	template <> struct is_character_type <unsigned char>	{ constexpr static inline bool value{true}; };
	
	template <typename t_type>
	constexpr static inline bool is_character_type_v{is_character_type <t_type>::value};
}

#endif
