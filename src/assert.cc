/*
 * Copyright (c) 2018 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#include <libbio/assert.hh>


namespace libbio { namespace detail {

	template <typename t_exception>
	void throw_with_trace(t_exception const &exc)
	{
		throw boost::enable_error_info(exc) << traced(boost::stacktrace::stacktrace());
	}

	
	void assertion_failure(char const *file, long const line, char const *assertion)
	{
		throw_with_trace(assertion_failure_exception(file, line, assertion));
	}
	
	
	void assertion_failure(char const *file, long const line)
	{
		throw_with_trace(assertion_failure_exception(file, line));
	}
}}
