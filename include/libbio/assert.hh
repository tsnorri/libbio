/*
 * Copyright (c) 2017-2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_ASSERT_HH
#define LIBBIO_ASSERT_HH

#include <iostream>


namespace libbio {
	
	inline void fail() { abort(); }
	
	
	inline void fail(char const *message)
	{
		std::cerr << message << std::endl;
		fail();
	}
	
	
	inline void always_assert(bool check) { if (!check) abort(); }
	
	
	inline void always_assert(bool check, char const *message)
	{
		if (!check)
			fail(message);
	}
	
	
	// If the check fails, call the given function and then call fail().
	template <typename t_fn>
	inline void always_assert(bool check, t_fn fn)
	{
		if (!check)
		{
			fn();
			fail();
		}
	}
}

#endif
