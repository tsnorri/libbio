/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdlib>
#include "rapidcheck_test_driver_cmdline.h"

#define BUILD_RAPIDCHECK_TEST_DRIVER
#include "rapidcheck_test_driver.hh"

namespace lb	= libbio;


namespace {

	bool run_test(::libbio::test_case &tc)
	{
		std::cerr << "* Running test: " << tc.message() << '\n';
		return tc.run_test();
	}
}


namespace libbio {

	void test_driver::list_tests()
	{
		for (auto ptr : m_test_cases)
			std::cout << ptr->message() << '\n';
	}

	
	std::size_t test_driver::run_all_tests()
	{
		std::size_t retval{};
		for (auto &ptr : m_test_cases)
		{
			if (!run_test(*ptr))
				++retval;
		}
		return retval;
	}


	std::size_t test_driver::run_given_tests(test_name_set const &names)
	{
		std::size_t retval{};
		for (auto &ptr : m_test_cases)
		{
			if (names.contains(ptr->message()) && !run_test(*ptr))
				++retval;
		}

		return retval;
	}
}


int main(int argc, char **argv)
{
	gengetopt_args_info args_info;
	if (0 != cmdline_parser (argc, argv, &args_info))
		std::exit(EXIT_FAILURE);

	auto &driver(::libbio::test_driver::shared());

	if (args_info.list_given)
	{
		driver.list_tests();
		return EXIT_SUCCESS;
	}

	std::size_t status{};
	if (0 == args_info.test_given)
		status = driver.run_all_tests();
	else
	{
		::libbio::test_driver::test_name_set const test_names(args_info.test_arg, args_info.test_arg + args_info.test_given);
		status = driver.run_given_tests(test_names);
	}

	return status ? EXIT_FAILURE : EXIT_SUCCESS;
}
