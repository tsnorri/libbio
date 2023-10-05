/*
 * Copyright (c) 2022 Tuukka Norri
 * This code is licensed under MIT license (see LICENSE for details).
 */

#ifndef LIBBIO_TUPLE_ACCESS_HH
#define LIBBIO_TUPLE_ACCESS_HH

#include <tuple>


namespace libbio::tuples {
	
	// Nicer accessors and constants.
	
	typedef std::tuple <>	empty;
	
	template <typename t_tuple>
	using head_t = std::tuple_element_t <0, t_tuple>;
	
	
	template <typename t_tuple>
	using second_t = std::tuple_element_t <1, t_tuple>;
	
	
	template <typename t_tuple, typename t_default, std::size_t t_size = std::tuple_size_v <t_tuple>>
	struct head_
	{
		typedef head_t <t_tuple>	type;
	};
	
	template <typename t_tuple, typename t_default>
	struct head_ <t_tuple, t_default, 0>
	{
		typedef t_default	type;
	};
	
	template <typename t_tuple, typename t_default>
	using head_t_ = typename head_ <t_tuple, t_default>::type;
	
	
	template <typename t_tuple>
	using last = std::tuple_element <std::tuple_size_v <t_tuple> - 1, t_tuple>;
	
	template <typename t_tuple>
	using last_t = typename last <t_tuple>::type;
	
	
	template <std::size_t t_idx, typename t_tuple, typename t_missing = void>
	struct conditional_element
	{
		template <bool t_is_less_than>
		struct helper
		{
			typedef std::tuple_element_t <t_idx, t_tuple> type;
		};
		
		template <>
		struct helper <false>
		{
			typedef t_missing type;
		};
		
		typedef helper <t_idx < std::tuple_size_v <t_tuple>>::type type;
	};
	
	template <std::size_t t_idx, typename t_tuple, typename t_missing = void>
	using conditional_element_t = conditional_element <t_idx, t_tuple, t_missing>::type;
}

#endif
