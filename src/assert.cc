/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>


namespace libbio { namespace detail {
	
	void assertion_failure(char const *file, long const line, char const *assertion)
	{
		throw assertion_failure_exception(file, line, assertion);
	}
	
	
	void assertion_failure(char const *file, long const line)
	{
		throw assertion_failure_exception(file, line);
	}
}}
