/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_UNIQUE_HH
#define LIBBIO_TUPLE_UNIQUE_HH

#include <libbio/tuple/erase.hh>
#include <libbio/tuple/slice.hh>


namespace libbio::tuples {
	
	// Erases duplicate types from t_tuple.
	// Could be more efficient if implemented as right fold (but tuples are not typically very big).
	template <typename t_tuple>
	struct unique
	{
		template <typename t_tuple_>
		consteval static auto helper()
		{
			if constexpr (0 == std::tuple_size_v <t_tuple_>)
				return std::tuple <>{};
			else
			{
				typedef std::tuple_element_t <0, t_tuple_>								head_type;
				typedef erase_t <slice_t <t_tuple_, 1>, head_type>						tail_type_;
				typedef std::invoke_result_t <decltype(&unique::helper <tail_type_>)>	tail_type;
				
				return std::tuple_cat(std::tuple <head_type>{}, tail_type{});
			}
		}
		
		typedef std::invoke_result_t <decltype(&unique::helper <t_tuple>)> type;
	};
	
	
	template <typename t_tuple>
	using unique_t = typename unique <t_tuple>::type;
}

#endif
