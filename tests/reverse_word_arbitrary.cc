/*
 * Copyright (c) 2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <bit>
#include <libbio/algorithm.hh>
#include <libbio/bits.hh>
#include <libbio/rapidcheck_test_driver.hh>

namespace lb		= libbio;

namespace {

	// Fwd.
	template <typename t_value>
	t_value reverse_bits(t_value value);


	template <>
	std::uint8_t reverse_bits <std::uint8_t>(std::uint8_t val)
	{
		// Adapted from https://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
		// original in public domain.
		return ((val * UINT64_C(0x8020'0802)) & UINT64_C(0x08'8442'2110)) * UINT64_C(0x01'0101'0101) >> 32;
	}


	template <typename t_value>
	t_value reverse_bits(t_value value)
	{
		std::array <std::uint8_t, sizeof(t_value)> buffer{};
		std::memcpy(buffer.data(), &value, sizeof(t_value));
		for (auto &vv : buffer)
			vv = reverse_bits(vv);
		std::memcpy(&value, buffer.data(), sizeof(t_value));
		return std::byteswap(value);
	}
}


TEMPLATE_TEST_CASE(
	"libbio::reverse_bits <1> works as expected",
	"[template][reverse_bits]",
	std::uint8_t,
	std::uint16_t,
	std::uint32_t,
	std::uint64_t
)
{
	return lb::rc_check(
		"libbio::reverse_bits <> works as expected",
		[](TestType const value){
			auto const expected{reverse_bits(value)};
			auto const actual{lb::reverse_bits <1>(value)};
			RC_ASSERT(expected == actual);
			return true;
		}
	);
}
