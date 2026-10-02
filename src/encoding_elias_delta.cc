/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bit_writing_stream.hh>
#include <libbio/bits.hh>
#include <libbio/encoding/elias_delta.hh>
#include <libbio/encoding/elias_gamma.hh>
#include <optional>
#include <utility>


namespace libbio::encoding {

	auto elias_delta::decode(std::uint64_t word) const -> std::optional <std::pair <value_type, std::uint8_t>>
	{
		// First decode the gamma encoded exponent.
		elias_gamma const eg{};
		auto const res{eg.decode(word)};
		if (!res) return res;

		auto const res_{*res};
		auto const exponent{res_.first};
		if (! (1U <= exponent && exponent <= 64)) return {std::nullopt};

		auto const exponent_{exponent - 1U};
		auto shift_amt{res_.second};
		word >>= shift_amt;

		auto value{UINT64_C(0x1) << exponent_};
		auto const mask{~(UINT64_C(0xFFFF'FFFF'FFFF'FFFF) << exponent_)};
		value |= word & mask;

		return {{value - 1U, shift_amt + exponent_}};
	}


	auto elias_delta::decode(bit_reading_stream &bs) const -> std::optional <value_type>
	{
		// First decode the gamma encoded exponent.
		elias_gamma const eg{};
		auto const res{eg.decode(bs)};
		if (!res) return res;

		auto const exponent{*res};
		if (! (1U <= exponent && exponent <= 64)) return {std::nullopt};

		auto const exponent_{exponent - 1U};
		if (bs.bits_remaining() < exponent_) return {std::nullopt};

		auto value{UINT64_C(0x1) << exponent_};
		auto const mask{~(UINT64_C(0xFFFF'FFFF'FFFF'FFFF) << exponent_)};
		value |= bs.value() & mask;
		bs >>= exponent_;

		return {value - 1U};
	}
}
