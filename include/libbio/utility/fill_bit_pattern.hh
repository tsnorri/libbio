/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_UTILITY_FILL_BIT_PATTERN_HH
#define LIBBIO_UTILITY_FILL_BIT_PATTERN_HH

#include <climits>
#include <cstdint>
#include <libbio/utility/misc.hh>


namespace libbio::detail {
	
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
}


namespace libbio {

	template <unsigned int t_pattern_length, typename t_word>
	constexpr t_word fill_bit_pattern(t_word pattern)
	{
		typedef detail::fill_bit_pattern_helper <CHAR_BIT * sizeof(t_word) - t_pattern_length> helper;
		return helper::template fill_bit_pattern <t_pattern_length>(pattern);
	}
	
	
	template <typename t_word>
	inline t_word fill_bit_pattern(t_word pattern, std::uint8_t const pattern_length)
	{
		switch (pattern_length)
		{
			case 0:
				return pattern;
			case 1:
				return fill_bit_pattern <1>(pattern);
			case 2:
				return fill_bit_pattern <2>(pattern);
			case 4:
				return fill_bit_pattern <4>(pattern);
			case 8:
				return fill_bit_pattern <8>(pattern);
			case 16:
				return fill_bit_pattern <16>(pattern);
			case 32:
				return fill_bit_pattern <32>(pattern);
			case 64:
				return pattern;
			default:
				throw std::runtime_error("Unexpected pattern length");
		}
	}
}

#endif
