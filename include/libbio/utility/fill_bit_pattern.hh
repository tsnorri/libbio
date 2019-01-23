/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_FILL_BIT_PATTERN_HH
#define LIBBIO_UTILITY_FILL_BIT_PATTERN_HH

#include <climits>
#include <libbio/utility/misc.hh>


namespace libbio { namespace detail {
	
	template <unsigned int t_length_diff>
	struct fill_bit_pattern_helper
	{
		template <unsigned int t_pattern_length, typename t_word>
		static constexpr t_word fill_bit_pattern(t_word pattern)
		{
			pattern |= pattern << t_pattern_length;
		
			typedef fill_bit_pattern_helper <CHAR_BIT * sizeof(t_word) - 2 * t_pattern_length> helper;
			return helper::template fill_bit_pattern <2 * t_pattern_length>(pattern);
		}
	};
	
	template <>
	struct fill_bit_pattern_helper <0>
	{
		template <unsigned int t_pattern_length, typename t_word>
		static constexpr t_word fill_bit_pattern(t_word pattern)
		{
			return pattern;
		}
	};
}}


namespace libbio {

	template <unsigned int t_pattern_length, typename t_word>
	constexpr t_word fill_bit_pattern(t_word pattern)
	{
		typedef detail::fill_bit_pattern_helper <CHAR_BIT * sizeof(t_word) - t_pattern_length> helper;
		return helper::template fill_bit_pattern <t_pattern_length>(pattern);
	}
}

#endif
