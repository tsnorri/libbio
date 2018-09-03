/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BITS_HH
#define LIBBIO_BITS_HH

#include <cstdint>


namespace libbio {
	
	
	inline std::uint32_t trailing_zeros(unsigned int const i)
	{
		if (0 == i) return 0;
		return __builtin_ctz(i);
	}
	
	inline std::uint32_t trailing_zeros(unsigned long const l)
	{
		if (0 == l) return 0;
		return __builtin_ctzl(l);
	}
	
	inline std::uint32_t trailing_zeros(unsigned long long const ll)
	{
		if (0 == ll) return 0;
		return __builtin_ctzll(ll);
	}
	
	
	inline std::uint32_t leading_zeros(unsigned int const i)
	{
		if (0 == i) return (CHAR_BIT * sizeof(unsigned int));
		return __builtin_clz(i);
	}
	
	inline std::uint32_t leading_zeros(unsigned long const l)
	{
		if (0 == l) return (CHAR_BIT * sizeof(unsigned long));
		return __builtin_clzl(l);
	}
	
	inline std::uint32_t leading_zeros(unsigned long long const ll)
	{
		if (0 == ll) return (CHAR_BIT * sizeof(unsigned long long));
		return __builtin_clzll(ll);
	}
}

#endif
