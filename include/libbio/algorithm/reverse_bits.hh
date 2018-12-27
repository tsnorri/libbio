/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_REVERSE_BITS_HH
#define LIBBIO_ALGORITHM_REVERSE_BITS_HH

#include <climits>
#include <cstdint>


namespace libbio {
	
	template <unsigned int t_bits, typename t_word>
	inline constexpr t_word reverse_bits(t_word const rr)
	{
		enum { WORD_BITS = sizeof(t_word) * CHAR_BIT };
		static_assert(1 == t_bits || 2 == t_bits || 4 == t_bits || 8 == t_bits || 16 == t_bits || 32 == t_bits || 64 == t_bits);
		static_assert(t_bits <= WORD_BITS);
		
		std::uint64_t r(rr);
		
		// Swap n-tuples of bits in order and return early if t_word size has been reached.
		if constexpr (t_bits <= 1)
			r = ((r & 0x5555555555555555UL) <<  1) | ((r & 0xaaaaaaaaaaaaaaaaUL) >> 1);		// Swap adjacent bits.
		
		if constexpr (t_bits <= 2)
			r = ((r & 0x3333333333333333UL) <<  2) | ((r & 0xccccccccccccccccUL) >> 2);		// Swap adjacent pairs.
		
		if constexpr (t_bits <= 4)
			r = ((r & 0x0f0f0f0f0f0f0f0fUL) <<  4) | ((r & 0xf0f0f0f0f0f0f0f0UL) >> 4);		// Swap adjacent 4's.
		
		if constexpr (8 == WORD_BITS) return r;
		
		if constexpr (t_bits <= 8)
			r = ((r & 0x00ff00ff00ff00ffUL) <<  8) | ((r & 0xff00ff00ff00ff00UL) >> 8);		// Swap adjacent 8's.
			
		if constexpr (16 == WORD_BITS) return r;
		
		if constexpr (t_bits <= 16)
			r = ((r & 0x0000ffff0000ffffUL) << 16) | ((r & 0xffff0000ffff0000UL) >> 16);	// Swap adjacent 16's.
		
		if constexpr (32 == WORD_BITS) return r;
		
		if constexpr (t_bits <= 32)
			r = ((r & 0x00000000ffffffffUL) << 32) | ((r & 0xffffffff00000000UL) >> 32);	// Swap adjacent 32's.
		
		return r;
	}
}

#endif
