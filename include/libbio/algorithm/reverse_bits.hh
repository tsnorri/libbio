/*
 * Copyright (c) 2018, 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ALGORITHM_REVERSE_BITS_HH
#define LIBBIO_ALGORITHM_REVERSE_BITS_HH

#include <bit>
#include <climits>
#include <cstdint>


namespace libbio {

	template <unsigned int t_bits, typename t_word>
	constexpr inline t_word reverse_bits(t_word const rr)
	{
		enum { WORD_BITS = sizeof(t_word) * CHAR_BIT };
		static_assert(1 == t_bits || 2 == t_bits || 4 == t_bits || 8 == t_bits || 16 == t_bits || 32 == t_bits || 64 == t_bits);
		static_assert(t_bits <= WORD_BITS);

		std::uint64_t r(rr);

		// Swap n-tuples of bits in order and return early if t_word size has been reached.
		if constexpr (t_bits <= 1)
			r = ((r & UINT64_C(0x5555'5555'5555'5555)) <<  1U) | ((r & UINT64_C(0xaaaa'aaaa'aaaa'aaaa)) >> 1U);		// Swap adjacent bits.

		if constexpr (t_bits <= 2)
			r = ((r & UINT64_C(0x3333'3333'3333'3333)) <<  2U) | ((r & UINT64_C(0xcccc'cccc'cccc'cccc)) >> 2U);		// Swap adjacent pairs.

		if constexpr (t_bits <= 4)
			r = ((r & UINT64_C(0x0f0f'0f0f'0f0f'0f0f)) <<  4U) | ((r & UINT64_C(0xf0f0'f0f0'f0f0'f0f0)) >> 4U);		// Swap adjacent 4's.

		if constexpr (8 == WORD_BITS) return r;

		if constexpr (t_bits <= 8)
			r = ((r & UINT64_C(0x00ff'00ff'00ff'00ff)) <<  8U) | ((r & UINT64_C(0xff00'ff00'ff00'ff00)) >> 8U);		// Swap adjacent 8's.

		if constexpr (16 == WORD_BITS) return r;

		if constexpr (t_bits <= 16)
			r = ((r & UINT64_C(0x0000'ffff'0000'ffff)) << 16U) | ((r & UINT64_C(0xffff'0000'ffff'0000)) >> 16U);	// Swap adjacent 16's.

		if constexpr (32 == WORD_BITS) return r;

		if constexpr (t_bits <= 32)
			r = ((r & UINT64_C(0x0000'0000'ffff'ffff)) << 32U) | ((r & UINT64_C(0xffff'ffff'0000'0000)) >> 32U);	// Swap adjacent 32's.

		return r;
	}


	constexpr inline std::uint64_t reverse_bits(std::uint64_t cc)
	{
		// First reverse the 4-byte words, then the 2-byte words, then
		// // bytes, nibbles, groups of 2 bits and finally single bits.
		//
		// For x86-64, Clang 21.1.0 produces 20 instructions (excluding ret)
		// for the extern version of this function, GCC 15 a few more.
		// (The first two swaps get replaced by something apparently more
		// efficient.) For ARM, both compilers produce a single instruction,
		// rbit.

		cc = std::rotr(cc, 32);

		{
			auto const mask{UINT64_C(0x0000'FFFF'0000'FFFF)};
			cc = ((mask & cc) << 16U) | ((~mask & cc) >> 16U);
		}

		{
			auto const mask{UINT64_C(0x00FF'00FF'00FF'00FF)};
			cc = ((mask & cc) << 8U) | ((~mask & cc) >> 8U);
		}

		{
			auto const mask{UINT64_C(0x0F0F'0F0F'0F0F'0F0F)};
			cc = ((mask & cc) << 4U) | ((~mask & cc) >> 4U);
		}

		{
			auto const mask{UINT64_C(0x3333'3333'3333'3333)};
			cc = ((mask & cc) << 2U) | ((~mask & cc) >> 2U);
		}

		{
			auto const mask{UINT64_C(0x5555'5555'5555'5555)};
			cc = ((mask & cc) << 1U) | ((~mask & cc) >> 1U);
		}

		return cc;
	}
}

#endif
