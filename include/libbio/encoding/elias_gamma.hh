/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ENCODING_ELIAS_GAMMA_HH
#define LIBBIO_ENCODING_ELIAS_GAMMA_HH

#include <array>
#include <cstdint>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bit_writing_stream.hh>
#include <libbio/bits.hh>
#include <libbio/encoding/detail.hh>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>


namespace libbio::encoding::detail {

	typedef std::uint64_t elias_gamma_value_type;
	typedef std::make_signed_t <elias_gamma_value_type> elias_gamma_signed_value_type;
}


namespace libbio::encoding {

	// The values are encoded in such a way that the zero bits of the encoded value
	// are written to the less significant bits of the stream.
	template <bool t_max_encoded_size_is_64_bits>
	struct elias_gamma_tpl
	{
		typedef detail::elias_gamma_value_type value_type;
		typedef detail::elias_gamma_signed_value_type signed_value_type;

		constexpr static inline bool max_encoded_size_is_64_bits{t_max_encoded_size_is_64_bits};

		constexpr static inline value_type max_value{
			max_encoded_size_is_64_bits
			? ((value_type{1} << value_type{31}) - 2)
			: std::numeric_limits <value_type>::max() - 2
		};

		constexpr static inline std::uint8_t max_exponent{
			max_encoded_size_is_64_bits
			? 31
			: 63
		};

		template <signed_value_type t_value>
		using signed_value_constant = std::integral_constant <signed_value_type, t_value>;

		typedef std::optional <std::pair <value_type, std::uint8_t>> decode_word_return_type;
		typedef std::optional <value_type> decode_return_type;

		template <typename t_target, signed_value_type t_diff = 1>
		bool encode(
			bit_writing_stream <t_target> &bs,
			value_type value,
			signed_value_constant <t_diff> = signed_value_constant <t_diff>{}
		) const;

		template <signed_value_type t_diff = -1>
		inline decode_word_return_type
		decode(
			std::uint64_t word,
			signed_value_constant <t_diff> = signed_value_constant <t_diff>{}
		) const
		requires (max_encoded_size_is_64_bits);

		template <signed_value_type t_diff = -1>
		inline decode_return_type
		decode(
			bit_reading_stream &bs,
			signed_value_constant <t_diff> = signed_value_constant <t_diff>{}
		) const;
	};

	typedef elias_gamma_tpl <true> elias_gamma;


	template <bool t_max_encoded_size_is_64_bits>
	template <typename t_target, detail::elias_gamma_signed_value_type t_diff>
	bool elias_gamma_tpl <t_max_encoded_size_is_64_bits>::encode(
		bit_writing_stream <t_target> &bs,
		value_type value,
		signed_value_constant <t_diff>
	) const
	{
		auto const value1{detail::add_signed_constant_to_unsigned <value_type, t_diff>(value)};
		auto const idx1{bits::highest_bit_set(value1)}; // 1-based.
		auto const idx{idx1 - 1U};

		value_type const mask{~((~(value_type{})) << idx)};
		value_type const lower{((value1 & mask) << 1U) | value_type{1}};

		if (bs.write_zeros(idx) && bs.write_bits(lower, idx1))
			return true;

		return false;
	}


	template <bool t_max_encoded_size_is_64_bits>
	template <detail::elias_gamma_signed_value_type t_diff>
	auto elias_gamma_tpl <t_max_encoded_size_is_64_bits>::decode(
		std::uint64_t word,
		signed_value_constant <t_diff>
	) const -> decode_word_return_type
	requires (max_encoded_size_is_64_bits)
	{
		// There should be at most 31 trailing zeros since we decode only one word.
		auto const tzc{bits::trailing_zeros(word)};
		if (max_exponent < tzc) return {std::nullopt};

		// Shift by the number of zeros.
		word >>= tzc + 1;
		value_type value{value_type{1} << tzc};
		value_type const mask{~((~(value_type{})) << tzc)};
		value |= word & mask;

		return {{detail::add_signed_constant_to_unsigned <value_type, t_diff>(value), 2 * tzc + 1}};
	}


	template <bool t_max_encoded_size_is_64_bits>
	template <detail::elias_gamma_signed_value_type t_diff>
	auto elias_gamma_tpl <t_max_encoded_size_is_64_bits>::decode(
		bit_reading_stream &bs,
		signed_value_constant <t_diff>
	) const -> decode_return_type
	{
		// Check if we can read from the stream.
		if (!bs) return {std::nullopt};

		std::array <value_type, 2> buffer{};
		bs.copy_to(std::span{buffer});

		// There should be at most value_bits - 1 trailing zeros.
		auto const tzc{bits::trailing_zeros(buffer.front())};
		if (max_exponent < tzc) return {std::nullopt};

		// Try to shift by the number of zeros.
		bs >>= tzc + 1;
		if (bs.bits_remaining() < tzc) return {std::nullopt};

		// Success; construct the value.
		bits::shift_span_right(std::span{buffer}, tzc + 1, std::true_type{});
		auto const mask{~((~(value_type{})) << tzc)};

		value_type value{value_type{1} << tzc};
		value |= buffer.front() & mask;

		bs >>= tzc;

		return {detail::add_signed_constant_to_unsigned <value_type, t_diff>(value)};
	}
}

#endif
