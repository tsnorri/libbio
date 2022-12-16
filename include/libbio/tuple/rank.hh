/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_RANK_HH
#define LIBBIO_TUPLE_RANK_HH

#include <libbio/tuple/filter.hh>
#include <libbio/tuple/slice.hh>


namespace libbio::tuples {
	
	// Determine the number of occurrences of t_item in the indices [0, t_rb) of t_tuple.
	template <typename t_tuple, std::size_t t_rb, typename t_item = std::tuple_element_t <t_rb, t_tuple>>
	struct rank
	{
		typedef slice_t <t_tuple, 0, t_rb>	slice_type;
		typedef find <slice_type, t_item>	find_result_type;
		
		constexpr static inline auto const value{std::tuple_size_v <typename find_result_type::matching_type>};
	};
	
	
	template <typename t_tuple, std::size_t t_rb, typename t_item = std::tuple_element_t <t_rb, t_tuple>>
	constexpr static inline auto const rank_v{rank <t_tuple, t_rb, t_item>::value};
}

#endif
