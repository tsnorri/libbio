/*
 * Copyright (c) 2022-2024 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <libbio/subprocess.hh>
#include <string>
#include <vector>

namespace lb	= libbio;


SCENARIO("Subprocess argument parser can parse simple argument lists", "[subprocess_argument_parser]")
{
	GIVEN("An empty command")
	{
		std::string const args;
		std::vector <std::string> const expected;

		WHEN("the string is parsed")
		{
			auto const res(lb::parse_command_arguments(args.data()));

			THEN("the result matches the expected")
			{
				CHECK(expected == res);
			}
		}
	}

	GIVEN("A simple command")
	{
		std::string const args("/path/to/executable --arg1=val1 --arg2=val2 pos1 pos2");
		std::vector <std::string> const expected{"/path/to/executable", "--arg1=val1", "--arg2=val2", "pos1", "pos2"};

		WHEN("the string is parsed")
		{
			auto const res(lb::parse_command_arguments(args.data()));

			THEN("the result matches the expected")
			{
				CHECK(expected == res);
			}
		}
	}
}


SCENARIO("Subprocess argument parser can parse complex argument lists", "[subprocess_argument_parser]")
{
	GIVEN("A command with quoted arguments and escaped quotes")
	{
		std::string const args("/path/to/executable \"--arg1=val1\" \"--arg2=\"\"\" \"pos1\" \"\"\"\"");
		std::vector <std::string> const expected{"/path/to/executable", "--arg1=val1", "--arg2=\"", "pos1", "\""};

		WHEN("the string is parsed")
		{
			auto const res(lb::parse_command_arguments(args.data()));

			THEN("the result matches the expected")
			{
				CHECK(expected == res);
			}
		}
	}

	GIVEN("A command with quoted arguments and escaped quotes (2)")
	{
		std::string const args("/path/to/executable \"--arg1=abc\"\"def\"");
		std::vector <std::string> const expected{"/path/to/executable", "--arg1=abc\"def"};

		WHEN("the string is parsed")
		{
			auto const res(lb::parse_command_arguments(args.data()));

			THEN("the result matches the expected")
			{
				CHECK(expected == res);
			}
		}
	}

	GIVEN("A command with quoted arguments and escaped quotes (2)")
	{
		std::string const args("/path/to/executable \"\"\"abc\"");
		std::vector <std::string> const expected{"/path/to/executable", "\"abc"};

		WHEN("the string is parsed")
		{
			auto const res(lb::parse_command_arguments(args.data()));

			THEN("the result matches the expected")
			{
				CHECK(expected == res);
			}
		}
	}
}


SCENARIO("Subprocess argument parser can report errors", "[subprocess_argument_parser]")
{
	GIVEN("An invalid command")
	{
		std::string const args("/path/to/executable \" pos2");

		WHEN("the string is parsed")
		{
			THEN("an exception with the correct message should be thrown")
			{
				REQUIRE_THROWS_WITH(lb::parse_command_arguments(args.data()), "Unepected character 0 at position 26");
			}
		}
	}
}
