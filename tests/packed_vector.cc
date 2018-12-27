/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <cstdint>
#include <libbio/packed_vector.hh>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


SCENARIO("A packed_vector may be created")
{
	GIVEN("a packed_vector <4, std::uint16_t>")
	{
		lb::packed_vector <4, std::uint16_t> vec(8);
		WHEN("queried for size")
		{
			THEN("it returns the correct values")
			{
				REQUIRE(16 == vec.word_bits());
				REQUIRE(4 == vec.element_bits());
				REQUIRE(4 == vec.element_count_in_word());
				REQUIRE(8 == vec.size());
				REQUIRE(2 == vec.word_size());
			}
		}
	}
}


SCENARIO("Values may be stored in a packed_vector")
{
	GIVEN("a packed_vector <4, std::uint16_t>")
	{
		lb::packed_vector <4, std::uint16_t> vec(8);
		REQUIRE(8 == vec.size());
		REQUIRE(2 == vec.word_size());
		
		WHEN("values are stored in the vector with fetch_or()")
		{
			std::size_t idx(0);
			for (auto proxy : vec)
				proxy.fetch_or(idx++);
			REQUIRE(8 == idx);
			
			THEN("the vector reports the correct values with a range-based for loop")
			{
				idx = 0;
				for (auto proxy : vec)
				{
					REQUIRE(idx == proxy.load());
					++idx;
				}
			}
			
			THEN("the vector reports the correct values with load()")
			{
				idx = 0;
				while (idx < 8)
				{
					REQUIRE(idx == vec.load(idx));
					++idx;
				}
			}
			
			THEN("the vector reports the correct values with word iterators")
			{
				REQUIRE(0x3210 == vec.word_begin()->load());
				REQUIRE(0x7654 == (vec.word_begin() + 1)->load());
			}
		}
	}
}


SCENARIO("The vector returns the previous stored value correctly")
{
	lb::packed_vector <4, std::uint16_t> vec(8);
	REQUIRE(8 == vec.size());
	REQUIRE(2 == vec.word_size());
	
	REQUIRE(0x0 == vec(1).fetch_or(0x2));
	REQUIRE(0x2 == vec(1).fetch_or(0x1));
	REQUIRE(0x3 == vec.load(1));
}
