/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ENCODING_ELIAS_GAMMA_HH
#define LIBBIO_ENCODING_ELIAS_GAMMA_HH

#include <cstdint>
#include <libbio/bit_reading_stream.hh>
#include <libbio/bit_writing_stream.hh>
#include <libbio/bits.hh>
#include <optional>
#include <utility>


namespace libbio::encoding {

	// The values are encoded in such a way that the zero bits of the encoded value
	// are written to the less significant bits of the stream.
	struct elias_gamma
	{
		typedef std::uint64_t value_type;

		std::optional <std::pair <value_type, std::uint8_t>> decode(std::uint64_t word) const;
		std::optional <value_type> decode(bit_reading_stream &bs) const;

		template <typename t_target>
		bool encode(bit_writing_stream <t_target> &bs, value_type value) const;
	};


	template <typename t_target>
	bool elias_gamma::encode(bit_writing_stream <t_target> &bs, value_type value) const
	{
		auto const value1{value + 1U};
		auto const idx1{bits::highest_bit_set(value1)}; // 1-based.
		auto const idx{idx1 - 1U};

		auto const mask{~(UINT64_C(0xFFFF'FFFF'FFFF'FFFF) << idx)};
		auto const lower{((value1 & mask) << 1U) | UINT64_C(0x1)};

		if (bs.write_zeros(idx) && bs.write_bits(lower, idx1))
			return true;

		return false;
	}
}

#endif
