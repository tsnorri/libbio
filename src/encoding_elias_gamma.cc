/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdint>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bit_writing_stream.hh>
#include <libbio/bits.hh>
#include <libbio/encoding/elias_gamma.hh>
#include <optional>
#include <utility>


namespace libbio::encoding {

	auto elias_gamma::decode(std::uint64_t word) const -> std::optional <std::pair <value_type, std::uint8_t>>
	{
		// There should be at most 63 trailing zeros.
		auto const tzc{bits::trailing_zeros(word)};
		if (64U <= tzc) return {std::nullopt};

		// Shift by the number of zeros.
		word >>= tzc + 1;
		auto value{UINT64_C(0x1) << tzc};
		auto const mask{~(UINT64_C(0xFFFF'FFFF'FFFF'FFFF) << tzc)};
		value |= word & mask;
		word >>= tzc;

		return {{value - 1U, 2 * tzc + 1}};
	}


	auto elias_gamma::decode(bit_reading_stream &bs) const -> std::optional <value_type>
	{
		// Check if we can read from the stream.
		if (!bs) return {std::nullopt};

		// There should be at most 63 trailing zeros.
		auto const tzc{bits::trailing_zeros(bs.value())};
		if (64U <= tzc) return {std::nullopt};

		// Try to shift by the number of zeros.
		bs >>= tzc + 1;
		if (bs.bits_remaining() < tzc) return {std::nullopt};

		// Success; it should be safe to read tzc bits.
		auto value{UINT64_C(1) << tzc};
		auto const mask{~(UINT64_C(0xFFFF'FFFF'FFFF'FFFF) << tzc)};
		value |= bs.value() & mask;
		bs >>= tzc;

		return {value - 1U};
	}
}
