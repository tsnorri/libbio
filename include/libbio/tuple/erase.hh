/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_ERASE_HH
#define LIBBIO_TUPLE_ERASE_HH

#include <libbio/tuple/filter.hh>
#include <libbio/tuple/utility.hh>	// libbio::tuples::negation
#include <type_traits> 				// std::bool_constant

namespace libbio::tuples::detail {
	
	template <typename t_type>
	struct is_same_as
	{
		template <typename t_other_type>
		struct type : public std::bool_constant <std::is_same_v <t_type, t_other_type>> {};
	};
}



namespace libbio::tuples {
	
	// Erases types that satisfy the given predicate from the given tuple.
	// Variable templates cannot be passed as template parameters as of C++20, hence the use of class template.
	template <typename t_tuple, template <typename...> typename t_predicate>
	struct erase_if
	{
		typedef filter_t <t_tuple, negation <t_predicate>::template with>	type;
	};
	
	
	template <typename t_tuple, template <typename...> typename t_predicate>
	using erase_if_t = typename erase_if <t_tuple, t_predicate>::type;
	
	
	template <typename t_tuple, typename t_type>
	using erase_t = erase_if_t <t_tuple, detail::is_same_as <t_type>::template type>;
}

#endif
