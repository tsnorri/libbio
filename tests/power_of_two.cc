/*
 * Copyright (c) 2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_template_test_macros.hpp>
#include <cstdint>
#include <libbio/bits.hh>

namespace lb		= libbio;


TEMPLATE_TEST_CASE(
	"bits::gte_power_of_2 works with simple valid inputs",
	"[gte_power_of_2]",
	std::uint8_t,
	std::uint16_t,
	std::uint32_t,
	std::uint64_t
)
{
	GIVEN("Value 1 of a tested type")
	{
		TestType const value(1);
		auto const res(lb::bits::gte_power_of_2(value));
		CHECK(TestType(1) == res);
	}
}
