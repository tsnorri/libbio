/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <libbio/utility/is_equal.hh>
#include <libbio/utility/misc.hh>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


namespace {
	
	template <typename t_lhs, typename t_rhs>
	struct type_pair
	{
		typedef t_lhs left_type;
		typedef t_rhs right_type;
	};
	
	
	template <typename t_lhs>
	struct tpg
	{
		template <typename t_rhs>
		using type = type_pair <t_lhs, t_rhs>;
	};
}


TEMPLATE_PRODUCT_TEST_CASE(
	"Templated is_equal tests (all combinations)",
	"[template]",
	(
		tpg <std::int8_t>::type,
		tpg <std::int16_t>::type,
		tpg <std::int32_t>::type,
		tpg <std::int64_t>::type,
		tpg <std::uint8_t>::type,
		tpg <std::uint16_t>::type,
		tpg <std::uint32_t>::type,
		tpg <std::uint64_t>::type
	),
	(
		std::int8_t,
		std::int16_t,
		std::int32_t,
		std::int64_t,
		std::uint8_t,
		std::uint16_t,
		std::uint32_t,
		std::uint64_t
	)
)
{
	typedef typename TestType::left_type left_type;
	typedef typename TestType::right_type right_type;
	
	SECTION("Test zero")
	{
		left_type const left(0);
		right_type const right(0);
		GIVEN("two zeros")
		{
			WHEN("comparing")
			{
				auto const res(lb::is_equal(left, right));
				THEN("is_equal returns true")
				{
					REQUIRE(true == res);
				}
			}
			
			WHEN("the values are swapped")
			{
				auto const res(lb::is_equal(right, left));
				THEN("is_equal returns true")
				{
					REQUIRE(true == res);
				}
			}
		}
	}
	
	SECTION("Test comparison (left positive)")
	{
		typedef typename TestType::left_type left_type;
		typedef typename TestType::right_type right_type;
		
		auto const left = GENERATE(gen::values <left_type>({0, 5, 7, 10}));
		INFO("left type: " << lb::type_name <left_type>());
		INFO("right type: " << lb::type_name <right_type>());
		INFO("left: " << +left);
		GIVEN("a non-negative left value, compared to a greater right value")
		{
			auto const right = GENERATE(gen::values <right_type>({11, 15, 20, 100, std::numeric_limits <right_type>::max()}));
			INFO("right: " << +right);
			WHEN("compared to a greater rhs value")
			{
				auto const res(lb::is_equal(left, right));
				THEN("is_equal returns false")
				{
					REQUIRE(false == res);
				}
			}
			
			WHEN("the values are swapped")
			{
				auto const res(lb::is_equal(right, left));
				THEN("is_equal returns false")
				{
					REQUIRE(false == res);
				}
			}
		}
		
		GIVEN("a non-negative left value, same right value")
		{
			auto const right(left);
			INFO("right: " << +right);
			WHEN("compared to a the same rhs value")
			{
				auto const res(lb::is_equal(left, right));
				THEN("is_equal returns true")
				{
					REQUIRE(true == res);
				}
			}
			
			WHEN("the values are swapped")
			{
				auto const res(lb::is_equal(right, left));
				THEN("is_equal returns true")
				{
					REQUIRE(true == res);
				}
			}
		}
	}
}


