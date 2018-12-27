/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <cstdint>
#include <libbio/algorithm.hh>
#include <libbio/utility.hh>
#include <ostream>
#include <vector>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	template <typename t_val>
	constexpr std::uint8_t uc(t_val val) { return val; }
}


SCENARIO("64-bit words may be reversed")
{
	GIVEN("a 64-bit word")
	{
		auto const &pair = GENERATE(gen::values <std::pair <std::uint64_t, std::uint64_t>>({
			{0xf0f0f0f0f0f0f0f0UL, 0x0f0f0f0f0f0f0f0f},
			{0xf0f0f0f0UL, 0x0f0f0f0f00000000},
			{0xff7f3f1f0f070301UL, 0x80c0e0f0f8fcfeff}
		}));
		
		WHEN("a value is reversed")
		{
			auto const val(pair.first);
			auto const expected(pair.second);
			auto const actual(lb::reverse_bits <1>(val));
			THEN("the reversed value has the bits in correct order")
			{
				REQUIRE(expected == actual);
			}
		}
	}
}


// FIXME: add similar tests for 16, 32, 64 bits
SCENARIO("8-bit words can be reversed")
{
	GIVEN("an 8-bit word")
	{
		auto const &tuple = GENERATE(gen::values <std::array <std::uint8_t, 5>>({
			lb::make_array <std::uint8_t>(uc(0xed), uc(0xb7), uc(0x7b), uc(0xde), uc(0xed))
		}));
		
		WHEN("a value is reversed")
		{
			auto const val(std::get <0>(tuple));
			auto const rev1(std::get <1>(tuple));
			auto const rev2(std::get <2>(tuple));
			auto const rev4(std::get <3>(tuple));
			auto const rev8(std::get <4>(tuple));
			
			THEN("the reversed value has the bits in correct order")
			{
				REQUIRE(rev1 == lb::reverse_bits <1>(val));
				REQUIRE(rev2 == lb::reverse_bits <2>(val));
				REQUIRE(rev4 == lb::reverse_bits <4>(val));
				REQUIRE(rev8 == lb::reverse_bits <8>(val));
			}
		}
	}
}
