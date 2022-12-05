/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_MAP_HH
#define LIBBIO_TUPLE_MAP_HH

#include <libbio/tuple/cat.hh>


namespace libbio::tuples {
	
	template <typename, template <typename> typename>
	struct map {};
	
	template <typename... t_args, template <typename> typename t_fn>
	struct map <std::tuple <t_args...>, t_fn>
	{
		typedef std::tuple <typename t_fn <t_args>::type...> type;
	};
	
	
	template <typename t_tuple, template <typename> typename t_fn>
	using map_t = typename map <t_tuple, t_fn>::type;
	
	
	template <typename, typename, template <typename, typename> typename>
	struct cross_product {};
	
	// FIXME: add requires t_fn <...>::type.
	template <typename... t_args_1, typename... t_args_2, template <typename, typename> typename t_fn>
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
	
	
	template <typename t_tuple_1, typename t_tuple_2, template <typename, typename> typename t_fn>
	using cross_product_t = typename cross_product <t_tuple_1, t_tuple_2, t_fn>::type;
}

#endif
