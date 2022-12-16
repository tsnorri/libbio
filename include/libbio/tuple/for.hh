/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_FOR_HH
#define LIBBIO_TUPLE_FOR_HH

#include <tuple>
#include <type_traits>	// std::integral_constant


namespace libbio::tuples {
	
	// Loop with an index.
	template <std::size_t t_limit, std::size_t t_idx = 0, typename t_fn>
	constexpr static inline void for_(t_fn &&fn)
	{
		if constexpr (t_idx < t_limit)
		{
			fn.template operator() <std::integral_constant <std::size_t, t_idx>>();
			for_ <t_limit, 1 + t_idx>(fn);
		}
	}
	
	
	// Apply a function to each of the tuple elements.
	template <std::size_t t_i = 0, typename t_tuple, typename t_fn>
	constexpr static inline void for_each(t_tuple &&tuple, t_fn &&fn)
	{
		if constexpr (t_i != std::tuple_size_v <t_tuple>)
		{
			fn.template operator() <std::integral_constant <std::size_t, t_i>>(std::get <t_i>(tuple));
			for_each <1 + t_i>(tuple, fn);
		}
	}
}

#endif
