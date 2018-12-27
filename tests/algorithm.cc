/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <cstdint>
#include <libbio/algorithm.hh>
#include <ostream>
#include <vector>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	
	struct unique_count_test_input
	{
		std::uint32_t	value{};
		std::uint32_t	count{};
		
		unique_count_test_input() = default;
		unique_count_test_input(std::uint32_t value_, std::uint32_t count_):
			value(value_),
			count(count_)
		{
		}
		
		bool operator==(unique_count_test_input const &other) const
		{
			return value == other.value;
		}
	};
	
	std::ostream &operator<<(std::ostream &os, unique_count_test_input const &input)
	{
		os << '(' << input.value << ", " << input.count << ')';
		return os;
	}
}


SCENARIO("Set symmetric difference can be determined")
{
	GIVEN("Two collections")
	{
		auto const &tup = GENERATE(gen::table({
			std::make_tuple(5, std::vector <unsigned int>({1, 3, 5, 81}), std::vector <unsigned int>({2, 3, 4, 5, 6})),
			std::make_tuple(0, std::vector <unsigned int>({2, 4, 6, 8}), std::vector <unsigned int>({2, 4, 6, 8})),
			std::make_tuple(0, std::vector <unsigned int>({1, 2, 3}), std::vector <unsigned int>({1, 2, 3})),
			std::make_tuple(6, std::vector <unsigned int>({1, 2, 3}), std::vector <unsigned int>({4, 5, 6})),
			std::make_tuple(6, std::vector <unsigned int>({1, 3, 5}), std::vector <unsigned int>({2, 4, 6}))
		}));

		
		WHEN("the collections are compared")
		{
			auto const expected_count(std::get <0>(tup));
			auto const &lhs(std::get <1>(tup));
			auto const &rhs(std::get <2>(tup));
			
			auto const actual_count(lb::set_symmetric_difference_size(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend()));

			THEN("the difference size matches the expected")
			{
				REQUIRE(expected_count == actual_count);
			}
		}
	}
}


SCENARIO("Set intersection size can be determined")
{
	GIVEN("Two collections")
	{
		auto const &tup = GENERATE(gen::table({
			std::make_tuple(2, std::vector <unsigned int>({1, 3, 5, 81}), std::vector <unsigned int>({2, 3, 4, 5, 6})),
			std::make_tuple(4, std::vector <unsigned int>({2, 4, 6, 8}), std::vector <unsigned int>({2, 4, 6, 8})),
			std::make_tuple(3, std::vector <unsigned int>({1, 2, 3}), std::vector <unsigned int>({1, 2, 3})),
			std::make_tuple(0, std::vector <unsigned int>({1, 2, 3}), std::vector <unsigned int>({4, 5, 6})),
			std::make_tuple(0, std::vector <unsigned int>({1, 3, 5}), std::vector <unsigned int>({2, 4, 6}))
		}));

		
		WHEN("the collections are compared")
		{
			auto const expected_count(std::get <0>(tup));
			auto const &lhs(std::get <1>(tup));
			auto const &rhs(std::get <2>(tup));
			
			auto const actual_count(lb::set_intersection_size(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend()));

			THEN("the difference size matches the expected")
			{
				REQUIRE(expected_count == actual_count);
			}
		}
	}
}


SCENARIO("Unique items can be counted")
{
	GIVEN("A collection of items")
	{
		typedef unique_count_test_input ti;
		
		auto const &tup = GENERATE(gen::table({
			std::make_tuple(std::vector <ti>({{1, 1}, {2, 1}, {2, 1}, {4, 1}, {4, 1}, {4, 1}, {5, 1}}), std::vector <ti>({{1, 1}, {2, 2}, {4, 3}, {5, 1}})),
			std::make_tuple(std::vector <ti>({{1, 1}, {2, 1}, {4, 1}}), std::vector <ti>({{1, 1}, {2, 1}, {4, 1}})),
			std::make_tuple(std::vector <ti>({{3, 1}, {3, 1}, {3, 1}}), std::vector <ti>({{3, 3}}))
		}));
		
		WHEN("the unique items are counted")
		{
			auto &input(std::get <0>(tup));
			auto const &expected(std::get <1>(tup));
			std::vector <ti> output;
			
			lb::unique_count(input.begin(), input.end(), output);
			
			THEN("the output matches the expected")
			{
				REQUIRE(expected.size() == output.size());
				for (std::size_t i(0), count(expected.size()); i < count; ++i)
				{
					auto const &output_val(output[i]);
					auto const &expected_val(expected[i]);
					REQUIRE(output_val == expected_val);
				}
			}
		}
	}
}
