/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_GENERIC_PARSER_FILTERS_HH
#define LIBBIO_GENERIC_PARSER_FILTERS_HH

#include <libbio/generic_parser/utility.hh>


// Character classes.
namespace libbio::parsing::filters {
	
	struct no_op
	{
		template <typename t_char>
		constexpr static bool check(t_char const) { return true; }
	};


	struct printable
	{
		template <typename t_char>
		constexpr static bool check(t_char const cc)
		{
			return detail::is_printable(cc);
		}
	};
}

#endif
