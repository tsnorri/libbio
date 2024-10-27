/*
 * Copyright (c) 2019-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdint>
#include <iterator>
#include <libbio/algorithm/merge_projected.hh>
#include <range/v3/all.hpp>
#include <tuple>
#include <vector>

namespace lb	= libbio;


SCENARIO("merge_projected can merge containers", "[merge_projected]")
{
	GIVEN("two vectors of values")
	{
		typedef std::uint32_t				input_type;
		typedef input_type					output_type;
		typedef std::vector <input_type>	input_vector;
		typedef input_vector				output_vector;

		typedef std::tuple <input_vector, input_vector, output_vector> tuple_type;
		auto const &tup = GENERATE(table <input_vector, input_vector, output_vector>({
			tuple_type{input_vector({1, 3, 5, 7, 9}),	input_vector({0, 2, 4, 6, 8}),	output_vector({0, 1, 2, 3, 4, 5, 6, 7, 8, 9})},
			tuple_type{input_vector({1, 3, 10, 7}),		input_vector({0, 2, 4, 6, 8}),	output_vector({0, 1, 2, 3, 4, 6, 8})}
		}));

		WHEN("the function is called")
		{
			auto const & [lhs, rhs, expected] = tup;
			output_vector dst;

			lb::merge_projected(
				lhs,
				rhs,
				std::back_inserter(dst),
				[](input_type val, bool &should_continue) -> output_type {
					if (val < 10)
						return val;

					should_continue = false;
					return 0;
				}
			);

			THEN("the return type size matches the expected size")
			{
				REQUIRE(dst == expected);
			}
		}
	}
}
