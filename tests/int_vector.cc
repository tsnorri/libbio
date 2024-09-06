/*
 * Copyright (c) 2018-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <libbio/int_vector.hh>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


SCENARIO("int_vector can be initialized with the correct number of values", "[int_vector]")
{
	GIVEN("an int_vector <8>")
	{
		WHEN("the vector is initialized with a given number of values")
		{
			lb::int_vector <8> vec(9, 0x55);
			
			THEN("the vector shall contain the correct values")
			{
				for (std::size_t i(0); i < 9; ++i)
					REQUIRE(0x55 == vec[i]);
			}
			
			THEN("the last word of the vector will contain the correct number of copies of the value")
			{
				REQUIRE(0x55 == *vec.word_crbegin());
			}
		}
	}
}


SCENARIO("Values can be added to an int_vector with push_back()", "[int_vector]")
{
	GIVEN("an int_vector <8> with values added using push_back()")
	{
		lb::int_vector <8> vec;
		for (std::size_t i(0); i < 10; ++i)
			vec.push_back(i);
		
		REQUIRE(10 == vec.size());
		
		WHEN("the values are accessed with operator[]")
		{
			THEN("the correct value will be returned")
			{
				for (std::size_t i(0); i < 10; ++i)
					REQUIRE(i == vec[i]);
			}
		}
		
		WHEN("the values are accessed with a range-based for loop")
		{
			THEN("the correct value will be returned")
			{
				std::size_t i(0);
				for (auto const val : vec)
					REQUIRE(i++ == val);
			}
		}
	}
}


SCENARIO("Multiple copies of a value may be added to an int_vector with push_back()", "[int_vector]")
{
	GIVEN("an int_vetor <8>")
	{
		lb::int_vector <8> vec;
		
		WHEN("values are added to the vector")
		{
			vec.push_back(0);
			vec.push_back(88, 18);
			
			THEN("the vector will report its size correctly")
			{
				REQUIRE(19 == vec.size());
			}
			
			THEN("the vector will contain the correct values")
			{
				REQUIRE(0 == vec[0]);
				for (std::size_t i(1); i < 19; ++i)
					REQUIRE(88 == vec[i]);
			}
		}
	}
}


SCENARIO("An int_vector may be reversed using reverse()", "[int_vector]")
{
	GIVEN("an int_vector <8> with values")
	{
		lb::int_vector <8> vec;
		for (std::size_t i(0); i < 10; ++i)
			vec.push_back(i);
		
		REQUIRE(10 == vec.size());
		
		WHEN("the vector is reversed")
		{
			vec.reverse();
			
			THEN("the vector will contain the values in the reversed order")
			{
				std::size_t i(0);
				for (auto const val : vec)
					REQUIRE(9 - i++ == val);
			}
		}
	}
}
