/*
 * Copyright (c) 2018-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_BITS_HH
#define LIBBIO_BITS_HH

#include <algorithm>
#include <bit>
#include <climits>
#include <cstddef>
#include <concepts>
#include <cstdint>
#include <span>
#include <stdexcept>	// std::range_error
#include <type_traits>


namespace libbio::bits::detail {

	template <std::unsigned_integral t_integer>
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
	inline std::uint8_t trailing_zeros(unsigned int const i)
	{
		if (0 == i) return (CHAR_BIT * sizeof(unsigned int));
		return __builtin_ctz(i);
	}

	inline std::uint8_t trailing_zeros(unsigned long const l)
	{
		if (0 == l) return (CHAR_BIT * sizeof(unsigned long));
		return __builtin_ctzl(l);
	}

	inline std::uint8_t trailing_zeros(unsigned long long const ll)
	{
		if (0 == ll) return (CHAR_BIT * sizeof(unsigned long long));
		return __builtin_ctzll(ll);
	}

	inline std::uint8_t trailing_zeros_(unsigned int const i)
	{
		if (0 == i) return 0;
		return 1 + __builtin_ctz(i);
	}

	inline std::uint8_t trailing_zeros_(unsigned long const l)
	{
		if (0 == l) return 0;
		return 1 + __builtin_ctzl(l);
	}

	inline std::uint8_t trailing_zeros_(unsigned long long const ll)
	{
		if (0 == ll) return 0;
		return 1 + __builtin_ctzll(ll);
	}

	inline std::uint8_t trailing_zeros(unsigned char const ii)  { typedef unsigned int uint; return trailing_zeros(uint(ii)); }
	inline std::uint8_t trailing_zeros(unsigned short const ii) { typedef unsigned int uint; return trailing_zeros(uint(ii)); }
	inline std::uint8_t trailing_zeros_(unsigned char const ii)  { typedef unsigned int uint; return trailing_zeros_(uint(ii)); }
	inline std::uint8_t trailing_zeros_(unsigned short const ii) { typedef unsigned int uint; return trailing_zeros_(uint(ii)); }


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
}


namespace libbio::bits {

#if defined(__clang__)
#	pragma clang diagnostic push
#	pragma clang diagnostic ignored "-Wredundant-consteval-if"
#endif
	template <std::unsigned_integral t_integer>
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


	template <std::unsigned_integral t_integer>
	constexpr inline std::uint8_t trailing_zeros(t_integer val)
	{
		// Currently we don’t have an efficient implementation without the compiler intrinsic.
		if consteval
		{
			// Use a naïve algorithm.
			std::uint8_t retval{};
			for (std::uint8_t ii{}; ii < CHAR_BIT * sizeof(t_integer); ++ii)
			{
				if (0x0 == (0x1 & val))
					break;

				++retval;
				val >>= 0x1;
			}
			return retval;
		}
		else
		{
			return detail::trailing_zeros(val);
		}
	}


	template <std::unsigned_integral t_integer>
	constexpr inline std::uint8_t trailing_zeros_(t_integer val)
	{
		// Currently we don’t have an efficient implementation without the compiler intrinsic.
		if consteval
		{
			if (!val) return 0;

			// Use a naïve algorithm.
			std::uint8_t retval{1};
			for (std::uint8_t ii{}; ii < CHAR_BIT * sizeof(t_integer); ++ii)
			{
				if (0x0 == (0x1 & val))
					break;

				++retval;
				val >>= 0x1;
			}
			return retval;
		}
		else
		{
			return detail::trailing_zeros_(val);
		}
	}


	template <std::unsigned_integral t_integer>
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


	template <std::unsigned_integral t_integer>
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
	constexpr t_value gte_power_of_2(t_value const val)
	{
		if (0 == val)
			return 1;

		constexpr static t_value const highest_mask{t_value(1) << (sizeof(t_value) * CHAR_BIT - 1)};
		constexpr static t_value const lower_mask{highest_mask - 1};
		if (val & highest_mask && val & lower_mask)
			return 0;

		auto const hbs(highest_bit_set(val));
		auto const power(t_value(1) << (hbs - 1));
		auto const mask(power - 1);
		if (val & mask)
			return power << 1;
		return power;
	}

	template <std::unsigned_integral t_value>
	constexpr t_value gte_power_of_2_(t_value const val)
	{
		auto const retval(gte_power_of_2(val));
		if (!retval)
			throw std::range_error("Unable to calculate the power of two");
		return retval;
	}


	template <std::unsigned_integral t_value, std::size_t t_size, bool t_can_shift_whole_words = false>
	constexpr void shift_span_left(
		std::span <t_value, t_size> span,
		std::uint8_t shift_amt,
		std::bool_constant <t_can_shift_whole_words> = std::bool_constant <t_can_shift_whole_words>{}
	)
	{
		constexpr auto const value_bits{CHAR_BIT * sizeof(t_value)};

		if (0 == shift_amt) return;

		std::size_t start{};
		if (t_can_shift_whole_words && value_bits <= shift_amt)
		{
			// Shift by this many whole words.
			auto const word_count{shift_amt / value_bits};

			// Check if we can just zero-fill the span.
			if (span.size() <= word_count)
			{
				std::fill(span.begin(), span.end(), 0);
				return;
			}

			// We may have some bits to shift.
			// Handle the whole words first.
			std::copy_backward(span.begin(), span.end() - word_count, span.end());
			start = word_count;

			// Fill the start with zeros.
			std::fill(span.begin(), span.begin() + word_count, 0);

			shift_amt %= value_bits;
			if (0 == shift_amt)
				return;
		}

		t_value const higher_mask{(~(t_value{})) << shift_amt};
		t_value const lower_mask{~higher_mask};
		t_value prev{};
		for (std::size_t ii{start}; ii < span.size(); ++ii)
		{
			span[ii] = std::rotl(span[ii], shift_amt);
			auto const next{span[ii] & lower_mask};
			span[ii] &= higher_mask;
			span[ii] |= prev;
			prev = next;
		}
	}


	template <std::unsigned_integral t_value, std::size_t t_size, bool t_can_shift_whole_words = false>
	constexpr void shift_span_right(
		std::span <t_value, t_size> span,
		std::uint8_t shift_amt,
		std::bool_constant <t_can_shift_whole_words> = std::bool_constant <t_can_shift_whole_words>{}
	)
	{
		constexpr auto const value_bits{CHAR_BIT * sizeof(t_value)};

		if (0 == shift_amt) return;

		std::size_t start{span.size()};
		if (t_can_shift_whole_words && value_bits <= shift_amt)
		{
			// Shift by this many whole words.
			auto const word_count{shift_amt / value_bits};

			// Check if we can just zero-fill the span.
			if (span.size() <= word_count)
			{
				std::fill(span.begin(), span.end(), 0);
				return;
			}

			// We may have some bits to shift.
			// Handle the whole words first.
			std::copy(span.begin() + word_count, span.end(), span.begin());
			start -= word_count;

			// Fill the rest with zeros.
			std::fill(span.begin() + word_count, span.end(), 0);

			shift_amt %= value_bits;
			if (0 == shift_amt)
				return;
		}

		t_value const higher_mask{(~(t_value{})) << shift_amt};
		t_value const lower_mask{~higher_mask};
		t_value prev{};
		for (std::size_t ii{start}; 0 < ii; --ii)
		{
			auto const ii_{ii - 1};
			span[ii_] = std::rotr(span[ii_], shift_amt);
			auto const next{span[ii_] & higher_mask};
			span[ii_] &= lower_mask;
			span[ii_] |= prev;
			prev = next;
		}
	}
}

#endif
