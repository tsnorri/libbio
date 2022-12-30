/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_MAP_HH
#define LIBBIO_TUPLE_MAP_HH

#include <libbio/tuple/cat.hh>
#include <libbio/tuple/utility.hh>	// libbio::is_tuple_v


namespace libbio::tuples {
	
	template <typename T, template <typename...> typename>
	requires is_tuple_v <T>
	struct map {};
	
	template <typename... t_args, template <typename...> typename t_fn>
	struct map <std::tuple <t_args...>, t_fn>
	{
		typedef std::tuple <t_fn <t_args>...> type;
	};
	
	
	template <typename t_tuple, template <typename...> typename t_fn>
	requires is_tuple_v <t_tuple>
	using map_t = typename map <t_tuple, t_fn>::type;
	
	
	template <typename T, typename, template <typename...> typename>
	requires is_tuple_v <T>
	struct cross_product {};
	
	// FIXME: add requires t_fn <...>::type.
	template <typename... t_args_1, typename... t_args_2, template <typename...> typename t_fn>
	struct cross_product <std::tuple <t_args_1...>, std::tuple <t_args_2...>, t_fn>
	{
		template <typename t_lhs>
		struct curry_fn
		{
			template <typename t_rhs>
			using type = t_fn <t_lhs, t_rhs>;
		};
		
		typedef std::tuple <
			map_t <
				std::tuple <t_args_2...>,
				curry_fn <t_args_1>::template type
			>...
		> type_;	// Tuple of tuples.
		typedef cat_with_t <type_> type;
	};
	
	
	template <typename t_tuple_1, typename t_tuple_2, template <typename...> typename t_fn>
	requires (is_tuple_v <t_tuple_1> && is_tuple_v <t_tuple_2>)
	using cross_product_t = typename cross_product <t_tuple_1, t_tuple_2, t_fn>::type;
}

#endif
