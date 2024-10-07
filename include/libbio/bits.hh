/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BITS_HH
#define LIBBIO_BITS_HH

#include <climits>
#include <concepts>
#include <cstdint>
#include <optional>


namespace libbio { namespace bits { namespace detail {
	
	template <typename t_integer>
	constexpr inline std::uint8_t count_bits_set_(t_integer val)
	{
		// Adapted from https://graphics.stanford.edu/~seander/bithacks.html, in public domain.
		val = val - ((val >> 1) & (t_integer(~t_integer(0))/3));
		val = (val & t_integer(~t_integer(0)) / 15 * 3) + ((val >> 2) & t_integer(~t_integer(0)) / 15 * 3);
		val = (val + (val >> 4)) & t_integer(~t_integer(0)) / 255 * 15;
		return t_integer(val * (t_integer(~t_integer(0)) / 255)) >> (sizeof(t_integer) - 1) * CHAR_BIT;
	}
	
	
	inline std::uint8_t count_bits_set(unsigned int const ii)
	{
#if __has_builtin(__builtin_popcount)
		return __builtin_popcount(ii);
#else
		return detail::count_bits_set_(ii);
#endif
	}
	
	inline std::uint8_t count_bits_set(unsigned long const ll)
	{
#if __has_builtin(__builtin_popcountl)
		return __builtin_popcountl(ll);
#else
		return detail::count_bits_set_(ll);
#endif
	}
	
	inline std::uint8_t count_bits_set(unsigned long long const ll)
	{
#if __has_builtin(__builtin_popcountll)
		return __builtin_popcountll(ll);
#else
		return detail::count_bits_set_(ll);
#endif
	}
	
	inline std::uint8_t count_bits_set(unsigned char const ii)  { typedef unsigned int uint; return count_bits_set(uint(ii)); }
	inline std::uint8_t count_bits_set(unsigned short const ii) { typedef unsigned int uint; return count_bits_set(uint(ii)); }
	
	
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
	
	inline std::uint8_t trailing_zeros(unsigned char const ii)  { typedef unsigned int uint; return trailing_zeros(uint(ii)); }
	inline std::uint8_t trailing_zeros(unsigned short const ii) { typedef unsigned int uint; return trailing_zeros(uint(ii)); }
	
	
	// Starting from the most significant bit position.
	inline std::uint8_t leading_zeros(unsigned int const ii)
	{
		if (0 == ii) return (CHAR_BIT * sizeof(unsigned int));
		return __builtin_clz(ii);
	}
	
	inline std::uint8_t leading_zeros(unsigned long const ll)
	{
		if (0 == ll) return (CHAR_BIT * sizeof(unsigned long));
		return __builtin_clzl(ll);
	}
	
	inline std::uint8_t leading_zeros(unsigned long long const ll)
	{
		if (0 == ll) return (CHAR_BIT * sizeof(unsigned long long));
		return __builtin_clzll(ll);
	}
	
	inline std::uint8_t leading_zeros(unsigned char const ii)
	{
		if (0 == ii) return (CHAR_BIT * sizeof(unsigned char));
		return __builtin_clz(ii) - (CHAR_BIT * (sizeof(unsigned int) - sizeof(unsigned char)));
	}
	
	inline std::uint8_t leading_zeros(unsigned short const ii)
	{
		if (0 == ii) return (CHAR_BIT * sizeof(unsigned short));
		return __builtin_clz(ii) - (CHAR_BIT * (sizeof(unsigned int) - sizeof(unsigned short)));
	}
}}}


namespace libbio { namespace bits {
	
#if defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wredundant-consteval-if"
#endif
	template <typename t_integer>
	constexpr std::uint8_t count_bits_set(t_integer const ii)
	{
		if consteval
		{
			return detail::count_bits_set_(ii);
		}
		else
		{
			return detail::count_bits_set(ii);
		}
	}
#if defined(__clang__)
#	pragma clang diagnostic pop
#endif
	
	
	template <typename t_integer>
	constexpr inline std::uint8_t trailing_zeros(t_integer val)
	{
		// Currently we don’t have an efficient implementation without the compiler intrinsic.
		if consteval
		{
			// Use a naïve algorithm.
			std::uint8_t retval{};
			while (0x0 == (0x1 & val))
			{
				++retval;
				val >> 0x1;
			}
			return retval;
		}
		else
		{
			return detail::trailing_zeros(val);
		}
	}
	
	
	template <typename t_integer>
	constexpr inline std::uint8_t leading_zeros(t_integer val)
	{
		// Currently we don’t have an efficient implementation without the compiler intrinsic.
		if consteval
		{
			// Use a naïve algorithm.
			std::uint8_t retval{CHAR_BIT * sizeof(val)};
			while (val)
			{
				--retval;
				val >>= 0x1;
			}
			return retval;
		}
		else
		{
			return detail::leading_zeros(val);
		}
	}
	
	
	template <typename t_integer>
	constexpr inline std::uint8_t highest_bit_set(t_integer const val)
	{
		// Return the 1-based index.
		return CHAR_BIT * sizeof(t_integer) - leading_zeros(val);
	}
	
	
	template <std::unsigned_integral t_value>
	constexpr bool is_power_of_2(t_value const val)
	{
		return 1 == count_bits_set(val);
	}


	template <std::unsigned_integral t_value>
	constexpr std::optional <t_value> gte_power_of_2(t_value const val)
	{
		if (0 == val)
			return {1};
		
		constexpr static t_value const highest_mask{t_value(1) << (sizeof(t_value) * CHAR_BIT - 1)};
		constexpr static t_value const lower_mask{highest_mask - 1};
		if (val & highest_mask && val & lower_mask)
			return {};
	
		auto const hbs(highest_bit_set(val));
		auto const power(t_value(1) << (hbs - 1));
		auto const mask(power - 1);
		if (val & mask)
			return {power << 1};
		return {power};
	}
}}

#endif
