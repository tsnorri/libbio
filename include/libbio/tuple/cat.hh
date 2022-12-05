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
		static auto helper()
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
	
	
	// Join the parameter lists of tuples in the given tuple.
	template <typename t_tuple>
	using cat_with_t = typename cat_with <t_tuple>::type;
	
	// Join the parameter lists of two or more tuples.
	template <typename... t_tuples>
	using cat_t = typename cat <t_tuples...>::type;
}

#endif