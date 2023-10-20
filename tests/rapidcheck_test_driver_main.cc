/*
 * Copyright (c) 2023 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <cstdlib>

#define BUILD_RAPIDCHECK_TEST_DRIVER
#include "rapidcheck_test_driver.hh"


namespace libbio {
	
	std::size_t test_driver::run_all_tests()
	{
		std::size_t retval{};
		for (auto &ptr : m_test_cases)
		{
			std::cerr << "* Running test: " << ptr->message() << '\n';
			if (!ptr->run_test())
				++retval;
		}
		return retval;
	}
}


int main(int argc, char **argv)
{
	auto const status(::libbio::test_driver::shared().run_all_tests());
	return status ? EXIT_FAILURE : EXIT_SUCCESS;
}
