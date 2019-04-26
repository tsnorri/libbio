/*
 * Copyright (c) 2019 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <catch2/catch.hpp>
#include <libbio/assert.hh>
#include <tuple>

namespace gen	= Catch::Generators;
namespace lb	= libbio;


// Wrap the assertions to make them work with Catch2’s macros.
#define libbio_wrap_assertion(FN_NAME, ASSERTION_NAME) \
	template <typename t_lhs, typename t_rhs> \
	void FN_NAME(t_lhs const &lhs, t_rhs const &rhs) { \
		ASSERTION_NAME(lhs, rhs); \
	} \
	\
	template <typename t_lhs, typename t_rhs> \
	void FN_NAME##_msg(t_lhs const &lhs, t_rhs const &rhs) { \
		ASSERTION_NAME##_msg(lhs, rhs, "Test message"); \
	}


// Test the given assertions.
#define libbio_test_assertion(EXPECTED_RES, LHS, RHS, ASSERTION) \
	do { \
		if ((EXPECTED_RES)) { \
			CHECK_NOTHROW(ASSERTION((LHS), (RHS))); \
			CHECK_NOTHROW(ASSERTION##_msg((LHS), (RHS))); \
		} \
		else { \
			CHECK_THROWS_AS(ASSERTION((LHS), (RHS)), lb::assertion_failure_exception); \
			CHECK_THROWS_AS(ASSERTION##_msg((LHS), (RHS)), lb::assertion_failure_exception); \
		} \
	} while (false)


namespace {
	
	void wrapped_fail()
	{
		libbio_fail("Test message");
	}
	
	template <typename t_val>
	void wrapped_assert(t_val const &val)
	{
		libbio_always_assert(val);
	}
	
	template <typename t_val>
	void wrapped_assert_msg(t_val const &val)
	{
		libbio_always_assert_msg(val, "Test message");
	}
	
	// ; not strictly required but makes the text editor happy.
	libbio_wrap_assertion(wrapped_lt,	libbio_always_assert_lt);
	libbio_wrap_assertion(wrapped_lte,	libbio_always_assert_lte);
	libbio_wrap_assertion(wrapped_gt,	libbio_always_assert_gt);
	libbio_wrap_assertion(wrapped_gte,	libbio_always_assert_gte);
	libbio_wrap_assertion(wrapped_eq,	libbio_always_assert_eq);
	libbio_wrap_assertion(wrapped_neq,	libbio_always_assert_neq);
	
	
	template <typename t_lhs, typename t_rhs>
	void test_assertion_macros(t_lhs const &lhs, t_rhs const &rhs, std::array <bool, 6> const &expected_results)
	{
		WHEN("the values are compared")
		{
			auto const lt_res(std::get <0>(expected_results));
			auto const lte_res(std::get <1>(expected_results));
			auto const gt_res(std::get <2>(expected_results));
			auto const gte_res(std::get <3>(expected_results));
			auto const eq_res(std::get <4>(expected_results));
			auto const neq_res(std::get <5>(expected_results));
			
			THEN("the assertion macro performs the check correctly")
			{
				libbio_test_assertion(lt_res, lhs, rhs, wrapped_lt);
				libbio_test_assertion(lte_res, lhs, rhs, wrapped_lte);
				libbio_test_assertion(gt_res, lhs, rhs, wrapped_gt);
				libbio_test_assertion(gte_res, lhs, rhs, wrapped_gte);
				libbio_test_assertion(eq_res, lhs, rhs, wrapped_eq);
				libbio_test_assertion(neq_res, lhs, rhs, wrapped_neq);
			}
		}
	}
}


SCENARIO("Assertion macros throw on failure")
{
	GIVEN("the always failing assertion macro")
	{
		WHEN("the macro is called")
		{
			THEN("the correct exception is thrown")
			{
				CHECK_THROWS_AS(wrapped_fail(), lb::assertion_failure_exception);
			}
		}
	}
	
	GIVEN("One value")
	{
		WHEN("the value is tested with the assertion macro")
		{
			THEN("the assertion macro performs the check correctly")
			{
				CHECK_NOTHROW(wrapped_assert(true));
				CHECK_NOTHROW(wrapped_assert_msg(true));
				CHECK_THROWS_AS(wrapped_assert(false), lb::assertion_failure_exception);
				CHECK_THROWS_AS(wrapped_assert_msg(false), lb::assertion_failure_exception);
			}
		}
	}
	
	GIVEN("Two values of the same type")
	{
		// Order should be: lt, lte, gt, gte, eq, neq
		auto const &tup = GENERATE(gen::table({
			std::make_tuple(4,	5,	lb::make_array <bool>(true, true, false, false, false, true)),
			std::make_tuple(5,	5,	lb::make_array <bool>(false, true, false, true, true, false)),
			std::make_tuple(6,	5,	lb::make_array <bool>(false, false, true, true, false, true)),
			std::make_tuple(-5,	5,	lb::make_array <bool>(true, true, false, false, false, true))
		}));
		
		std::apply(test_assertion_macros <int, int>, tup);
	}
	
	GIVEN("Two values of different types")
	{
		test_assertion_macros(-5, 5U, lb::make_array <bool>(true, true, false, false, false, true));
	}
}
