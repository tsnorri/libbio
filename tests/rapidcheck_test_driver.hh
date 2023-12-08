/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

// Minimal test driver for RapidCheck for using reproduce mode which is not supported with Catch2.
#ifndef LIBBIO_RAPIDCHECK_TEST_DRIVER
#define LIBBIO_RAPIDCHECK_TEST_DRIVER

#include <exception>										// Needed by RapidCheck
#include <iostream>
#include <libbio/utility/compare_strings_transparent.hh>
#include <rapidcheck.h>
#include <set>
#include <string>
#include <utility>											// std::forward
#include <vector>


#ifdef BUILD_RAPIDCHECK_TEST_DRIVER

namespace libbio {

	struct test_case;
	
	
	class test_driver
	{
	public:
		typedef std::set <
			std::string,
			libbio::compare_strings_transparent
		> test_name_set;

	private:
		std::vector <test_case *> m_test_cases;
		
	public:
		static test_driver &shared()
		{
			static test_driver retval{};
			return retval;
		}
		
		std::size_t add_test_case(test_case &tc)
		{
			m_test_cases.emplace_back(&tc);
			return m_test_cases.size() - 1;
		}
		
		void list_tests();
		std::size_t run_all_tests();
		std::size_t run_given_tests(test_name_set const &names);
	};
	
	
	struct test_case
	{
		std::size_t	identifier{};
		
		test_case()
		{
			identifier = test_driver::shared().add_test_case(*this);
		}
		
		virtual ~test_case() {}
		virtual char const *message() = 0;
		virtual bool run_test() = 0;
	};
	
	template <typename t_unique_type>
	class test_case_tpl {};
	
	
	template <typename ... t_args>
	auto rc_check(t_args && ... args)
	{
		return rc::check(std::forward <t_args>(args)...);
	}
}

#	define LIBBIO_CONCAT_TOKEN_2(lhs, rhs) lhs ## rhs
#	define LIBBIO_CONCAT_TOKEN(lhs, rhs) LIBBIO_CONCAT_TOKEN_2(lhs, rhs)
#	define LIBBIO_TEST_FN_NAME LIBBIO_CONCAT_TOKEN(LIBBIO_TEST_, __LINE__) // Use uppercase to make the function easier to notice from the call stack.
#	define LIBBIO_TEST_CASE_VARIABLE_NAME LIBBIO_CONCAT_TOKEN(libio_test_case_instance_, __LINE__)

// It is very important that all of the following are on the same line.
#	define TEST_CASE(MESSAGE, TAGS) \
		static inline bool __attribute__ ((always_inline)) LIBBIO_TEST_FN_NAME(); /* Forward declaration */ \
		static struct : public ::libbio::test_case { \
			char const *message() override { return MESSAGE; } \
			bool run_test() override { return LIBBIO_TEST_FN_NAME(); } \
		} LIBBIO_TEST_CASE_VARIABLE_NAME; \
		static inline bool __attribute__ ((always_inline)) LIBBIO_TEST_FN_NAME()
	
#else
#	include <catch2/catch.hpp>
#	include <rapidcheck/catch.h>	// rc::prop

namespace libbio {
	template <typename ... t_args>
	auto rc_check(t_args && ... args)
	{
		// Use Catch2 instead.
		return rc::prop(std::forward <t_args>(args)...);
	}
}
#endif // BUILD_RAPIDCHECK_TEST_DRIVER

#endif
