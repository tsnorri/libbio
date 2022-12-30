/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_CAT_HH
#define LIBBIO_TUPLE_CAT_HH

#include <libbio/tuple/access.hh>	// libbio::tuples::empty
#include <libbio/tuple/fold.hh>
#include <libbio/tuple/utility.hh>	// libbio::is_tuple_v


namespace libbio::tuples {

	// Could not make this work with just std::apply and std::tuple_cat.
	
	// For producing an error message.
	template <typename t_tuple>
	requires is_tuple_v <t_tuple>
	struct cat_with {};
	
	template <typename... t_tuples>
	requires (is_tuple_v <t_tuples> && ...)
	struct cat_with <std::tuple <t_tuples...>>
	{
		template <typename t1, typename t2>
		requires (is_tuple_v <t1> && is_tuple_v <t2>)
		struct fold_fn_ {};
		
		template <typename... t_args_1, typename... t_args_2>
		struct fold_fn_ <std::tuple <t_args_1...>, std::tuple <t_args_2...>>
		{
			typedef std::tuple <t_args_1..., t_args_2...>	type;
		};
		
		template <typename t_acc, typename t_item>
		using fold_fn = typename fold_fn_ <t_acc, t_item>::type;
		
		typedef foldl_t <fold_fn, empty, std::tuple <t_tuples...>>	type;
	};
	
	
	template <typename... t_tuples>
	struct cat
	{
		typedef typename cat_with <std::tuple <t_tuples...>>::type type;
	};
	
	
	// Join the parameter lists of the tuples in the given tuple.
	template <typename t_tuple>
	requires is_tuple_v <t_tuple>
	using cat_with_t = typename cat_with <t_tuple>::type;
	
	// Join the parameter lists of two or more tuples.
	template <typename... t_tuples>
	requires (is_tuple_v <t_tuples> && ...)
	using cat_t = typename cat <t_tuples...>::type;
	
	// Prepend to a tuple.
	template <typename t_type, typename t_tuple>
	requires is_tuple_v <t_tuple>
	using prepend_t = cat_t <std::tuple <t_type>, t_tuple>;
	
	// Append to a tuple.
	template <typename t_tuple, typename t_type>
	requires is_tuple_v <t_tuple>
	using append_t = cat_t <t_tuple, std::tuple <t_type>>;
}

#endif
