/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <catch2/catch.hpp>
#include <libbio/radix_sort.hh>
#include <vector>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


SCENARIO("Radix sort can sort a sequence of numbers")
{
	GIVEN("A sequence of numbers")
	{
		auto const vec = GENERATE(gen::values({
			std::vector <unsigned int>({1, 5, 81, 22, 16, 55, 8}),
			std::vector <unsigned int>({55, 12, 74878, 456, 24, 887, 56}),
			std::vector <unsigned int>({123, 3924, 23, 904324, 2320, 99})
		}));
		
		WHEN("the items are sorted")
		{
			auto v(vec);
			decltype(v) buf;
		
			lb::radix_sort_check_bits_set(v, buf);
			
			THEN("the sequence is sorted")
			{
				REQUIRE(std::is_sorted(v.begin(), v.end()));
			}
		}
	}
}
