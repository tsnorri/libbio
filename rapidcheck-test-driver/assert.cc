/*
 * Copyright (c) 2025 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <iostream>
#include <stdexcept>

// Throw instead of calling std::abort to get the configuration to reproduce the failing test.

namespace libbio::tests {
	class assertion_failure : std::runtime_error
	{
		using std::runtime_error::runtime_error;
	};
}


#if defined(__APPLE__)

extern "C" [[noreturn]] void __assert_rtn(const char *, const char *file, int line, const char *assertion)
{
	std::cerr << "Assertion failure in " << file << ':' << line << ": " << assertion << ".\n";
	throw libbio::tests::assertion_failure("Assertion failure");
}

#elif defined(__linux__)

extern "C" [[noreturn]] void __assert_fail(char const *assertion, char const *file, unsigned int line, char const *function)
{
	std::cerr << "Assertion failure in " << file << ':' << line << ", function " << function << ": " << assertion << ".\n";
	throw libbio::tests::assertion_failure("Assertion failure");
}

#endif
