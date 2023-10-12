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
	requires (is_tuple_v <T>  || is_index_sequence_v <T>)
	struct map {};
	
	template <typename... t_args, template <typename...> typename t_fn>
	struct map <std::tuple <t_args...>, t_fn>
	{
		typedef std::tuple <t_fn <t_args>...> type;
	};
	
	template <std::size_t... t_idxs, template <typename...> typename t_fn>
	struct map <std::index_sequence <t_idxs...>, t_fn>
	{
		// We use std::integral_constant here b.c. t_fn (likely) needs to be a template
		// that takes zero or more typenames (not a std::size_t before them).
		typedef std::tuple <t_fn <std::integral_constant <std::size_t, t_idxs>>...> type;
	};
	
	
	template <typename t_type, template <typename...> typename t_fn>
	requires (is_tuple_v <t_type> || is_index_sequence_v <t_type>)
	using map_t = typename map <t_type, t_fn>::type;
	
	
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
	
	
	template <typename t_tuple>
	using index_constant_sequence_for_tuple = map_t <forward_t <t_tuple, std::index_sequence_for>, std::type_identity_t>;
}

#endif