TEMPLATE_PRODUCT_TEST_CASE(
	"Templated check_lte tests (signed and unsigned)",
	"[template]",
	(
		tpg <std::int8_t>::type,
		tpg <std::int16_t>::type,
		tpg <std::int32_t>::type,
		tpg <std::int64_t>::type
	),
	(
		std::int8_t,
		std::int16_t,
		std::int32_t,
		std::int64_t,
		std::uint8_t,
		std::uint16_t,
		std::uint32_t,
		std::uint64_t
	)
)
{
	typedef typename TestType::left_type left_type;
	typedef typename TestType::right_type right_type;
	
	SECTION("Test negative left")
	{
		auto const left = GENERATE(gen::values <left_type>({-5, -10, -20, -40, std::numeric_limits <left_type>::min()}));
		INFO("left type: " << lb::type_name <left_type>());
		INFO("right type: " << lb::type_name <right_type>());
		INFO("left: " << +left);
		GIVEN("a negative lhs value")
		{
			auto const right = GENERATE(gen::values <right_type>({0, 5, 10, 100, std::numeric_limits <right_type>::max()}));
			INFO("right: " << +right);
			WHEN("compared to a non-negative rhs value")
			{
				auto const res(lb::is_equal(left, right));
				THEN("is_equal returns false")
				{
					REQUIRE(false == res);
				}
			}
			
			WHEN("the values are swapped")
			{
				auto const res(lb::is_equal(right, left));
				THEN("is_equal returns false")
				{
					REQUIRE(false == res);
				}
			}
		}
	}
	
	SECTION("Test non-negative left")
	{
		auto const left = GENERATE(gen::values <left_type>({0, 1, 5, 10, 15}));
		INFO("left type: " << lb::type_name <left_type>());
		INFO("right type: " << lb::type_name <right_type>());
		INFO("left: " << +left);
		GIVEN("a non-negative lhs value")
		{
			auto const right = GENERATE(gen::values <right_type>({20, 40, 100, std::numeric_limits <right_type>::max()}));
			INFO("right: " << +right);
			WHEN("compared to a greater rhs value")
			{
				auto const res(lb::is_equal(left, right));
				THEN("is_equal returns equal")
				{
					REQUIRE(false == res);
				}
			}
			
			WHEN("the values are swapped")
			{
				auto const res(lb::is_equal(right, left));
				THEN("is_equal returns false")
				{
					REQUIRE(false == res);
				}
			}
		}
		
		GIVEN("a non-negative left value, same right value")
		{
			auto const right(left);
			INFO("right: " << +right);
			WHEN("compared to the same rhs value")
			{
				auto const res(lb::is_equal(left, right));
				THEN("is_equal returns true")
				{
					REQUIRE(true == res);
				}
			}
			
			WHEN("the values are swapped")
			{
				auto const res(lb::is_equal(right, left));
				THEN("is_equal returns true")
				{
					REQUIRE(true == res);
				}
			}
		}
	}
}


TEMPLATE_PRODUCT_TEST_CASE(
	"Templated check_lte tests (signed)",
	"[template]",
	(
		tpg <std::int8_t>::type,
		tpg <std::int16_t>::type,
		tpg <std::int32_t>::type,
		tpg <std::int64_t>::type
	),
	(
		std::int8_t,
		std::int16_t,
		std::int32_t,
		std::int64_t
	)
)
{
	SECTION("Test comparison (left negative)")
	{
		typedef typename TestType::left_type left_type;
		typedef typename TestType::right_type right_type;
		
		auto const left = GENERATE(gen::values <left_type>({-5, -10, -20, -100, std::numeric_limits <left_type>::min()}));
		INFO("left type: " << lb::type_name <left_type>());
		INFO("right type: " << lb::type_name <right_type>());
		INFO("left: " << +left);
		GIVEN("a negative left value")
		{
			auto const right = GENERATE(gen::values <right_type>({-4, 0, 10, 20, 100, std::numeric_limits <right_type>::max()}));
			INFO("right: " << +right);
			WHEN("compared to a greater rhs value")
			{
				auto const res(lb::is_equal(left, right));
				THEN("is_equal returns false")
				{
					REQUIRE(false == res);
				}
			}
			
			WHEN("the values are swapped")
			{
				auto const res(lb::is_equal(right, left));
				THEN("is_equal returns false")
				{
					REQUIRE(false == res);
				}
			}
		}
	}
	
	SECTION("Test comparison (left negative without min.)")
	{
		typedef typename TestType::left_type left_type;
		
		auto const left = GENERATE(gen::values <left_type>({-5, -10, -20, -100}));
		INFO("left type: " << lb::type_name <left_type>());
		INFO("left: " << +left);
		GIVEN("a negative left value")
		{
			auto const right(left);
			INFO("right: " << +right);
			WHEN("compared to the same rhs value")
			{
				auto const res(lb::is_equal(left, right));
				THEN("is_equal returns true")
				{
					REQUIRE(true == res);
				}
			}
			
			WHEN("the values are swapped")
			{
				auto const res(lb::is_equal(right, left));
				THEN("is_equal returns true")
				{
					REQUIRE(true == res);
				}
			}
		}
	}
}
