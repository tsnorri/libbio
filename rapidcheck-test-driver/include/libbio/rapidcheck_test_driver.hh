/*
 * Copyright (c) 2023-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

// Minimal test driver for RapidCheck for using reproduce mode which is not supported with Catch2.
#ifndef LIBBIO_RAPIDCHECK_TEST_DRIVER
#define LIBBIO_RAPIDCHECK_TEST_DRIVER

#include <exception>											// Needed by RapidCheck

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-braces"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"	// std::aligned_storage is used but has been deprecated in C++23.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"		// same for GCC
#include <rapidcheck.h>											// IWYU pragma: export
#pragma clang diagnostic pop
#pragma GCC diagnostic pop

#include <boost/preprocessor.hpp>
#include <boost/type_index.hpp>									// boost::typeindex::type_id <t_type>().pretty_name()
#include <cstddef>
#include <format>
#include <iostream>
#include <libbio/assert.hh>
#include <libbio/tuple/map.hh>
#include <libbio/utility/compare_strings_transparent.hh>
#include <set>
#include <string_view>
#include <utility>												// std::forward
#include <vector>


#ifdef LIBBIO_BUILD_RAPIDCHECK_TEST_DRIVER

namespace libbio::tests {

	struct test_case_base;
	struct test_case;
	struct template_test_case;


	class test_driver
	{
		friend struct test_case_base;

	public:
		typedef std::vector <std::string_view> test_name_vector;
		typedef std::vector <test_case_base *> test_case_pointer_vector;

	private:
		std::vector <test_case *>			m_test_cases;
		std::vector <template_test_case *>	m_template_test_cases;
		test_case_pointer_vector			m_failed_test_cases;
		test_case_base						*m_current_test_case{};

	private:
		bool run_test(test_case_base &tc);

	public:
		static test_driver &shared()
		{
			static test_driver retval{};
			return retval;
		}

		test_case_base *current_test_case() const { return m_current_test_case; }

		std::size_t add_test_case(test_case &tc)
		{
			m_test_cases.emplace_back(&tc);
			return m_test_cases.size() - 1;
		}

		std::size_t add_template_test_case(template_test_case &tc)
		{
			m_template_test_cases.emplace_back(&tc);
			return m_template_test_cases.size() - 1;
		}

		void list_tests();
		void list_template_tests();
		void reset_failed_test_cases() { m_failed_test_cases.clear(); }
		std::size_t run_all_tests();
		std::size_t run_given_tests(test_name_vector const &names); // names needs to be sorted.
		std::size_t run_given_template_tests(test_name_vector const &names); // names needs to be sorted.
		test_case_pointer_vector const &failed_test_cases() const { return m_failed_test_cases; }
	};


	struct test_case_base
	{
		std::size_t	identifier{};

	private:
		virtual bool run_test_(test_driver &driver) = 0;

	public:
		virtual ~test_case_base() {}
		virtual char const *message() const = 0;

		bool run_test(test_driver &driver)
		{
			auto *prev_test_case{driver.m_current_test_case};
			driver.m_current_test_case = this;
			auto const retval{run_test_(driver)};
			driver.m_current_test_case = prev_test_case;
			return retval;
		}
	};


	struct test_case : public test_case_base
	{
		test_case()
		{
			identifier = test_driver::shared().add_test_case(*this);
		}
	};


	struct template_test_case : public test_case_base
	{
		template_test_case()
		{
			identifier = test_driver::shared().add_template_test_case(*this);
		}
	};
}


namespace libbio {

	template <typename... t_args>
	auto rc_test(t_args && ... args)
	{
		auto &driver{tests::test_driver::shared()};
		auto *current_test_case{driver.current_test_case()};
		libbio_assert(current_test_case);

		return rc::check(current_test_case->message(), std::forward <t_args>(args)...);
	}


	template <typename... t_args>
	auto rc_check(t_args && ... args)
	{
		return rc::check(std::forward <t_args>(args)...);
	}
}


#	define LIBBIO_CONCAT_TOKEN_2(lhs, rhs) lhs ## rhs
#	define LIBBIO_CONCAT_TOKEN(lhs, rhs) LIBBIO_CONCAT_TOKEN_2(lhs, rhs)
#	define LIBBIO_TEST_FN_NAME LIBBIO_CONCAT_TOKEN(LIBBIO_TEST_, __LINE__) // Use uppercase to make the function easier to notice from the call stack.
#	define LIBBIO_TEST_CASE_VARIABLE_NAME LIBBIO_CONCAT_TOKEN(libio_test_case_instance_, __LINE__)

#	define LIBBIO_UNPAREN_(R, DATA, ELEMENT) , BOOST_PP_REMOVE_PARENS(ELEMENT)
#	define LIBBIO_UNPAREN_EACH(FIRST, ...) BOOST_PP_REMOVE_PARENS(FIRST) __VA_OPT__(BOOST_PP_SEQ_FOR_EACH(LIBBIO_UNPAREN_, "", BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))

// Given tuple X = (x1, x2), output (x1 <x2>).
#	define LIBBIO_TEMPLATE_PRODUCT__(X) (BOOST_PP_TUPLE_ELEM(0, X) <BOOST_PP_TUPLE_ELEM(1, X)>)
#	define LIBBIO_TEMPLATE_PRODUCT_(R, PAIR) LIBBIO_TEMPLATE_PRODUCT__(BOOST_PP_SEQ_TO_TUPLE(PAIR))

// Make a sequence of the two given tuples converted to sequences.
#	define LIBBIO_TEMPLATE_PRODUCT_MAKE_SEQ(TYPES_1, TYPES_2) \
	(BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_REMOVE_PARENS(TYPES_1)))(BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_REMOVE_PARENS(TYPES_2)))

// In the Cartesian product of TYPES_1 and TYPES_2, for each (t1, t2), pass t2 as a template parameter
// to t1, i.e. t1 <t2>.
#	define LIBBIO_TEMPLATE_PRODUCT(TYPES_1, TYPES_2) \
	BOOST_PP_REMOVE_PARENS(BOOST_PP_SEQ_TO_TUPLE(BOOST_PP_SEQ_FOR_EACH_PRODUCT(LIBBIO_TEMPLATE_PRODUCT_, LIBBIO_TEMPLATE_PRODUCT_MAKE_SEQ(TYPES_1, TYPES_2))))


// It is very important that all of the following are on the same line.
#	define TEST_CASE(MESSAGE, TAGS) \
		static inline bool __attribute__ ((always_inline)) LIBBIO_TEST_FN_NAME(); /* Forward declaration */ \
		static struct : public ::libbio::tests::test_case { \
			char const *message() const override { return MESSAGE; } \
			bool run_test_(libbio::tests::test_driver &driver) override { return LIBBIO_TEST_FN_NAME(); } \
		} LIBBIO_TEST_CASE_VARIABLE_NAME; \
		static inline bool __attribute__ ((always_inline)) LIBBIO_TEST_FN_NAME()

