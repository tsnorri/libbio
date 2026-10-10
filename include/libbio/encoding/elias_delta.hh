/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ENCODING_ELIAS_DELTA_HH
#define LIBBIO_ENCODING_ELIAS_DELTA_HH

#include <array>
#include <cstdint>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bit_writing_stream.hh>
#include <libbio/bits.hh>
#include <libbio/encoding/detail.hh>
#include <libbio/encoding/elias_gamma.hh>
#include <limits>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>


namespace libbio::encoding::detail {

	typedef std::uint64_t elias_delta_value_type;
	typedef std::make_signed_t <elias_delta_value_type> elias_delta_signed_value_type;
}


namespace libbio::encoding {

	// The values are encoded in such a way that the gamma encoded exponent
	// is written to the less significant bits of the stream.
	template <bool t_max_encoded_size_is_64_bits>
	struct elias_delta_tpl
	{
		typedef detail::elias_delta_value_type value_type;
		typedef detail::elias_delta_signed_value_type signed_value_type;

		constexpr static inline bool max_encoded_size_is_64_bits{t_max_encoded_size_is_64_bits};

		constexpr static inline value_type max_value{
			max_encoded_size_is_64_bits
			? ((value_type{1} << value_type{54}) - 2)
			: std::numeric_limits <value_type>::max() - 2
		};

		constexpr static inline std::uint8_t max_exponent{
			max_encoded_size_is_64_bits
			? 53
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

	typedef elias_delta_tpl <true> elias_delta;


	template <bool t_max_encoded_size_is_64_bits>
	template <typename t_target, detail::elias_delta_signed_value_type t_diff>
	bool elias_delta_tpl <t_max_encoded_size_is_64_bits>::encode(
		bit_writing_stream <t_target> &bs,
		value_type value,
		signed_value_constant <t_diff>
	) const
	{
		auto const value1{detail::add_signed_constant_to_unsigned <value_type, t_diff>(value)};
		auto const idx1{bits::highest_bit_set(value1)}; // 1-based.
		auto const idx{idx1 - 1U};

		// First encode the value of the exponent plus one.
		elias_gamma const eg{};
		if (!eg.encode(bs, idx1, elias_gamma::signed_value_constant <0>{})) return false;

		value_type const mask{~((~(value_type{})) << idx)};
		auto const lower{value1 & mask};
		return bs.write_bits(lower, idx);
	}


	template <bool t_max_encoded_size_is_64_bits>
	template <detail::elias_delta_signed_value_type t_diff>
	auto elias_delta_tpl <t_max_encoded_size_is_64_bits>::decode(
		std::uint64_t word,
		signed_value_constant <t_diff>
	) const -> decode_word_return_type
	requires (max_encoded_size_is_64_bits)
	{
		// First decode the gamma encoded exponent.
		elias_gamma const eg{};
		auto const res{eg.decode(word, elias_gamma::signed_value_constant <0>{})};
		if (!res) return res;

		auto const res_{*res};
		auto const exponent{res_.first};
		if (max_exponent < exponent) return {std::nullopt};

		auto const exponent_{exponent - 1U};
		auto shift_amt{res_.second};
		word >>= shift_amt;

		value_type value{value_type{1} << exponent_};
		value_type const mask{~((~(value_type{})) << exponent_)};
		value |= word & mask;

		return {{detail::add_signed_constant_to_unsigned <value_type, t_diff>(value), shift_amt + exponent_}};
	}


	template <bool t_max_encoded_size_is_64_bits>
	template <detail::elias_delta_signed_value_type t_diff>
	auto elias_delta_tpl <t_max_encoded_size_is_64_bits>::decode(
		bit_reading_stream &bs,
		signed_value_constant <t_diff>
	) const -> decode_return_type
	{
		std::array <value_type, max_encoded_size_is_64_bits ? 2 : 3> buffer{};
		bs.copy_to(std::span{buffer});

		// First decode the gamma encoded exponent.
		elias_gamma const eg{};
		auto const res{eg.decode(buffer.front(), elias_gamma::signed_value_constant <0>{})};
		if (!res) return {std::nullopt};

		// Check that we got a valid value.
		auto const res_{*res};
		auto const exponent{res_.first};
		if (max_exponent < exponent) return {std::nullopt};

		auto const exponent_{exponent - 1U};
		auto const exponent_bits{res_.second};

		// Shift and check.
		bs >>= exponent_bits;
		if (bs.bits_remaining() < exponent_) return {std::nullopt};
		bits::shift_span_right(std::span{buffer}, exponent_bits);

		// Construct the decoded value.
		value_type value{value_type{1} << exponent_};
		auto const mask{~((~(value_type{})) << exponent_)};
		value |= buffer.front() & mask;
		bs >>= exponent_;

		return {detail::add_signed_constant_to_unsigned <value_type, t_diff>(value)};
	}
}

#endif
