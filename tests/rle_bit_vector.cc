/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <libbio/rle_bit_vector.hh>
#include <libbio/utility.hh>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


SCENARIO("RLE bit vector can store runs")
{
	GIVEN("An RLE bit vector")
	{
		lb::rle_bit_vector <std::uint32_t> vec;
		
		WHEN("runs are stored into the vector")
		{
			for (std::size_t i(0); i < 10; ++i)
				vec.push_back(i % 2, i + 1);
			
			THEN("the same runs may be accessed with const_runs().")
			{
				REQUIRE(vec.starts_with_zero());
				std::size_t i(0);
				for (auto const val : vec.const_runs())
					REQUIRE(val == ++i);
			}
		}
	}
}


SCENARIO("RLE bit vector can collapse runs")
{
	GIVEN("An RLE bit vector")
	{
		lb::rle_bit_vector <std::uint32_t> vec;
		
		WHEN("runs with non-alternating values are stored into the vector")
		{
			vec.push_back(0, 5);
			vec.push_back(0, 2);
			vec.push_back(1, 3);
			vec.push_back(1, 1);
			
			THEN("the collapsed runs may be accessed with const_runs().")
			{
				REQUIRE(vec.starts_with_zero());
				
				std::size_t i(0);
				auto const expected(lb::make_array <std::uint32_t>(7U, 4U));
				for (auto const val : vec.const_runs())
					REQUIRE(val == expected[i++]);
			}
		}
	}
}