// Also here.
#	define TEMPLATE_TEST_CASE(MESSAGE, TAGS, ...) \
		template <typename TestType> \
		static bool LIBBIO_TEST_FN_NAME(); /* Forward declaration */ \
		static struct : public ::libbio::tests::test_case { \
			template <typename t_type> \
			class test_case final : public ::libbio::tests::template_test_case { \
			private: \
				std::string m_message; \
			public: \
				test_case(): \
					m_message{std::format("{} [{}]", MESSAGE, boost::typeindex::type_id <t_type>().pretty_name())} \
				{ \
				} \
				char const *message() const override { return m_message.data(); } \
				bool run_test_(libbio::tests::test_driver &driver) override { return LIBBIO_TEST_FN_NAME <t_type>(); } \
			}; \
			libbio::tuples::map_t <std::tuple <LIBBIO_UNPAREN_EACH(__VA_ARGS__)>, test_case> tests{}; \
			char const *message() const override { return MESSAGE; } \
			bool run_test_(libbio::tests::test_driver &driver) override { return std::apply([&driver](auto... tt){ return (tt.run_test(driver) && ...); }, tests); } \
		} LIBBIO_TEST_CASE_VARIABLE_NAME; \
		template <typename TestType> \
		static bool LIBBIO_TEST_FN_NAME()

// And here.
#	define TEMPLATE_PRODUCT_TEST_CASE(MESSAGE, TAGS, TYPES_1, TYPES_2) \
		TEMPLATE_TEST_CASE(MESSAGE, TAGS, LIBBIO_TEMPLATE_PRODUCT((TYPES_1), (TYPES_2)))

#else
#	include <catch2/catch_test_macros.hpp>
#	include <catch2/catch_template_test_macros.hpp>
#	include <rapidcheck/catch.h>	// rc::prop

namespace libbio {
	template <typename ... t_args>
	auto rc_test(t_args && ... args)
	{
		// Use Catch2 instead.
		return rc::prop(
			Catch::getResultCapture().getCurrentTestName(),
			std::forward <t_args>(args)...
		);
	}


	template <typename... t_args>
	auto rc_check(t_args && ... args)
	{
		return rc::check(std::forward <t_args>(args)...);
	}
}
#endif // LIBBIO_BUILD_RAPIDCHECK_TEST_DRIVER

#endif
