/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_CAT_HH
#define LIBBIO_TUPLE_CAT_HH

#include <tuple>
#include <utility>


namespace libbio::tuples {

	// Could not make this work with just std::apply and std::tuple_cat.
	template <typename>
	struct cat_with {};
	
	template <typename... t_tuples>
	struct cat_with <std::tuple <t_tuples...>>
	{
		consteval static auto helper()
		{
			// For some reason using std::declval <t_tuples>() instead of t_tuples{}
			// causes a static assertion to fail in libstdc++.
			return std::tuple_cat(t_tuples{}...);
		}
		
		typedef std::invoke_result_t <decltype(&cat_with::helper)> type;
	};
	
	
	template <typename... t_tuples>
	struct cat
	{
		typedef typename cat_with <std::tuple <t_tuples...>>::type type;
	};
	
	
	// Join the parameter lists of the tuples in the given tuple.
	template <typename t_tuple>
	using cat_with_t = typename cat_with <t_tuple>::type;
	
	// Join the parameter lists of two or more tuples.
	template <typename... t_tuples>
	using cat_t = typename cat <t_tuples...>::type;
	
	// Prepend to a tuple.
	template <typename t_type, typename t_tuple>
	using prepend_t = cat_t <std::tuple <t_type>, t_tuple>;
	
	// Append to a tuple.
	template <typename t_tuple, typename t_type>
	using append_t = cat_t <t_tuple, std::tuple <t_type>>;
}

#endif
