/*
 * Copyright (c) 2018-2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BITS_HH
#define LIBBIO_BITS_HH

#include <climits>
#include <cstdint>


namespace libbio { namespace bits {
	
	// Starting from the least significant bit position.
	// WARNING: These have been changed s.t. if the parameter has at least one bit set,
	// the returned value is non-zero.
	inline std::uint8_t trailing_zeros(unsigned int const i)
	{
		if (0 == i) return 0;
		return 1 + __builtin_ctz(i);
	}
	
	inline std::uint8_t trailing_zeros(unsigned long const l)
	{
		if (0 == l) return 0;
		return 1 + __builtin_ctzl(l);
	}
	
	inline std::uint8_t trailing_zeros(unsigned long long const ll)
	{
		if (0 == ll) return 0;
		return 1 + __builtin_ctzll(ll);
	}
	
	
	// Starting from the most significant bit position.
	inline std::uint8_t leading_zeros(unsigned int const i)
	{
		if (0 == i) return (CHAR_BIT * sizeof(unsigned int));
		return __builtin_clz(i);
	}
	
	inline std::uint8_t leading_zeros(unsigned long const l)
	{
		if (0 == l) return (CHAR_BIT * sizeof(unsigned long));
		return __builtin_clzl(l);
	}
	
	inline std::uint8_t leading_zeros(unsigned long long const ll)
	{
		if (0 == ll) return (CHAR_BIT * sizeof(unsigned long long));
		return __builtin_clzll(ll);
	}
	
	
	template <typename t_integer>
	inline std::uint8_t highest_bit_set(t_integer const val)
	{
		// Return the 1-based index.
		return CHAR_BIT * sizeof(t_integer) - leading_zeros(val);
	}
}}

#endif
