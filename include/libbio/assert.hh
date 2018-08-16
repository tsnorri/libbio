/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ASSERT_HH
#define LIBBIO_ASSERT_HH

#include <iostream>

// Contracts not yet available in either Clang or GCC.
#define libbio_fail(MESSAGE) do { ::libbio::detail::do_fail(__FILE__, __LINE__, MESSAGE); } while (false)
#define libbio_always_assert(...) do { ::libbio::detail::do_always_assert(__FILE__, __LINE__, __VA_ARGS__); } while (false)


namespace libbio { namespace detail {

	inline void log_assertion_failure(char const *file, int const line)
	{
		std::cerr << "Assertion failed at " << file << ':' << line << ": ";
	}


	inline void do_fail(char const *file, int const line, char const *message)
	{
		detail::log_assertion_failure(file, line);
		std::cerr << message << std::endl;
		abort();
	}
	
	
	inline void do_always_assert(char const *file, int const line, bool check)
	{
		if (!check)
		{
			detail::log_assertion_failure(file, line);
			abort();
		}
	}
	
	
	inline void do_always_assert(char const *file, int const line, bool check, char const *message)
	{
		if (!check)
			do_fail(file, line, message);
	}
	
	
	// If the check fails, call the given function and then call fail().
	template <typename t_fn>
	inline void do_always_assert(char const *file, int const line, bool check, t_fn fn)
	{
		if (!check)
		{
			detail::log_assertion_failure(file, line);
			fn();
			abort();
		}
	}
}}

#endif
