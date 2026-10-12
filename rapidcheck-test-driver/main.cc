/*
 * Copyright (c) 2023-2026 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <algorithm>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <libbio/utility.hh>
#include <string>
#include <vector>
//#include "cmdline.h"

#if !defined(LIBBIO_BUILD_RAPIDCHECK_TEST_DRIVER)
#	define LIBBIO_BUILD_RAPIDCHECK_TEST_DRIVER
#endif
#include <libbio/rapidcheck_test_driver.hh>

namespace po = boost::program_options;


namespace libbio::tests {

	bool test_driver::run_test(test_case_base &tc)
	{
		std::cerr << "\n** Running test: " << tc.message() << '\n';
		return tc.run_test(*this);
	}


	void test_driver::list_tests()
	{
		for (auto ptr : m_test_cases)
			std::cout << ptr->message() << '\n';
	}


	void test_driver::list_template_tests()
	{
		for (auto ptr : m_template_test_cases)
			std::cout << ptr->message() << '\n';
	}


	std::size_t test_driver::run_all_tests()
	{
		std::size_t retval{};
		for (auto &ptr : m_test_cases)
		{
			if (!run_test(*ptr))
			{
				++retval;
				m_failed_test_cases.push_back(ptr);
			}
		}
		return retval;
	}


	std::size_t test_driver::run_given_tests(test_name_vector const &names)
	{
		std::size_t retval{};
		for (auto &ptr : m_test_cases)
		{
			auto const found{std::binary_search(names.begin(), names.end(), ptr->message(), compare_strings_transparent{})};
			if (found && !run_test(*ptr))
			{
				++retval;
				m_failed_test_cases.push_back(ptr);
			}
		}

		return retval;
	}


	std::size_t test_driver::run_given_template_tests(test_name_vector const &names)
	{
		std::size_t retval{};
		for (auto &ptr : m_template_test_cases)
		{
			auto const found{std::binary_search(names.begin(), names.end(), ptr->message(), compare_strings_transparent{})};
			if (found && !run_test(*ptr))
			{
				++retval;
				m_failed_test_cases.push_back(ptr);
			}
		}

		return retval;
	}
}


int main(int argc, char **argv)
{
	typedef libbio::tests::test_driver::test_name_vector test_name_vector;
	typedef std::vector <std::string> test_name_vector_;

	po::options_description description(
		"libbio RapidCheck test driver\n"
		"Run tests written with RapidCheck possibly in reproduce mode"
	);

	description.add_options()
		("help,h", "Display this help message and exit.")
		("version,V", "Display the version number and exit.")
		("list,l", "List the tests without executing any.")
		("list-templates,L", "List the template tests without executing any.")
		("test,t", po::value <test_name_vector_>(), "Run only the given test(s)")
		("template-test,T", po::value <test_name_vector_>(), "Run only the given template test(s)");

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(description).run(), vm);
	po::notify(vm);

	if (vm.count("help"))
	{
		std::cout << description;
		return EXIT_SUCCESS;
	}

	if (vm.count("version"))
	{
		std::cout << "Version 0.1\n";
		return EXIT_SUCCESS;
	}

	auto &driver(::libbio::tests::test_driver::shared());

	bool should_exit_early{};
	if (vm.count("list"))
	{
		driver.list_tests();
		should_exit_early = true;
	}

	if (vm.count("list-templates"))
	{
		driver.list_template_tests();
		should_exit_early = true;
	}

	if (should_exit_early)
		return EXIT_SUCCESS;

	std::size_t status{};
	driver.reset_failed_test_cases();

	if (vm.count("test"))
	{
		auto const &test_names_{vm["test"].as <test_name_vector_>()};
		test_name_vector test_names{test_names_.begin(), test_names_.end()};
		std::sort(test_names.begin(), test_names.end());
		status += driver.run_given_tests(test_names);
	}

	if (vm.count("template-test"))
	{
		auto const &test_names_{vm["template-test"].as <test_name_vector_>()};
		test_name_vector test_names{test_names_.begin(), test_names_.end()};
		std::sort(test_names.begin(), test_names.end());
		status += driver.run_given_template_tests(test_names);
	}

	if (0 == vm.count("test") && 0 == vm.count("template-test"))
		status += driver.run_all_tests();

	if (status)
	{
		std::cerr << "\n** The following test cases failed:\n";
		for (auto const *ptr : driver.failed_test_cases())
			std::cerr << ptr->message() << '\n';
		std::cerr << '\n';

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
